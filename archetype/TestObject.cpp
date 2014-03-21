//
//  TestObject.cpp
//  archetype
//
//  Created by Derek Jones on 3/17/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>

#include "TestObject.h"
#include "TestRegistry.h"
#include "Universe.h"
#include "Expression.h"
#include "Statement.h"
#include "TokenStream.h"
#include "Expression.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestObject);
    
    inline Expression make_expr_from_str(string src_str) {
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Expression expr = make_expr(token_stream);
        return expr;
    }
    
    inline Statement make_stmt_from_str(string src_str) {
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Statement stmt = make_statement(token_stream);
        return stmt;
    }
    
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
        room_type->setAttribute(desc_id, Value(new StringValue("room")));
        Expression expr = make_expr_from_str("\"an unremarkable \" & desc");
        int full_id = Universe::instance().Identifiers.index("full");
        room_type->setAttribute(full_id, std::move(expr));
        
        ObjectPtr basement = Universe::instance().defineNewObject(room_type->id());
        Universe::instance().assignObjectIdentifier(basement, "basement");
        basement->setAttribute(desc_id, Value(new StringValue("dank cellar of a room")));
        
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
        monster->setAttribute(health_id, Value(new NumericValue(10)));
        Universe::instance().assignObjectIdentifier(monster, "monster");
        Statement kill_stmt = make_stmt_from_str("{\n"
                                                 "health := health - 1\n"
                                                 "write \"The monster is down to \", health, \".\"\n"
                                                 "health }\n");
        int kill_message_id = Universe::instance().Vocabulary.index("kill");
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
        int growl_message_id = Universe::instance().Vocabulary.index("growl");
        animal_type->setAttribute(desc_id, Value(new StringValue("animal")));
        animal_type->setMethod(growl_message_id, std::move(growl_stmt));
        animal_type->setPrototype(true);
        Universe::instance().assignObjectIdentifier(animal_type, "animal");

        ObjectPtr dog = Universe::instance().defineNewObject(animal_type->id());
        Universe::instance().assignObjectIdentifier(dog, "dog");
        dog->setAttribute(desc_id, Value(new StringValue("dog")));
        
        ObjectPtr cat = Universe::instance().defineNewObject(animal_type->id());
        Universe::instance().assignObjectIdentifier(cat, "cat");
        cat->setAttribute(desc_id, Value(new StringValue("cat")));
        Statement meow_stmt = make_stmt_from_str("{ message --> animal; write \"The cat does a double-take.\"}");
        cat->setMethod(growl_message_id, std::move(meow_stmt));
        
        Statement stmt1 = make_stmt_from_str("{ 'growl' -> dog; 'growl' -> cat }");
        ostringstream sout;
        Value val1 = stmt1->execute(sout);
        string expected1 = "The dog growls.\nThe cat growls.\nThe cat does a double-take.\n";
        string actual1 = sout.str();
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
    }
    
    void TestObject::runTests_() {
        testObjects_();
        testInheritance_();
        testMethods_();
        testMessagePassing_();
    }
}