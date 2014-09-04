//
//  ReadEvalPrintLoop.cpp
//  archetype
//
//  Created by Derek Jones on 9/3/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "ReadEvalPrintLoop.h"
#include "ConsoleInput.h"
#include "ConsoleOutput.h"

namespace archetype {
    int repl() {
        int errors = 0;
        try {
            UserInput in(new ConsoleInput);
            UserOutput out(new ConsoleOutput);
            Universe::instance().setInput(in);
            Universe::instance().setOutput(out);
            for (;;) {
                out->put("> ");
                string command = in->getLine();
                if (command.empty()) {
                    out->put("Goodbye.\n");
                    break;
                }
                if (command[command.size() - 1] == '\n') {
                    command.resize(command.size() - 1);
                }
                out->put("You typed \"" + command + "\"\n");
                if (command == "exit") {
                    out->put("Goodbye.\n");
                    break;
                }
            }
        } catch (const std::exception& e) {
            cout << "Exception: " << e.what() << endl;
            errors++;
        }
        Universe::destroy();
        return errors;
    }
}