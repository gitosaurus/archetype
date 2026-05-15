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

    // SITREP returns a list of [key value] pairs; the emitter unpacks each
    // into its own archetype:<key> predicate on archetype:situation.
    static char program_with_sitrep[] =
    "null place\n"
    "end\n"
    "null main\n"
    "  s : UNDEFINED\n"
    "methods\n"
    "  'UPDATE' : write \"tick\"\n"
    "  'SITREP' : {\n"
    "    s := UNDEFINED\n"
    "    s := [ \"location\" place ] @ s\n"
    "    s := [ \"mode\" \"calm\" ] @ s\n"
    "    s\n"
    "    }\n"
    "end\n"
    ;

    void TestUpdateUniverse::testSitrepUnpacksPairs_() {
        Universe::destroy();
        TokenStream t(make_source_from_str("update_test", program_with_sitrep));
        ARCHETYPE_TEST(Universe::instance().make(t));
        MemoryStorage in_mem;
        in_mem << Universe::instance();

        MemoryStorage out_mem;
        string result = update_universe(in_mem, out_mem, "", 0,
                                        /* sitrep = */ true,
                                        /* inspect = */ false);

        // Situation report is emitted as proper Turtle with one predicate
        // per SITREP pair.
        ARCHETYPE_TEST(result.find("archetype:situation a archetype:SituationReport") != string::npos);
        ARCHETYPE_TEST(result.find("archetype:location obj:place") != string::npos);
        ARCHETYPE_TEST(result.find("archetype:mode \"calm\"") != string::npos);

        // No vestige of the old ad-hoc "SITREP ( ... )" label.
        ARCHETYPE_TEST(result.find("SITREP (") == string::npos);
    }

    void TestUpdateUniverse::testInspectAppendsStateRdf_() {
        MemoryStorage in_mem;
        loadProgram_(in_mem);

        MemoryStorage out_mem;
        string result = update_universe(in_mem, out_mem, "", 0,
                                        /* sitrep = */ false,
                                        /* inspect = */ true);

        // Game output, then post-turn world state: prefixes and objects.
        // Parser vocabulary/state is reserved for the --full output.
        ARCHETYPE_TEST(result.find("tick") != string::npos);
        ARCHETYPE_TEST(result.find("@prefix archetype:") != string::npos);
        ARCHETYPE_TEST(result.find("obj:main a type:null") != string::npos);
        ARCHETYPE_TEST(result.find("archetype:parser a archetype:SystemParser") == string::npos);
    }

    void TestUpdateUniverse::runTests_() {
        testPlainUpdate_();
        testSitrepAppendsParserRdf_();
        testSitrepUnpacksPairs_();
        testInspectAppendsStateRdf_();
    }
}
