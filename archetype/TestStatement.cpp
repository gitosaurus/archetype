//
//  TestStatement.cpp
//  archetype
//
//  Created by Derek Jones on 2/28/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>

#include "TestStatement.h"
#include "TestRegistry.h"
#include "Statement.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestStatement);
    
    inline Statement make_stmt_from_str(string src_str) {
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Statement stmt = make_statement(token_stream);
        return stmt;
    }
    
    void TestStatement::runTests_(ostream& out) {
        Statement stmt1 = make_stmt_from_str("{}");
        ARCHETYPE_TEST(stmt1 != nullptr);
        CompoundStatement* dstmt1 = dynamic_cast<CompoundStatement*>(stmt1.get());
        ARCHETYPE_TEST(dstmt1 != nullptr);
        ARCHETYPE_TEST_EQUAL(dstmt1->statements().size(), size_t(0));
        
        Statement stmt2 = make_stmt_from_str("guard.location := main.dobj");
        ARCHETYPE_TEST(stmt2 != nullptr);
        ExpressionStatement* dstmt2 = dynamic_cast<ExpressionStatement*>(stmt2.get());
        ARCHETYPE_TEST(dstmt2 != nullptr);
        Expression expr = dstmt2->expression();
        ARCHETYPE_TEST_EQUAL(expr->nodeCount(), 7);
        
        Statement stmt3 = make_stmt_from_str("if player.location = dungeon then \"Oops!\" else \"Nice.\"");
        ARCHETYPE_TEST(stmt3 != nullptr);
        IfStatement* dstmt3 = dynamic_cast<IfStatement*>(stmt3.get());
        ARCHETYPE_TEST(dstmt3 != nullptr);
    }
}