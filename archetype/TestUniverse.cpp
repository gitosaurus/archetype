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
    
    inline Statement make_stmt_from_str(string src_str) {
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr src(new SourceFile("test", in));
        TokenStream token_stream(src);
        Statement stmt = make_statement(token_stream);
        return stmt;
    }
    
    void TestUniverse::testBasicObjects_() {
        Universe::destroy();
        stream_ptr in1(new istringstream(program1));
        SourceFilePtr src1(new SourceFile("program1", in1));
        TokenStream t1(src1);
        ARCHETYPE_TEST(Universe::instance().make(t1));
        ostringstream out1;
        Statement stmt1 = make_stmt_from_str("'heft' -> vase");
        stmt1->execute(out1);
        string actual1 = out1.str();
        string expected1 = "The vase has a weight of 1.\n";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
        
        stream_ptr in2(new istringstream(program2));
        SourceFilePtr src2(new SourceFile("program2", in2));
        TokenStream t2(src2);
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
        
        // TODO:  Uh oh.  Why so infinite-loopy?

        Wellspring::instance().put("snack.ach", make_string_source("snack.ach", program2));
        stream_ptr in3(new istringstream(program3));
        SourceFilePtr src3(new SourceFile("program3", in3));
        TokenStream t3(src3);
        ARCHETYPE_TEST(Universe::instance().make(t3));
        ostringstream out;
        Statement stmt = make_stmt_from_str("'look' -> conservatory");
        stmt->execute(out);
        string actual = out.str();
        string expected = "A cracker is here.\n";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }
    
    void TestUniverse::runTests_() {
        testBasicObjects_();
        testInclusion_();
    }
}