//
//  TestSystemObject.cc
//  archetype
//
//  Created by Derek Jones on 4/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <deque>

#include "TestSystemObject.hh"
#include "TestRegistry.hh"
#include "Universe.hh"
#include "Statement.hh"
#include "TokenStream.hh"
#include "SourceFile.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestSystemObject);

    void TestSystemObject::testSorting_() {
        Universe::destroy();

        Statement stmt = make_stmt_from_str("'OPEN SORTER' -> system");
        stmt->execute();
        deque<string> to_sort = {"dog", "xylem", "cat", "Ajax", "several", "Zebra"};
        for (auto const& s : to_sort) {
            // Because we are creating statements to be executed, the strings need to be in double
            // quotes.  Otherwise they'll get evaluated to UNDEFINED, since they are indeed
            // undefined identifiers.
            stmt = make_stmt_from_str("\"" + s + "\" -> system");
            Value ans = stmt->execute();
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
        }
        stmt = make_stmt_from_str("'CLOSE SORTER' -> system");
        stmt->execute();
        deque<string> sorted = to_sort;
        ranges::sort(sorted);
        stmt = make_stmt_from_str("'NEXT SORTED' -> system");
        for (auto const& s : sorted) {
            Value ans = stmt->execute();
            ARCHETYPE_TEST(ans->isDefined());
            Value ans_str = ans->stringConversion();
            ARCHETYPE_TEST(ans_str->isDefined());
            ARCHETYPE_TEST_EQUAL(ans_str->getString(), s);
            cout << "NEXT SORTED: " << ans_str->getString() << endl;
        }
        // Now it should be exhausted, and NEXT SORTED should be undefined
        Value no_more = stmt->execute();
        ARCHETYPE_TEST(not no_more->isDefined());
    }

    static char program1[] =
    "type lexable based on null\n"
    "methods\n"
    "  'BUILD' : name -> system\n"
    "end\n"
    "\n"
    "lexable take name : 'take|get|grab' end\n"
    "lexable money name : 'money|cash|bag of coins'\n"
    ;

    void TestSystemObject::testParsing_() {
        Universe::destroy();

        TokenStream t1(make_source_from_str("program1", program1));
        Universe::instance().make(t1);
        string build_vocab =
            "{'OPEN PARSER' -> system;"
            "'BUILD' -> take;"
            "'BUILD' -> money;"
            "'CLOSE PARSER' -> system;"
            "'PLAYER CMD' -> system;"
            "\"grab all the cash\" -> system;"
            "'PARSE' -> system}"
        ;
        Statement stmt = make_stmt_from_str(build_vocab);
        stmt->execute();

        stmt = make_stmt_from_str("'NEXT OBJECT' -> system");
        Value val = stmt->execute();
        list<Value> parsed;
        while (val->isDefined()) {
            parsed.push_back(std::move(val));
            val = stmt->execute();
        }
        ARCHETYPE_TEST_EQUAL(parsed.size(), size_t(3));
        list<Value> expected;
        int take_obj_id = Universe::instance().getObject("take")->id();
        int money_obj_id = Universe::instance().getObject("money")->id();
        expected.push_back(make_unique<ObjectValue>(take_obj_id));
        expected.push_back(make_unique<StringValue>("all"));
        expected.push_back(make_unique<ObjectValue>(money_obj_id));
        // Two ranges:  a parse that came back short used to slip past the
        // three-legged equal, which only ever walked the first one.
        bool are_equal = ranges::equal(parsed, expected,
                                       [](const Value& x, const Value& y){ return x->isSameValueAs(y);} );
        ARCHETYPE_TEST(are_equal);
    }

    // Two ways to write a name list with nothing in one of its slots:  a
    // doubled '|', and a slot holding only whitespace.  Both are typos, and
    // both used to produce a phrase of no words -- which matched at the front
    // of every command, whatever the player typed.
    static char program2[] =
    "type lexable based on null\n"
    "methods\n"
    "  'BUILD' : name -> system\n"
    "end\n"
    "\n"
    "lexable take name : 'take||get' end\n"
    "lexable money name : 'money| |cash'\n"
    ;

    void TestSystemObject::testEmptyPhraseNeverMatches_() {
        Universe::destroy();

        TokenStream t2(make_source_from_str("program2", program2));
        Universe::instance().make(t2);
        string build_vocab =
            "{'OPEN PARSER' -> system;"
            "'BUILD' -> take;"
            "'BUILD' -> money;"
            "'CLOSE PARSER' -> system;"
            "'PLAYER CMD' -> system;"
            "\"cash\" -> system;"
            "'PARSE' -> system}"
        ;
        Statement stmt = make_stmt_from_str(build_vocab);
        stmt->execute();

        stmt = make_stmt_from_str("'NEXT OBJECT' -> system");
        Value val = stmt->execute();
        list<Value> parsed;
        while (val->isDefined()) {
            parsed.push_back(std::move(val));
            val = stmt->execute();
        }
        // The one word the player typed, and nothing else.  Before the fix
        // this came back as three:  the empty slot in each name list matched
        // at the front, so both objects arrived ahead of what was asked for.
        ARCHETYPE_TEST_EQUAL(parsed.size(), size_t(1));
        list<Value> expected;
        expected.push_back(make_unique<ObjectValue>(Universe::instance().getObject("money")->id()));
        bool are_equal = ranges::equal(parsed, expected,
                                       [](const Value& x, const Value& y){ return x->isSameValueAs(y);} );
        ARCHETYPE_TEST(are_equal);
    }

    // "<-" exists mainly for system's protocols, which are sequences of
    // messages sent for effect.  These check the two shapes that buys:  a
    // chain of effects, and a selector primed in place so that the argument's
    // reply can be used in the same expression.
    void TestSystemObject::testArrowSequencing_() {
        Universe::destroy();

        // The whole sorter protocol as one chain.  Each "<-" hands system back
        // to the next, so the messages arrive in written order.
        Statement stmt = make_stmt_from_str(
            "system <- 'INIT SORTER' <- \"dog\" <- \"Ajax\" <- \"cat\" <- 'CLOSE SORTER'"
        );
        stmt->execute();
        deque<string> expected = {"Ajax", "cat", "dog"};
        stmt = make_stmt_from_str("'NEXT SORTED' -> system");
        for (auto const& s : expected) {
            Value ans = stmt->execute()->stringConversion();
            ARCHETYPE_TEST(ans->isDefined());
            ARCHETYPE_TEST_EQUAL(ans->getString(), s);
        }
        ARCHETYPE_TEST(not stmt->execute()->isDefined());

        // A chain evaluates to the recipient, so it can be sent to again.
        Value chained = make_stmt_from_str("(system <- 'INIT SORTER') = system")->execute();
        ARCHETYPE_TEST(chained->isTrueEnough());
        make_stmt_from_str("'CLOSE SORTER' -> system")->execute();

        // 'WHICH OBJECT' followed by its argument is one logical operation,
        // but it took two statements to write before.  This only comes out
        // right because "->" evaluates its message first and its recipient
        // second:  system is put into WHICH_OBJECT state after "grab" is
        // evaluated and immediately before the send.
        TokenStream t1(make_source_from_str("program1", program1));
        Universe::instance().make(t1);
        make_stmt_from_str(
            "{'OPEN PARSER' -> system;"
            "'BUILD' -> take;"
            "'BUILD' -> money;"
            "'CLOSE PARSER' -> system}"
        )->execute();

        int take_obj_id = Universe::instance().getObject("take")->id();
        Value one_liner = make_stmt_from_str("\"grab\" -> (system <- 'WHICH OBJECT')")->execute();
        Value take_obj = make_unique<ObjectValue>(take_obj_id);
        ARCHETYPE_TEST(one_liner->isSameValueAs(take_obj));

        // Same answer as the two-statement form it replaces.
        Value two_statements = make_stmt_from_str(
            "{'WHICH OBJECT' -> system; \"grab\" -> system}"
        )->execute();
        ARCHETYPE_TEST(two_statements->isSameValueAs(take_obj));
    }

    // A list message is the staged dance in one send:  the head first, then
    // each element of the tail in order, with the reply of the whole being
    // the reply of the last.
    void TestSystemObject::testListMessages_() {
        Universe::destroy();

        // The sorter loaded in one message.  The reply is the last add's
        // echo, the same value the staged dance would have ended on.
        Value last = make_stmt_from_str(
            "['INIT SORTER' \"dog\" \"Ajax\" \"cat\"] -> system"
        )->execute()->stringConversion();
        ARCHETYPE_TEST(last->isDefined());
        ARCHETYPE_TEST_EQUAL(last->getString(), string{"cat"});
        make_stmt_from_str("'CLOSE SORTER' -> system")->execute();
        deque<string> expected = {"Ajax", "cat", "dog"};
        Statement stmt = make_stmt_from_str("'NEXT SORTED' -> system");
        for (auto const& s : expected) {
            Value ans = stmt->execute()->stringConversion();
            ARCHETYPE_TEST(ans->isDefined());
            ARCHETYPE_TEST_EQUAL(ans->getString(), s);
        }
        ARCHETYPE_TEST(not stmt->execute()->isDefined());

        // A list of one is the bare message, to system like to everything.
        Value singleton = make_stmt_from_str("['NEXT SORTED'] -> system")->execute();
        ARCHETYPE_TEST(not singleton->isDefined());

        // A control message rides in a tail like any other element, so a
        // whole open-feed-close protocol fits in one send.
        make_stmt_from_str("['INIT SORTER' \"b\" \"a\" 'CLOSE SORTER'] -> system")->execute();
        Value first = make_stmt_from_str("'NEXT SORTED' -> system")->execute()->stringConversion();
        ARCHETYPE_TEST(first->isDefined());
        ARCHETYPE_TEST_EQUAL(first->getString(), string{"a"});
        make_stmt_from_str("'NEXT SORTED' -> system")->execute();
        ARCHETYPE_TEST(not make_stmt_from_str("'NEXT SORTED' -> system")->execute()->isDefined());

        // One-shot 'WHICH OBJECT':  no priming, no second send, and the
        // answer comes back as the reply of the only send there is.
        TokenStream t1(make_source_from_str("program1", program1));
        Universe::instance().make(t1);
        make_stmt_from_str(
            "{'OPEN PARSER' -> system;"
            "'BUILD' -> take;"
            "'BUILD' -> money;"
            "'CLOSE PARSER' -> system}"
        )->execute();
        int take_obj_id = Universe::instance().getObject("take")->id();
        Value one_shot = make_stmt_from_str("['WHICH OBJECT' \"grab\"] -> system")->execute();
        Value take_obj = make_unique<ObjectValue>(take_obj_id);
        ARCHETYPE_TEST(one_shot->isSameValueAs(take_obj));
    }

    void TestSystemObject::runTests_() {
        testSorting_();
        testParsing_();
        testEmptyPhraseNeverMatches_();
        testArrowSequencing_();
        testListMessages_();
    }
}
