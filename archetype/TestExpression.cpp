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
        string actual1 = expr1->evaluate()->toString();
        string expected1 = "Hello, world!";
        SHOW(expected1);
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
    }
    
    void TestExpression::runTests_() {
        testTranslation_();
        testEvaluation_();
    }
}