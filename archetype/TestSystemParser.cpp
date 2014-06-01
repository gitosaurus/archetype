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
        ObjectPtr jump(new Object);
        jump->setId(jump_id);
        parser->addParseable(jump, "jump|jump over");

        parser->setMode(SystemParser::NOUNS);
        int dogs_id = 353;
        ObjectPtr dogs(new Object);
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
    
    void TestSystemParser::testPartialParsing_() {
        unique_ptr<SystemParser> parser(new SystemParser);
        
        parser->setMode(SystemParser::VERBS);
        int examine_id = 50;
        ObjectPtr examine(new Object);
        examine->setId(examine_id);
        parser->addParseable(examine, "examine|look|x");
        
        parser->setMode(SystemParser::NOUNS);
        int button_id = 51;
        ObjectPtr button(new Object);
        button->setId(button_id);
        parser->addParseable(button, "button");
        
        parser->close();
        
        parser->parse("examine the paper");
        Value v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value examine_obj = v->objectConversion();
        ARCHETYPE_TEST(examine_obj->isDefined());
        ARCHETYPE_TEST_EQUAL(examine_obj->getObject(), examine_id);
        
        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value paper_obj = v->objectConversion();
        ARCHETYPE_TEST(!paper_obj->isDefined());
        Value paper_str = v->stringConversion();
        ARCHETYPE_TEST(paper_str->isDefined());
        ARCHETYPE_TEST_EQUAL(paper_str->getString(), string("paper"));
        
        v = parser->nextObject();
        ARCHETYPE_TEST(!v->isDefined());

        parser->parse("press button");
        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value press_obj = v->objectConversion();
        ARCHETYPE_TEST(!press_obj->isDefined());
        Value press_str = v->stringConversion();
        ARCHETYPE_TEST(press_str->isDefined());
        ARCHETYPE_TEST_EQUAL(press_str->getString(), string("press"));
        
        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value button_obj = v->objectConversion();
        ARCHETYPE_TEST(button_obj->isDefined());
        ARCHETYPE_TEST_EQUAL(button_obj->getObject(), button_id);
        
        v = parser->nextObject();
        ARCHETYPE_TEST(!v->isDefined());
    }
    
    void TestSystemParser::testProximity_() {
        unique_ptr<SystemParser> parser(new SystemParser);

        parser->setMode(SystemParser::VERBS);
        int press_id = 50;
        ObjectPtr examine(new Object);
        examine->setId(press_id);
        parser->addParseable(examine, "press|push");
        
        parser->setMode(SystemParser::NOUNS);
        
        // Put in a "green herring"
        int green_button_id = 80;
        ObjectPtr green_button(new Object);
        green_button->setId(green_button_id);
        parser->addParseable(green_button, "green button|button");
        
        int red_button_id = 60;
        ObjectPtr red_button(new Object);
        red_button->setId(red_button_id);
        parser->addParseable(red_button, "red button|button");
        
        int blue_button_id = 70;
        ObjectPtr blue_button(new Object);
        blue_button->setId(blue_button_id);
        parser->addParseable(blue_button, "blue button|button");
        
        parser->close();
        
        parser->rollCall();
        parser->announcePresence(blue_button);
        
        // This should choose the red one even though it's not nearby
        parser->parse("press the red button");
        Value v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value press_obj = v->objectConversion();
        ARCHETYPE_TEST(press_obj->isDefined());
        ARCHETYPE_TEST_EQUAL(press_obj->getObject(), press_id);
        
        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value noun_obj_1 = v->objectConversion();
        ARCHETYPE_TEST(noun_obj_1->isDefined());
        ARCHETYPE_TEST_EQUAL(noun_obj_1->getObject(), red_button_id);
        v = parser->nextObject();
        ARCHETYPE_TEST(!v->isDefined());
        
        // But just 'button' by itself should choose blue.
        parser->parse("press button");
        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        press_obj = v->objectConversion();
        ARCHETYPE_TEST(press_obj->isDefined());
        ARCHETYPE_TEST_EQUAL(press_obj->getObject(), press_id);
        
        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value noun_obj_2 = v->objectConversion();
        ARCHETYPE_TEST(noun_obj_2->isDefined());
        ARCHETYPE_TEST_EQUAL(noun_obj_2->getObject(), blue_button_id);
        v = parser->nextObject();
        ARCHETYPE_TEST(!v->isDefined());
        
        // New roll call:  now the red is near, the same phrase should match red instead.
        parser->rollCall();
        parser->announcePresence(red_button);
        
        parser->parse("press button");
        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        press_obj = v->objectConversion();
        ARCHETYPE_TEST(press_obj->isDefined());
        ARCHETYPE_TEST_EQUAL(press_obj->getObject(), press_id);

        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value noun_obj_3 = v->objectConversion();
        ARCHETYPE_TEST(noun_obj_3->isDefined());
        ARCHETYPE_TEST_EQUAL(noun_obj_3->getObject(), red_button_id);
        v = parser->nextObject();
        ARCHETYPE_TEST(!v->isDefined());
    }
    
    void TestSystemParser::runTests_() {
        testNormalization_();
        testBasicParsing_();
        testPartialParsing_();
        testProximity_();
    }
}