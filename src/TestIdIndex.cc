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

        // read() ignores the order records arrive in, so nothing about a round
        // trip can detect this:  the file has to be inspected directly.
        // Records must come out in registry order, occupied slots only.
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

        // Free slots are filled lowest first, so both indexes must hand out 1
        // and then 2, and neither may reach a fifth slot until both are taken.
        ARCHETYPE_TEST_EQUAL(holey.index("four"), 1);
        ARCHETYPE_TEST_EQUAL(resumed.index("four"), 1);
        ARCHETYPE_TEST_EQUAL(resumed.index("five"), 2);
        ARCHETYPE_TEST_EQUAL(resumed.count(), 4);
        ARCHETYPE_TEST_EQUAL(resumed.index("six"), 4);

        // Removing the last entry is not a hole at all:  the registry simply
        // gets shorter.  And it keeps getting shorter for as long as the slot
        // newly exposed at the end is free too, so a run of holes reachable
        // from the end collapses in one call.
        IdIndex<string> trimmed;
        for (string s : {"zero", "one", "two", "three"}) {
            trimmed.index(s);
        }
        trimmed.remove(2);
        ARCHETYPE_TEST_EQUAL(trimmed.count(), 4);   // slot 3 still holds "three"
        trimmed.remove(3);
        // Freeing 3 pops it, which exposes the already-free 2, which pops too.
        ARCHETYPE_TEST_EQUAL(trimmed.count(), 2);
        ARCHETYPE_TEST_EQUAL(trimmed.get(1), string("one"));
        ARCHETYPE_TEST(not trimmed.hasIndex(2));

        // Nothing is free any more -- the trim gave those slots back by making
        // them not exist -- so the next entry appends rather than reusing.
        ARCHETYPE_TEST_EQUAL(trimmed.index("four"), 2);

        // A trimmed registry has to come back at its shorter length, and
        // total_entries is the only record of where it now ends.
        MemoryStorage shortened;
        shortened << trimmed;
        IdIndex<string> reloaded;
        shortened >> reloaded;
        ARCHETYPE_TEST_EQUAL(reloaded.count(), 3);
        ARCHETYPE_TEST_EQUAL(reloaded.get(2), string("four"));
        ARCHETYPE_TEST_EQUAL(reloaded.index("five"), 3);
    }

}
