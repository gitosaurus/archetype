//
//  TestTokenStream.cc
//  archetype
//
//  Created by Derek Jones on 2/19/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <iostream>
#include <sstream>
#include <deque>

#include "TestTokenStream.hh"
#include "TestRegistry.hh"
#include "TokenStream.hh"
#include "SourceFile.hh"
#include "Keywords.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestTokenStream);

    ostream& operator<< (ostream& out, const deque<Token>& tokens) {
        for (auto token_p = tokens.begin(); token_p != tokens.end(); ++token_p) {
            if (token_p != tokens.begin()) out << ", ";
            out << *token_p;
        }
        return out;
    }

    inline deque<Token> tokenize(string src_str) {
        deque<Token> actual;
        stream_ptr in{new istringstream{src_str}};
        SourceFilePtr src{make_shared<SourceFile>("test", in)};
        TokenStream token_stream(src);
        while (token_stream.fetch()) {
            actual.push_back(token_stream.token());
        }
        return actual;
    }

    void TestTokenStream::runTests_() {
        Universe::destroy();

        deque<Token> actual1 = tokenize("3 + 4");
        deque<Token> expected1 = {{Token::NUMERIC, 3}, {Token::OPERATOR, Keywords::OP_PLUS}, {Token::NUMERIC, 4}};
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        deque<Token> actual2 = tokenize("*:=:=*");
        deque<Token> expected2 = {
            {Token::OPERATOR, Keywords::OP_C_MULTIPLY},
            {Token::OPERATOR, Keywords::OP_ASSIGN},
            {Token::OPERATOR, Keywords::OP_MULTIPLY}};
        ARCHETYPE_TEST_EQUAL(actual2, expected2);

        deque<Token> actual3 = tokenize("'START' -> main");
        deque<Token> expected3 = {
            {Token::MESSAGE, 0},
            {Token::OPERATOR, Keywords::OP_SEND},
            {Token::IDENTIFIER, 2}};
        ARCHETYPE_TEST_EQUAL(actual3, expected3);

        deque<Token> actual4 = tokenize("main.subj = main.dobj");
        deque<Token> expected4 = {
            {Token::IDENTIFIER, 2}, {Token::OPERATOR, Keywords::OP_DOT}, {Token::IDENTIFIER, 3},
            {Token::OPERATOR, Keywords::OP_EQ},
            {Token::IDENTIFIER, 2}, {Token::OPERATOR, Keywords::OP_DOT}, {Token::IDENTIFIER, 4}
        };
        ARCHETYPE_TEST_EQUAL(actual4, expected4);

        deque<Token> actual5 = tokenize("\n\n# Nothing but commentary.\n# Authorial indulgence\n\n");
        deque<Token> expected5;
        ARCHETYPE_TEST_EQUAL(actual5, expected5);
    }
}
