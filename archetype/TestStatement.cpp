//
//  TestStatement.cpp
//  archetype
//
//  Created by Derek Jones on 2/28/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>
#include <list>
#include <utility>

#include "TestStatement.h"
#include "TestRegistry.h"
#include "Statement.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestStatement);
    
    inline Statement make_stmt_from_str(string src_str) {
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr src(new SourceFile("test", in));
        TokenStream token_stream(src);
        Statement stmt = make_statement(token_stream);
        return stmt;
    }
    
    void TestStatement::testConstruction_() {
        Statement stmt1 = make_stmt_from_str("{}");
        ARCHETYPE_TEST(stmt1 != nullptr);
        CompoundStatement* dstmt1 = dynamic_cast<CompoundStatement*>(stmt1.get());
        ARCHETYPE_TEST(dstmt1 != nullptr);
        ARCHETYPE_TEST_EQUAL(dstmt1->statements().size(), size_t(0));
        
        Statement stmt2 = make_stmt_from_str("guard.location := main.dobj");
        ARCHETYPE_TEST(stmt2 != nullptr);
        ExpressionStatement* dstmt2 = dynamic_cast<ExpressionStatement*>(stmt2.get());
        ARCHETYPE_TEST(dstmt2 != nullptr);
        const Expression& expr = dstmt2->expression();
        ARCHETYPE_TEST_EQUAL(expr->nodeCount(), 7);
        
        Statement stmt3 = make_stmt_from_str("if player.location = dungeon then stop \"Oops!\" else write \"Nice.\"");
        ARCHETYPE_TEST(stmt3 != nullptr);
        IfStatement* dstmt3 = dynamic_cast<IfStatement*>(stmt3.get());
        ARCHETYPE_TEST(dstmt3 != nullptr);
        
        Statement stmt4 = make_stmt_from_str("case timer +:= 1 of { 3 : write \"Careful\" 4 : stop \"Dead!\" default : treasure +:= 1 }");
        ARCHETYPE_TEST(stmt4 != nullptr);
        CaseStatement* dstmt4 = dynamic_cast<CaseStatement*>(stmt4.get());
        ARCHETYPE_TEST(dstmt4 != nullptr);
    }
    
    void TestStatement::testExecution_() {
        Statement stmt1 = make_stmt_from_str("{ 3 + \"7\"; 12 + 34 }");
        ARCHETYPE_TEST(stmt1 != nullptr);
        Value val1 = stmt1->execute(out())->numericConversion();
        ARCHETYPE_TEST(val1->isDefined());
        int actual1 = val1->getNumber();
        int expected1 = 46;
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        
        Statement stmt2 = make_stmt_from_str("if 2 < 3 then \"Sakes alive\" else 95");
        ARCHETYPE_TEST(stmt2 != nullptr);
        Value val2 = stmt2->execute(out())->stringConversion();
        ARCHETYPE_TEST(val2->isDefined());
        string actual2 = val2->getString();
        string expected2 = "Sakes alive";
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
        
        Statement stmt3 = make_stmt_from_str("if \"something\" ~= UNDEFINED then 314 else 999");
        ARCHETYPE_TEST(stmt3 != nullptr);
        Value val3 = stmt3->execute(out())->numericConversion();
        ARCHETYPE_TEST(val3->isDefined());
        int actual3 = val3->getNumber();
        int expected3 = 314;
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
        
        Statement stmt4 = make_stmt_from_str("write \"Hello, \", \"world!\"");
        ARCHETYPE_TEST(stmt4 != nullptr);
        ostringstream sout;
        Value val4 = stmt4->execute(sout)->stringConversion();
        ARCHETYPE_TEST(val4->isDefined());
        string actual4_1 = val4->getString();
        string expected4_1 = "world!";
        ARCHETYPE_TEST_EQUAL(actual4_1, expected4_1);
        string actual_4_2 = sout.str();
        string expected_4_2 = "Hello, world!\n";
        ARCHETYPE_TEST_EQUAL(actual_4_2, expected_4_2);
    }
    
    void TestStatement::testLoopBreaks_() {
        Universe::destroy();
        ObjectPtr x = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(x, "x");
        int i_id = Universe::instance().Identifiers.index("i");
        Expression init{new ValueExpression{Value{new NumericValue{0}}}};
        x->setAttribute(i_id, move(init));
        Statement loop_w = make_stmt_from_str("{while TRUE do { writes x.i, ' '; if (x.i := x.i + 1) > 5 then { break } } write; x.i}");
        ARCHETYPE_TEST(loop_w != nullptr);
        ostringstream out;
        Value val = loop_w->execute(cout)->numericConversion();
        ARCHETYPE_TEST(val->isDefined());
        ARCHETYPE_TEST_EQUAL(val->getNumber(), 6);
    }
    
    void TestStatement::testSerialization_() {
        list<pair<string, string>> statements = {
            {
                "{writes 'The monster '; if health <= death then write \" dies\" else write ' lives'}",
                "{writes 'The monster '; if (<= health death) then write ' dies' else write ' lives'}"
            }
        };
        for (auto const& p : statements) {
            Statement stmt = make_stmt_from_str(p.first);
            MemoryStorage mem;
            mem << stmt;
            Statement stmt_back;
            mem >> stmt_back;
            ostringstream out;
            stmt_back->display(out);
            string stmt_back_str = out.str();
            ARCHETYPE_TEST_EQUAL(stmt_back_str, p.second);
        }

    }

    void TestStatement::runTests_() {
        testConstruction_();
        testExecution_();
        testLoopBreaks_();
        testSerialization_();
    }
}