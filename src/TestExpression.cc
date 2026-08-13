//
//  TestExpression.cc
//  archetype
//
//  Created by Derek Jones on 2/25/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <format>
#include <sstream>
#include <list>
#include <utility>

#include "TestExpression.hh"
#include "TestRegistry.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Expression.hh"
#include "Serialization.hh"
#include "StringInput.hh"
#include "StringOutput.hh"
#include "ReadEvalPrintLoop.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {
    using enum Keywords::Reserved_e;
    ARCHETYPE_TEST_REGISTER(TestExpression);

    inline TokenStream tokens_from_str(string src_str) {
        stream_ptr in = make_unique<istringstream>(src_str);
        SourceFilePtr src{make_shared<SourceFile>("test", in)};
        return TokenStream(src);
    }

    inline Expression form_expr_from_str(string src_str) {
        stream_ptr in = make_unique<istringstream>(src_str);
        SourceFilePtr src{make_shared<SourceFile>("test", in)};
        TokenStream token_stream(src);
        Expression expr = form_expr(token_stream);
        return expr;
    }

    inline string as_prefix(const Expression& expr) {
        return format("{}", expr);
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
        Expression tight_expr1 = tighten(std::move(expr1));
        string tight_actual1 = as_prefix(tight_expr1);
        ARCHETYPE_TEST_EQUAL(tight_actual1, expected1);
        int node_count_2 = tight_expr1->nodeCount();
        ARCHETYPE_TEST(node_count_1 > node_count_2);
        SHOW(node_count_1);
        SHOW(node_count_2);
        tight_expr1 = tighten(std::move(tight_expr1));
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
        ARCHETYPE_TEST_EQUAL(tokens7.token(), Token(RW_THEN));

        Expression expr8 = make_expr_from_str("(x.i := x.i + 1) > 5");
        string actual8 = as_prefix(expr8);
        string expected8 = "(> (:= (. x i) (+ (. x i) 1)) 5)";
        ARCHETYPE_TEST_EQUAL(actual8, expected8);

        // "<-" groups to the left, which is what lets it chain:  each send
        // yields the recipient back for the next one to use.
        Expression expr9 = make_expr_from_str("system <- 'BANNER' <- '='");
        string actual9 = as_prefix(expr9);
        string expected9 = "(<- (<- system 'BANNER') '=')";
        ARCHETYPE_TEST_EQUAL(actual9, expected9);

        // Parenthesized, "<-" lands on the right side of "->", so it primes
        // the recipient rather than consuming the reply.
        Expression expr10 = make_expr_from_str("verb -> (system <- 'WHICH OBJECT')");
        string actual10 = as_prefix(expr10);
        string expected10 = "(-> verb (<- system 'WHICH OBJECT'))";
        ARCHETYPE_TEST_EQUAL(actual10, expected10);
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

        string src_str = "null scratch x : 0 methods 'hello' : x +:= 1 end";
        stream_ptr in = make_unique<istringstream>(src_str);
        SourceFilePtr src{make_shared<SourceFile>("scratch", in)};
        TokenStream token_stream(src);
        ARCHETYPE_TEST(Universe::instance().make(token_stream));
        list<pair<string, Value>> testing_pairs;
        auto expect = [&](string src, Value expected) {
            testing_pairs.emplace_back(std::move(src), std::move(expected));
        };
            expect("numeric \"35\"", make_unique<NumericValue>(35));
            expect("+ \"hello\"", make_unique<UndefinedValue>());
            expect("+ (\"1\" & 9)", make_unique<NumericValue>(19));
            expect("& (13 + 7)", make_unique<StringValue>("20"));
            expect("string 82", make_unique<StringValue>("82"));
            expect("FALSE and TRUE", make_unique<BooleanValue>(false));
            expect("FALSE or TRUE", make_unique<BooleanValue>(true));
            expect("not FALSE", make_unique<BooleanValue>(true));
            // Try reacting to UNDEFINED as a truth value, also
            expect("not ('nothing' -> nowhere)", make_unique<BooleanValue>(true));
            expect("13 - - 14", make_unique<NumericValue>(27));
            expect("2^4", make_unique<NumericValue>(16));
            expect("3 ^ \"4\"", make_unique<NumericValue>(81));

            // Testing these cumulative operators requires executing these statements
            // in order.  Be careful of rearranging the tests.
            expect("scratch.n := 5", make_unique<NumericValue>(5));
            expect("scratch.n +:= 7", make_unique<NumericValue>(12));
            expect("scratch.n", make_unique<NumericValue>(12));
            expect("scratch.n /:= 2", make_unique<NumericValue>(6));
            expect("scratch.n -:= 1", make_unique<NumericValue>(5));
            expect("scratch.n *:= 7", make_unique<NumericValue>(35));
            expect("scratch.s := \"hello\"", make_unique<TextLiteralValue>(Universe::instance().TextLiterals.index("hello")));
            expect("scratch.s &:= \" world\"", make_unique<StringValue>("hello world"));

            // The nasty tests of definition.  So tricky.
            expect("3 ~= UNDEFINED", make_unique<BooleanValue>(true));
            expect("UNDEFINED ~= 3", make_unique<BooleanValue>(true));
            expect("UNDEFINED = UNDEFINED", make_unique<BooleanValue>(true));
            expect("UNDEFINED ~= UNDEFINED", make_unique<BooleanValue>(false));
            expect("(& \"hello\") ~= UNDEFINED", make_unique<BooleanValue>(true));
            expect("UNDEFINED ~= (& \"hello\")", make_unique<BooleanValue>(true));
            expect("(\"world\" within scratch.s) ~= UNDEFINED", make_unique<BooleanValue>(true));
            expect("(\"Mars\" within scratch.s) = UNDEFINED", make_unique<BooleanValue>(true));
            expect("(\"\" within scratch.s)", make_unique<UndefinedValue>());
            expect("scratch.n ~= UNDEFINED", make_unique<BooleanValue>(true));
            expect("scratch.less = UNDEFINED", make_unique<BooleanValue>(true));
            expect("scratch = system", make_unique<BooleanValue>(false));
            expect("(scratch.nothing := null) = null", make_unique<BooleanValue>(true));
            expect("scratch.nothing ~= system", make_unique<BooleanValue>(true));
            expect("scratch ~= system", make_unique<BooleanValue>(true));
            expect("scratch.nothing = system", make_unique<BooleanValue>(false));
            expect("system = system", make_unique<BooleanValue>(true));
            expect("(scratch.sys := system) = system", make_unique<BooleanValue>(true));
            expect("scratch.sys ~= system", make_unique<BooleanValue>(false));
            expect("'never' -> scratch = ABSENT", make_unique<BooleanValue>(true));
            expect("'never' -> scratch ~= ABSENT", make_unique<BooleanValue>(false));
            expect("'hello' -> scratch = ABSENT", make_unique<BooleanValue>(false));
            expect("'hello' -> scratch ~= ABSENT", make_unique<BooleanValue>(true));

            // "<-" yields the recipient, not the reply, so a chain of them
            // stays anchored on the same object.  scratch.x has been climbing
            // by one for every 'hello' sent above; these two send two more.
            expect("scratch.x := 0", make_unique<NumericValue>(0));
            expect("(scratch <- 'hello' <- 'hello') = scratch", make_unique<BooleanValue>(true));
            expect("scratch.x", make_unique<NumericValue>(2));
            // An unknown message is still a send; it just accomplishes nothing.
            expect("(scratch <- 'never') = scratch", make_unique<BooleanValue>(true));
            expect("scratch.x", make_unique<NumericValue>(2));
            // An undefined recipient swallows the whole chain rather than
            // throwing:  every later "<-" then has an undefined left side.
            expect("nowhere <- 'hello' <- 'hello'", make_unique<UndefinedValue>());
            expect("scratch.x", make_unique<NumericValue>(2));

        for (auto& p : testing_pairs) {
            Expression expr = make_expr_from_str(p.first);
            out() << "Testing: {" << p.first << "}" << endl;
            ARCHETYPE_TEST(expr != nullptr);
            // We're not testing for AttributeValue equivalence here.
            // It does matter, but in this case, we're wanting to be sure the value got through.
            Value val = expr->evaluate()->valueConversion();
            Value test_value = std::move(p.second);
            ARCHETYPE_TEST(val->isSameValueAs(test_value));
            // Compare by string, too, so that it's easier to catch in the test output
            ARCHETYPE_TEST_EQUAL(format("{}", val), format("{}", test_value));
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
            },
            {
                "read -> (system <- 'SAVE STATE')",
                "(-> read (<- system 'SAVE STATE'))"
            }
        };
        for (auto const& p : expressions) {
            Expression expr = make_expr_from_str(p.first);
            MemoryStorage mem;
            mem << expr;
            Expression expr_back;
            mem >> expr_back;
            ARCHETYPE_TEST_EQUAL(format("{}", expr_back), p.second);
        }
    }

    void TestExpression::testInput_() {
        UserInput input_seq = make_shared<StringInput>("Hello, world!\nyGoodbye, world.\n");
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
        // Now test the EOF conditions of both, which should be UNDEFINED
        val = read_expr->evaluate()->stringConversion();
        ARCHETYPE_TEST(!val->isDefined());
        val = key_expr->evaluate()->stringConversion();
        ARCHETYPE_TEST(!val->isDefined());
    }

    void TestExpression::testVerification_() {
        Expression expr1 = make_expr_from_str("3 + obj.attr - ?6");
        ARCHETYPE_TEST(expr1 != nullptr);
        Expression expr2 = make_expr_from_str("3 + obj.45 - ?6");
        ARCHETYPE_TEST(expr2 == nullptr);
        Expression expr3 = make_expr_from_str("14 / obj.\"hello\" * 6");
        ARCHETYPE_TEST(expr3 == nullptr);
        Expression expr4 = make_expr_from_str("obj.attr.5.hello.\"world\".eight");
        ARCHETYPE_TEST(expr4 == nullptr);
        Expression expr5 = make_expr_from_str("player.something.number := 6");
        ARCHETYPE_TEST(expr5 != nullptr);
        Expression expr6 = make_expr_from_str("5 := 6");
        ARCHETYPE_TEST(expr6 == nullptr);
        Expression expr7 = make_expr_from_str("('hello' -> world).tricky := 5");
        ARCHETYPE_TEST(expr7 != nullptr);

        // "->" and "<-" share a precedence and both group left, so mixing them
        // unparenthesized reassociates into something nobody means.  Refused
        // rather than quietly obeyed.
        Expression expr8 = make_expr_from_str("verb -> system <- 'WHICH OBJECT'");
        ARCHETYPE_TEST(expr8 == nullptr);
        Expression expr9 = make_expr_from_str("system <- 'WHICH OBJECT' -> verb");
        ARCHETYPE_TEST(expr9 == nullptr);
        Expression expr10 = make_expr_from_str("message --> parent <- 'DONE'");
        ARCHETYPE_TEST(expr10 == nullptr);

        // Parentheses say which reading was meant, and both readings are legal.
        Expression expr11 = make_expr_from_str("verb -> (system <- 'WHICH OBJECT')");
        ARCHETYPE_TEST(expr11 != nullptr);

        // Chains of a single arrow are never ambiguous.
        Expression expr12 = make_expr_from_str("system <- 'BANNER' <- '='");
        ARCHETYPE_TEST(expr12 != nullptr);
        Expression expr13 = make_expr_from_str("'GENERATE' -> namesakes -> system");
        ARCHETYPE_TEST(expr13 != nullptr);
    }

    void TestExpression::testListLiterals_() {
        // A list literal uses square brackets and evaluates to a PairValue
        // chain whose display round-trips to the same bracket form.
        Expression list_expr = make_expr_from_str("[1 2 3]");
        ARCHETYPE_TEST(list_expr != nullptr);
        Value list_val = list_expr->evaluate()->valueConversion();
        ARCHETYPE_TEST_EQUAL(format("{}", list_val), string{"[1 2 3]"});

        // The empty list still parses.
        Expression empty_expr = make_expr_from_str("[]");
        ARCHETYPE_TEST(empty_expr != nullptr);

        // Nested list literals.
        Expression nested_expr = make_expr_from_str("[[1 2] [3 4]]");
        ARCHETYPE_TEST(nested_expr != nullptr);
        Value nested_val = nested_expr->evaluate()->valueConversion();
        ARCHETYPE_TEST_EQUAL(format("{}", nested_val), string{"[[1 2] [3 4]]"});

        // Curly braces in expression position no longer form a list literal.
        Expression curly_expr = make_expr_from_str("{1 2 3}");
        ARCHETYPE_TEST(curly_expr == nullptr);

        // "length" counts what is in a list rather than measuring how wide it
        // prints, and it counts an improper tail as one of them.
        Value length_val = make_expr_from_str("length [1 2 3]")->evaluate()->numericConversion();
        ARCHETYPE_TEST(length_val->isDefined());
        ARCHETYPE_TEST_EQUAL(length_val->getNumber(), 3);
        Value nested_length = make_expr_from_str("length [[1 2] [3 4]]")->evaluate()->numericConversion();
        ARCHETYPE_TEST_EQUAL(nested_length->getNumber(), 2);
        Value pair_length = make_expr_from_str("length (1 @ 2)")->evaluate()->numericConversion();
        ARCHETYPE_TEST_EQUAL(pair_length->getNumber(), 2);
        Value empty_length = make_expr_from_str("length []")->evaluate()->numericConversion();
        ARCHETYPE_TEST(not empty_length->isDefined());

        // Equality asks what a list is made of; it does not settle for the two
        // of them printing alike.
        Value same = make_expr_from_str("[1 2 3] = [1 2 3]")->evaluate()->valueConversion();
        ARCHETYPE_TEST(same->isTrueEnough());
        Value different = make_expr_from_str("[1 2 3] ~= [1 2 4]")->evaluate()->valueConversion();
        ARCHETYPE_TEST(different->isTrueEnough());
        Value list_vs_text = make_expr_from_str("[1 2 3] = \"[1 2 3]\"")->evaluate()->valueConversion();
        ARCHETYPE_TEST(list_vs_text->isDefined());
        ARCHETYPE_TEST(not list_vs_text->isTrueEnough());

        // Ordering a list against anything is UNDEFINED rather than FALSE:
        // there is no answer, which is not the same as the answer being no.
        for (auto const& source : {"[1 2] < [3]", "[1 2] > [3]", "[1 2] <= [3]",
                                   "[1 2] >= [3]", "[1 2] < 5", "\"a\" < [1 2]"}) {
            Value ordered = make_expr_from_str(source)->evaluate()->valueConversion();
            ARCHETYPE_TEST(not ordered->isDefined());
        }

        // Text surgery stops at the edge of a list rather than cutting up the
        // form it prints as, leaving "within" free to mean membership one day.
        for (auto const& source : {"\"2\" within [1 2 3]", "[1 2] within \"[1 2 3]\"",
                                   "[1 2 3] leftfrom 3", "[1 2 3] rightfrom 4"}) {
            Value surgery = make_expr_from_str(source)->evaluate()->valueConversion();
            ARCHETYPE_TEST(not surgery->isDefined());
        }

        // But "&" still asks for text, and a list still has some
        Value joined = make_expr_from_str("\"items \" & [1 2 3]")->evaluate()->stringConversion();
        ARCHETYPE_TEST(joined->isDefined());
        ARCHETYPE_TEST_EQUAL(joined->getString(), string{"items [1 2 3]"});
    }

    void TestExpression::testReplDisplay_() {
        // Drive the REPL with a few inputs and verify each result is echoed
        // once with the `=> ` prefix — the display form only, matching the
        // Python/Ruby/Lisp convention. No duplicated stringConversion tail.
        UserInput prior_input = Universe::instance().input();
        UserOutput prior_output = Universe::instance().output();
        UserInput repl_input = make_shared<StringInput>("3 + 5\n[1 2 3]\n\"hi\"\nexit\n");
        UserOutput repl_output = make_shared<StringOutput>();
        Universe::instance().setInput(repl_input);
        Universe::instance().setOutput(repl_output);
        int errors = repl();
        ARCHETYPE_TEST_EQUAL(errors, 0);
        string text = dynamic_cast<StringOutput*>(repl_output.get())->getOutput();
        ARCHETYPE_TEST(text.find("=> 8\n") != string::npos);
        ARCHETYPE_TEST(text.find("=> 8 8") == string::npos);
        ARCHETYPE_TEST(text.find("=> [1 2 3]\n") != string::npos);
        ARCHETYPE_TEST(text.find("=> \"hi\"\n") != string::npos);
        ARCHETYPE_TEST(text.find("\"hi\" hi") == string::npos);
        Universe::instance().setInput(prior_input);
        Universe::instance().setOutput(prior_output);
    }

    void TestExpression::runTests_() {
        testTranslation_();
        testEvaluation_();
        testSerialization_();
        testInput_();
        testVerification_();
        testListLiterals_();
        testReplDisplay_();
    }
}
