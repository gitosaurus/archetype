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
    
    static char program1[] =
    "null vase\n"
    "  desc: \"vase\"\n"
    "  weight: 1\n"
    "methods\n"
    "  'heft': write \"The \", desc, \" has a weight of \", weight, \".\"\n"
    "end\n"
    ;
    
    static char program2[] =
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

        TokenStream t1(make_source_from_str("program1", program1));
        ARCHETYPE_TEST(Universe::instance().make(t1));
        Capture capture1;
        Statement stmt1 = make_stmt_from_str("'heft' -> vase");
        stmt1->execute();
        string actual1 = capture1.getCapture();
        string expected1 = "The vase has a weight of 1.\n";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        
        TokenStream t2(make_source_from_str("program2", program2));
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
    
    static char program3[] =
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

        Wellspring::instance().put("snack.ach", make_string_source("snack.ach", program2));
        TokenStream t3(make_source_from_str("program3", program3));
        ARCHETYPE_TEST(Universe::instance().make(t3));
        Capture capture;
        Statement stmt = make_stmt_from_str("'look' -> conservatory");
        stmt->execute();
        string actual = capture.getCapture();
        string expected = "A salty cracker is here.\n";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }
    
    static char program4[] =
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
        
        TokenStream t4(make_source_from_str("program4", program4));
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
    
    static char program5[] =
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
        
        TokenStream t5(make_source_from_str("program5", program5));
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
    
    void TestUniverse::runTests_() {
        testBasicObjects_();
        testInclusion_();
        testDefaultMethods_();
        testMessagingKeywords_();
    }
}