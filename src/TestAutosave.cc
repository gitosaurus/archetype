//
//  TestAutosave.cc
//  archetype
//

#include <filesystem>
#include <string>
#include <vector>

#include "TestAutosave.hh"
#include "TestRegistry.hh"
#include "Autosave.hh"
#include "FileStorage.hh"
#include "Object.hh"
#include "Serialization.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Universe.hh"
#include "update_universe.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestAutosave);

    // Minimal game whose every turn leaves a visible mark on the world, so a
    // checkpoint can be checked for having caught the right moment.
    static char program[] =
    "null main\n"
    "  turns : 0\n"
    "methods\n"
    "  'UPDATE' : turns := turns + 1\n"
    "end\n"
    ;

    static void loadProgram_() {
        Autosave::destroy();
        Universe::destroy();
        TokenStream t(make_source_from_str("autosave_test", program));
        Universe::instance().make(t);
    }

    static int turnsRecorded_() {
        ObjectPtr main_object = Universe::instance().getObject("main");
        int turns_id = Universe::instance().Identifiers.index("turns");
        return main_object->getAttributeValue(turns_id)->numericConversion()->getNumber();
    }

    // Each test gets its own directory so that a stray .tmp or .bak from one
    // cannot be mistaken for another's.
    static filesystem::path scratchDir_(const string& name) {
        filesystem::path dir = filesystem::temp_directory_path() / ("archetype_autosave_" + name);
        error_code ec;
        filesystem::remove_all(dir, ec);
        filesystem::create_directories(dir, ec);
        return dir;
    }

    static vector<Storage::Byte> readFile_(const filesystem::path& path) {
        InFileStorage in(path.string());
        vector<Storage::Byte> bytes(static_cast<size_t>(in.remaining()));
        if (not bytes.empty()) {
            in.read(bytes);
        }
        return bytes;
    }

    void TestAutosave::testTargetDerivation_() {
        string actual1 = Autosave::deriveTarget("games/gorreven.acx");
        string expected1 = "games/gorreven.save.acx";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        // Already a save: keep updating it rather than saving a save.
        string actual2 = Autosave::deriveTarget("games/gorreven.save.acx");
        string expected2 = "games/gorreven.save.acx";
        ARCHETYPE_TEST_EQUAL(actual2, expected2);

        // Compiling from source derives from the source name the same way.
        string actual3 = Autosave::deriveTarget("games/gorreven.arch");
        string expected3 = "games/gorreven.save.acx";
        ARCHETYPE_TEST_EQUAL(actual3, expected3);

        // A dot in a directory name is not an extension on the file.
        string actual4 = Autosave::deriveTarget("my.games/gorreven");
        string expected4 = "my.games/gorreven.save.acx";
        ARCHETYPE_TEST_EQUAL(actual4, expected4);
    }

    void TestAutosave::testAtomicWriteRoundTrips_() {
        loadProgram_();
        filesystem::path dir = scratchDir_("roundtrip");
        filesystem::path target = dir / "game.acx";

        dispatch_to_universe("UPDATE");
        string error;
        bool written = writeUniverseAtomically(target, /* keep_backup = */ false, error);
        ARCHETYPE_TEST(written);
        ARCHETYPE_TEST(filesystem::exists(target));

        // What went out must come back as a working universe.
        Universe::destroy();
        InFileStorage in(target.string());
        ARCHETYPE_TEST(in.ok());
        in >> Universe::instance();
        int actual = turnsRecorded_();
        int expected = 1;
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    void TestAutosave::testBackupHoldsPreviousContents_() {
        loadProgram_();
        filesystem::path dir = scratchDir_("backup");
        filesystem::path target = dir / "game.acx";
        filesystem::path backup = dir / "game.acx.bak";

        string error;
        dispatch_to_universe("UPDATE");
        ARCHETYPE_TEST(writeUniverseAtomically(target, /* keep_backup = */ true, error));
        // Nothing to rotate on the first write.
        ARCHETYPE_TEST(not filesystem::exists(backup));
        vector<Storage::Byte> after_first = readFile_(target);

        dispatch_to_universe("UPDATE");
        ARCHETYPE_TEST(writeUniverseAtomically(target, /* keep_backup = */ true, error));
        ARCHETYPE_TEST(filesystem::exists(backup));

        // The backup is the previous turn, byte for byte: one turn of undo.
        vector<Storage::Byte> backup_bytes = readFile_(backup);
        ARCHETYPE_TEST(backup_bytes == after_first);
        vector<Storage::Byte> current_bytes = readFile_(target);
        ARCHETYPE_TEST(current_bytes != after_first);

        // And the backup really does load as the earlier state.
        Universe::destroy();
        InFileStorage in(backup.string());
        ARCHETYPE_TEST(in.ok());
        in >> Universe::instance();
        int actual = turnsRecorded_();
        int expected = 1;
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    void TestAutosave::testNoBackupLeavesNoLitter_() {
        loadProgram_();
        filesystem::path dir = scratchDir_("nolitter");
        filesystem::path target = dir / "game.acx";

        string error;
        ARCHETYPE_TEST(writeUniverseAtomically(target, /* keep_backup = */ false, error));
        ARCHETYPE_TEST(writeUniverseAtomically(target, /* keep_backup = */ false, error));

        // The --update contract: exactly one file, no .tmp and no .bak.
        ARCHETYPE_TEST(not filesystem::exists(dir / "game.acx.tmp"));
        ARCHETYPE_TEST(not filesystem::exists(dir / "game.acx.bak"));
        int actual = static_cast<int>(distance(filesystem::directory_iterator(dir),
                                               filesystem::directory_iterator()));
        int expected = 1;
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    void TestAutosave::testFailedWriteLeavesTargetIntact_() {
        loadProgram_();
        filesystem::path dir = scratchDir_("failed");
        filesystem::path target = dir / "game.acx";

        string error;
        dispatch_to_universe("UPDATE");
        ARCHETYPE_TEST(writeUniverseAtomically(target, /* keep_backup = */ false, error));
        vector<Storage::Byte> good_bytes = readFile_(target);

        // A directory in the way of the temporary file makes the write fail
        // after the universe has already been serialized.
        filesystem::path blocker = dir / "game.acx.tmp";
        error_code ec;
        filesystem::create_directory(blocker, ec);

        dispatch_to_universe("UPDATE");
        bool written = writeUniverseAtomically(target, /* keep_backup = */ false, error);
        ARCHETYPE_TEST(not written);
        ARCHETYPE_TEST(not error.empty());

        // The previous save is untouched, not truncated.
        vector<Storage::Byte> still_there = readFile_(target);
        ARCHETYPE_TEST(still_there == good_bytes);

        filesystem::remove_all(blocker, ec);
    }

    void TestAutosave::testArmingDoesNotInventUpdateMessage_() {
        // A game with no 'UPDATE' message at all.  Arming must not conjure one
        // into the Messages index: index() inserts on a miss, and the invented
        // entry would then be written into every save that followed.
        Autosave::destroy();
        Universe::destroy();
        static char quiet_program[] =
        "null main\n"
        "methods\n"
        "  'START' : write \"hush\"\n"
        "end\n"
        ;
        TokenStream t(make_source_from_str("autosave_quiet", quiet_program));
        Universe::instance().make(t);

        int before = Universe::instance().Messages.count();
        ARCHETYPE_TEST(Universe::instance().Messages.find("UPDATE") < 0);

        filesystem::path dir = scratchDir_("noupdate");
        Autosave::instance().arm((dir / "game.acx").string(),
                                 Autosave::Cadence::Turn,
                                 /* keep_backup = */ false);

        // Still absent, and the index has not grown.
        ARCHETYPE_TEST(Universe::instance().Messages.find("UPDATE") < 0);
        int after = Universe::instance().Messages.count();
        ARCHETYPE_TEST_EQUAL(after, before);

        // With no turn boundary to watch, it falls back to saving at exit.
        ARCHETYPE_TEST(Autosave::instance().armed());
        ARCHETYPE_TEST(not Autosave::watchingTurns());
        ARCHETYPE_TEST(Autosave::instance().cadence() == Autosave::Cadence::Exit);

        Autosave::instance().disarm();
    }

    void TestAutosave::testCheckpointCapturesTurnState_() {
        loadProgram_();
        filesystem::path dir = scratchDir_("perturn");
        filesystem::path target = dir / "game.acx";

        Autosave::instance().arm(target.string(), Autosave::Cadence::Turn,
                                 /* keep_backup = */ false);
        ARCHETYPE_TEST(Autosave::watchingTurns());

        // No explicit checkpoint call: completing the turn is what saves.
        dispatch_to_universe("UPDATE");
        ARCHETYPE_TEST(filesystem::exists(target));
        dispatch_to_universe("UPDATE");

        Autosave::instance().disarm();
        ARCHETYPE_TEST(not Autosave::watchingTurns());

        // The file holds both completed turns.
        Universe::destroy();
        InFileStorage in(target.string());
        ARCHETYPE_TEST(in.ok());
        in >> Universe::instance();
        int actual = turnsRecorded_();
        int expected = 2;
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    void TestAutosave::runTests_() {
        testTargetDerivation_();
        testAtomicWriteRoundTrips_();
        testBackupHoldsPreviousContents_();
        testNoBackupLeavesNoLitter_();
        testFailedWriteLeavesTargetIntact_();
        testArmingDoesNotInventUpdateMessage_();
        testCheckpointCapturesTurnState_();
        Autosave::destroy();
    }
}
