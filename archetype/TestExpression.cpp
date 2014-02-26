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
    
    inline string as_prefix(Expression expr) {
        ostringstream out;
        expr->prefixDisplay(out);
        return out.str();
    }
    
    void TestExpression::runTests_(ostream& out) {
        string src_str = "3 + 4 * 5";
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Expression expr = form_expr(token_stream);
        ARCHETYPE_TEST(expr != nullptr);
        string expected1 = "(+ 3 (* 4 5))";
        string actual1 = as_prefix(expr);
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
    }
}