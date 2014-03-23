//
//  TestUniverse.cpp
//  archetype
//
//  Created by Derek Jones on 3/22/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <sstream>

#include "TestUniverse.h"
#include "TestRegistry.h"
#include "Universe.h"
#include "SourceFile.h"
#include "TokenStream.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestUniverse);
    
    static char program1[] =
        "null vase\n"
        "  desc: \"vase\"\n"
        "  weight: 1\n"
        "methods\n"
        "  'heft': write \"The \", desc, \" has a weight of \", weight, \".\"\n"
        "end\n";
    
    inline Statement make_stmt_from_str(string src_str) {
        istringstream in(src_str);
        SourceFile src("test", in);
        TokenStream token_stream(src);
        Statement stmt = make_statement(token_stream);
        return stmt;
    }
    
    void TestUniverse::runTests_() {
        istringstream in1(program1);
        SourceFile src1("program1", in1);
        TokenStream t1(src1);
        ARCHETYPE_TEST(Universe::instance().make(t1));
        ostringstream out1;
        Statement stmt1 = make_stmt_from_str("'heft' -> vase");
        stmt1->execute(out1);
        string actual1 = out1.str();
        string expected1 = "The vase has a weight of 1.\n";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);
    }
}