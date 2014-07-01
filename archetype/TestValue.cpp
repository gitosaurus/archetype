//
//  TestValue.cpp
//  archetype
//
//  Created by Derek Jones on 6/30/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>

#include "TestValue.h"
#include "TestRegistry.h"
#include "Value.h"
#include "Serialization.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestValue);
    
    void TestValue::testSerialization_() {
        auto samples = {
            Value(new UndefinedValue),
            Value(new StringValue("Hello, world")),
            Value(new MessageValue(88)),
            Value(new NumericValue(42)),
            Value(new ReservedConstantValue(Keywords::RW_TRUE)),
            Value(new IdentifierValue(13)),
            Value(new ObjectValue(9)),
            Value(new AttributeValue(9, 13))
        };
        MemoryStorage mem;
        for (auto const &v : samples) {
            mem << v;
        }
        for (auto const& v : samples) {
            Value read_back;
            mem >> read_back;
            ARCHETYPE_TEST(read_back->isSameValueAs(v));
        }
        cout << "Value serialization test finished." << endl;
    }
    
    void TestValue::runTests_() {
        testSerialization_();
    }
}