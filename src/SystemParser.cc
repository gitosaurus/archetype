//
//  SystemParser.cc
//  archetype
//
//  Created by Derek Jones on 4/26/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <string_view>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <concepts>
#include <iterator>
#include <list>
#include <ranges>
#include <cctype>

#include "SystemParser.hh"

using namespace std;

namespace archetype {

    inline string lowercase(string_view s) {
        string r;
        r.reserve(s.size());
        ranges::transform(s, back_inserter(r), ::tolower);
        return r;
    }

    // Wraps a word that has already been through lowercase(), and that the
    // caller is giving away rather than lending.
    inline Value make_string_value_as_is(string s) {
        return make_unique<StringValue>(std::move(s));
    }

    inline Value make_string_value(string_view s) {
        return make_string_value_as_is(lowercase(s));
    }

    SystemParser::SystemParser():
    mode_{SystemParser::VERBS}
    { }

    void SystemParser::addParseable(int sender, std::string names) {
        list<string> name_list;
        istringstream in{std::move(names)};
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
            ranges::transform(ranges::istream_view<string>(in),
                              back_inserter(verbMatches_.back().first),
                              [](const string& s) { return make_string_value(s); });
            verbMatches_.back().second = verb_phrase.second;
        }
        nouns_.sort(longest_phrase_first);
        for (auto const& noun_phrase : nouns_) {
            nounMatches_.push_back(PhraseMatch{});
            istringstream in(noun_phrase.first);
            ranges::transform(ranges::istream_view<string>(in),
                              back_inserter(nounMatches_.back().first),
                              [](const string& s) { return make_string_value(s); });
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
        erase_if(wordValues, [fillers](const Value& v) {
            return ranges::find(fillers, v->getString()) != end(fillers);
        });
    }

    // Both matchers below use ranges::search, which hands back the matched
    // subrange rather than just where it starts.  That removes the advance()
    // by phrase.size(), and with it any way for the two ends to disagree.
    //
    // The test for a miss stays positional rather than becoming
    // match.empty(), because those are not the same question.  A miss is an
    // empty subrange at the end of wordValues; an *empty phrase* is an empty
    // subrange at the front, and has always matched here.  A game can produce
    // one with a doubled '|' in a name list, so the difference is reachable.
    void SystemParser::matchVerbs_(std::list<Value>& wordValues) {
        for (const auto& [phrase, verb_id] : verbMatches_) {
            auto match = ranges::search(wordValues, phrase, equal_string_values);
            if (match.begin() != end(wordValues)) {
                auto after = wordValues.erase(match.begin(), match.end());
                wordValues.insert(after, make_unique<ObjectValue>(verb_id));
            }
        }
    }

    void SystemParser::matchNouns_(std::list<Value>& wordValues) {
        for (auto np = begin(nounMatches_); np != end(nounMatches_); ++np) {
            auto match = ranges::search(wordValues, np->first, equal_string_values);
            if (match.begin() != end(wordValues)) {
                size_t phrase_size = np->first.size();
                int matched_obj_id = np->second;
                // At this point we have at least one match.  If it's proximate, we're completely
                // done.  But if it isn't, then we want to check all remaining matches at this
                // place, of this size, for a better match that is proximate.
                if (not proximate_.contains(matched_obj_id)) {
                    auto next_np = np;
                    while (++next_np != end(nounMatches_) and next_np->first.size() == phrase_size) {
                        // Sizes are equal by the loop condition, and
                        // ranges::equal is the overload that insists on that
                        // rather than trusting it.
                        if (ranges::equal(match, next_np->first, equal_string_values) and
                            proximate_.contains(next_np->second)) {
                            // This is a nearer version of the same match phrase
                            matched_obj_id = next_np->second;
                            break;
                        }
                    }
                }
                auto after = wordValues.erase(match.begin(), match.end());
                wordValues.insert(after, make_unique<ObjectValue>(matched_obj_id));
            }
        }
    }

    void SystemParser::parse(std::string command_line) {
        string unpunctuated;
        ranges::copy_if(command_line, back_inserter(unpunctuated),
                        [](char ch) { return not ispunct(ch)  or  ch == '-'; });
        // Read the command through before taking it:  the parser keeps the
        // player's own spelling of it, so the copy the caller made is the one
        // that gets stored.
        playerCommand_ = std::move(command_line);
        istringstream in(std::move(unpunctuated));
        list<string> words;
        ranges::transform(ranges::istream_view<string>(in), back_inserter(words), lowercase);

        ostringstream out;
        out << ' ';
        ranges::copy(words, ostream_iterator<string>{out, " "});
        normalized_ = out.str();

        parsedValues_.clear();
        // The words are already lowercase -- that happened on the way out of
        // the stream, and normalized_ above is built from them -- so hand each
        // one over rather than lowercasing and copying it a second time.
        transform(make_move_iterator(begin(words)), make_move_iterator(end(words)),
                  back_inserter(parsedValues_), make_string_value_as_is);

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
            return make_unique<UndefinedValue>();
        } else {
            Value result = std::move(parsedValues_.front());
            parsedValues_.pop_front();
            return result;
        }
    }

    Value SystemParser::whichObject(std::string phrase) {
        istringstream in(std::move(phrase));
        list<Value> words;
        ranges::transform(ranges::istream_view<string>(in), back_inserter(words),
                          [](const string& s) { return make_string_value(s); });
        remove_fillers(words);
        matchNouns_(words);
        matchVerbs_(words);
        if (words.size() == 1) {
            return words.front()->objectConversion();
        } else {
            return make_unique<UndefinedValue>();
        }
    }

    template <typename T1, typename T2>
    Storage& operator<<(Storage& out, const std::pair<T1, T2>& p) {
        return out << p.first << p.second;
    }

    // Storage already has non-template overloads for int and std::string, and
    // an unconstrained template here is a candidate for those types too: the
    // non-template wins only by the tie-break that prefers it, and would fail
    // to compile if it ever lost.  Naming what these templates are actually for
    // states the intent instead of relying on that.
    template <typename T>
    concept StorableSequence =
        std::ranges::input_range<T> and
        not std::convertible_to<const T&, std::string_view>;

    template <typename T>
    concept ReadableSequence =
        StorableSequence<T> and
        requires (T& seq, std::ranges::range_value_t<T> element) {
            seq.insert(std::ranges::end(seq), std::move(element));
        };

    template <StorableSequence Tseq>
    Storage& operator<<(Storage& out, const Tseq& seq) {
        const int zero = 0;
        const int one = 1;
        for (const auto& element : seq) {
            out << one << element;
        }
        out << zero;
        return out;
    }

    template <typename T1, typename T2>
    Storage& operator>>(Storage& in, std::pair<T1, T2>& p) {
        return in >> p.first >> p.second;
    }

    template <ReadableSequence Tseq>
    Storage& operator>>(Storage& in, Tseq& seq) {
        int more;
        in >> more;
        while (more) {
            std::ranges::range_value_t<Tseq> element;
            in >> element;
            seq.insert(std::ranges::end(seq), std::move(element));
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
