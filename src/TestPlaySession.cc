//
//  TestPlaySession.cc
//  archetype
//
//  Created by Derek Jones on 2026-08-01.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#include <sstream>
#include <string>
#include <vector>

#include "TestPlaySession.hh"
#include "TestRegistry.hh"
#include "Serialization.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Universe.hh"
#include "inspect_universe.hh"
#include "update_universe.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestPlaySession);

    // A driver that keeps the universe resident between turns -- the browser --
    // takes the same turns as a driver that reloads the binary every turn -- the
    // Cloud Run one -- but skips the round trip through bytes in between.  These
    // tests pin the two paths to each other so the resident one cannot silently
    // drift.
    //
    // Note what is *not* compared: the serialized bytes.  IdIndex::write walks
    // std::map index_, so records come out ordered by the stored value rather
    // than by id -- lexicographically for the string indices, and by pointer
    // for the object registry.  That is reproducible for a given allocation
    // history, but it is not a property worth pinning a test to, and it says
    // nothing about whether the two paths agree.  dump_universe_rdf walks
    // object ids in order and is a direct picture of the state itself.

    // Every turn both consumes a command and leaves a mark on the world, so a
    // divergence in either input handling or state shows up.
    static char program[] =
    "null main\n"
    "  turns : 0\n"
    "  last  : UNDEFINED\n"
    "methods\n"
    "  'UPDATE' : {\n"
    "    turns := turns + 1\n"
    "    last := read\n"
    "    write \"turn \", turns, \": \", last\n"
    "    }\n"
    "end\n"
    ;

    static const vector<string> commands = {
        "alpha", "beta", "gamma", "delta"
    };

    static void compileProgram_(char* source, MemoryStorage& out) {
        Universe::destroy();
        TokenStream t(make_source_from_str("play_session_test", source));
        Universe::instance().make(t);
        out << Universe::instance();
    }

    static MemoryStorage storageOver_(const vector<Storage::Byte>& bytes) {
        MemoryStorage storage;
        storage.bytes() = bytes;
        return storage;
    }

    static string currentStateRdf_() {
        ostringstream rdf;
        dump_universe_rdf(rdf);
        return rdf.str();
    }

    void TestPlaySession::testResidentMatchesStateless_() {
        MemoryStorage pristine;
        compileProgram_(program, pristine);
        vector<Storage::Byte> start = pristine.bytes();

        // Stateless: reload, take one turn, save, exactly as --update does.
        string stateless_narrative;
        vector<Storage::Byte> carried = start;
        for (const string& command : commands) {
            MemoryStorage in = storageOver_(carried);
            MemoryStorage out;
            stateless_narrative += update_universe(in, out, command);
            carried = out.bytes();
        }
        string stateless_rdf = currentStateRdf_();

        // Resident: load once, then nothing but turns.
        Universe::destroy();
        MemoryStorage in = storageOver_(start);
        load_universe(in);
        string resident_narrative;
        for (const string& command : commands) {
            resident_narrative += run_turn(command);
        }
        string resident_rdf = currentStateRdf_();

        ARCHETYPE_TEST_EQUAL(resident_narrative, stateless_narrative);
        ARCHETYPE_TEST_EQUAL(resident_rdf, stateless_rdf);

        // Guard against both paths being trivially empty or stuck on turn one.
        ARCHETYPE_TEST(resident_narrative.find("turn 1: alpha") != string::npos);
        ARCHETYPE_TEST(resident_narrative.find("turn 4: delta") != string::npos);
    }

    // A save written from a resident session is an ordinary .acx: loading it
    // back reproduces the state exactly, which is what lets a browser save
    // resume under the desktop interpreter.
    void TestPlaySession::testSaveResumesResidentSession_() {
        MemoryStorage pristine;
        compileProgram_(program, pristine);

        MemoryStorage in = storageOver_(pristine.bytes());
        load_universe(in);
        for (const string& command : commands) {
            run_turn(command);
        }
        string before_rdf = currentStateRdf_();

        MemoryStorage saved;
        save_universe(saved);

        Universe::destroy();
        MemoryStorage resumed = storageOver_(saved.bytes());
        load_universe(resumed);
        ARCHETYPE_TEST_EQUAL(currentStateRdf_(), before_rdf);

        // And play goes on from where it left off rather than restarting.
        string next = run_turn("epsilon");
        ARCHETYPE_TEST(next.find("turn 5: epsilon") != string::npos);
    }

    // 'stop' ends the universe, and dispatching into an ended universe throws.
    // The WASM bindings rely on arch_ended() to keep that from ever happening,
    // so pin down both halves of the contract.
    static char stopping_program[] =
    "null main\n"
    "methods\n"
    "  'UPDATE' : stop \"done.\"\n"
    "end\n"
    ;

    void TestPlaySession::testEndedUniverseRefusesAnotherTurn_() {
        MemoryStorage pristine;
        compileProgram_(stopping_program, pristine);

        MemoryStorage in = storageOver_(pristine.bytes());
        load_universe(in);
        string narrative = run_turn("anything");
        ARCHETYPE_TEST(narrative.find("done.") != string::npos);
        ARCHETYPE_TEST(Universe::instance().ended());

        // The ended flag survives a save/load round trip, so a resumed save of a
        // finished game is still finished.
        MemoryStorage saved;
        save_universe(saved);
        Universe::destroy();
        MemoryStorage resumed = storageOver_(saved.bytes());
        load_universe(resumed);
        ARCHETYPE_TEST(Universe::instance().ended());

        bool threw = false;
        try {
            run_turn("again");
        } catch (const std::exception&) {
            threw = true;
        }
        ARCHETYPE_TEST(threw);
    }

    void TestPlaySession::runTests_() {
        testResidentMatchesStateless_();
        testSaveResumesResidentSession_();
        testEndedUniverseRefusesAnotherTurn_();
        // Leave no loaded universe behind for whichever suite runs next.
        Universe::destroy();
    }
}
