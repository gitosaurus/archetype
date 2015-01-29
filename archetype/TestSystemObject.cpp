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

    void TestSystemObject::testSorting_() {
        Universe::destroy();

        Statement stmt = make_stmt_from_str("'OPEN SORTER' -> system");
        stmt->execute();
        deque<string> to_sort = {"dog", "xylem", "cat", "Ajax", "several", "Zebra"};
        for (auto const& s : to_sort) {
            // Because we are creating statements to be executed, the strings need to be in double
            // quotes.  Otherwise they'll get evaluated to UNDEFINED, since they are indeed
            // undefined identifiers.
            stmt = make_stmt_from_str("\"" + s + "\" -> system");
            Value ans = stmt->execute();
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
        }
        stmt = make_stmt_from_str("'CLOSE SORTER' -> system");
        stmt->execute();
        deque<string> sorted = to_sort;
        sort(sorted.begin(), sorted.end());
        stmt = make_stmt_from_str("'NEXT SORTED' -> system");
        for (auto const& s : sorted) {
            Value ans = stmt->execute();
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
            cout << "NEXT SORTED: " << ans_str->getString() << endl;
        }
        // Now it should be exhausted, and NEXT SORTED should be undefined
        Value no_more = stmt->execute();
        ARCHETYPE_TEST(not no_more->isDefined());
    }

    static char program1[] =
    "type lexable based on null\n"
    "methods\n"
    "  'BUILD' : name -> system\n"
    "end\n"
    "\n"
    "lexable take name : 'take|get|grab' end\n"
    "lexable money name : 'money|cash|bag of coins'\n"
    ;

    void TestSystemObject::testParsing_() {
        Universe::destroy();

        TokenStream t1(make_source_from_str("program1", program1));
        Universe::instance().make(t1);
        string build_vocab =
            "{'OPEN PARSER' -> system;"
            "'BUILD' -> take;"
            "'BUILD' -> money;"
            "'CLOSE PARSER' -> system;"
            "'PLAYER CMD' -> system;"
            "\"grab all the cash\" -> system;"
            "'PARSE' -> system}"
        ;
        Statement stmt = make_stmt_from_str(build_vocab);
        stmt->execute();

        stmt = make_stmt_from_str("'NEXT OBJECT' -> system");
        Value val = stmt->execute();
        list<Value> parsed;
        while (val->isDefined()) {
            parsed.push_back(move(val));
            val = stmt->execute();
        }
        ARCHETYPE_TEST_EQUAL(parsed.size(), size_t(3));
        list<Value> expected;
        int take_obj_id = Universe::instance().getObject("take")->id();
        int money_obj_id = Universe::instance().getObject("money")->id();
        expected.push_back(Value(new ObjectValue(take_obj_id)));
        expected.push_back(Value(new StringValue("all")));
        expected.push_back(Value(new ObjectValue(money_obj_id)));
        bool are_equal = equal(parsed.begin(), parsed.end(), expected.begin(),
                               [](const Value& x, const Value& y){ return x->isSameValueAs(y);} );
        ARCHETYPE_TEST(are_equal);
    }

    void TestSystemObject::runTests_() {
        testSorting_();
        testParsing_();
    }
}
