//
//  TestSystemObject.cpp
//  archetype
//
//  Created by Derek Jones on 4/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <deque>

#include "TestSystemObject.h"
#include "TestRegistry.h"
#include "Universe.h"
#include "Statement.h"
#include "TokenStream.h"
#include "SourceFile.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestSystemObject);
    
    inline SourceFilePtr make_source_from_str(string name, string src_str) {
        stream_ptr in(new istringstream(src_str));
        return SourceFilePtr(new SourceFile(name, in));
    }
    
    inline Statement make_stmt_from_str(string src_str) {
        TokenStream token_stream(make_source_from_str("test", src_str));
        Statement stmt = make_statement(token_stream);
        return stmt;
    }
    
    void TestSystemObject::testSorting_() {
        Universe::destroy();
        
        ostringstream out;
        Statement stmt = make_stmt_from_str("'OPEN SORTER' -> system");
        stmt->execute(out);
        deque<string> to_sort = {"dog", "xylem", "cat", "Ajax", "several", "Zebra"};
        for (string s : to_sort) {
            // Because we are creating statements to be executed, the strings need to be in double
            // quotes.  Otherwise they'll get evaluated to UNDEFINED, since they are indeed
            // undefined identifiers.
            stmt = make_stmt_from_str("\"" + s + "\" -> system");
            Value ans = stmt->execute(out);
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
        }
        stmt = make_stmt_from_str("'CLOSE SORTER' -> system");
        stmt->execute(out);
        deque<string> sorted = to_sort;
        sort(sorted.begin(), sorted.end());
        stmt = make_stmt_from_str("'NEXT SORTED' -> system");
        for (string s : sorted) {
            Value ans = stmt->execute(out);
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
            cout << "NEXT SORTED: " << ans_str->getString() << endl;
        }
        // Now it should be exhausted, and NEXT SORTED should be undefined
        Value no_more = stmt->execute(out);
        ARCHETYPE_TEST(not no_more->isDefined());
    }
    
    void TestSystemObject::testParsing_() {
        Universe::destroy();
        
        
    }
    
    void TestSystemObject::runTests_() {
        testSorting_();
        testParsing_();
    }
}