//
//  TestSystemParser.cpp
//  archetype
//
//  Created by Derek Jones on 5/25/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "TestSystemParser.h"
#include "TestRegistry.h"
#include "SystemParser.h"
#include "Object.h"

#include <memory>
#include <iostream>

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestSystemParser);

    void TestSystemParser::testNormalization_() {
        unique_ptr<SystemParser> parser(new SystemParser);
        parser->close();
        parser->parse("The quick Brown Fox     jumped over   the lazy    dogs");
        string n1_str = parser->normalized();
        ARCHETYPE_TEST_EQUAL(n1_str, string(" the quick brown fox jumped over the lazy dogs "));
    }
    
    void TestSystemParser::testBasicParsing_() {
        unique_ptr<SystemParser> parser(new SystemParser);

        parser->setMode(SystemParser::VERBS);
        int jump_id = 101;
        ObjectPtr jump(new Object());
        jump->setId(jump_id);
        parser->addParseable(jump, "jump|jump over");

        parser->setMode(SystemParser::NOUNS);
        int dogs_id = 353;
        ObjectPtr dogs(new Object());
        dogs->setId(dogs_id);
        parser->addParseable(dogs, "lazy dogs|dogs");
        
        parser->close();
        
        parser->parse("jump over the lazy dogs");
        list<int> expected_ids = {jump_id, dogs_id};
        Value v = parser->nextObject();
        while (v->isDefined()) {
            Value object_value = v->objectConversion();
            ARCHETYPE_TEST(object_value->isDefined());
            ARCHETYPE_TEST_EQUAL(object_value->getObject(), expected_ids.front());
            expected_ids.pop_front();
            v = parser->nextObject();
        }
    }
    
    void TestSystemParser::runTests_() {
        testNormalization_();
        testBasicParsing_();
    }
}