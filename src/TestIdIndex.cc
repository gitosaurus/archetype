//
//  TestIdIndex.cc
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <string>
#include <vector>

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

        // An id is retired with its entry.  Removing leaves the count where
        // it was, because the count is how far the ids have got and not how
        // many entries are left.
        IdIndex<string> holey;
        for (string s : {"zero", "one", "two", "three"}) {
            holey.index(s);
        }
        ARCHETYPE_TEST(holey.remove(1));
        ARCHETYPE_TEST(holey.remove(2));
        ARCHETYPE_TEST_EQUAL(holey.count(), 4);
        ARCHETYPE_TEST_EQUAL(holey.find("one"), IdIndex<string>::npos);
        ARCHETYPE_TEST(not holey.hasIndex(1));
        ARCHETYPE_TEST(holey.hasIndex(3));
        ARCHETYPE_TEST(holey.indices() == (vector<int>{0, 3}));

        // Removing what is already gone, or was never there, is not an error.
        ARCHETYPE_TEST(not holey.remove(1));
        ARCHETYPE_TEST(not holey.remove(99));
        ARCHETYPE_TEST(not holey.remove(-1));

        // read() ignores the order records arrive in, so nothing about a round
        // trip can detect this:  the file has to be inspected directly.
        // Records must come out in id order, live entries only.
        MemoryStorage ordered;
        ordered << holey;
        int total_entries, indexed_entries;
        ordered >> total_entries >> indexed_entries;
        ARCHETYPE_TEST_EQUAL(total_entries, 4);
        ARCHETYPE_TEST_EQUAL(indexed_entries, 2);
        int previous_slot = -1;
        for (int ii = 0; ii < indexed_entries; ++ii) {
            int slot;
            string value;
            ordered >> slot >> value;
            ARCHETYPE_TEST(slot > previous_slot);
            previous_slot = slot;
        }
        ARCHETYPE_TEST_EQUAL(previous_slot, 3);
        ARCHETYPE_TEST_EQUAL(ordered.remaining(), 0);

        // A round trip has to arrive at an index that behaves the same, not
        // merely one that answers the same questions:  the retired ids are
        // implied by the counts rather than written out, and an index that
        // forgot them would hand one out a second time.
        MemoryStorage store;
        store << holey;
        IdIndex<string> resumed;
        store >> resumed;

        ARCHETYPE_TEST_EQUAL(resumed.count(), 4);
        ARCHETYPE_TEST_EQUAL(resumed.get(0), string("zero"));
        ARCHETYPE_TEST_EQUAL(resumed.get(3), string("three"));
        ARCHETYPE_TEST_EQUAL(resumed.find("one"), IdIndex<string>::npos);
        ARCHETYPE_TEST(not resumed.hasIndex(2));

        // Neither index may give 1 or 2 to anything else, ever -- not even to
        // the very value that used to hold it.
        ARCHETYPE_TEST_EQUAL(holey.index("four"), 4);
        ARCHETYPE_TEST_EQUAL(resumed.index("four"), 4);
        ARCHETYPE_TEST_EQUAL(resumed.index("one"), 5);
        ARCHETYPE_TEST_EQUAL(resumed.count(), 6);

        // The last entry is no exception.  Its id is the easiest of all to
        // hand back out, and the one a reference is likeliest still to hold.
        IdIndex<string> tail;
        for (string s : {"zero", "one", "two"}) {
            tail.index(s);
        }
        tail.remove(2);
        ARCHETYPE_TEST_EQUAL(tail.count(), 3);
        ARCHETYPE_TEST_EQUAL(tail.index("three"), 3);

        // And that survives a save, where the total is the only record of an
        // id retired off the end:  no record names it.
        tail.remove(3);
        MemoryStorage shortened;
        shortened << tail;
        IdIndex<string> reloaded;
        shortened >> reloaded;
        ARCHETYPE_TEST_EQUAL(reloaded.count(), 4);
        ARCHETYPE_TEST_EQUAL(reloaded.get(1), string("one"));
        ARCHETYPE_TEST_EQUAL(reloaded.index("four"), 4);

        // An index that has retired far more ids than its file has bytes is
        // still a file that loads.  Holes cost nothing to write, so nothing
        // about them may be held against the size of what was written.
        IdIndex<string> churned;
        churned.index("keeper");
        for (int ii = 0; ii < 5000; ++ii) {
            churned.remove(churned.index("passing through"));
        }
        MemoryStorage small;
        small << churned;
        ARCHETYPE_TEST(small.remaining() < 100);
        IdIndex<string> survivor;
        small >> survivor;
        ARCHETYPE_TEST_EQUAL(survivor.count(), 5001);
        ARCHETYPE_TEST_EQUAL(survivor.get(0), string("keeper"));
        ARCHETYPE_TEST_EQUAL(survivor.index("newcomer"), 5001);
    }

}
