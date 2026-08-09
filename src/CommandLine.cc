//
//  CommandLine.cc
//  archetype
//
//  Created by Derek Jones on 2026-08-08.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#include <algorithm>
#include <format>
#include <list>
#include <map>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "CommandLine.hh"

using namespace std;

namespace archetype {

    // The command line as data.  Both the parser and the usage listing read
    // this table, so an option cannot be added to one without appearing in the
    // other, and the help column is arithmetic rather than a hand-counted run
    // of spaces.
    static constexpr Option OptionTable[] = {
        { .name = "help",
          .help = "Print this message and exit." },

        { .name = "test",
          .help = "Run all test suites." },

        { .name = "repl", .takes = Takes::Optional, .argname = "file.acx",
          .help = "Enter the REPL (Read-Eval-Print Loop), optionally loading\n"
                  "file.acx first." },

        { .name = "silent",
          .help = "Produce only game output and no other advisory output." },

        { .name = "source", .takes = Takes::Required, .argname = "file.arch",
          .help = "Read, compile, and run the given program." },

        { .name = "include", .takes = Takes::Required, .nested = true,
          .argname = "path[:path...]",
          .help = "Colon-separated list of paths to search for source." },

        { .name = "create", .takes = Takes::Optional, .nested = true,
          .argname = "file.acx",
          .help = "Don't run, but write the program given by --source to a\n"
                  "binary file." },

        { .name = "seed", .takes = Takes::Required, .nested = true,
          .argname = "N",
          .help = "Draw '?' from seed N, so that a run repeats exactly." },

        { .name = "perform", .takes = Takes::Required, .argname = "file.acx",
          .help = "Load a saved binary file and send 'START' -> main." },

        { .name = "autosave", .takes = Takes::Optional, .argname = "file.acx",
          .help = "Keep the game state on disk as it is played.  Without a\n"
                  "filename, writes alongside the game: g.acx -> g.save.acx." },

        { .name = "autosave-at", .takes = Takes::Required, .nested = true,
          .argname = "when",
          .help = "'turn' (default) saves after every turn and at exit;\n"
                  "'exit' saves only as the interpreter is going away." },

        { .name = "update", .takes = Takes::Required, .argname = "file.acx",
          .help = "Load binary, send 'UPDATE' -> main, save resulting binary\n"
                  "to the same file." },

        { .name = "input", .takes = Takes::Required, .nested = true,
          .argname = "string",
          .help = "In combination with --update, provide command input as a\n"
                  "string." },

        { .name = "width", .takes = Takes::Required, .nested = true,
          .argname = "N",
          .help = "In combination with --update, wrap output at N columns\n"
                  "(default 80)." },

        { .name = "sitrep", .nested = true,
          .help = "In combination with --update, append a situation report\n"
                  "(RDF/Turtle)." },

        { .name = "universe", .nested = true,
          .help = "In combination with --update, append the post-turn world\n"
                  "state (RDF/Turtle)." },

        { .name = "inspect", .takes = Takes::Required, .argname = "file.acx",
          .help = "Load a saved binary file and dump its world state as\n"
                  "RDF/Turtle." },

        { .name = "full", .nested = true,
          .help = "Add method signatures and parser vocabulary to the RDF\n"
                  "output." },
    };

    span<const Option> allOptions() {
        return OptionTable;
    }

    optional<Option> findOption(string_view name) {
        auto found = ranges::find(OptionTable, name, &Option::name);
        if (found == ranges::end(OptionTable)) {
            return nullopt;
        }
        return *found;
    }

    // How the option is spelled in the listing.  The brackets around an
    // optional value are not decoration: they mark exactly the options for
    // which a following word is not taken as the value.
    static string labelOf(const Option& option) {
        string label = format("{}--{}", option.nested ? "   " : " ", option.name);
        switch (option.takes) {
            case Takes::Nothing:
                break;
            case Takes::Required:
                label += format("={}", option.argname);
                break;
            case Takes::Optional:
                label += format("[={}]", option.argname);
                break;
        }
        return label;
    }

    // Walking a delimited string by find-slice-advance, the way
    // add_search_paths walks a search path.  C++20's views::split does not
    // hand back contiguous ranges, so a string_view cannot be made from one
    // without more ceremony than this is worth.
    static void putHelp(ostream& out, string_view label, string_view help,
                        size_t column) {
        while (true) {
            auto newline = help.find('\n');
            out << format("{:<{}}", label, column) << help.substr(0, newline)
                << endl;
            if (newline == string_view::npos) {
                break;
            }
            help.remove_prefix(newline + 1);
            label = "";
        }
    }

    void usage(ostream& out) {
        size_t column = 0;
        for (const Option& option : OptionTable) {
            column = max(column, labelOf(option).size());
        }
        column += 2;

        out << "Usage: " << endl << endl;
        for (const Option& option : OptionTable) {
            putHelp(out, labelOf(option), option.help, column);
        }
        out << endl << "Environment:" << endl;
        putHelp(out, " ARCHETYPE_INCLUDE",
                "Colon-separated paths to search for source, after any\n"
                "given by --include.", column);
    }

    CommandLine parseCommandLine(list<string> words) {
        CommandLine result;
        result.args = std::move(words);
        list<string>& args = result.args;
        for (auto a = args.begin(); a != args.end();) {
            if (not a->starts_with("--")) {
                ++a;
                continue;
            }
            auto iequal = ranges::find(*a, '=');
            string name(a->begin() + 2, iequal);
            optional<Option> option = findOption(name);
            if (not option) {
                result.error = format("unknown option --{}", name);
                return result;
            }
            bool given = iequal != a->end();
            string value;
            if (given) {
                if (option->takes == Takes::Nothing) {
                    result.error = format("--{} does not take a value", name);
                    return result;
                }
                value.assign(iequal + 1, a->end());
            }
            a = args.erase(a);
            // The next word is the value only for an option that always needs
            // one.  An option that can stand alone offers nothing to
            // distinguish its value from an unrelated argument, so '=' is the
            // only way to give it one.  A word beginning with '--' is a
            // forgotten value rather than a value that looks like an option;
            // saying so beats consuming the next option silently.
            if (not given and option->takes == Takes::Required) {
                if (a == args.end() or a->starts_with("--")) {
                    result.error = format("--{} needs a value: --{}={}",
                                          name, name, option->argname);
                    return result;
                }
                value = *a;
                a = args.erase(a);
            }
            result.opts[name] = value;
        }
        return result;
    }
}
