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

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestUniverse);
    
    inline SourceFilePtr make_string_source(string name, string contents) {
        unique_ptr<istream> in_ptr(new istringstream(contents));
        return SourceFilePtr(new SourceFile(name, in_ptr));
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
        "edible cracker desc : \"cracker\" end\n"
        "edible granola_bar end\n"
    ;
    
    static char program3[] =
        "include \"snack.ach\"\n"
        "null conservatory\n"
        "  desc: \"A nicely appointed room overlooking the garden.\"\n"
        "  contents: cracker\n"
        "methods\n"
        "  'look' : write \"A \", contents.desc, \" is here.\"\n"
        "end\n"
    ;
    
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
    
    static char program5[] =
        "null echo\n"
        "methods\n"
        "  default: message\n"
        "end\n"
    ;
    
    inline SourceFilePtr make_source_from_str(string name, string src_str) {
        stream_ptr in(new istringstream(src_str));
        return SourceFilePtr(new SourceFile(name, in));
    }
    
    inline Statement make_stmt_from_str(string src_str) {
        TokenStream token_stream(make_source_from_str("test", src_str));
        Statement stmt = make_statement(token_stream);
        return stmt;
    }
    
    void TestUniverse::testBasicObjects_() {
        Universe::destroy();

        TokenStream t1(make_source_from_str("program1", program1));
        ARCHETYPE_TEST(Universe::instance().make(t1));
        ostringstream out1;
        Statement stmt1 = make_stmt_from_str("'heft' -> vase");
        stmt1->execute(out1);
        string actual1 = out1.str();
        string expected1 = "The vase has a weight of 1.\n";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        
        TokenStream t2(make_source_from_str("program2", program2));
        ARCHETYPE_TEST(Universe::instance().make(t2));
        ostringstream out2;
        Statement stmt2 = make_stmt_from_str("'eat' -> cracker");
        stmt2->execute(out2);
        string actual2 = out2.str();
        string expected2 = "You gobble down the cracker.\n";
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
        ostringstream out3;
        Statement stmt3 = make_stmt_from_str("'eat' -> granola_bar");
        stmt3->execute(out3);
        string actual3 = out3.str();
        string expected3 = "You gobble down the snack.\n";
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
    }
    
    void TestUniverse::testInclusion_() {
        Universe::destroy();

        Wellspring::instance().put("snack.ach", make_string_source("snack.ach", program2));
        TokenStream t3(make_source_from_str("program3", program3));
        ARCHETYPE_TEST(Universe::instance().make(t3));
        ostringstream out;
        Statement stmt = make_stmt_from_str("'look' -> conservatory");
        stmt->execute(out);
        string actual = out.str();
        string expected = "A cracker is here.\n";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }
    
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
        for (auto t : test_pairs) {
            cout << "TESTING: {" << t.first << "}" << endl;
            ostringstream out;
            Statement stmt = make_stmt_from_str(t.first);
            Value actual = stmt->execute(out)->stringConversion();
            ARCHETYPE_TEST(actual->isDefined());
            ARCHETYPE_TEST_EQUAL(actual->getString(), t.second);
        }
    }
    
    void TestUniverse::testMessagingKeywords_() {
        Universe::destroy();
        
        TokenStream t5(make_source_from_str("program5", program5));
        ARCHETYPE_TEST(Universe::instance().make(t5));

        ostringstream out;
        Statement stmt1 = make_stmt_from_str("'some message' -> echo");
        Value actual1 = stmt1->execute(out)->stringConversion();
        ARCHETYPE_TEST(actual1->isDefined());
        string expected1 = "some message";
        ARCHETYPE_TEST_EQUAL(actual1->getString(), expected1);
        
        Statement stmt2 = make_stmt_from_str("\"Random string\" -> echo");
        Value actual2 = stmt2->execute(out)->stringConversion();
        ARCHETYPE_TEST(actual2->isDefined());
        ARCHETYPE_TEST_EQUAL(actual2->getString(), string("Random string"));
    }
    
    void TestUniverse::runTests_() {
        testBasicObjects_();
        testInclusion_();
        testDefaultMethods_();
        // testMessagingKeywords_();
    }
}