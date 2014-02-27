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
    
    inline Expression expr_from_str(string src_str) {
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Expression expr = form_expr(token_stream);
        return expr;
    }
    
    inline string as_prefix(Expression expr) {
        ostringstream out;
        expr->prefixDisplay(out);
        return out.str();
    }

#define SHOW(expr) cout << #expr << " == " << (expr) << endl;
    
    void TestExpression::runTests_(ostream& out) {
        Expression expr1 = expr_from_str("3 + 4 * 5");
        ARCHETYPE_TEST(expr1 != nullptr);
        string expected1 = "(+ 3 (* 4 5))";
        string actual1 = as_prefix(expr1);
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        int node_count_1 = expr1->nodeCount();
        Expression tight_expr1 = tighten(expr1);
        string tight_actual1 = as_prefix(tighten(expr1));
        ARCHETYPE_TEST_EQUAL(tight_actual1, expected1);
        int node_count_2 = tight_expr1->nodeCount();
        ARCHETYPE_TEST(node_count_1 > node_count_2);
        SHOW(node_count_1);
        SHOW(node_count_2);
        int node_count_3 = tighten(tight_expr1)->nodeCount();
        SHOW(node_count_3);
        ARCHETYPE_TEST_EQUAL(node_count_2, node_count_3);
        
        Expression expr2 = expr_from_str("main.dobj.isARoom");
        ARCHETYPE_TEST(expr2 != nullptr);
        string expected2 = "(. (. main dobj) isARoom)";
        string actual2 = as_prefix(expr2);
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
        
        Expression expr3 = expr_from_str("a &:= b +:= c *:= d -:= 5");
        ARCHETYPE_TEST(expr3 != nullptr);
        string expected3 = "(&:= a (+:= b (*:= c (-:= d 5))))";
        string actual3 = as_prefix(expr3);
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
        
        Expression expr4 = expr_from_str("1 + 2 + 3");
        ARCHETYPE_TEST(expr4 != nullptr);
        string expected4 = "(+ (+ 1 2) 3)";
        string actual4 = as_prefix(expr4);
        ARCHETYPE_TEST_EQUAL(actual4, expected4);
    }
}