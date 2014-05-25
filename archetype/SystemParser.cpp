//
//  SystemParser.cpp
//  archetype
//
//  Created by Derek Jones on 4/26/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <list>
#include <cctype>

#include "SystemParser.h"

using namespace std;

namespace archetype {
    
    inline string lowercase(string s) {
        string r;
        transform(begin(s), end(s), back_inserter(r), ::tolower);
        return r;
    }
    
    inline Value make_string_value(string s) {
        return Value(new StringValue(lowercase(s)));
    }
    
    SystemParser::SystemParser():
    mode_(SystemParser::VERBS)
    { }
    
    void SystemParser::addParseable(ObjectPtr sender, std::string names) {
        list<string> name_list;
        istringstream in(names);
        string word;
        while (getline(in, word, '|')) {
            switch (mode_) {
                case VERBS:
                    verbs_.push_back(Parseable(word, sender));
                    break;
                case NOUNS:
                    nouns_.push_back(Parseable(word, sender));
                    break;
            }
        }
    }
    
    inline bool longest_phrase_first(const SystemParser::Parseable& p1, const SystemParser::Parseable& p2) {
        if (p1.first.size() == p2.first.size()) {
            return p1.first > p2.first;
        } else {
            return p1.first.size() > p2.first.size();
        }
    }
    
    void SystemParser::close() {
        verbs_.sort(longest_phrase_first);
        for (auto verb_phrase : verbs_) {
            verbMatches_.push_back(PhraseMatch());
            istringstream in(verb_phrase.first);
            transform(istream_iterator<string>(in), istream_iterator<string>(), back_inserter(verbMatches_.back().first), [](string s) { return make_string_value(lowercase(s)); });
            verbMatches_.back().second = verb_phrase.second;
        }
        nouns_.sort(longest_phrase_first);
        for (auto noun_phrase : nouns_) {
            nounMatches_.push_back(PhraseMatch());
            istringstream in(noun_phrase.first);
            transform(istream_iterator<string>(in), istream_iterator<string>(), back_inserter(nounMatches_.back().first), [](string s) { return make_string_value(lowercase(s)); });
            nounMatches_.back().second = noun_phrase.second;
        }
    }
    
    inline bool equal_string_values(const Value& v1, const Value& v2) {
        Value sv1 = v1->stringConversion();
        Value sv2 = v2->stringConversion();
        return (sv1->isDefined() && sv2->isDefined() && sv1->getString() == sv2->getString());
    }
    
    void SystemParser::parse(std::string command_line) {
        playerCommand_ = command_line;
        normalized_.clear();
        istringstream in(command_line);
        list<string> words;
        transform(istream_iterator<string>(in), istream_iterator<string>(), back_inserter(words), lowercase);

        ostringstream out;
        out << ' ';
        copy(begin(words), end(words), ostream_iterator<string>(out, " "));
        normalized_ = out.str();
        
        parsedValues_.clear();
        transform(begin(words), end(words), back_inserter(parsedValues_), make_string_value);
        
        auto fillers = {"a", "an", "the"};
        parsedValues_.erase(remove_if(begin(parsedValues_), end(parsedValues_),
                                      [fillers](const Value& v) { return find(begin(fillers), end(fillers), v->getString()) != end(fillers); }),
                            end(parsedValues_));
        
        for (auto vp = begin(verbMatches_); vp != end(verbMatches_); ++vp) {
            auto match = search(begin(parsedValues_), end(parsedValues_), begin(vp->first), end(vp->first), equal_string_values);
            if (match != end(parsedValues_)) {
                auto match_end = match;
                advance(match_end, vp->first.size());
                parsedValues_.erase(match, match_end);
                parsedValues_.insert(match_end, Value(new ObjectValue(vp->second->id())));
            }
        }
        
        // Similar, but we want to find the best match when there are
        // several same-named candidates.  Original Archetype kept a 'near-match'
        // and a 'far-match'.  At the end it used the near match if it existed.
        //
        // TODO:  how to implement?
        for (auto np = begin(nounMatches_); np != end(nounMatches_); ++np) {
            auto match = search(begin(parsedValues_), end(parsedValues_), begin(np->first), end(np->first), equal_string_values);
            if (match != end(parsedValues_)) {
                auto match_end = match;
                advance(match_end, np->first.size());
                parsedValues_.erase(match, match_end);
                parsedValues_.insert(match_end, Value(new ObjectValue(np->second->id())));
                // TODO:  check proximity.  But we can puzzle this out after testing what we have
            }
        }
    }
    
    string SystemParser::normalized() const {
        return normalized_;
    }
    
    void SystemParser::rollCall() {
        proximate_.clear();
    }
    
    void SystemParser::announcePresence(ObjectPtr sender) {
        proximate_.insert(sender);
    }
    
    Value SystemParser::nextObject() {
        if (parsedValues_.empty()) {
            return Value(new UndefinedValue);
        } else {
            Value result = std::move(parsedValues_.front());
            parsedValues_.pop_front();
            return result;
        }
    }
}