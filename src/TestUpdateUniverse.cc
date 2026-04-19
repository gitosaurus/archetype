//
//  TestUpdateUniverse.cc
//  archetype
//
//  Created by Derek Jones on 2026-04-19.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#include <string>

#include "TestUpdateUniverse.hh"
#include "TestRegistry.hh"
#include "Universe.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Serialization.hh"
#include "update_universe.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestUpdateUniverse);

    // Minimal game: main responds to 'UPDATE' (required by update_universe) and
    // writes a fixed string so the non-RDF portion of the output is easy to check.
    static char program[] =
    "null main\n"
    "methods\n"
    "  'UPDATE' : write \"tick\"\n"
    "end\n"
    ;

    static void loadProgram_(MemoryStorage& out) {
        Universe::destroy();
        TokenStream t(make_source_from_str("update_test", program));
        Universe::instance().make(t);
        out << Universe::instance();
    }

    void TestUpdateUniverse::testPlainUpdate_() {
        MemoryStorage in_mem;
        loadProgram_(in_mem);

        MemoryStorage out_mem;
        string result = update_universe(in_mem, out_mem, "");

        // Game output is returned verbatim; with neither flag set, nothing else.
        ARCHETYPE_TEST(result.find("tick") != string::npos);
        ARCHETYPE_TEST(result.find("@prefix") == string::npos);
        ARCHETYPE_TEST(result.find("archetype:parser") == string::npos);
    }

    void TestUpdateUniverse::testSitrepAppendsParserRdf_() {
        MemoryStorage in_mem;
        loadProgram_(in_mem);

        MemoryStorage out_mem;
        string result = update_universe(in_mem, out_mem, "", 0,
                                        /* sitrep = */ true,
                                        /* inspect = */ false);

        // Game output comes first.
        ARCHETYPE_TEST(result.find("tick") != string::npos);

        // --sitrep appends a self-contained Turtle fragment: prefix preamble
        // followed by the parser block.
        ARCHETYPE_TEST(result.find("@prefix archetype:") != string::npos);
        ARCHETYPE_TEST(result.find("archetype:parser a archetype:SystemParser") != string::npos);

        // Without vocabulary in this tiny game, there are no matchesPhrase lines.
        ARCHETYPE_TEST(result.find("archetype:matchesPhrase") == string::npos);
    }

    void TestUpdateUniverse::testInspectAppendsFullRdf_() {
        MemoryStorage in_mem;
        loadProgram_(in_mem);

        MemoryStorage out_mem;
        string result = update_universe(in_mem, out_mem, "", 0,
                                        /* sitrep = */ false,
                                        /* inspect = */ true);

        // Game output, then full universe dump: prefixes, objects, and the
        // parser block at the end.
        ARCHETYPE_TEST(result.find("tick") != string::npos);
        ARCHETYPE_TEST(result.find("@prefix archetype:") != string::npos);
        ARCHETYPE_TEST(result.find("obj:main a type:null") != string::npos);
        ARCHETYPE_TEST(result.find("archetype:parser a archetype:SystemParser") != string::npos);
    }

    void TestUpdateUniverse::runTests_() {
        testPlainUpdate_();
        testSitrepAppendsParserRdf_();
        testInspectAppendsFullRdf_();
    }
}
