//
//  SystemParser.cc
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

#include "SystemParser.hh"

using namespace std;

namespace archetype {

    inline string lowercase(string s) {
        string r;
        transform(begin(s), end(s), back_inserter(r), ::tolower);
        return r;
    }

    inline Value make_string_value(string s) {
        return Value{new StringValue{lowercase(s)}};
    }

    SystemParser::SystemParser():
    mode_{SystemParser::VERBS}
    { }

    void SystemParser::addParseable(int sender, std::string names) {
        list<string> name_list;
        istringstream in{names};
        string word;
        while (getline(in, word, '|')) {
            switch (mode_) {
                case VERBS:
                    verbs_.push_back(Parseable{word, sender});
                    break;
                case NOUNS:
                    nouns_.push_back(Parseable{word, sender});
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
        for (auto const& verb_phrase : verbs_) {
            verbMatches_.push_back(PhraseMatch{});
            istringstream in(verb_phrase.first);
            transform(istream_iterator<string>{in}, istream_iterator<string>{},
                      back_inserter(verbMatches_.back().first),
                      [](string s) { return make_string_value(lowercase(s)); });
            verbMatches_.back().second = verb_phrase.second;
        }
        nouns_.sort(longest_phrase_first);
        for (auto const& noun_phrase : nouns_) {
            nounMatches_.push_back(PhraseMatch{});
            istringstream in(noun_phrase.first);
            transform(istream_iterator<string>{in}, istream_iterator<string>{},
                      back_inserter(nounMatches_.back().first),
                      [](string s) { return make_string_value(lowercase(s)); });
            nounMatches_.back().second = noun_phrase.second;
        }
    }

    inline bool equal_string_values(const Value& v1, const Value& v2) {
        Value sv1 = v1->stringConversion();
        Value sv2 = v2->stringConversion();
        return (sv1->isDefined() and sv2->isDefined() and sv1->getString() == sv2->getString());
    }

    inline void remove_fillers(list<Value>& wordValues) {
        auto fillers = {"a", "an", "the"};
        wordValues.erase(remove_if(begin(wordValues), end(wordValues),
                                   [fillers](const Value& v) {
                                       return find(begin(fillers), end(fillers), v->getString()) != end(fillers);
                                   }),
                         end(wordValues));
    }

    void SystemParser::matchVerbs_(std::list<Value>& wordValues) {
        for (auto vp = begin(verbMatches_); vp != end(verbMatches_); ++vp) {
            auto match = search(begin(wordValues), end(wordValues),
                                begin(vp->first), end(vp->first), equal_string_values);
            if (match != end(wordValues)) {
                auto match_end = match;
                advance(match_end, vp->first.size());
                wordValues.erase(match, match_end);
                wordValues.insert(match_end, Value{new ObjectValue{vp->second}});
            }
        }
    }

    void SystemParser::matchNouns_(std::list<Value>& wordValues) {
        for (auto np = begin(nounMatches_); np != end(nounMatches_); ++np) {
            auto match = search(begin(wordValues), end(wordValues),
                                begin(np->first), end(np->first), equal_string_values);
            if (match != end(wordValues)) {
                size_t phrase_size = np->first.size();
                auto match_end = match;
                advance(match_end, phrase_size);
                int matched_obj_id = np->second;
                // At this point we have at least one match.  If it's proximate, we're completely
                // done.  But if it isn't, then we want to check all remaining matches at this
                // place, of this size, for a better match that is proximate.
                if (not proximate_.count(matched_obj_id)) {
                    auto next_np = np;
                    while (++next_np != end(nounMatches_) and next_np->first.size() == phrase_size) {
                        if (equal(match, match_end, begin(next_np->first), equal_string_values) and
                            proximate_.count(next_np->second)) {
                            // This is a nearer version of the same match phrase
                            matched_obj_id = next_np->second;
                            break;
                        }
                    }
                }
                wordValues.erase(match, match_end);
                wordValues.insert(match_end, Value{new ObjectValue{matched_obj_id}});
            }
        }
    }

    void SystemParser::parse(std::string command_line) {
        playerCommand_ = command_line;
        string unpunctuated;
        copy_if(begin(command_line), end(command_line),
                back_inserter(unpunctuated),
                [](char ch) { return not ispunct(ch)  or  ch == '-'; });
        istringstream in(unpunctuated);
        list<string> words;
        transform(istream_iterator<string>(in), istream_iterator<string>(), back_inserter(words), lowercase);

        ostringstream out;
        out << ' ';
        copy(begin(words), end(words), ostream_iterator<string>{out, " "});
        normalized_ = out.str();

        parsedValues_.clear();
        transform(begin(words), end(words), back_inserter(parsedValues_), make_string_value);

        remove_fillers(parsedValues_);
        matchVerbs_(parsedValues_);
        matchNouns_(parsedValues_);
    }

    string SystemParser::normalized() const {
        return normalized_;
    }

    void SystemParser::rollCall() {
        proximate_.clear();
    }

    void SystemParser::announcePresence(int sender) {
        proximate_.insert(sender);
    }

    Value SystemParser::nextObject() {
        if (parsedValues_.empty()) {
            return Value{new UndefinedValue};
        } else {
            Value result = std::move(parsedValues_.front());
            parsedValues_.pop_front();
            return result;
        }
    }

    Value SystemParser::whichObject(std::string phrase) {
        istringstream in(phrase);
        list<Value> words;
        transform(istream_iterator<string>{in}, istream_iterator<string>{},
                  back_inserter(words),
                  [](string s) { return make_string_value(lowercase(s)); });
        remove_fillers(words);
        matchNouns_(words);
        matchVerbs_(words);
        if (words.size() == 1) {
            return words.front()->objectConversion();
        } else {
            return Value{new UndefinedValue};
        }
    }

    template <typename T1, typename T2>
    Storage& operator<<(Storage& out, const std::pair<T1, T2>& p) {
        return out << p.first << p.second;
    }

    template <typename Tseq>
    Storage& operator<<(Storage& out, const Tseq& seq) {
        const int zero = 0;
        const int one = 1;
        for (auto ii = begin(seq); ii != end(seq); ++ii) {
            out << one << *ii;
        }
        out << zero;
        return out;
    }

    template <typename T1, typename T2>
    Storage& operator>>(Storage& in, std::pair<T1, T2>& p) {
        return in >> p.first >> p.second;
    }

    template <typename Tseq>
    Storage& operator>>(Storage& in, Tseq& seq) {
        int more;
        in >> more;
        while (more) {
            typename Tseq::value_type element;
            in >> element;
            seq.insert(end(seq), std::move(element));
            in >> more;
        }
        return in;
    }

    Storage& operator<<(Storage& out, const SystemParser& p) {
        out << static_cast<int>(p.mode_);
        out << p.proximate_ << p.verbs_ << p.nouns_;
        out << p.verbMatches_ << p.nounMatches_;
        out << p.playerCommand_ << p.normalized_ << p.parsedValues_;
        return out;
    }

    Storage& operator>>(Storage&in, SystemParser& p) {
        int mode;
        in >> mode;
        p.mode_ = static_cast<SystemParser::Mode_e>(mode);
        in >> p.proximate_ >> p.verbs_ >> p.nouns_;
        in >> p.verbMatches_ >> p.nounMatches_;
        in >> p.playerCommand_ >> p.normalized_ >> p.parsedValues_;
        return in;
    }

}
