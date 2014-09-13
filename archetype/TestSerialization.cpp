//
//  TestSerialization.cpp
//  archetype
//
//  Created by Derek Jones on 6/17/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>

#include "TestSerialization.h"
#include "TestRegistry.h"
#include "Serialization.h"

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
    }
}
