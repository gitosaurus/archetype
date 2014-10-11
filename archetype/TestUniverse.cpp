//
//  TestUniverse.cpp
//  archetype
//
//  Created by Derek Jones on 3/22/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>
#include <memory>
#include <vector>

#include "TestUniverse.h"
#include "TestRegistry.h"
#include "Universe.h"
#include "SourceFile.h"
#include "TokenStream.h"
#include "Wellspring.h"
#include "Capture.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestUniverse);

    inline SourceFilePtr make_string_source(string name, string contents) {
        unique_ptr<istream> in_ptr(new istringstream(contents));
        return SourceFilePtr(new SourceFile(name, in_ptr));
    }

    inline SourceFilePtr make_source_from_str(string name, string src_str) {
        stream_ptr in(new istringstream(src_str));
        return SourceFilePtr(new SourceFile(name, in));
    }

    inline Statement make_stmt_from_str(string src_str) {
        TokenStream token_stream(make_source_from_str("test", src_str));
        Statement stmt = make_statement(token_stream);
        return stmt;
    }

    static char program_basic_1[] =
    "null vase\n"
    "  desc: \"vase\"\n"
    "  weight: 1\n"
    "methods\n"
    "  'heft': write \"The \", desc, \" has a weight of \", weight, \".\"\n"
    "end\n"
    ;

    static char program_basic_2[] =
    "type edible based on null\n"
    "  desc: \"snack\"\n"
    "methods\n"
    "  'eat': write \"You gobble down the \", desc, \".\"\n"
    "end\n"
    "edible cracker desc : \"cracker\" flavor : \"salty\" end\n"
    "edible granola_bar end\n"
    ;

    void TestUniverse::testBasicObjects_() {
        Universe::destroy();

        TokenStream t1(make_source_from_str("program_basic_1", program_basic_1));
        ARCHETYPE_TEST(Universe::instance().make(t1));
        Capture capture1;
        Statement stmt1 = make_stmt_from_str("'heft' -> vase");
        stmt1->execute();
        string actual1 = capture1.getCapture();
        string expected1 = "The vase has a weight of 1.\n";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        TokenStream t2(make_source_from_str("program_basic_2", program_basic_2));
        ARCHETYPE_TEST(Universe::instance().make(t2));
        Capture capture2;
        Statement stmt2 = make_stmt_from_str("'eat' -> cracker");
        stmt2->execute();
        string actual2 = capture2.getCapture();
        string expected2 = "You gobble down the cracker.\n";
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
        Capture capture3;
        Statement stmt3 = make_stmt_from_str("'eat' -> granola_bar");
        stmt3->execute();
        string actual3 = capture3.getCapture();
        string expected3 = "You gobble down the snack.\n";
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
    }

    static char program_nullity[] =
    "type thing based on null methods 'look' : write(\"some thing\") end\n"
    "thing null methods 'get' : write(\"You cannot.\") end\n"
    "null slop methods 'throw' : write(\"Splat!\") end\n"
    ;

    void TestUniverse::testNullIsNull_() {
        Universe::destroy();

        TokenStream t1(make_source_from_str("program_nullity", program_nullity));
        ARCHETYPE_TEST(Universe::instance().make(t1));
    }

    static char program_dynamic[] =
    "type blinds based on null\n"
    "  shade : UNDEFINED\n"
    "methods\n"
    "  'look' : write \"A crummy shade of \", shade, \".\"\n"
    "end\n"
    "\n"
    "null customer left : UNDEFINED right : UNDEFINED other_ref : UNDEFINED methods\n"
    "  'buy' : { create blinds named left; left.shade := \"white\"\n"
    "            create blinds named right; right.shade := \"alabaster\"; other_ref := right }\n"
    "end\n"
    ;

    void TestUniverse::testDynamicObjects_() {
        Universe::destroy();

        ObjectPtr obj1 = Universe::instance().defineNewObject();
        // 0 = null, 1 = system, so 2 is the first possible
        ARCHETYPE_TEST_EQUAL(obj1->id(), 2);

        ObjectPtr obj2 = Universe::instance().defineNewObject();
        ARCHETYPE_TEST_EQUAL(obj2->id(), 3);

        Universe::instance().destroyObject(2);

        ObjectPtr obj3 = Universe::instance().defineNewObject();
        ARCHETYPE_TEST_EQUAL(obj1->id(), Object::INVALID);

        // Verify that the "hole" from the destroyed object is reused
        ARCHETYPE_TEST_EQUAL(obj3->id(), 2);

        TokenStream t(make_source_from_str("dynamic.ach", program_dynamic));
        ARCHETYPE_TEST(Universe::instance().make(t));
        Capture capture;
        Statement stmt = make_stmt_from_str("{'buy' -> customer; 'look' -> customer.left; 'look' -> customer.right}");
        stmt->execute();
        string actual = capture.getCapture();
        ARCHETYPE_TEST_EQUAL(actual, string("A crummy shade of white.\nA crummy shade of alabaster.\n"));

        // Cause some compilation errors
        Statement no_type_stmt = make_stmt_from_str("create nonsense named customer.left");
        ARCHETYPE_TEST(no_type_stmt == nullptr);

        Statement type_not_instance = make_stmt_from_str("create customer named customer.left");
        ARCHETYPE_TEST(type_not_instance == nullptr);

        // Destroy an object and verify that all references to and through it become undefined
        Statement ref1 = make_stmt_from_str("customer.right.shade");
        Statement ref2 = make_stmt_from_str("customer.other_ref.shade");
        // First, check the references before destruction
        Value val1 = ref1->execute()->stringConversion();
        Value val2 = ref2->execute()->stringConversion();
        ARCHETYPE_TEST(val1->isDefined());
        ARCHETYPE_TEST(val2->isDefined());
        ARCHETYPE_TEST(val1->getString() == val2->getString());
        Statement destroy_stmt = make_stmt_from_str("destroy customer.right");
        val1 = ref1->execute()->objectConversion();
        val2 = ref1->execute()->objectConversion();
        ARCHETYPE_TEST(not val1->isDefined());
        ARCHETYPE_TEST(not val2->isDefined());
    }

    static char program_inclusion[] =
    "include \"snack.ach\"\n"
    "null flavor desc : \"Confusing object with same name as an attribute\" end\n"
    "null conservatory\n"
    "  desc: \"A nicely appointed room overlooking the garden.\"\n"
    "  contents: cracker\n"
    "methods\n"
    "  'look' : write \"A \", contents.flavor, \" \", contents.desc, \" is here.\"\n"
    "end\n"
    ;

    void TestUniverse::testInclusion_() {
        Universe::destroy();

        Wellspring::instance().put("snack.ach", make_string_source("snack.ach", program_basic_2));
        TokenStream t3(make_source_from_str("program_inclusion", program_inclusion));
        ARCHETYPE_TEST(Universe::instance().make(t3));
        Capture capture;
        Statement stmt = make_stmt_from_str("'look' -> conservatory");
        stmt->execute();
        string actual = capture.getCapture();
        string expected = "A salty cracker is here.\n";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    static char program_default_methods[] =
    "type something based on null\n"
    "methods\n"
    "  'foo' : \"bar\"\n"
    "   default : \"nothing from something\"\n"
    "end\n"
    "\n"
    "something thing\n"
    "methods\n"
    "  'fizzle' : \"bazzle\"\n"
    "  default: \"something has everything\"\n"
    "end\n"
    "\n"
    "something nothing methods 'sizzle' : \"gristle\" end\n"
    ;

    void TestUniverse::testDefaultMethods_() {
        Universe::destroy();

        TokenStream t4(make_source_from_str("program_default_methods", program_default_methods));
        ARCHETYPE_TEST(Universe::instance().make(t4));

        list<pair<string, string>> test_pairs = {
            { "'foo' -> thing", "bar" },
            { "'fee' -> nothing", "nothing from something" },
            { "'fizzle' -> nothing", "nothing from something" },
            { "'sizzle' -> nothing", "gristle" },
            { "'fee' -> thing", "something has everything" },
            { "'fizzle' -> thing", "bazzle" },
        };
        for (auto const& t : test_pairs) {
            cout << "TESTING: {" << t.first << "}" << endl;
            Statement stmt = make_stmt_from_str(t.first);
            Value actual = stmt->execute()->stringConversion();
            ARCHETYPE_TEST(actual->isDefined());
            ARCHETYPE_TEST_EQUAL(actual->getString(), t.second);
        }
    }

    static char program_messaging[] =
    "type echo_type based on null\n"
    "  desc: \"Echo Type!\"\n"
    "methods\n"
    "  'hiya' : 'hello' --> echo\n"
    "  default: message\n"
    "end\n"
    "\n"
    "null echo\n"
    "methods\n"
    "  'hello': \"Hello, \" & sender.desc\n"
    "  'goodbye' : \"Goodbye from \" & desc\n"
    "  default: message\n"
    "end\n"
    "\n"
    "echo_type me_too\n"
    "  desc: \"me too\"\n"
    "methods\n"
    "  'greet' : 'hello' -> echo\n"
    "end\n"
    "\n"
    "null another\n"
    "  desc: \"another\"\n"
    "methods\n"
    "  'chirp' : 'hiya' -> me_too\n"
    "  'cheep' : 'chirp' -> self\n"
    "  'goodbye' : message --> echo\n"
    "end\n"
    ;

    void TestUniverse::testMessagingKeywords_() {
        Universe::destroy();

        TokenStream t5(make_source_from_str("program_messaging", program_messaging));
        ARCHETYPE_TEST(Universe::instance().make(t5));

        Statement stmt1 = make_stmt_from_str("'some message' -> echo");
        Value actual1 = stmt1->execute()->stringConversion();
        ARCHETYPE_TEST(actual1->isDefined());
        string expected1 = "some message";
        ARCHETYPE_TEST_EQUAL(actual1->getString(), expected1);

        Statement stmt2 = make_stmt_from_str("\"Random string\" -> echo");
        Value actual2 = stmt2->execute()->stringConversion();
        ARCHETYPE_TEST(actual2->isDefined());
        ARCHETYPE_TEST_EQUAL(actual2->getString(), string("Random string"));

        Statement stmt3 = make_stmt_from_str("42 -> echo");
        Value actual3 = stmt3->execute()->numericConversion();
        ARCHETYPE_TEST(actual3->isDefined());
        ARCHETYPE_TEST_EQUAL(actual3->getNumber(), 42);

        Statement stmt4 = make_stmt_from_str("42 -> me_too");
        Value actual4 = stmt4->execute()->numericConversion();
        ARCHETYPE_TEST(actual4->isDefined());
        ARCHETYPE_TEST_EQUAL(actual4->getNumber(), 42);

        Statement stmt5 = make_stmt_from_str("'yelp' -> another");
        Value actual5 = stmt5->execute();
        ARCHETYPE_TEST(actual5->isDefined());
        Value expected5 = Value{new AbsentValue};
        ARCHETYPE_TEST(actual5->isSameValueAs(expected5));

        Statement stmt6 = make_stmt_from_str("'con' & 'structed' -> another");
        Value actual6 = stmt6->execute();
        ARCHETYPE_TEST(actual6->isDefined());
        Value expected6 = Value{new AbsentValue};
        ARCHETYPE_TEST(actual6->isSameValueAs(expected6));

        list<pair<string, string>> test_pairs = {
            { "'greet' -> me_too", "Hello, me too" },
            { "'chirp' -> another", "Hello, another" },
            { "'cheep' -> another", "Hello, another" },
            { "'goodbye' -> another", "Goodbye from another" },
        };
        for (auto const& t : test_pairs) {
            cout << "TESTING: {" << t.first << "}" << endl;
            Statement stmt = make_stmt_from_str(t.first);
            Value actual = stmt->execute()->stringConversion();
            ARCHETYPE_TEST(actual->isDefined());
            ARCHETYPE_TEST_EQUAL(actual->getString(), t.second);
        }
    }

    static char program_serialization[] =
    "type stuff based on null\n"
    "  desc : \"nothing special\"\n"
    "methods\n"
    "  'inspect' : write desc, \" is on the coffee table\"\n"
    "end\n"
    "\n"
    "null coffee_table\n"
    "  desc : \"coffee table\"\n"
    "  views : 0\n"
    "methods \n"
    "  'desc' : write desc\n"
    "  'look' : { views +:= 1; 'check' }\n"
    "  'check' : { writes \"You have looked \"\n"
    "              if views = 1 then write \"once.\" else write views, \" times.\" }\n"
    "end\n"
    ;

    void TestUniverse::testSerialization_() {
        Universe::destroy();

        // Test the ability to create a program, modify it a little, save it, get it back
        TokenStream t1(make_source_from_str("serialization", program_serialization));
        ARCHETYPE_TEST(Universe::instance().make(t1));
        Statement look_stmt = make_stmt_from_str("'look' -> coffee_table");
        Capture look1;
        look_stmt->execute();
        ARCHETYPE_TEST_EQUAL(look1.getCapture(), string("You have looked once.\n"));

        // Put some stuff on the coffee table
        vector<int> objects;
        for (auto s : {"cup", "magazine", "remote", "saltshaker" }) {
            Statement create_stmt = make_stmt_from_str("create stuff named coffee_table." + string(s));
            Statement rename_stmt = make_stmt_from_str(string(s) + ".desc := \"" + string(s) + "\"");
            int obj_id = create_stmt->execute()->objectConversion()->getObject();
            objects.push_back(obj_id);
            rename_stmt->execute();
        }

        // Now create some holes by deleting one of the middle ones
        Statement holes_stmt = make_stmt_from_str("{destroy coffee_table.magazine; destroy coffee_table.remote}");
        holes_stmt->execute();

        MemoryStorage mem;
        mem << Universe::instance();

        look_stmt->execute(); // look 2
        look_stmt->execute(); // look 3
        look_stmt->execute(); // look 4
        Capture look5;
        look_stmt->execute();
        ARCHETYPE_TEST_EQUAL(look5.getCapture(), string("You have looked 5 times.\n"));

        // Add a new dynamic object. It should take the place of the remote.
        Statement create_another = make_stmt_from_str("create stuff named coffee_table.napkin");
        int napkin_id = create_another->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(napkin_id, objects.at(2));

        // Restore old state!
        mem >> Universe::instance();
        Capture after_undo;
        look_stmt->execute();
        ARCHETYPE_TEST_EQUAL(after_undo.getCapture(), string("You have looked 2 times.\n"));

        // Verify that the cup and saltshaker are still at their old addresses
        Statement find_cup = make_stmt_from_str("coffee_table.cup");
        int cup_id = find_cup->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(cup_id, objects.at(0));

        Statement find_saltshaker = make_stmt_from_str("coffee_table.saltshaker");
        int saltshaker_id = find_saltshaker->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(saltshaker_id, objects.at(3));

        // And be sure the others are unusable.  They won't evaluate to UNDEFINED,
        // only because Archetype doesn't force an early lookup in object conversion,
        // but the resulting object should be null in the Universe.
        Statement find_magazine = make_stmt_from_str("coffee_table.magazine");
        int magazine_id = find_magazine->execute()->objectConversion()->getObject();
        ObjectPtr magazine = Universe::instance().getObject(magazine_id);
        ARCHETYPE_TEST(not magazine);

        Statement find_remote = make_stmt_from_str("coffee_table.remote");
        int remote_id = find_remote->execute()->objectConversion()->getObject();
        ObjectPtr remote = Universe::instance().getObject(remote_id);
        ARCHETYPE_TEST(not remote);
    }

    void TestUniverse::runTests_() {
        testBasicObjects_();
        testNullIsNull_();
        testDynamicObjects_();
        testInclusion_();
        testDefaultMethods_();
        testMessagingKeywords_();
        testSerialization_();
    }
}
