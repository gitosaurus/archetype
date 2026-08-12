//
//  TestSerialization.cc
//  archetype
//
//  Created by Derek Jones on 6/17/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <array>
#include <iostream>
#include <span>

#include "TestSerialization.hh"
#include "TestRegistry.hh"
#include "Serialization.hh"
#include "StringIdIndex.hh"
#include <stdexcept>

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestSerialization);

    void TestSerialization::runTests_() {
        auto sample_integers = {0, 1, -1, 65, 351, 23487573, -8234924, 19};
        MemoryStorage mem;
        for (auto sample : sample_integers) {
            cout << "Testing serialization of: " << sample << endl;
            mem.writeInteger(sample);
        }
        for (auto sample : sample_integers) {
            ARCHETYPE_TEST(mem.remaining() > 0);
            cout << "Bytes remaining to deserialize: " << mem.remaining() << endl;
            int next_integer = mem.readInteger();
            cout << "Deserialized: " << next_integer << endl;
            ARCHETYPE_TEST_EQUAL(next_integer, sample);
        }
        ARCHETYPE_TEST_EQUAL(mem.remaining(), 0);

        bool threw = false;
        try {
            mem.readInteger();
        } catch (const invalid_argument&) {
            threw = true;
        } catch (...) {
            threw = true;
        }
        ARCHETYPE_TEST(threw);

        testPeek_();
        testFormatHeader_();
        testCountsAreBounded_();
    }

    // A count in the stream is a promise about bytes that follow, and every one
    // of these used to be believed without checking whether the stream could
    // keep it.  The registry case is the one that mattered most: it reached an
    // out-of-bounds write and took the process down with it.
    void TestSerialization::testCountsAreBounded_() {
        auto rejects = [](MemoryStorage& mem) {
            try {
                readCount(mem, "test count");
            } catch (const invalid_argument&) {
                return true;
            }
            return false;
        };

        MemoryStorage negative;
        negative.writeInteger(-99);
        ARCHETYPE_TEST(rejects(negative));

        // Three bytes cannot be followed by a million of anything.
        MemoryStorage overlong;
        overlong.writeInteger(1000000);
        ARCHETYPE_TEST(rejects(overlong));

        // A count the stream can actually back is none of this function's
        // business, and comes through untouched.
        MemoryStorage honest;
        honest.writeInteger(3);
        honest.write(array<Storage::Byte, 3>{'a', 'b', 'c'});
        ARCHETYPE_TEST_EQUAL(readCount(honest, "test count"), 3);

        // A string is the commonest promise of all, and the one an .acx makes
        // hundreds of times.
        MemoryStorage lying;
        lying.writeInteger(500);
        lying.write(array<Storage::Byte, 2>{'h', 'i'});
        string scratch;
        bool threw = false;
        try {
            lying >> scratch;
        } catch (const invalid_argument&) {
            threw = true;
        }
        ARCHETYPE_TEST(threw);

        // The registry: two slots claimed, one record, and that record naming a
        // slot fifty million past the end.  This is the shape that segfaulted.
        MemoryStorage out_of_range;
        out_of_range.writeInteger(2);
        out_of_range.writeInteger(1);
        out_of_range.writeInteger(50000000);
        out_of_range.writeInteger(0);
        StringIdIndex index;
        threw = false;
        try {
            index.read(out_of_range);
        } catch (const invalid_argument&) {
            threw = true;
        }
        ARCHETYPE_TEST(threw);

        // A registry cannot hold more records than it has slots either.
        MemoryStorage overfull;
        overfull.writeInteger(1);
        overfull.writeInteger(2);
        overfull.writeInteger(0);
        overfull.writeInteger(0);
        threw = false;
        try {
            index.read(overfull);
        } catch (const invalid_argument&) {
            threw = true;
        }
        ARCHETYPE_TEST(threw);
    }

    void TestSerialization::testPeek_() {
        MemoryStorage mem;
        mem.writeInteger(4242);
        int written = mem.remaining();

        array<Storage::Byte, 2> looked{};
        ARCHETYPE_TEST_EQUAL(mem.peek(looked), 2);
        // A look that cost nothing:  the bytes are still all there, and the
        // integer still reads back whole.
        ARCHETYPE_TEST_EQUAL(mem.remaining(), written);
        ARCHETYPE_TEST_EQUAL(mem.readInteger(), 4242);
        ARCHETYPE_TEST_EQUAL(mem.remaining(), 0);

        // Peeking past the end reports what was actually available, and still
        // leaves the stream where it found it.
        MemoryStorage tiny;
        tiny.write(span<const Storage::Byte>{FormatCookie.data(), 1});
        array<Storage::Byte, 4> too_many{};
        ARCHETYPE_TEST_EQUAL(tiny.peek(too_many), 1);
        ARCHETYPE_TEST_EQUAL(tiny.remaining(), 1);
    }

    void TestSerialization::testFormatHeader_() {
        MemoryStorage mem;
        writeFormatHeader(mem);
        mem.writeInteger(99);
        ARCHETYPE_TEST_EQUAL(readFormatHeader(mem), CurrentFormatVersion);
        // The header is consumed, so what follows it is next.
        ARCHETYPE_TEST_EQUAL(mem.readInteger(), 99);

        // A stream with no cookie reports the unversioned format and is left
        // exactly as it was, which is what lets an older .acx be read on.
        MemoryStorage headerless;
        headerless.writeInteger(0);
        headerless.writeInteger(77);
        ARCHETYPE_TEST_EQUAL(readFormatHeader(headerless), UnversionedFormat);
        ARCHETYPE_TEST_EQUAL(headerless.readInteger(), 0);
        ARCHETYPE_TEST_EQUAL(headerless.readInteger(), 77);

        // Too short to hold a cookie is not a cookie, and must not be mistaken
        // for one on the strength of the bytes that are there.
        MemoryStorage truncated;
        truncated.write(span<const Storage::Byte>{FormatCookie.data(), 2});
        ARCHETYPE_TEST_EQUAL(readFormatHeader(truncated), UnversionedFormat);
        ARCHETYPE_TEST_EQUAL(truncated.remaining(), 2);

        // Ours, but from the future:  the one case that earns a precise
        // complaint rather than a shrug.
        MemoryStorage from_the_future;
        from_the_future.write(FormatCookie);
        from_the_future.writeInteger(CurrentFormatVersion + 1);
        bool threw = false;
        try {
            readFormatHeader(from_the_future);
        } catch (const invalid_argument&) {
            threw = true;
        }
        ARCHETYPE_TEST(threw);
    }
}
