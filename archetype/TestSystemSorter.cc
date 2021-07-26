//
//  TestSystemSorter.cc
//  archetype
//
//  Created by Derek Jones on 7/26/21.
//  Copyright © 2021 Derek Jones. All rights reserved.
//


#include <iostream>
#include <string>
#include <sstream>
#include <deque>
#include <iterator>

#include "TestSystemSorter.hh"
#include "TestRegistry.hh"

#include "SystemSorter.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestSystemSorter);

    void TestSystemSorter::testSortAndSerialize_() {
        // Contains an extra couple of "cat" strings to make sure none are lost
        deque<string> to_sort = {"dog", "cat", "xylem", "cat", "Ajax", "several", "cat", "Zebra"};
        SystemSorter original;
        for (auto const& s : to_sort) {
            original.add(s);
        }
        // Now serialize it and then deserialize into a new one.
        MemoryStorage mem;
        mem << original;
        SystemSorter derived;
        mem >> derived;

        deque<string> sorted = to_sort;
        sort(sorted.begin(), sorted.end());

        for (auto const& expected : sorted) {
            string actual = derived.nextSorted()->getString();
            ARCHETYPE_TEST_EQUAL(actual, expected);
        }
        // At the end, nothing should be left but UNDEFINED
        ARCHETYPE_TEST(derived.nextSorted()->isSameValueAs(Value{new UndefinedValue}));
    }

    void TestSystemSorter::runTests_() {
        testSortAndSerialize_();
    }

}
