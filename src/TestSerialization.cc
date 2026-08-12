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
