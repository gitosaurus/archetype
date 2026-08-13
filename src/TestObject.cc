//
//  TestObject.cc
//  archetype
//
//  Created by Derek Jones on 3/17/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>

#include "TestObject.hh"
#include "TestRegistry.hh"
#include "Universe.hh"
#include "Expression.hh"
#include "Statement.hh"
#include "TokenStream.hh"
#include "Expression.hh"
#include "Capture.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestObject);

    void TestObject::testObjects_() {
        ObjectPtr test = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(test, "test");
        int seven_id = Universe::instance().Identifiers.index("seven");
        test->setAttribute(seven_id, make_expr_from_str("3 + 4"));
        Expression expr1 = make_expr_from_str("test.seven + 5");
        Value val1 = expr1->evaluate()->numericConversion();
        ARCHETYPE_TEST(val1->isDefined());
        int actual1 = val1->getNumber();
        int expected1 = 12;
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        ObjectPtr test2 = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(test2, "test2");
        int another_id = Universe::instance().Identifiers.index("another");
        test->setAttribute(another_id, make_expr_from_str("test2"));
        Expression expr2 = make_expr_from_str("test.another");
        Value val2 = expr2->evaluate()->objectConversion();
        ARCHETYPE_TEST(val2->isDefined());
        int actual2 = val2->getObject();
        int expected2 = test2->id();
        ARCHETYPE_TEST_EQUAL(actual2, expected2);

        Expression expr3 = make_expr_from_str("test.seven := 77");
        // How is an expression like a statement?  When it's an assignment
        expr3->evaluate();
        Expression expr4 = make_expr_from_str("test.seven");
        Value val4 = expr4->evaluate()->numericConversion();
        ARCHETYPE_TEST(val4->isDefined());
        int actual4 = val4->getNumber();
        int expected4 = 77;
        ARCHETYPE_TEST_EQUAL(actual4, expected4);

        Expression expr5 = make_expr_from_str("test.someone := test2");
        expr5->evaluate();
        int someone_id = Universe::instance().Identifiers.index("someone");
        Value val5 = test->getAttributeValue(someone_id)->objectConversion();
        ARCHETYPE_TEST(val5->isDefined());
        int actual5 = val5->getObject();
        int expected5 = test2->id();
        ARCHETYPE_TEST_EQUAL(actual5, expected5);
    }

    void TestObject::testInheritance_() {
        ObjectPtr room_type = Universe::instance().defineNewObject();
        room_type->setPrototype(true);
        Universe::instance().assignObjectIdentifier(room_type, "room");
        int desc_id = Universe::instance().Identifiers.index("desc");
        room_type->setAttribute(desc_id, make_unique<StringValue>("room"));
        Expression expr = make_expr_from_str("\"an unremarkable \" & desc");
        int full_id = Universe::instance().Identifiers.index("full");
        room_type->setAttribute(full_id, std::move(expr));

        ObjectPtr basement = Universe::instance().defineNewObject(room_type->id());
        Universe::instance().assignObjectIdentifier(basement, "basement");
        basement->setAttribute(desc_id, make_unique<StringValue>("dank cellar of a room"));

        ObjectPtr courtyard = Universe::instance().defineNewObject(room_type->id());
        Universe::instance().assignObjectIdentifier(courtyard, "courtyard");

        Expression expr_b_1 = make_expr_from_str("courtyard.desc");
        Value val_b_1 = expr_b_1->evaluate()->stringConversion();
        ARCHETYPE_TEST(val_b_1->isDefined());
        string actual1 = val_b_1->getString();
        string expected1 = "room";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        Expression expr_c_1 = make_expr_from_str("basement.desc");
        Value val_c_1 = expr_c_1->evaluate()->stringConversion();
        ARCHETYPE_TEST(val_c_1->isDefined());
        string actual2 = val_c_1->getString();
        string expected2 = "dank cellar of a room";
        ARCHETYPE_TEST_EQUAL(actual2, expected2);

        Expression expr_b_2 = make_expr_from_str("basement.full");
        Value val_b_2 = expr_b_2->evaluate()->stringConversion();
        ARCHETYPE_TEST(val_b_2->isDefined());
        string expected3 = "an unremarkable dank cellar of a room";
        string actual3 = val_b_2->getString();
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
    }

    void TestObject::testMethods_() {
        ObjectPtr monster = Universe::instance().defineNewObject();
        int health_id = Universe::instance().Identifiers.index("health");
        monster->setAttribute(health_id, make_unique<NumericValue>(10));
        Universe::instance().assignObjectIdentifier(monster, "monster");
        Statement kill_stmt = make_stmt_from_str("{\n"
                                                 "health := health - 1\n"
                                                 "write \"The monster is down to \", health, \".\"\n"
                                                 "health }\n");
        int kill_message_id = Universe::instance().Messages.index("kill");
        monster->setMethod(kill_message_id, std::move(kill_stmt));
        Expression expr1 = make_expr_from_str("'kill' -> monster");
        Value val1 = expr1->evaluate()->numericConversion();
        ARCHETYPE_TEST(val1->isDefined());
        int expected1 = 9;
        int actual1 = val1->getNumber();
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
    }

    void TestObject::testMessagePassing_() {
        ObjectPtr animal_type = Universe::instance().defineNewObject();
        int desc_id = Universe::instance().Identifiers.index("desc");
        Statement growl_stmt = make_stmt_from_str("write \"The \", desc, \" growls.\"");
        int growl_message_id = Universe::instance().Messages.index("growl");
        animal_type->setAttribute(desc_id, make_unique<StringValue>("animal"));
        animal_type->setMethod(growl_message_id, std::move(growl_stmt));
        animal_type->setPrototype(true);
        Universe::instance().assignObjectIdentifier(animal_type, "animal");

        ObjectPtr dog = Universe::instance().defineNewObject(animal_type->id());
        Universe::instance().assignObjectIdentifier(dog, "dog");
        dog->setAttribute(desc_id, make_unique<StringValue>("dog"));

        ObjectPtr cat = Universe::instance().defineNewObject(animal_type->id());
        Universe::instance().assignObjectIdentifier(cat, "cat");
        cat->setAttribute(desc_id, make_unique<StringValue>("cat"));
        Statement meow_stmt = make_stmt_from_str("{ message --> animal; write \"The cat does a double-take.\"}");
        cat->setMethod(growl_message_id, std::move(meow_stmt));

        Statement stmt1 = make_stmt_from_str("{ 'growl' -> dog; 'growl' -> cat }");
        Capture capture1;
        Value val1 = stmt1->execute();
        string expected1 = "The dog growls.\nThe cat growls.\nThe cat does a double-take.\n";
        string actual1 = capture1.getCapture();
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        // Test that the single arrow works as a pass with types
        ObjectPtr goat = Universe::instance().defineNewObject(animal_type->id());
        Universe::instance().assignObjectIdentifier(goat, "goat");
        goat->setAttribute(desc_id, make_unique<StringValue>("goat"));
        Statement baa_stmt = make_stmt_from_str("{ message -> animal; write \"The goat coughs, embarrassed.\" }");
        goat->setMethod(growl_message_id, std::move(baa_stmt));

        Statement stmt2 = make_stmt_from_str("'growl' -> goat");
        Capture capture2;
        string expected2 = "The goat growls.\nThe goat coughs, embarrassed.\n";
        stmt2->execute();
        string actual2 = capture2.getCapture();
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
    }

    void TestObject::testListMessages_() {
        ObjectPtr vase = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(vase, "vase");
        int move_to_id = Universe::instance().Messages.index("MOVE TO");
        // The parentheses around "head tail message" are not decoration: head and
        // tail bind more loosely than "=", so without them the comparison would
        // happen first, against the message itself
        vase->setMethod(move_to_id,
                        make_stmt_from_str("{ if (head tail message) = UNDEFINED then {\n"
                                           "    write \"the vase stays put\"\n"
                                           "  } else {\n"
                                           "    write \"the vase moves to \", head tail message\n"
                                           "  } }\n"));
        // The default method sees the message the sender wrote, list and all
        vase->setMethod(DefaultMethod,
                        make_stmt_from_str("write \"the vase ignores \", head message"));

        Statement stmt1 = make_stmt_from_str("['MOVE TO' \"the attic\"] -> vase");
        Capture capture1;
        stmt1->execute();
        string expected1 = "the vase moves to the attic\n";
        string actual1 = capture1.getCapture();
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        // A plain message still dispatches, and asking it for arguments is not an
        // error; a list with nothing after its head is the very same silence.
        Statement stmt2 = make_stmt_from_str("{ 'MOVE TO' -> vase; ['MOVE TO'] -> vase }");
        Capture capture2;
        stmt2->execute();
        string expected2 = "the vase stays put\nthe vase stays put\n";
        string actual2 = capture2.getCapture();
        ARCHETYPE_TEST_EQUAL(actual2, expected2);

        // An unclaimed head falls to the default method, as an unclaimed message
        // always has; a head that is no kind of message falls there too.
        Statement stmt3 = make_stmt_from_str("{ ['SHATTER' 3] -> vase; [42 7] -> vase }");
        Capture capture3;
        stmt3->execute();
        string expected3 = "the vase ignores SHATTER\nthe vase ignores 42\n";
        string actual3 = capture3.getCapture();
        ARCHETYPE_TEST_EQUAL(actual3, expected3);

        // The reply comes back from a list message like any other, which is what
        // lets a method refuse an argument instead of undoing it
        int bump_id = Universe::instance().Messages.index("BUMP");
        vase->setMethod(bump_id, make_stmt_from_str("(head tail message) + 1"));
        Expression expr4 = make_expr_from_str("['BUMP' 41] -> vase");
        Value val4 = expr4->evaluate()->numericConversion();
        ARCHETYPE_TEST(val4->isDefined());
        ARCHETYPE_TEST_EQUAL(val4->getNumber(), 42);

        // Passing hands the whole list up, so arguments survive the trip to a
        // parent's method without anyone naming them along the way
        ObjectPtr furniture = Universe::instance().defineNewObject();
        furniture->setPrototype(true);
        Universe::instance().assignObjectIdentifier(furniture, "furniture");
        furniture->setMethod(move_to_id,
                             make_stmt_from_str("write \"the furniture moves to \", head tail message"));
        ObjectPtr crate = Universe::instance().defineNewObject(furniture->id());
        Universe::instance().assignObjectIdentifier(crate, "crate");
        crate->setMethod(move_to_id,
                         make_stmt_from_str("{ message --> furniture; write \"the crate settles\" }"));

        Statement stmt5 = make_stmt_from_str("['MOVE TO' \"the cellar\"] -> crate");
        Capture capture5;
        stmt5->execute();
        string expected5 = "the furniture moves to the cellar\nthe crate settles\n";
        string actual5 = capture5.getCapture();
        ARCHETYPE_TEST_EQUAL(actual5, expected5);
    }

    void TestObject::testReadDoesNotMutate_() {
        ObjectPtr subject = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(subject, "subject");
        int ghost_id = Universe::instance().Identifiers.index("ghost");

        ARCHETYPE_TEST(not subject->hasAttribute(ghost_id));

        Expression expr = make_expr_from_str("subject.ghost");
        Value val = expr->evaluate()->valueConversion();
        ARCHETYPE_TEST(not val->isDefined());

        ARCHETYPE_TEST(not subject->hasAttribute(ghost_id));

        expr->evaluate();
        expr->evaluate();
        ARCHETYPE_TEST(not subject->hasAttribute(ghost_id));

        ObjectPtr animal = Universe::instance().defineNewObject();
        animal->setPrototype(true);
        Universe::instance().assignObjectIdentifier(animal, "animal");
        int legs_id = Universe::instance().Identifiers.index("legs");
        animal->setAttribute(legs_id, make_unique<NumericValue>(4));

        ObjectPtr dog = Universe::instance().defineNewObject(animal->id());
        Universe::instance().assignObjectIdentifier(dog, "dog");

        ARCHETYPE_TEST(dog->hasAttribute(legs_id));
        ARCHETYPE_TEST(not dog->hasLocalAttribute(legs_id));

        Expression inherited = make_expr_from_str("dog.legs");
        Value inherited_val = inherited->evaluate()->numericConversion();
        ARCHETYPE_TEST_EQUAL(inherited_val->getNumber(), 4);

        ARCHETYPE_TEST(not dog->hasLocalAttribute(legs_id));
    }

    void TestObject::runTests_() {
        testObjects_();
        testInheritance_();
        testMethods_();
        testMessagePassing_();
        testListMessages_();
        testReadDoesNotMutate_();
    }
}
