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
#include <format>
#include <map>
#include <list>
#include <ranges>
#include <string>
#include <string_view>
#include <fstream>

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


#if NDEBUG
#  define SHOW(expr)
#else
#  define SHOW(expr) std::cerr << #expr << " == " << (expr) << std::endl
#endif


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
        << " --perform=file.acx      Load a saved binary file and send 'START' -> main." << endl
        << " --update=file.acx       Load binary, send 'UPDATE' -> main, save resulting binary to the same file." << endl
        << "   --input=<string>          In combination with --update, provide command input as a string." << endl
        << "   --width=N                 In combination with --update, wrap output at N columns (default 80)." << endl
        << "   --sitrep                  In combination with --update, append a situation report (RDF/Turtle)." << endl
        << "   --inspect                 In combination with --update, append post-turn world state (RDF/Turtle)." << endl
        << " --inspect=file.acx      Load a saved binary file and dump its world state as RDF/Turtle." << endl
        << "   --full                    Add method signatures and parser vocabulary to the RDF output." << endl
    ;
}

static void from_source(map<std::string, std::string> &opts) {
    auto it_source = opts.find("source");
    string source_path = it_source->second;
    opts.erase(it_source);
    SourceFilePtr source = Wellspring::instance().primarySource(source_path);
    if (not source) {
        throw invalid_argument(format("Cannot open \"{}\"", source_path));
    }
    if (auto it_include = opts.find("include"); it_include != opts.end()) {
      string includes = it_include->second;
      opts.erase(it_include);
      istringstream in(includes);
      string path; while (getline(in, path, ':')) {
        SHOW(path);
        Wellspring::instance().addSearchPath(path);
      }
    }
    TokenStream tokens(source);
    if (not Universe::instance().make(tokens)) {
        throw CompilationFailure();
    }
    Universe::instance().reportUndefinedIdentifiers();
    if (auto it_create = opts.find("create"); it_create == opts.end()) {
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
            } catch (const std::exception& e) {
                cerr << "ERROR: " << e.what() << endl;
                return 1;
            }
        }
        if (int e = unknown_options_error()) return e;
        int errors = repl();
        return errors;
    }

    if (opts.contains("source")) {
        try {
            from_source(opts);
        } catch (const archetype::QuitGame&) {
            if (int e = unknown_options_error()) return e;
            return 0;
        } catch (const std::exception& e) {
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
          dispatch_to_universe("START");
        } catch (const archetype::QuitGame&) {
            if (int e = unknown_options_error()) return e;
            return 0;
        } catch (const std::exception& e) {
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
    if (int e = unknown_options_error()) return e;
    return 0;
}
