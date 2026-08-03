//
//  main.cc
//  archetype
//
//  Created by Derek Jones on 2/5/14.
//  Copyright (c) 2014, 2022 Derek Jones. All rights reserved.
//

#include <iostream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <map>
#include <list>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <fstream>

#include "Autosave.hh"
#include "TestRegistry.hh"
#include "ReadEvalPrintLoop.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Universe.hh"
#include "WrappedOutput.hh"
#include "ConsoleOutput.hh"
#include "StringInput.hh"
#include "Keywords.hh"
#include "FileStorage.hh"
#include "Wellspring.hh"

#include "update_universe.hh"
#include "inspect_universe.hh"


namespace archetype {
  static constexpr std::string_view VersionString = "3.0";

  class CompilationFailure : public std::runtime_error {
  public:
    CompilationFailure(): runtime_error("Could not compile.") { }
  };

  class Session {
    public:
        Session():
        silent_{false}
        { }

        void silent(bool value) {
            silent_ = value;
        }

        ~Session() {
            if (not silent_) {
                std::cerr << "Archetype " << VersionString << std::endl;
            }
            Autosave::destroy();
            TestRegistry::destroy();
            Universe::destroy();
            Wellspring::destroy();
            Keywords::destroy();
        }

    private:
        bool silent_;
    };

} // namespace archetype


using namespace std;
using namespace archetype;

void usage() {
    cout
        << "Usage: " << endl
        << endl
        << " --help                  Print this message and exit." << endl
        << " --test                  Run all test suites." << endl
        << " --repl [file.acx]       Enter the REPL (Read-Eval-Print Loop), optionally loading file.acx first" << endl
        << " --silent                Produce only game output and no other advisory output." << endl
        << " --source=file.arch      Read, compile, and run the given program." << endl
        << "   --include=path[:path...]  Colon-separated list of paths to search for source." << endl
        << "   --create[=file.acx]       Don't run, but write the program given by --source to a binary file." << endl
        << "   --seed=N                  Draw '?' from seed N, so that a run repeats exactly." << endl
        << " --perform=file.acx      Load a saved binary file and send 'START' -> main." << endl
        << " --autosave[=file.acx]   Keep the game state on disk as it is played.  Without a" << endl
        << "                         filename, writes alongside the game: g.acx -> g.save.acx." << endl
        << "   --autosave-at=when        'turn' (default) saves after every turn and at exit;" << endl
        << "                             'exit' saves only as the interpreter is going away." << endl
        << " --update=file.acx       Load binary, send 'UPDATE' -> main, save resulting binary to the same file." << endl
        << "   --input=<string>          In combination with --update, provide command input as a string." << endl
        << "   --width=N                 In combination with --update, wrap output at N columns (default 80)." << endl
        << "   --sitrep                  In combination with --update, append a situation report (RDF/Turtle)." << endl
        << "   --inspect                 In combination with --update, append post-turn world state (RDF/Turtle)." << endl
        << " --inspect=file.acx      Load a saved binary file and dump its world state as RDF/Turtle." << endl
        << "   --full                    Add method signatures and parser vocabulary to the RDF output." << endl
        << endl
        << "Environment:" << endl
        << " ARCHETYPE_INCLUDE       Colon-separated paths to search for source, after any" << endl
        << "                         given by --include." << endl
    ;
}

// Gathered from the command line before we know which file the game will come
// from; the target is resolved against that file at arming time.
struct AutosaveOptions {
    bool requested = false;
    string target;      // empty means "derive from the game file"
    Autosave::Cadence cadence = Autosave::Cadence::Turn;
};

static void arm_autosave(const AutosaveOptions& options, const string& game_path) {
    if (not options.requested) {
        return;
    }
    string target = options.target;
    if (target.empty()) {
        target = Autosave::deriveTarget(game_path);
    } else if (target.rfind('.') == string::npos) {
        target += ".acx";
    }
    Autosave::instance().arm(target, options.cadence, /* keep_backup = */ true);
}

// Set by --seed, so that a run can be repeated exactly.  Without it a
// playthrough draws its own seed and keeps it, which is what a player wants
// and what a transcript, a regression test, or a bug report cannot use.
static std::optional<std::uint64_t> forced_seed;

// Called where a universe has just been made or loaded and is about to be
// played.  Dispatching would seed it anyway; the point here is that an
// explicit seed has to win over the one a save arrived with.
static void seed_universe() {
    if (forced_seed) {
        Universe::instance().seedWith(*forced_seed);
    } else {
        Universe::instance().ensureSeeded();
    }
}

// Under the default per-turn cadence the last completed turn is already on
// disk; this additionally captures a clean exit through quit or ^D.
static void checkpoint_at_exit() {
    if (Autosave::instance().armed()) {
        Autosave::instance().checkpoint();
    }
}

