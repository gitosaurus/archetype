//
//  TestTokenStream.cpp
//  archetype
//
//  Created by Derek Jones on 2/19/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <iostream>
#include <sstream>
#include <deque>

#include "TestTokenStream.h"
#include "TestRegistry.h"
#include "TokenStream.h"
#include "SourceFile.h"
#include "Keywords.h"

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
    
    void TestTokenStream::runTests_(std::ostream& out) {
        string src_str = "3 + 4";
        istringstream in(src_str);
        SourceFile src("simpleplus", in);
        TokenStream token_stream(src);
        
        deque<Token> expected = {{Token::NUMERIC, 3}, {Token::OPERATOR, Keywords::OP_PLUS}, {Token::NUMERIC, 4}};
        deque<Token> actual;
        while (token_stream.fetch()) {
            actual.push_back(token_stream.token());
        }
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }
}