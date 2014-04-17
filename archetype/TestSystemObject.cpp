//
//  TestSystemObject.cpp
//  archetype
//
//  Created by Derek Jones on 4/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <string>
#include <algorithm>
#include <deque>

#include "TestSystemObject.h"
#include "TestRegistry.h"
#include "SystemObject.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestSystemObject);
    
    void TestSystemObject::testSorting_() {
        SystemObject sys;
        sys.send(Value(new StringValue("OPEN SORTER")));
        deque<string> to_sort = {"dog", "xylem", "cat", "Ajax", "several", "Zebra"};
        for (string s : to_sort) {
            Value ans = sys.send(Value(new StringValue(s)));
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
        }
        sys.send(Value(new StringValue("CLOSE SORTER")));
        deque<string> sorted = to_sort;
        sort(sorted.begin(), sorted.end());
        for (string s : sorted) {
            Value ans = sys.send(Value(new StringValue("NEXT SORTED")));
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
            cout << "NEXT SORTED: " << ans_str->getString() << endl;
        }
        // Now it should be exhausted, and NEXT SORTED should be undefined
        Value no_more = sys.send(Value(new StringValue("NEXT SORTED")));
        ARCHETYPE_TEST(not no_more->isDefined());
    }
    
    void TestSystemObject::runTests_() {
        testSorting_();
    }
}