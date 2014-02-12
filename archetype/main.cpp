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

#include "TestRegistry.h"

using namespace std;
using namespace archetype;

void usage() {
    cout
        << "Usage: " << endl
        << endl
        << " --help    Print this message and exit." << endl
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
            args.erase(a);
            opts[opt_name] = opt_value;
        }
    }
    if (opts.count("help")) {
        usage();
        return 0;
    }
    if (opts.count("test")) {
        bool success = TestRegistry::instance().runAllTestSuites(cout);
        return success ? 0 : 1;
    }
}

