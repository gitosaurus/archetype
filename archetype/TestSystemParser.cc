//
//  TestSystemParser.cpp
//  archetype
//
//  Created by Derek Jones on 5/25/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "TestSystemParser.hh"
#include "TestRegistry.hh"
#include "SystemParser.hh"

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
        
        parser->close();
        parser->parse("Look, above, at the 'bulb'.");
        string n2_str = parser->normalized();
        ARCHETYPE_TEST_EQUAL(n2_str, string(" look above at the bulb "));
    }

    void TestSystemParser::testBasicParsing_() {
        unique_ptr<SystemParser> parser(new SystemParser);

        parser->setMode(SystemParser::VERBS);
        int jump_id = 101;
        parser->addParseable(jump_id, "jump|jump over");

        parser->setMode(SystemParser::NOUNS);
        int dogs_id = 353;
        parser->addParseable(dogs_id, "lazy dogs|dogs");

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
        parser->addParseable(examine_id, "examine|look|x");

        parser->setMode(SystemParser::NOUNS);
        int button_id = 51;
        parser->addParseable(button_id, "button");

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
        parser->addParseable(press_id, "press|push");

        parser->setMode(SystemParser::NOUNS);

        // Put in a "green herring"
        int green_button_id = 80;
        parser->addParseable(green_button_id, "green button|button");

        int red_button_id = 60;
        parser->addParseable(red_button_id, "red button|button");

        int blue_button_id = 70;
        parser->addParseable(blue_button_id, "blue button|button");

        parser->close();

        parser->rollCall();
        parser->announcePresence(blue_button_id);

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
        parser->announcePresence(red_button_id);

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

        // Double-check the results with whichObject
        ARCHETYPE_TEST_EQUAL(parser->whichObject("the red button")->objectConversion()->getObject(), red_button_id);
        ARCHETYPE_TEST_EQUAL(parser->whichObject("green button")->objectConversion()->getObject(), green_button_id);
        ARCHETYPE_TEST_EQUAL(parser->whichObject("a blue button")->objectConversion()->getObject(), blue_button_id);
        ARCHETYPE_TEST_EQUAL(parser->whichObject("button")->objectConversion()->getObject(), red_button_id);
        ARCHETYPE_TEST_EQUAL(parser->whichObject("push")->objectConversion()->getObject(), press_id);
        ARCHETYPE_TEST(!parser->whichObject("dog")->isDefined());
    }

    void TestSystemParser::testSerialization_() {
        // Prepare a parser, bounce through serialization, see it if still works
        unique_ptr<SystemParser> parser(new SystemParser);
        parser->setMode(SystemParser::VERBS);
        int press_id = 50;
        parser->addParseable(press_id, "press|push");

        parser->setMode(SystemParser::NOUNS);

        // Put in a "green herring"
        int green_button_id = 80;
        parser->addParseable(green_button_id, "green button|button");

        int red_button_id = 60;
        parser->addParseable(red_button_id, "red button|button");

        int blue_button_id = 70;
        parser->addParseable(blue_button_id, "blue button|button");

        parser->close();

        parser->rollCall();
        parser->announcePresence(blue_button_id);

        MemoryStorage mem;
        mem << *parser;
        // Wipe out the previous parser, and replace
        parser.reset(new SystemParser);
        mem >> *parser;

        // Verify that the objects are detected and that proximity works as desired
        // Just 'button' by itself should choose the blue one.
        parser->parse("press button");
        Value v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value press_obj = v->objectConversion();
        ARCHETYPE_TEST(press_obj->isDefined());
        ARCHETYPE_TEST_EQUAL(press_obj->getObject(), press_id);

        v = parser->nextObject();
        ARCHETYPE_TEST(v->isDefined());
        Value noun_obj_2 = v->objectConversion();
        ARCHETYPE_TEST(noun_obj_2->isDefined());
        ARCHETYPE_TEST_EQUAL(noun_obj_2->getObject(), blue_button_id);
        v = parser->nextObject();
        ARCHETYPE_TEST(!v->isDefined());
    }

    void TestSystemParser::runTests_() {
        testNormalization_();
        testBasicParsing_();
        testPartialParsing_();
        testProximity_();
        testSerialization_();
    }
}
