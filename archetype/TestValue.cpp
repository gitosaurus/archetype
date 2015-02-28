//
//  TestValue.cpp
//  archetype
//
//  Created by Derek Jones on 6/30/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <string>

#include "TestValue.h"
#include "TestRegistry.h"
#include "Value.h"
#include "Serialization.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestValue);

    string display(const Value& v) {
        ostringstream out;
        v->display(out);
        return out.str();
    }

    void TestValue::testSerialization_() {
        auto samples = {
            Value{new UndefinedValue},
            Value{new StringValue{"Hello, world"}},
            Value{new StringValue{""}},
            Value{new StringValue{" "}},
            Value{new BreakValue},
            Value{new MessageValue{88}},
            Value{new NumericValue{42}},
            Value{new BooleanValue{true}},
            Value{new BooleanValue{false}},
            Value{new AbsentValue},
            Value{new IdentifierValue{13}},
            Value{new ObjectValue{9}},
            Value{new AttributeValue{9, 13}}
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

    void TestValue::testConversion_() {
        Value number_192{new NumericValue{192}};
        Value string_192{number_192->stringConversion()};
        ARCHETYPE_TEST_EQUAL(string_192->getString(), string{"192"});
        Value number_192_back{string_192->numericConversion()};
        ARCHETYPE_TEST_EQUAL(number_192_back->getNumber(), 192);

        Value false_value{new BooleanValue{false}};
        Value string_false{false_value->stringConversion()};
        ARCHETYPE_TEST_EQUAL(string_false->getString(), string{"FALSE"});
        Value number_false{false_value->numericConversion()};
        ARCHETYPE_TEST_EQUAL(number_false->getNumber(), 0);

        // Most other conversions require going through the Universe,
        // though this makes it much harder to test Value by itself
        Universe::destroy();
        // set up objects, identifiers, messages
    }

    void TestValue::testPairs_() {
        Value a{new NumericValue{1}};
        Value b{new NumericValue{2}};
        Value ab{new PairValue{move(a), move(b)}};
        string actual = display(ab);
        string expected = "(1 @ 2)";
        ARCHETYPE_TEST_EQUAL(actual, expected);

        // Now create a couple of short lists
        Value node1{new PairValue{move(Value{new StringValue{"world"}}), move(Value{new UndefinedValue})}};
        actual = display(node1);
        expected = "{\"world\"}";
        ARCHETYPE_TEST_EQUAL(actual, expected);
        Value node2{new PairValue{move(Value{new StringValue{"hello"}}), move(node1)}};
        actual = display(node2);
        expected = "{\"hello\" \"world\"}";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    void TestValue::runTests_() {
        testSerialization_();
        testConversion_();
        testPairs_();
    }
}
