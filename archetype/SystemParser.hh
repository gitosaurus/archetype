//
//  SystemParser.h
//  archetype
//
//  Created by Derek Jones on 4/26/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__SystemParser__
#define __archetype__SystemParser__

#include <iostream>
#include <string>
#include <set>
#include <list>

#include "Value.hh"

namespace archetype {
    class SystemParser {
    public:
        enum Mode_e { VERBS, NOUNS };

        typedef std::pair<std::string, int> Parseable;
        typedef std::pair<std::list<Value>, int> PhraseMatch;

        SystemParser();
        SystemParser(const SystemParser&) = delete;
        SystemParser& operator=(const SystemParser&) = delete;

        void setMode(Mode_e mode) { mode_ = mode; }

        // Names are given as a bar-separated string of synonyms, e.g. "get|take"
        void addParseable(int sender, std::string names);

        // Closes the parser, organizing the vocabulary received so far to
        // prepare for a call to parse()
        void close();

        // Clears the presence of all objects
        void rollCall();

        // Sender announces it is present.  If another, non-present object has its
        // same name, the parser will choose the present one.
        void announcePresence(int sender);

        // Accepts a player's command, normalizes and parses it.
        void parse(std::string command_line);

        // Returns the normalized form of the last string given to parse.
        std::string normalized() const;

        // Returns the next value in the parse stream.  If an object was matched,
        // that object is returned; otherwise returns the unparseable string value.
        // Returns UndefinedValue when the list of values is exhausted.
        Value nextObject();

        // Given a name or phrase, returns the object matching it after the last parse.
        // Looks for nouns first, then verbs.  Returns undefined if no match.
        Value whichObject(std::string phrase);

    private:
        Mode_e mode_;
        std::set<int> proximate_;

        std::list<Parseable> verbs_;
        std::list<Parseable> nouns_;

        std::list<PhraseMatch> verbMatches_;
        std::list<PhraseMatch> nounMatches_;

        std::string playerCommand_;
        std::string normalized_;
        std::list<Value> parsedValues_;

        void matchVerbs_(std::list<Value>& wordValues);
        void matchNouns_(std::list<Value>& wordValues);
    };
}

#endif /* defined(__archetype__SystemParser__) */
