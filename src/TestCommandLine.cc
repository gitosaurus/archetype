//
//  TestCommandLine.cc
//  archetype
//
//  Created by Derek Jones on 2026-08-08.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <list>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>

#include "TestCommandLine.hh"
#include "TestRegistry.hh"
#include "CommandLine.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestCommandLine);

    static CommandLine parse(initializer_list<const char*> words) {
        return parseCommandLine(list<string>(words.begin(), words.end()));
    }

    static string valueOf(const CommandLine& command_line, const string& name) {
        auto found = command_line.opts.find(name);
        return found == command_line.opts.end() ? "<absent>" : found->second;
    }

    void TestCommandLine::testEqualsForm_() {
        // The spelling every existing caller uses -- the drivers, the snap, the
        // golden regeneration script -- has to keep parsing exactly as it did.
        CommandLine c = parse({"--source=games/gorreven.arch", "--include=games",
                               "--create=games/gorreven.acx"});
        ARCHETYPE_TEST(c.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(c, "source"), string("games/gorreven.arch"));
        ARCHETYPE_TEST_EQUAL(valueOf(c, "include"), string("games"));
        ARCHETYPE_TEST_EQUAL(valueOf(c, "create"), string("games/gorreven.acx"));
        ARCHETYPE_TEST(c.args.empty());

        // An explicit empty value is a value, not an absent one: --input= is
        // how a driver passes a player's empty command.
        CommandLine empty_input = parse({"--update=g.acx", "--input="});
        ARCHETYPE_TEST(empty_input.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(empty_input, "input"), string(""));

        out() << "TestCommandLine::testEqualsForm_ finished." << endl;
    }

    void TestCommandLine::testSeparateWordForm_() {
        // An option that always needs a value may take the next word.
        CommandLine c = parse({"--source", "games/gorreven.arch",
                               "--include", "games"});
        ARCHETYPE_TEST(c.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(c, "source"), string("games/gorreven.arch"));
        ARCHETYPE_TEST_EQUAL(valueOf(c, "include"), string("games"));
        ARCHETYPE_TEST(c.args.empty());

        // The case from the bug report: --perform, with arg of test.
        CommandLine performed = parse({"--perform", "test"});
        ARCHETYPE_TEST(performed.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(performed, "perform"), string("test"));
        ARCHETYPE_TEST(performed.args.empty());

        // Both spellings in one line, and a value that is itself a filename
        // with an '=' in it stays intact.
        CommandLine mixed = parse({"--update", "g.acx", "--input=go=west"});
        ARCHETYPE_TEST(mixed.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(mixed, "update"), string("g.acx"));
        ARCHETYPE_TEST_EQUAL(valueOf(mixed, "input"), string("go=west"));

        // The three modes that name a binary all take it the same way, which is
        // what splitting --universe off --inspect was for.
        for (const char* mode : {"--perform", "--update", "--inspect"}) {
            CommandLine spaced = parse({mode, "g.acx"});
            ARCHETYPE_TEST(spaced.ok());
            ARCHETYPE_TEST_EQUAL(spaced.opts.size(), size_t(1));
            ARCHETYPE_TEST_EQUAL(spaced.opts.begin()->second, string("g.acx"));
        }

        out() << "TestCommandLine::testSeparateWordForm_ finished." << endl;
    }

    void TestCommandLine::testStandaloneOptionsNeedEquals_() {
        // --create can stand alone, so the word after it is left alone: there
        // is nothing to tell a value from an unrelated argument.
        CommandLine c = parse({"--source=g.arch", "--create", "leftover"});
        ARCHETYPE_TEST(c.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(c, "create"), string(""));
        ARCHETYPE_TEST_EQUAL(c.args.size(), size_t(1));
        ARCHETYPE_TEST_EQUAL(c.args.front(), string("leftover"));

        // A flag that takes nothing never consumes anything either.
        CommandLine flags = parse({"--silent", "--test"});
        ARCHETYPE_TEST(flags.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(flags, "silent"), string(""));
        ARCHETYPE_TEST_EQUAL(valueOf(flags, "test"), string(""));

        out() << "TestCommandLine::testStandaloneOptionsNeedEquals_ finished." << endl;
    }

    void TestCommandLine::testPositionalArguments_() {
        // What is not an option keeps its order and is left for the mode that
        // wants it -- only --repl does, and only ever one.
        CommandLine c = parse({"--repl", "saved.acx"});
        ARCHETYPE_TEST(c.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(c, "repl"), string(""));
        ARCHETYPE_TEST_EQUAL(c.args.size(), size_t(1));
        ARCHETYPE_TEST_EQUAL(c.args.front(), string("saved.acx"));

        CommandLine valued = parse({"--repl=saved.acx"});
        ARCHETYPE_TEST(valued.ok());
        ARCHETYPE_TEST_EQUAL(valueOf(valued, "repl"), string("saved.acx"));
        ARCHETYPE_TEST(valued.args.empty());

        out() << "TestCommandLine::testPositionalArguments_ finished." << endl;
    }

    void TestCommandLine::testErrors_() {
        // A misspelling is caught by name, rather than surviving as far as
        // "unused options" at the end of a run that already did something.
        CommandLine misspelled = parse({"--sorce=g.arch"});
        ARCHETYPE_TEST(not misspelled.ok());
        ARCHETYPE_TEST(misspelled.error.find("--sorce") != string::npos);

        // A required value that never arrived, because the line ended.
        CommandLine truncated = parse({"--source"});
        ARCHETYPE_TEST(not truncated.ok());
        ARCHETYPE_TEST(truncated.error.find("--source=file.arch") != string::npos);

        // A required value that never arrived, because the next word was
        // plainly the next option.  Swallowing it would misreport this as a
        // missing --update rather than a missing --source.
        CommandLine swallowed = parse({"--source", "--update=g.acx"});
        ARCHETYPE_TEST(not swallowed.ok());
        ARCHETYPE_TEST(swallowed.error.find("--source") != string::npos);

        // A value handed to a flag that has no use for one.
        CommandLine overfed = parse({"--silent=yes"});
        ARCHETYPE_TEST(not overfed.ok());
        ARCHETYPE_TEST(overfed.error.find("does not take a value") != string::npos);

        // --universe is the --update companion and never names a file; --inspect
        // always does.  Getting these backwards is the mistake the rename was
        // meant to make impossible to write silently.
        CommandLine companion = parse({"--update=g.acx", "--universe=g.acx"});
        ARCHETYPE_TEST(not companion.ok());
        ARCHETYPE_TEST(companion.error.find("does not take a value") != string::npos);

        CommandLine dangling = parse({"--inspect"});
        ARCHETYPE_TEST(not dangling.ok());
        ARCHETYPE_TEST(dangling.error.find("--inspect=file.acx") != string::npos);

        out() << "TestCommandLine::testErrors_ finished." << endl;
    }

    void TestCommandLine::testUsageCoversTable_() {
        // The point of the table is that the listing cannot drift out of
        // agreement with the parser: every option the parser knows has to show
        // up in the help, and every name in the table has to be findable.
        ostringstream listing;
        usage(listing);
        string text = listing.str();
        for (const Option& option : allOptions()) {
            string spelled = "--" + string(option.name);
            ARCHETYPE_TEST(text.find(spelled) != string::npos);
            ARCHETYPE_TEST(findOption(option.name).has_value());
            // An option that takes a value needs something to call it, or the
            // listing renders "--source=" and the error message trails off.
            if (option.takes != Takes::Nothing) {
                ARCHETYPE_TEST(not option.argname.empty());
            }
        }
        ARCHETYPE_TEST(not findOption("no-such-option").has_value());

        // Names are unique, since the parser resolves by the first match.
        for (const Option& option : allOptions()) {
            auto matching = ranges::count(allOptions(), option.name, &Option::name);
            ARCHETYPE_TEST_EQUAL(matching, ptrdiff_t(1));
        }

        out() << "TestCommandLine::testUsageCoversTable_ finished." << endl;
    }

    void TestCommandLine::runTests_() {
        testEqualsForm_();
        testSeparateWordForm_();
        testStandaloneOptionsNeedEquals_();
        testPositionalArguments_();
        testErrors_();
        testUsageCoversTable_();
    }
}