// A colon-separated list of directories, in the shape PATH taught everyone to
// expect, searched in the order written.  Splitting the view in place rather
// than through an istringstream means the only strings allocated are the ones
// actually kept, which addSearchPath takes by value and moves.
static void add_search_paths(string_view list) {
    while (not list.empty()) {
        auto colon = list.find(':');
        Wellspring::instance().addSearchPath(string{list.substr(0, colon)});
        if (colon == string_view::npos) {
            break;
        }
        list.remove_prefix(colon + 1);
    }
}

static void from_source(map<std::string, std::string> &opts,
                        const AutosaveOptions& autosave_opts) {
    auto it_source = opts.find("source");
    string source_path = it_source->second;
    opts.erase(it_source);
    SourceFilePtr source = Wellspring::instance().primarySource(source_path);
    if (not source) {
        throw invalid_argument(format("Cannot open \"{}\"", source_path));
    }
    if (auto it_include = opts.find("include"); it_include != opts.end()) {
        add_search_paths(it_include->second);
        opts.erase(it_include);
    }
    // The environment comes second, so an explicit --include still wins.  This
    // is what lets a packaged interpreter point at the library it ships with --
    // the snap knows where standard.arch ended up, and the player should not
    // have to.  Set but empty counts as unset, the way it usually does; an
    // empty entry *within* a list still means the current directory, the way it
    // does in PATH.
    if (const char* from_env = getenv("ARCHETYPE_INCLUDE");
        from_env != nullptr and *from_env != '\0') {
        add_search_paths(from_env);
    }
    TokenStream tokens(source);
    if (not Universe::instance().make(tokens)) {
        throw CompilationFailure();
    }
    Universe::instance().reportUndefinedIdentifiers();
    if (auto it_create = opts.find("create"); it_create == opts.end()) {
        arm_autosave(autosave_opts, source_path);
        seed_universe();
        dispatch_to_universe("START");
    } else {
        string filename_out = it_create->second;
        opts.erase(it_create);
        if (filename_out.empty()) {
            auto iext = source_path.rfind('.');
            filename_out = source_path.substr(0, iext);
        }
        if (not filename_out.ends_with(".acx")) {
            filename_out += ".acx";
        }
        if (source_path == filename_out) {
            throw invalid_argument(format("Cannot use {} as output", filename_out));
        }
        OutFileStorage save_file(filename_out);
        if (save_file.ok()) {
            save_file << Universe::instance();
            cout << format("Created {}", filename_out) << endl;
        } else {
            throw runtime_error(format("Could not write to {}", filename_out));
        }
    }
}

