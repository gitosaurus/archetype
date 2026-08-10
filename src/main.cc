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
#include "CommandLine.hh"
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
        // The one mode that never plays, so the seed has nowhere to go: a
        // universe is seeded when it is first played, deliberately, so that
        // --create output stays byte-for-byte what it always was.  Saying so
        // beats accepting the option and quietly ignoring it.
        if (forced_seed) {
            throw invalid_argument("--seed has no effect with --create; a binary "
                                   "draws its seed when it is first played");
        }
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
    CommandLine command_line = parseCommandLine(list<string>(argv + 1, argv + argc));
    if (not command_line.ok()) {
        cerr << "ERROR: " << command_line.error << endl;
        return 1;
    }
    map<string, string>& opts = command_line.opts;
    list<string>& args = command_line.args;
    if (opts.empty() and args.empty()) {
        usage(cout);
        return 0;
    }
    // Every option here was recognized at parse time, so anything still
    // standing was understood but does not apply to the mode that ran --
    // --width without --update, say.  That is worth a different complaint than
    // a misspelling, which the parser has already rejected by name.
    auto unused_options_error = [&]() -> int {
        if (opts.empty() and args.empty()) return 0;
        cerr << "ERROR: unused options or arguments:";
        for (auto const& opt_name : opts | views::keys) cerr << " --" << opt_name;
        for (auto const& a : args) cerr << " " << a;
        cerr << endl;
        return 1;
    };
    // Silence is a modifier on whatever else runs, so it comes off the line
    // before any mode claims it -- otherwise --silent --help reports --silent
    // as the option nothing used.
    if (auto it_silent = opts.find("silent"); it_silent != opts.end()) {
        session.silent(true);
        opts.erase(it_silent);
    }
    if (auto it_help = opts.find("help"); it_help != opts.end()) {
        opts.erase(it_help);
        if (int e = unused_options_error()) return e;
        usage(cout);
        return 0;
    }
    if (auto it_seed = opts.find("seed"); it_seed != opts.end()) {
        string requested = it_seed->second;
        opts.erase(it_seed);
        try {
            // Digits only, checked here because stoull would accept a sign
            // and quietly wrap a negative into a huge seed.
            if (requested.empty() or
                requested.find_first_not_of("0123456789") != string::npos) {
                throw invalid_argument(requested);
            }
            forced_seed = stoull(requested);
        } catch (const std::exception&) {
            cerr << format("ERROR: --seed must be a whole number, not '{}'",
                           requested) << endl;
            return 1;
        }
    }
    if (auto it_test = opts.find("test"); it_test != opts.end()) {
        opts.erase(it_test);
        if (int e = unused_options_error()) return e;
        bool success = TestRegistry::instance().runAllTestSuites(cout);
        int exit_code = success ? 0 : 1;
        return exit_code;
    }
    if (auto it_repl = opts.find("repl"); it_repl != opts.end()) {
        string filename = it_repl->second;
        opts.erase(it_repl);
        // --repl can stand alone, so the parser will not hand it the next word;
        // it arrives instead as the one positional argument the interpreter
        // takes.  Both spellings work, and --repl=file.acx is the one the
        // listing documents.
        if (filename.empty() and not args.empty()) {
            filename = args.front();
            args.pop_front();
        }
        if (not filename.empty()) {
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
        if (int e = unused_options_error()) return e;
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
            if (int e = unused_options_error()) return e;
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
            if (int e = unused_options_error()) return e;
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
          bool inspect_after = opts.erase("universe") > 0;
          string input;
          if (auto it_input = opts.find("input"); it_input != opts.end()) {
              input = it_input->second;
              opts.erase(it_input);
          }
          MemoryStorage out_mem;
          // The three phases update_universe runs in one call, opened up so
          // that an explicit seed lands on the loaded universe before the turn
          // draws from it.  A save arrives already seeded and dispatching seeds
          // whatever is not, so between the load and the turn is the only
          // moment --seed can win -- which is the same moment the other three
          // modes apply it.
          load_universe(in_mem);
          seed_universe();
          cout << run_turn(std::move(input), width, sitrep, inspect_after);
          save_universe(out_mem);
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
    if (int e = unused_options_error()) return e;
    return 0;
}
