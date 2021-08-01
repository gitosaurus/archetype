//
//  main.cpp
//  archetype
//
//  Created by Derek Jones on 2/5/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
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
#include "Keywords.hh"
#include "FileStorage.hh"
#include "Wellspring.hh"

namespace archetype {
    static const char VersionString[] = "3.0";

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
                UserOutput output = Universe::instance().output();
                output->endLine();
                output->put("Archetype ");
                output->put(VersionString);
                output->endLine();
            }
            TestRegistry::destroy();
            Universe::destroy();
            Wellspring::destroy();
            Keywords::destroy();
        }

    private:
        bool silent_;
    };

}

using namespace std;
using namespace archetype;

void usage() {
    cout
        << "Usage: " << endl
        << endl
        << " --help                  Print this message and exit." << endl
        << " --test                  Run all test suites." << endl
        << " --repl                  Enter the REPL (Read-Eval-Print Loop)." << endl
        << " --silent                Produce only game output and no other advisory output." << endl
        << " --source=file.ach       Read, compile, and run the given program." << endl
        << "   --create[=file.acx]   Don't run, but write the program given by --source to a binary file." << endl
        << " --perform=file.acx      Load a saved binary file and send 'START' -> main." << endl
        << " --update=file.acx       Load binary, send 'UPDATE' -> main, save resulting binary to the same file." << endl
    ;
}

Value dispatch_to_universe(string message) {
    ObjectPtr main_object = Universe::instance().getObject("main");
    if (not main_object) {
        throw invalid_argument("No 'main' object");
    }
    int start_id = Universe::instance().Messages.index(message);
    Value start{new MessageValue{start_id}};
    Value result = Object::send(main_object, move(start));
    if (result->isSameValueAs(Value{new AbsentValue})) {
        throw invalid_argument("No method for '" + message + "' on main object");
    }
    return result;
}

Value update_universe() {
    const string message = "START";
    ObjectPtr main_object = Universe::instance().getObject("main");
    if (not main_object) {
        throw invalid_argument("No 'main' object");
    }
    int start_id = Universe::instance().Messages.index(message);
    Value start{new MessageValue{start_id}};
    Value result = Object::send(main_object, move(start));
    if (result->isSameValueAs(Value{new AbsentValue})) {
        throw invalid_argument("No method for '" + message + "' on main object");
    }
    return result;
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
    if (opts.count("help")) {
        usage();
        return 0;
    }
    if (opts.count("silent")) {
        session.silent(true);
    }
    if (opts.count("test")) {
        bool success = TestRegistry::instance().runAllTestSuites(cout);
        int exit_code = success ? 0 : 1;
        return exit_code;
    }
    if (opts.count("repl")) {
        int errors = repl();
        return errors;
    }

    if (opts.count("source")) {
        try {
            string source_path = opts["source"];
            SourceFilePtr source = Wellspring::instance().primarySource(source_path);
            if (not source) {
                throw invalid_argument("Cannot open \"" + source_path + "\"");
            }
            TokenStream tokens(source);
            if (not Universe::instance().make(tokens)) {
                return 1;
            }
            Universe::instance().reportUndefinedIdentifiers();
            if (not opts.count("create")) {
                dispatch_to_universe("START");
            } else {
                string filename_out = opts["create"];
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
        } catch (const archetype::QuitGame&) {
            return 0;
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
    }

    if (opts.count("perform")) {
        string filename = opts["perform"];
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        InFileStorage in(filename);
        if (not in.ok()) {
            cerr << "Cannot open " << filename << endl;
            return 1;
        }
        in >> Universe::instance();
        try {
            dispatch_to_universe("START");
        } catch (const archetype::QuitGame&) {
            return 0;
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
        return 0;
    } else if (opts.count("update")) {
        string filename = opts["update"];
        if (filename.rfind('.') == string::npos) {
            filename += ".acx";
        }
        {
            InFileStorage in(filename);
            if (not in.ok()) {
                cerr << "Cannot open " << filename << endl;
                return 1;
            }
            in >> Universe::instance();
        }
        try {
            dispatch_to_universe("UPDATE");
        } catch (const archetype::QuitGame&) {
            return 0;
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
        {
            OutFileStorage out(filename);
            if (not out.ok()) {
                cerr << "Cannot write to " << filename << endl;
            }
            out << Universe::instance();
        }
        return 0;
    }
}

