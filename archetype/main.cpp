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

using namespace std;
using namespace archetype;

void usage() {
    cout
        << "Usage: " << endl
        << endl
        << " --help    Print this message and exit." << endl
        << " --test    Run all test suites." << endl
        << " --repl    Enter the REPL (Read-Eval-Print Loop)" << endl;
    ;
}

int main(int argc, const char* argv[]) {
    list<string> args(argv + 1, argv + argc);
    map<string, string> opts;
    for (auto a = args.begin(); a != args.end(); ++a) {
        if (a->find("--") == 0) {
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
        return repl();
    }
    
    if (opts.count("source")) {
        string filename = opts["source"];
        stream_ptr in(new ifstream(filename.c_str()));
        SourceFilePtr source(new SourceFile(filename, in));
        TokenStream tokens(source);
        if (not Universe::instance().make(tokens)) {
            return 1;
        }
        
        ObjectPtr main_object = Universe::instance().getObject("main");
        if (not main_object) {
            cout << "ERROR:  No 'main' object" << endl;
            return 1;
        } else {
            try {
                int start_id = Universe::instance().Messages.index("START");
                Value start{new MessageValue{start_id}};
                ContextScope c;
                c->senderObject = c->selfObject;
                c->selfObject = main_object;
                main_object->dispatch(move(start));
            } catch (const std::exception& e) {
                // TODO:  gentler catch for system exit
                cerr << "Caught:  " << e.what() << endl;
            }
            return 0;
        }
    }
}

