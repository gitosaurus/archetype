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
#include <map>
#include <list>
#include <string>
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
  static const char VersionString[] = "3.0";

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
        throw invalid_argument("Cannot open \"" + source_path + "\"");
    }
    auto it_include = opts.find("include");
    if (it_include != opts.end()) {
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
    auto it_create = opts.find("create");
    if (it_create == opts.end()) {
        dispatch_to_universe("START");
    } else {
        string filename_out = it_create->second;
        opts.erase(it_create);
        if (filename_out.empty()) {
            auto iext = source_path.rfind('.');
            filename_out = source_path.substr(0, iext);
        }
        string acx = ".acx";
        if (filename_out.rfind(acx) != filename_out.length() - acx.length()) {
            filename_out += acx;
        }
        if (source_path == filename_out) {
            throw invalid_argument("Cannot use " + filename_out + " as output");
        }
        OutFileStorage save_file(filename_out);
        if (save_file.ok()) {
            save_file << Universe::instance();
            cout << "Created " + filename_out << endl;
        } else {
            throw runtime_error("Could not write to " + filename_out);
        }
    }
}

int main(int argc, const char* argv[]) {
    Session session;
    list<string> args(argv + 1, argv + argc);
    map<string, string> opts;
    for (auto a = args.begin(); a != args.end();) {
        if (a->find("--") != 0) {
            ++a;
        } else {
            auto iequal = find(a->begin(), a->end(), '=');
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
        for (auto const& kv : opts) cerr << " --" << kv.first;
        for (auto const& a : args) cerr << " " << a;
        cerr << endl;
        return 1;
    };
    auto it_help = opts.find("help");
    if (it_help != opts.end()) {
        opts.erase(it_help);
        if (int e = unknown_options_error()) return e;
        usage();
        return 0;
    }
    auto it_silent = opts.find("silent");
    if (it_silent != opts.end()) {
        session.silent(true);
        opts.erase(it_silent);
    }
    auto it_test = opts.find("test");
    if (it_test != opts.end()) {
        opts.erase(it_test);
        if (int e = unknown_options_error()) return e;
        bool success = TestRegistry::instance().runAllTestSuites(cout);
        int exit_code = success ? 0 : 1;
        return exit_code;
    }
    auto it_repl = opts.find("repl");
    if (it_repl != opts.end()) {
        opts.erase(it_repl);
        if (!args.empty()) {
            std::string filename = args.front();
            args.pop_front();
            cout << "Loading " << filename << endl;
            InFileStorage in(filename);
            if (!in.ok()) {
                throw runtime_error("Cannot open \"" + filename + "\"");
            }
            in >> Universe::instance();
        }
        if (int e = unknown_options_error()) return e;
        int errors = repl();
        return errors;
    }

    if (opts.find("source") != opts.end()) {
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

    auto it_perform = opts.find("perform");
    auto it_update  = opts.find("update");
    auto it_inspect = opts.find("inspect");
    if (it_perform != opts.end()) {
        string filename = it_perform->second;
        opts.erase(it_perform);
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        try {
          InFileStorage in(filename);
          if (!in.ok()) {
            throw runtime_error("Cannot open \"" + filename + "\"");
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
    } else if (it_update != opts.end()) {
        string filename = it_update->second;
        opts.erase(it_update);
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        int width = 80;
        auto it_width = opts.find("width");
        if (it_width != opts.end()) {
            width = stoi(it_width->second);
            opts.erase(it_width);
        }
        try {
          MemoryStorage in_mem;
          {
              ifstream f_in(filename.c_str());
              if (!f_in) {
                throw invalid_argument("Cannot read from " + filename);
              }
              copy(istreambuf_iterator<char>{f_in}, {}, back_inserter(in_mem.bytes()));
          }
          auto it_sitrep = opts.find("sitrep");
          bool sitrep = (it_sitrep != opts.end());
          if (sitrep) opts.erase(it_sitrep);
          // --inspect with an empty value pairs with --update; a non-empty value
          // selects the standalone --inspect=file.acx path handled below.
          bool inspect_after = false;
          if (it_inspect != opts.end()) {
              inspect_after = it_inspect->second.empty();
              opts.erase(it_inspect);
          }
          string input;
          auto it_input = opts.find("input");
          if (it_input != opts.end()) {
              input = it_input->second;
              opts.erase(it_input);
          }
          MemoryStorage out_mem;
          cout << update_universe(in_mem, out_mem, input, width, sitrep, inspect_after);
          ofstream f_out(filename.c_str());
          if (!f_out) {
              throw invalid_argument("Cannot write to " + filename);
          }
          copy(out_mem.bytes().begin(), out_mem.bytes().end(), ostreambuf_iterator<char>{f_out});
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
    } else if (it_inspect != opts.end()) {
        string filename = it_inspect->second;
        opts.erase(it_inspect);
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        try {
            InFileStorage in(filename);
            if (!in.ok()) {
                throw runtime_error("Cannot open \"" + filename + "\"");
            }
            auto it_full = opts.find("full");
            bool full = (it_full != opts.end());
            if (full) opts.erase(it_full);
            inspect_universe(in, cout, full);
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
    }
    if (int e = unknown_options_error()) return e;
    return 0;
}
