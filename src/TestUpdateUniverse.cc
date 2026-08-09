//
//  TestUpdateUniverse.cc
//  archetype
//
//  Created by Derek Jones on 2026-04-19.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

    // Draws from '?', so that what the turn writes depends on the generator
    // state the universe is holding when the turn starts.
    static char rolling_program[] =
    "null main\n"
    "methods\n"
    "  'UPDATE' : { write \"roll \", ? 1000, \" \", ? 1000 }\n"
    "end\n"
    ;

    static void loadProgram_(MemoryStorage& out, const char* text = program) {
        Universe::destroy();
        TokenStream t(make_source_from_str("update_test", text));
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

    // A game that asks a question in the middle of a turn.  The first 'read' is
    // the command, the way games/intrptr.arch consumes it; the second is the
    // one a browser has no way to answer without replaying the turn.
    static char program_with_prompt[] =
    "null main\n"
    "  cmd : UNDEFINED\n"
    "  ans : UNDEFINED\n"
    "methods\n"
    "  'UPDATE' : {\n"
    "    cmd := read\n"
    "    writes \"combination? \"\n"
    "    ans := read\n"
    "    write \"[\", cmd, \"/\", ans, \"]\"\n"
    "    }\n"
    "end\n"
    ;

    static bool loadPromptingProgram_() {
        Universe::destroy();
        TokenStream t(make_source_from_str("update_test", program_with_prompt));
        return Universe::instance().make(t);
    }

    void TestUpdateUniverse::testTurnAsksForInput_() {
        ARCHETYPE_TEST(loadPromptingProgram_());

        // Seeded up front so the snapshot below has one: a turn seeds an
        // unseeded universe before rolling anything back, and the comparison
        // would otherwise be against a universe from before there was a seed.
        // Pinning it also puts the random state under the rollback check.
        Universe::instance().seedWith(0x5EEDC0FFEEULL);

        MemoryStorage before;
        before << Universe::instance();

        TurnResult asked = run_turn_collecting({ string("open") });
        ARCHETYPE_TEST(asked.status == TurnResult::Status::NeedsLine);

        // Everything the game wrote up to the question, and nothing past it.
        ARCHETYPE_TEST(asked.text.find("combination?") != string::npos);
        ARCHETYPE_TEST(asked.text.find("[open/") == string::npos);

        // The turn did not happen.  'cmd := read' had already run and mutated
        // main, so this only holds if the snapshot was rolled back -- and since
        // the bytes are a pure function of state, comparing them is the whole
        // check.
        MemoryStorage after;
        after << Universe::instance();
        ARCHETYPE_TEST(before.bytes() == after.bytes());

        // The same turn, replayed with the answer in hand, runs to completion.
        TurnResult done = run_turn_collecting({ string("open"), string("472") });
        ARCHETYPE_TEST(done.status == TurnResult::Status::Complete);
        ARCHETYPE_TEST(done.text.find("[open/472]") != string::npos);
    }

    void TestUpdateUniverse::testDeclinedInputEndsTheTurn_() {
        ARCHETYPE_TEST(loadPromptingProgram_());

        // An absent item is the player pressing Escape, or ^D at a console.
        // The turn finishes rather than asking again, which is what keeps a
        // driver from being stuck in a conversation it has no way to end.
        TurnResult declined = run_turn_collecting({ string("open"), nullopt });
        ARCHETYPE_TEST(declined.status == TurnResult::Status::Complete);
        ARCHETYPE_TEST(declined.text.find("[open/") != string::npos);
        ARCHETYPE_TEST(declined.text.find("472") == string::npos);
    }

    void TestUpdateUniverse::testSeedBetweenLoadAndTurn_() {
        // What --update has to do to honour --seed.  A save arrives already
        // seeded, and dispatching seeds whatever is not, so between the load
        // and the turn is the only moment an explicit seed can win -- which is
        // why --update runs the three phases separately rather than calling
        // update_universe.
        auto seededTurn = [](uint64_t seed, MemoryStorage& saved) {
            MemoryStorage in_mem;
            loadProgram_(in_mem, rolling_program);
            load_universe(in_mem);
            Universe::instance().seedWith(seed);
            string narrative = run_turn("");
            save_universe(saved);
            return narrative;
        };

        MemoryStorage first_save, second_save, other_save;
        string first = seededTurn(7, first_save);
        string second = seededTurn(7, second_save);
        string other = seededTurn(8, other_save);

        // The point of the option: the same seed replays the same draws, and
        // the binary that results is byte-for-byte the same.
        ARCHETYPE_TEST_EQUAL(first, second);
        ARCHETYPE_TEST(first_save.bytes() == second_save.bytes());

        // And the seed is reaching the draws at all, rather than being applied
        // somewhere the turn never reads.
        ARCHETYPE_TEST(first != other);
        ARCHETYPE_TEST(first_save.bytes() != other_save.bytes());

        out() << "TestUpdateUniverse::testSeedBetweenLoadAndTurn_ finished." << endl;
    }

    void TestUpdateUniverse::runTests_() {
        testPlainUpdate_();
        testSitrepAppendsParserRdf_();
        testSitrepUnpacksPairs_();
        testInspectAppendsStateRdf_();
        testTurnAsksForInput_();
        testDeclinedInputEndsTheTurn_();
        testSeedBetweenLoadAndTurn_();
    }
}
