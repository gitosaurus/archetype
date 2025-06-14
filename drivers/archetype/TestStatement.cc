//
//  TestStatement.cc
//  archetype
//
//  Created by Derek Jones on 2/28/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>
#include <list>
#include <utility>

#include "TestStatement.hh"
#include "TestRegistry.hh"
#include "Statement.hh"
#include "Universe.hh"
#include "Capture.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestStatement);

    void TestStatement::testConstruction_() {
        Statement stmt1 = make_stmt_from_str("{}");
        ARCHETYPE_TEST(stmt1 != nullptr);
        CompoundStatement* dstmt1 = dynamic_cast<CompoundStatement*>(stmt1.get());
        ARCHETYPE_TEST(dstmt1 != nullptr);
        if (dstmt1) {
            ARCHETYPE_TEST_EQUAL(dstmt1->statements().size(), size_t(0));
        }

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

        string program4 = "case timer +:= 1 of { 3 : write \"Careful\" 4 : stop \"Dead!\" default : treasure +:= 1 }";
        Statement stmt4 = make_stmt_from_str(program4);
        ARCHETYPE_TEST(stmt4 != nullptr);
        CaseStatement* dstmt4 = dynamic_cast<CaseStatement*>(stmt4.get());
        ARCHETYPE_TEST(dstmt4 != nullptr);
    }

    void TestStatement::testExecution_() {
        Statement stmt1 = make_stmt_from_str("{ 3 + \"7\"; 12 + 34 }");
        ARCHETYPE_TEST(stmt1 != nullptr);
        Value val1 = stmt1->execute()->numericConversion();
        ARCHETYPE_TEST(val1->isDefined());
        int actual1 = val1->getNumber();
        int expected1 = 46;
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        Statement stmt2 = make_stmt_from_str("if 2 < 3 then \"Sakes alive\" else 95");
        ARCHETYPE_TEST(stmt2 != nullptr);
        Value val2 = stmt2->execute()->stringConversion();
        ARCHETYPE_TEST(val2->isDefined());
        string actual2 = val2->getString();
        string expected2 = "Sakes alive";
        ARCHETYPE_TEST_EQUAL(actual2, expected2);

        Statement stmt3 = make_stmt_from_str("if \"something\" ~= UNDEFINED then 314 else 999");
        ARCHETYPE_TEST(stmt3 != nullptr);
        Value val3 = stmt3->execute()->numericConversion();
        ARCHETYPE_TEST(val3->isDefined());
        int actual3 = val3->getNumber();
        int expected3 = 314;
        ARCHETYPE_TEST_EQUAL(actual3, expected3);

        Statement stmt4 = make_stmt_from_str("write \"Hello, \", \"world!\"");
        ARCHETYPE_TEST(stmt4 != nullptr);
        Capture capture4;
        Value val4 = stmt4->execute()->stringConversion();
        ARCHETYPE_TEST(val4->isDefined());
        string actual4_1 = val4->getString();
        string expected4_1 = "world!";
        ARCHETYPE_TEST_EQUAL(actual4_1, expected4_1);
        string actual_4_2 = capture4.getCapture();
        string expected_4_2 = "Hello, world!\n";
        ARCHETYPE_TEST_EQUAL(actual_4_2, expected_4_2);

        Statement stmt5 = make_stmt_from_str(">>Now, this is a \"quote\".");
        ARCHETYPE_TEST(stmt5 != nullptr);
        Capture capture5;
        Value val5 = stmt5->execute()->stringConversion();
        ARCHETYPE_TEST(val5->isDefined());
        ARCHETYPE_TEST_EQUAL(val5->getString(), string("Now, this is a \"quote\"."));
        string actual5 = capture5.getCapture();
        ARCHETYPE_TEST_EQUAL(actual5, string("Now, this is a \"quote\".\n"));

        Statement stmt6 = make_stmt_from_str(">>Now hear this,\n>>followed by that.");
        ARCHETYPE_TEST(stmt6 != nullptr);
        Capture capture6;
        Value val6 = stmt6->execute()->stringConversion();
        ARCHETYPE_TEST_EQUAL(val6->getString(), string("followed by that."));
        string actual6 = capture6.getCapture();
        ARCHETYPE_TEST_EQUAL(actual6, string("Now hear this, followed by that.\n"));

        Statement stmt7 = make_stmt_from_str(">>And he said:\n>>Play the best song in the world");
        ARCHETYPE_TEST(stmt7 != nullptr);
        Capture capture7;
        stmt7->execute();
        string actual7 = capture7.getCapture();
        ARCHETYPE_TEST_EQUAL(actual7, string("And he said:  Play the best song in the world\n"));

        Statement stmt8 = make_stmt_from_str(">>And he said,\n>>play whatever you like.");
        ARCHETYPE_TEST(stmt8 != nullptr);
        Capture capture8;
        stmt8->execute();
        string actual8 = capture8.getCapture();
        ARCHETYPE_TEST_EQUAL(actual8, string("And he said, play whatever you like.\n"));

        Statement stmt9 = make_stmt_from_str(">>Here's a poem:\n>> Roses are red\n>> Violets are blue\n>>\n>>The end.");
        ARCHETYPE_TEST(stmt9 != nullptr);
        Capture capture9;
        stmt9->execute();
        string actual9 = capture9.getCapture();
        ARCHETYPE_TEST_EQUAL(actual9, string("Here's a poem:\n Roses are red\n Violets are blue\n\nThe end.\n"));
    }

    void TestStatement::testLoopBreaks_() {
        Universe::destroy();
        ObjectPtr x = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(x, "x");
        int i_id = Universe::instance().Identifiers.index("i");
        Expression init{new ValueExpression{Value{new NumericValue{0}}}};
        x->setAttribute(i_id, std::move(init));
        string loop_str = "{while TRUE do { writes x.i, ' '; if (x.i := x.i + 1) > 5 then { break } } write; x.i}";
        Statement loop_w = make_stmt_from_str(loop_str);
        ARCHETYPE_TEST(loop_w != nullptr);
        ostringstream out;
        Value val = loop_w->execute()->numericConversion();
        ARCHETYPE_TEST(val->isDefined());
        ARCHETYPE_TEST_EQUAL(val->getNumber(), 6);
    }

    static char program_prototypes[] =
    "type parent_type based on null\n"
    "  name : UNDEFINED\n"
    "methods\n"
    "  'INITIAL' : write \"I'm a \", name\n"
    "end\n"
    "parent_type ball name : \"ball\" end\n"
    "parent_type cube name : \"cube\" end\n"
    ;

    void TestStatement::testForEach_() {
        Universe::destroy();
        TokenStream t{make_source_from_str("prototypes", program_prototypes)};
        Universe::instance().make(t);
        Capture capture;
        Statement for_stmt{make_stmt_from_str("for each do 'INITIAL' -> each")};
        ARCHETYPE_TEST(for_stmt != nullptr);
        for_stmt->execute();
        string actual = capture.getCapture();
        string expected = "I'm a ball\nI'm a cube\n";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    void TestStatement::testSerialization_() {
        list<pair<string, string>> statements = {
            {
                "{writes 'The monster '; if health <= death then write \" dies\" else write ' lives'}",
                "{writes 'The monster '; if (<= health death) then write \" dies\" else write ' lives'}"
            },
            {
                "for each.isAverb and each.unlisted do { if 'register' -> each then each.unlisted := FALSE }",
                "for (and (. each isAverb) (. each unlisted)) do "
                  "{if (-> 'register' each) then (:= (. each unlisted) FALSE)}"
            }
        };
        for (auto const& p : statements) {
            Statement stmt = make_stmt_from_str(p.first);
            string stmt_back_str;
            try {
                MemoryStorage mem;
                mem << stmt;
                Statement stmt_back;
                mem >> stmt_back;
                ARCHETYPE_TEST_EQUAL(mem.remaining(), 0);
                if (not mem.remaining()) {
                    ostringstream out;
                    stmt_back->display(out);
                    stmt_back_str = out.str();
                }
            } catch (const std::exception& e) {
                stmt_back_str = "Could not deserialize; caught " + string(e.what());
            }
            ARCHETYPE_TEST_EQUAL(stmt_back_str, p.second);
        }

    }

    void TestStatement::runTests_() {
        testConstruction_();
        testExecution_();
        testLoopBreaks_();
        testForEach_();
        testSerialization_();
    }
}