int main(int argc, const char* argv[]) {
    Session session;
    list<string> args(argv + 1, argv + argc);
    map<string, string> opts;
    for (auto a = args.begin(); a != args.end();) {
        if (not a->starts_with("--")) {
            ++a;
        } else {
            auto iequal = ranges::find(*a, '=');
            string opt_name(a->begin() + 2, iequal);
            string opt_value;
            if (iequal != a->end()) {
                opt_value.assign(iequal + 1, a->end());
            }
            a = args.erase(a);
            opts[opt_name] = opt_value;
        }
    }
    if (opts.empty() and args.empty()) {
        usage();
        return 0;
    }
    auto unknown_options_error = [&]() -> int {
        if (opts.empty() and args.empty()) return 0;
        cerr << "ERROR: unknown options or arguments:";
        for (auto const& opt_name : opts | views::keys) cerr << " --" << opt_name;
        for (auto const& a : args) cerr << " " << a;
        cerr << endl;
        return 1;
    };
    if (auto it_help = opts.find("help"); it_help != opts.end()) {
        opts.erase(it_help);
        if (int e = unknown_options_error()) return e;
        usage();
        return 0;
    }
    if (auto it_silent = opts.find("silent"); it_silent != opts.end()) {
        session.silent(true);
        opts.erase(it_silent);
    }
    if (auto it_seed = opts.find("seed"); it_seed != opts.end()) {
        string requested = it_seed->second;
        opts.erase(it_seed);
        try {
            size_t consumed = 0;
            forced_seed = stoull(requested, &consumed);
            if (consumed != requested.size()) throw invalid_argument(requested);
        } catch (const std::exception&) {
            cerr << format("ERROR: --seed must be a whole number, not '{}'",
                           requested) << endl;
            return 1;
        }
    }
    if (auto it_test = opts.find("test"); it_test != opts.end()) {
        opts.erase(it_test);
        if (int e = unknown_options_error()) return e;
        bool success = TestRegistry::instance().runAllTestSuites(cout);
        int exit_code = success ? 0 : 1;
        return exit_code;
    }
    if (auto it_repl = opts.find("repl"); it_repl != opts.end()) {
        opts.erase(it_repl);
        if (!args.empty()) {
            std::string filename = args.front();
            args.pop_front();
            cout << "Loading " << filename << endl;
            try {
                InFileStorage in(filename);
                if (!in.ok()) {
                    throw runtime_error(format("Cannot open \"{}\"", filename));
                }
                in >> Universe::instance();
                // The REPL evaluates expressions without dispatching, so '?'
                // can be reached here without ever passing the usual gate.
                seed_universe();
            } catch (const std::exception& e) {
                cerr << "ERROR: " << e.what() << endl;
                return 1;
            }
        }
        if (int e = unknown_options_error()) return e;
        int errors = repl();
        return errors;
    }

    AutosaveOptions autosave_opts;
    if (auto it_autosave = opts.find("autosave"); it_autosave != opts.end()) {
        autosave_opts.requested = true;
        autosave_opts.target = it_autosave->second;
        opts.erase(it_autosave);
    }
    if (auto it_when = opts.find("autosave-at"); it_when != opts.end()) {
        string when = it_when->second;
        opts.erase(it_when);
        if (not autosave_opts.requested) {
            cerr << "ERROR: --autosave-at requires --autosave" << endl;
            return 1;
        }
        if (when == "turn") {
            autosave_opts.cadence = Autosave::Cadence::Turn;
        } else if (when == "exit") {
            autosave_opts.cadence = Autosave::Cadence::Exit;
        } else {
            cerr << format("ERROR: --autosave-at must be 'turn' or 'exit', not '{}'", when) << endl;
            return 1;
        }
    }

    if (opts.contains("source")) {
        try {
            from_source(opts, autosave_opts);
        } catch (const archetype::QuitGame&) {
            checkpoint_at_exit();
            if (int e = unknown_options_error()) return e;
            return 0;
        } catch (const std::exception& e) {
            checkpoint_at_exit();
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
    }

    if (auto it_perform = opts.find("perform"); it_perform != opts.end()) {
        string filename = it_perform->second;
        opts.erase(it_perform);
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        try {
          InFileStorage in(filename);
          if (!in.ok()) {
            throw runtime_error(format("Cannot open \"{}\"", filename));
          }
          in >> Universe::instance();
          seed_universe();
          arm_autosave(autosave_opts, filename);
          dispatch_to_universe("START");
        } catch (const archetype::QuitGame&) {
            checkpoint_at_exit();
            if (int e = unknown_options_error()) return e;
            return 0;
        } catch (const std::exception& e) {
            checkpoint_at_exit();
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
    } else if (auto it_update = opts.find("update"); it_update != opts.end()) {
        string filename = it_update->second;
        opts.erase(it_update);
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        int width = 80;
        try {
          // Parsed inside the try: stoi throws on a non-numeric value, and
          // outside it that exception would escape main uncaught.
          if (auto it_width = opts.find("width"); it_width != opts.end()) {
              string width_str = it_width->second;
              opts.erase(it_width);
              size_t consumed = 0;
              int parsed = 0;
              try {
                  parsed = stoi(width_str, &consumed);
              } catch (const std::exception&) {
                  consumed = 0;
              }
              if (width_str.empty() or consumed != width_str.size() or parsed < 1) {
                  throw invalid_argument(format("Invalid --width value: {}", width_str));
              }
              width = parsed;
          }
          MemoryStorage in_mem;
          {
              ifstream f_in(filename.c_str(), ios::in | ios::binary);
              if (!f_in) {
                throw invalid_argument(format("Cannot read from {}", filename));
              }
              copy(istreambuf_iterator<char>{f_in}, {}, back_inserter(in_mem.bytes()));
          }
          bool sitrep = opts.erase("sitrep") > 0;
          // --inspect with an empty value pairs with --update; a non-empty value
          // selects the standalone --inspect=file.acx path handled below.
          bool inspect_after = false;
          if (auto it_inspect = opts.find("inspect"); it_inspect != opts.end()) {
              inspect_after = it_inspect->second.empty();
              opts.erase(it_inspect);
          }
          string input;
          if (auto it_input = opts.find("input"); it_input != opts.end()) {
              input = it_input->second;
              opts.erase(it_input);
          }
          MemoryStorage out_mem;
          cout << update_universe(in_mem, out_mem, input, width, sitrep, inspect_after);
          // No backup: --update rewrites a file the caller already owns a copy
          // of (the Cloud Run driver downloads a blob to a temp directory), so
          // a .bak alongside it would be litter rather than safety.
          string error;
          if (not writeBytesAtomically(filename, out_mem.bytes(), /* keep_backup = */ false, error)) {
              throw invalid_argument(format("Cannot write to {}: {}", filename, error));
          }
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
    } else if (auto it_inspect = opts.find("inspect"); it_inspect != opts.end()) {
        string filename = it_inspect->second;
        opts.erase(it_inspect);
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        try {
            InFileStorage in(filename);
            if (!in.ok()) {
                throw runtime_error(format("Cannot open \"{}\"", filename));
            }
            bool full = opts.erase("full") > 0;
            inspect_universe(in, cout, full);
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
    }
    checkpoint_at_exit();
    if (int e = unknown_options_error()) return e;
    return 0;
}
