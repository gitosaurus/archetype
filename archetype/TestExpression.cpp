//
//  TestExpression.cpp
//  archetype
//
//  Created by Derek Jones on 2/25/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "TestExpression.h"
#include "TestRegistry.h"
#include "TokenStream.h"
#include "Expression.h"
#include "GameDefinition.h"

#include <string>
#include <sstream>

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestExpression);
    
    inline Expression form_expr_from_str(string src_str) {
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Expression expr = form_expr(token_stream);
        return expr;
    }
    
    inline Expression make_expr_from_str(string src_str) {
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Expression expr = make_expr(token_stream);
        return expr;
    }
    
    inline string as_prefix(const Expression& expr) {
        ostringstream out;
        expr->prefixDisplay(out);
        return out.str();
    }

#define SHOW(expr) out() << #expr << " == " << (expr) << endl;
    
    void TestExpression::testTranslation_() {
        string expr_str_1 = "3 + 4 * 5";
        Expression expr1 = form_expr_from_str(expr_str_1);
        ARCHETYPE_TEST(expr1 != nullptr);
        string expected1 = "(+ 3 (* 4 5))";
        string actual1 = as_prefix(expr1);
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        int node_count_1 = expr1->nodeCount();
        Expression tight_expr1 = move(tighten(move(expr1)));
        string tight_actual1 = as_prefix(tight_expr1);
        ARCHETYPE_TEST_EQUAL(tight_actual1, expected1);
        int node_count_2 = tight_expr1->nodeCount();
        ARCHETYPE_TEST(node_count_1 > node_count_2);
        SHOW(node_count_1);
        SHOW(node_count_2);
        tight_expr1 = move(tighten(move(tight_expr1)));
        int node_count_3 = tight_expr1->nodeCount();
        SHOW(node_count_3);
        ARCHETYPE_TEST_EQUAL(node_count_2, node_count_3);
        Expression mexpr1 = make_expr_from_str(expr_str_1);
        ARCHETYPE_TEST_EQUAL(mexpr1->nodeCount(), node_count_2);
        ARCHETYPE_TEST_EQUAL(as_prefix(mexpr1), expected1);

        
        Expression expr2 = make_expr_from_str("main.dobj.isARoom");
        ARCHETYPE_TEST(expr2 != nullptr);
        string expected2 = "(. (. main dobj) isARoom)";
        string actual2 = as_prefix(expr2);
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
        
        Expression expr3 = make_expr_from_str("a &:= b +:= c *:= d -:= 5");
        ARCHETYPE_TEST(expr3 != nullptr);
        string expected3 = "(&:= a (+:= b (*:= c (-:= d 5))))";
        string actual3 = as_prefix(expr3);
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
        
        Expression expr4 = make_expr_from_str("1 + 2 + 3");
        ARCHETYPE_TEST(expr4 != nullptr);
        string expected4 = "(+ (+ 1 2) 3)";
        string actual4 = as_prefix(expr4);
        ARCHETYPE_TEST_EQUAL(actual4, expected4);
        
        // Newlines don't get in the way of this expression
        Expression expr5 = make_expr_from_str("5 + \n    9 - (\n3 + 7) * 4");
        string actual5 = as_prefix(expr5);
        string expected5 = "(- (+ 5 9) (* (+ 3 7) 4))";
        ARCHETYPE_TEST_EQUAL(actual5, expected5);
        
        // Here, newlines stop the expression
        Expression expr6 = make_expr_from_str("3 + 5\n+6");
        string actual6 = as_prefix(expr6);
        string expected6 = "(+ 3 5)";
        ARCHETYPE_TEST_EQUAL(actual6, expected6);
    }
    
    void TestExpression::testEvaluation_() {
        Expression expr1 = make_expr_from_str("\"Hello,\" & \" \" & \"world!\"");
        string actual1 = expr1->evaluate()->stringConversion()->getString();
        string expected1 = "Hello, world!";
        SHOW(expected1);
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        
        Expression expr2 = make_expr_from_str("3 + 4 * 5");
        int actual2 = expr2->evaluate()->numericConversion()->getNumber();
        int expected2 = 23;
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
        
        Expression expr3 = make_expr_from_str("\"Hiya\" leftfrom 2");
        string actual3 = expr3->evaluate()->stringConversion()->getString();
        string expected3 = "Hi";
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
        
        Expression expr4 = make_expr_from_str("\"Hiya\" rightfrom 3");
        string actual4 = expr4->evaluate()->stringConversion()->getString();
        string expected4 = "ya";
        ARCHETYPE_TEST_EQUAL(actual4, expected4);
        
        Expression expr5 = make_expr_from_str("\"Hi\" rightfrom 5");
        string actual5 = expr5->evaluate()->stringConversion()->getString();
        string expected5 = "";
        ARCHETYPE_TEST_EQUAL(actual5, expected5);
        
        Expression expr6 = make_expr_from_str("\"\" rightfrom 1");
        string actual6 = expr6->evaluate()->stringConversion()->getString();
        string expected6 = "";
        ARCHETYPE_TEST_EQUAL(actual6, expected6);
        
        Expression expr7 = make_expr_from_str("5 = 6");
        int actual7 = expr7->evaluate()->numericConversion()->getNumber();
        int expected7 = 0;
        ARCHETYPE_TEST_EQUAL(actual7, expected7);
        
        Expression expr8 = make_expr_from_str("\"Hello\" = \"World\"");
        int actual8 = expr8->evaluate()->numericConversion()->getNumber();
        int expected8 = 0;
        ARCHETYPE_TEST_EQUAL(actual8, expected8);
        
        Expression expr9 = make_expr_from_str("5 + 7 = \"1\" & \"2\"");
        int actual9 = expr9->evaluate()->numericConversion()->getNumber();
        int expected9 = 1;
        ARCHETYPE_TEST_EQUAL(actual9, expected9);
        
        Expression expr10 = make_expr_from_str("2 ~= 3");
        int actual10 = expr10->evaluate()->numericConversion()->getNumber();
        int expected10 = 1;
        ARCHETYPE_TEST_EQUAL(actual10, expected10);
        
        Expression expr11 = make_expr_from_str("\"35\" = \" 35 \"");
        int actual11 = expr11->evaluate()->numericConversion()->getNumber();
        int expected11 = 0;
        ARCHETYPE_TEST_EQUAL(actual11, expected11);
        
        Expression expr12 = make_expr_from_str("\"35\" = \"35X\"");
        string actual12 = expr12->evaluate()->stringConversion()->getString();
        string expected12 = "FALSE";
        ARCHETYPE_TEST_EQUAL(actual12, expected12);
    }
    
    void TestExpression::testObjects_() {
        ObjectPtr test = GameDefinition::instance().defineNewObject();
        GameDefinition::instance().assignObjectIdentifier(test, "test");
        int seven_id = GameDefinition::instance().Identifiers.index("seven");
        test->setAttribute(seven_id, make_expr_from_str("3 + 4"));
        Expression expr1 = make_expr_from_str("test.seven + 5");
        Value val1 = expr1->evaluate()->numericConversion();
        ARCHETYPE_TEST(val1->isDefined());
        int actual1 = val1->getNumber();
        int expected1 = 12;
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        
        ObjectPtr test2 = GameDefinition::instance().defineNewObject();
        GameDefinition::instance().assignObjectIdentifier(test2, "test2");
        int another_id = GameDefinition::instance().Identifiers.index("another");
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
        int someone_id = GameDefinition::instance().Identifiers.index("someone");
        Value val5 = test->getAttributeValue(someone_id)->objectConversion();
        ARCHETYPE_TEST(val5->isDefined());
        int actual5 = val5->getObject();
        int expected5 = test2->id();
        ARCHETYPE_TEST_EQUAL(actual5, expected5);
    }
    
    void TestExpression::testInheritance_() {
        ObjectPtr room_type = GameDefinition::instance().defineNewType();
        GameDefinition::instance().assignTypeIdentifier(room_type, "room");
        int desc_id = GameDefinition::instance().Identifiers.index("desc");
        room_type->setAttribute(desc_id, Value(new StringValue("room")));
        Expression expr = make_expr_from_str("\"an unremarkable \" & desc");
        int full_id = GameDefinition::instance().Identifiers.index("full");
        room_type->setAttribute(full_id, std::move(expr));
        
        ObjectPtr basement = GameDefinition::instance().defineNewObject(room_type->id());
        GameDefinition::instance().assignObjectIdentifier(basement, "basement");
        basement->setAttribute(desc_id, Value(new StringValue("dank cellar of a room")));
        
        ObjectPtr courtyard = GameDefinition::instance().defineNewObject(room_type->id());
        GameDefinition::instance().assignObjectIdentifier(courtyard, "courtyard");
        
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
    
    void TestExpression::runTests_() {
        testTranslation_();
        testEvaluation_();
        testObjects_();
        testInheritance_();
    }
}