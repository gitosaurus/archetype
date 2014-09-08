//
//  TestExpression.cpp
//  archetype
//
//  Created by Derek Jones on 2/25/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>
#include <list>
#include <utility>

#include "TestExpression.h"
#include "TestRegistry.h"
#include "SourceFile.h"
#include "TokenStream.h"
#include "Expression.h"
#include "Serialization.h"
#include "StringInput.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestExpression);
    
    inline TokenStream tokens_from_str(string src_str) {
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr src(new SourceFile("test", in));
        return TokenStream(src);
    }
    
    inline Expression form_expr_from_str(string src_str) {
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr src(new SourceFile("test", in));
        TokenStream token_stream(src);
        Expression expr = form_expr(token_stream);
        return expr;
    }
    
    inline string as_prefix(const Expression& expr) {
        ostringstream out;
        expr->prefixDisplay(out);
        return out.str();
    }

#define SHOW(expr) out() << #expr << " == " << (expr) << endl;
    
    void TestExpression::testTranslation_() {
        Universe::destroy();
        
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
        
        // Verify that an expression knows when to stop.
        // In this case it needs to not consume the 'then'.
        string source7 = "not 'AFFIRM' -> main then 3 else 4";
        TokenStream tokens7 = tokens_from_str(source7);
        Expression expr7 = make_expr(tokens7);
        string actual7 = as_prefix(expr7);
        string expected7 = "(not (-> 'AFFIRM' main))";
        ARCHETYPE_TEST_EQUAL(actual7, expected7);
        ARCHETYPE_TEST(tokens7.fetch());
        ARCHETYPE_TEST_EQUAL(tokens7.token(), Token(Token::RESERVED_WORD, Keywords::RW_THEN));
        
        Expression expr8 = make_expr_from_str("(x.i := x.i + 1) > 5");
        string actual8 = as_prefix(expr8);
        string expected8 = "(> (:= (. x i) (+ (. x i) 1)) 5)";
        ARCHETYPE_TEST_EQUAL(actual8, expected8);
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
        
        // More ways of testing evaluation.  Throw in a single object for
        // attribute scratch space.
        
        string src_str = "null scratch end";
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr src(new SourceFile("scratch", in));
        TokenStream token_stream(src);
        ARCHETYPE_TEST(Universe::instance().make(token_stream));
        list<pair<string, IValue*>> testing_pairs = {
            {"numeric \"35\"", new NumericValue{35}},
            {"+ \"hello\"", new UndefinedValue},
            {"+ (\"1\" & 9)", new NumericValue{19}},
            {"& (13 + 7)", new StringValue{"20"}},
            {"string 82", new StringValue{"82"}},
            {"FALSE and TRUE", new BooleanValue{false}},
            {"FALSE or TRUE", new BooleanValue{true}},
            {"not FALSE", new BooleanValue{true}},
            // Try reacting to UNDEFINED as a truth value, also
            {"not ('nothing' -> nowhere)", new BooleanValue{true}},
            {"13 - - 14", new NumericValue{27}},
            
            // Testing these cumulative operators requires executing these statements
            // in order.  Be careful of rearranging the tests.
            {"scratch.n := 5", new NumericValue{5}},
            {"scratch.n +:= 7", new NumericValue{12}},
            {"scratch.n", new NumericValue{12}},
            {"scratch.n /:= 2", new NumericValue{6}},
            {"scratch.n -:= 1", new NumericValue{5}},
            {"scratch.n *:= 7", new NumericValue{35}},
            {"scratch.s := \"hello\"", new TextLiteralValue{Universe::instance().TextLiterals.index("hello")}},
            {"scratch.s &:= \" world\"", new StringValue{"hello world"}}
        };
        for (auto p : testing_pairs) {
            Expression expr = make_expr_from_str(p.first);
            ARCHETYPE_TEST(expr != nullptr);
            // We're not testing for AttributeValue equivalence here.
            // It does matter, but in this case, we're wanting to be sure the value got through.
            Value val = expr->evaluate()->valueConversion();
            // This will adopt and destroy the pointer in the pair
            Value test_value{p.second};
            p.second = nullptr;
            ARCHETYPE_TEST(val->isSameValueAs(test_value));
            // Compare by string, too, so that it's easier to catch in the test output
            ostringstream out1, out2;
            val->display(out1);
            test_value->display(out2);
            ARCHETYPE_TEST_EQUAL(out1.str(), out2.str());
        }
    }
    
    void TestExpression::testSerialization_() {
        list<pair<string, string>> expressions = {
            {
                "monster.health -:= 'damage' -> damage_calculator",
                "(-:= (. monster health) (-> 'damage' damage_calculator))"
            },
            {
                "message & '...' & main.dobj --> self.verbiage",
                "(--> (& (& message '...') (. main dobj)) (. self verbiage))"
            },
            {
                "\"Hello \" & \"world\"",
                "(& \"Hello \" \"world\")"
            }
        };
        for (auto const& p : expressions) {
            Expression expr = make_expr_from_str(p.first);
            MemoryStorage mem;
            mem << expr;
            Expression expr_back;
            mem >> expr_back;
            ostringstream out;
            expr_back->prefixDisplay(out);
            string expr_back_str = out.str();
            ARCHETYPE_TEST_EQUAL(expr_back_str, p.second);
        }
    }
    
    void TestExpression::testInput_() {
        UserInput input_seq{new StringInput{"Hello, world!\nyGoodbye, world."}};
        Universe::instance().setInput(input_seq);
        Expression read_expr = make_expr_from_str("read");
        Value val = read_expr->evaluate()->stringConversion();
        ARCHETYPE_TEST(val->isDefined());
        ARCHETYPE_TEST_EQUAL(val->getString(), string{"Hello, world!"});
        Expression key_expr = make_expr_from_str("'Save file? (y/n) ' & key");
        val = key_expr->evaluate()->stringConversion();
        ARCHETYPE_TEST(val->isDefined());
        ARCHETYPE_TEST_EQUAL(val->getString(), string{"Save file? (y/n) y"});
        val = read_expr->evaluate()->stringConversion();
        ARCHETYPE_TEST(val->isDefined());
        ARCHETYPE_TEST_EQUAL(val->getString(), string{"Goodbye, world."});
        // Now test the EOF conditions of both, which are blank strings
        val = read_expr->evaluate()->stringConversion();
        ARCHETYPE_TEST_EQUAL(val->getString(), string{});
        val = key_expr->evaluate()->stringConversion();
        ARCHETYPE_TEST_EQUAL(val->getString(), string{"Save file? (y/n) "});
    }
    
    void TestExpression::runTests_() {
        testTranslation_();
        testEvaluation_();
        testSerialization_();
        testInput_();
    }
}