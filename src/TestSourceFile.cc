//
//  TestSourceFile.cc
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <string>
#include <sstream>

#include "TestSourceFile.hh"
#include "TestRegistry.hh"
#include "SourceFile.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestSourceFile);

    void TestSourceFile::runTests_() {
        stream_ptr in(new istringstream("Now is the time!"));
        SourceFile f_in("test-src", in);

        ARCHETYPE_TEST_EQUAL(f_in.readChar(), 'N');

        ostringstream sout;
        f_in.showPosition(sout);
        ARCHETYPE_TEST_EQUAL(sout.str(), string("At test-src, line 1, column 1:\nNow is the time!\n^\n"));

        while (f_in.readChar() != '!')
            ;
        ostringstream sout2;
        f_in.showPosition(sout2);
        ARCHETYPE_TEST_EQUAL(sout2.str(),
                             string("At test-src, line 1, column 16:\nNow is the time!\n               ^\n"));
        ARCHETYPE_TEST_EQUAL(f_in.readChar(), '\n');
        ARCHETYPE_TEST_EQUAL(f_in.readChar(), '\0');
        // And repeated attempts to read beyond should "bounce" at zero
        ARCHETYPE_TEST_EQUAL(f_in.readChar(), '\0');
    }
}
