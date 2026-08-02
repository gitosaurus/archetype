//
//  TestIdIndex.cc
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <string>

#include "TestIdIndex.hh"
#include "TestRegistry.hh"
#include "IdIndex.hh"
#include "Serialization.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestIdIndex);

    void TestIdIndex::runTests_() {
        IdIndex<string> strindex;
        ARCHETYPE_TEST_EQUAL(strindex.has("First"), false);

        strindex.index("First");
        ARCHETYPE_TEST_EQUAL(strindex.has("First"), true);
        ARCHETYPE_TEST_EQUAL(strindex.index("Second"), 1);
        ARCHETYPE_TEST_EQUAL(strindex.index("First"), 0);

        ARCHETYPE_TEST_EQUAL(strindex.get(1), string("Second"));

        // Removing anything but the last entry leaves a hole, which the next
        // index() fills rather than growing the registry.
        IdIndex<string> holey;
        for (string s : {"zero", "one", "two", "three"}) {
            holey.index(s);
        }
        holey.remove(1);
        holey.remove(2);
        ARCHETYPE_TEST_EQUAL(holey.count(), 4);
        ARCHETYPE_TEST_EQUAL(holey.find("one"), IdIndex<string>::npos);

        // A round trip has to arrive at an index that behaves the same, not
        // merely one that answers the same questions:  the holes are implied
        // by the counts rather than written out, and an index that forgot them
        // would append where the original would have reused.
        MemoryStorage store;
        store << holey;
        IdIndex<string> resumed;
        store >> resumed;

        ARCHETYPE_TEST_EQUAL(resumed.count(), 4);
        ARCHETYPE_TEST_EQUAL(resumed.get(0), string("zero"));
        ARCHETYPE_TEST_EQUAL(resumed.get(3), string("three"));
        ARCHETYPE_TEST_EQUAL(resumed.find("one"), IdIndex<string>::npos);

        // Holes are filled from the back, so both indexes must hand out 2 and
        // then 1, and neither may reach a fifth slot.
        ARCHETYPE_TEST_EQUAL(holey.index("four"), 2);
        ARCHETYPE_TEST_EQUAL(resumed.index("four"), 2);
        ARCHETYPE_TEST_EQUAL(resumed.index("five"), 1);
        ARCHETYPE_TEST_EQUAL(resumed.count(), 4);
        ARCHETYPE_TEST_EQUAL(resumed.index("six"), 4);
    }

}
