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

#include "TestRegistry.h"
#include "ReadEvalPrintLoop.h"
#include "SourceFile.h"
#include "TokenStream.h"
#include "Universe.h"
#include "Keywords.h"
#include "FileStorage.h"
#include "Wellspring.h"

namespace archetype {
    static const char VersionString[] = "2.0";

    class Session {
    public:
        ~Session() {
            UserOutput output = Universe::instance().output();
            output->endLine();
            output->put("Archetype ");
            output->put(VersionString);
            output->endLine();
            TestRegistry::destroy();
            Universe::destroy();
            Wellspring::destroy();
            Keywords::destroy();
        }
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
        << " --repl                  Enter the REPL (Read-Eval-Print Loop)" << endl
        << " --source=file.ach       Read, compile, and run the given program." << endl
        << "   --create[=file.acx]   Write the program given with --source to a binary file." << endl
        << " --perform=file.acx      Load the saved binary file and send 'START' -> main." << endl
    ;
}

Value run_universe() {
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
                run_universe();
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
        } catch (const archetype::QuitGame& quit_game) {
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
            run_universe();
        } catch (const archetype::QuitGame& quit_game) {
            return 0;
        } catch (const std::exception& e) {
            cerr << "ERROR: " << e.what() << endl;
            return 1;
        }
        return 0;
    }
}

