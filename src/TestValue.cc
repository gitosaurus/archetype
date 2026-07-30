//
//  TestValue.cc
//  archetype
//
//  Created by Derek Jones on 6/30/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <string>

#include "TestValue.hh"
#include "TestRegistry.hh"
#include "Value.hh"
#include "Serialization.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestValue);

    string display(const Value& v) {
        ostringstream out;
        v->display(out);
        return out.str();
    }

    void TestValue::testSerialization_() {
        initializer_list<Value> samples = {
            make_unique<UndefinedValue>(),
            make_unique<StringValue>("Hello, world"),
            make_unique<StringValue>(""),
            make_unique<StringValue>(" "),
            make_unique<BreakValue>(),
            make_unique<MessageValue>(88),
            make_unique<NumericValue>(42),
            make_unique<BooleanValue>(true),
            make_unique<BooleanValue>(false),
            make_unique<AbsentValue>(),
            make_unique<IdentifierValue>(13),
            make_unique<ObjectValue>(9),
            make_unique<AttributeValue>(9, 13)
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
        Value number_192 = make_unique<NumericValue>(192);
        Value string_192{number_192->stringConversion()};
        ARCHETYPE_TEST_EQUAL(string_192->getString(), string{"192"});
        Value number_192_back{string_192->numericConversion()};
        ARCHETYPE_TEST_EQUAL(number_192_back->getNumber(), 192);

        Value false_value = make_unique<BooleanValue>(false);
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
        Value a = make_unique<NumericValue>(1);
        Value b = make_unique<NumericValue>(2);
        Value ab = make_unique<PairValue>(std::move(a), std::move(b));
        string actual = display(ab);
        string expected = "(1 @ 2)";
        ARCHETYPE_TEST_EQUAL(actual, expected);

        // Now create a couple of short lists
        Value node1 = make_unique<PairValue>(make_unique<StringValue>("world"), make_unique<UndefinedValue>());
        actual = display(node1);
        expected = "[\"world\"]";
        ARCHETYPE_TEST_EQUAL(actual, expected);
        Value node2 = make_unique<PairValue>(make_unique<StringValue>("hello"), std::move(node1));
        actual = display(node2);
        expected = "[\"hello\" \"world\"]";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    void TestValue::runTests_() {
        testSerialization_();
        testConversion_();
        testPairs_();
    }
}
