//
//  ReadEvalPrintLoop.cpp
//  archetype
//
//  Created by Derek Jones on 9/3/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <sstream>
#include <string>

#include "ReadEvalPrintLoop.h"
#include "ConsoleInput.h"
#include "ConsoleOutput.h"
#include "SourceFile.h"
#include "TokenStream.h"
#include "Statement.h"

using namespace std;

namespace archetype {
    // TODO:  Used outside of TestUniverse now.  Should be elevated to its own module
    
    inline TokenStream make_tokens_from_string(string name, string src_str) {
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr source(new SourceFile(name, in));
        return TokenStream(source);
    }
    
    inline bool is_object_declaration(const Token& t) {
        // TODO:  Do TYPE and CLASS have to be checked for every time like this?
        if (t == Token(Token::RESERVED_WORD, Keywords::RW_TYPE) ||
            t == Token(Token::RESERVED_WORD, Keywords::RW_CLASS)) {
            return true;
        }
        if (t.type() != Token::IDENTIFIER) {
            return false;
        }
        if (not Universe::instance().ObjectIdentifiers.count(t.number())) {
            return false;
        }
        int object_id = Universe::instance().ObjectIdentifiers[t.number()];
        ObjectPtr object = Universe::instance().getObject(object_id);
        return object->isPrototype();
    }
    
    int repl() {
        int errors = 0;
            UserInput in(new ConsoleInput);
            UserOutput out(new ConsoleOutput);
            Universe::instance().setInput(in);
        Universe::instance().setOutput(out);
        for (;;) {
            try {
                out->put("> ");
                string command = in->getLine();
                if (command.empty()) {
                    out->put("Goodbye.\n");
                    break;
                }
                if (command[command.size() - 1] == '\n') {
                    command.resize(command.size() - 1);
                }
                if (command == "exit") {
                    out->put("Goodbye.\n");
                    break;
                }
                TokenStream input_tokens = make_tokens_from_string("input", command);
                if (not input_tokens.fetch()) {
                    continue;
                }
                if (is_object_declaration(input_tokens.token())) {
                    input_tokens.didNotConsume();
                    if (Universe::instance().make(input_tokens)) {
                        out->put("Added to Universe.\n");
                    }
                } else {
                    input_tokens.didNotConsume();
                    Statement stmt = make_statement(input_tokens);
                    if (stmt) {
                        Value result = stmt->execute();
                        ostringstream sout;
                        result->display(sout);
                        sout << endl;
                        out->put(sout.str());
                    } else {
                        out->put("Could not compile as a statement.\n");
                    }
                }
            } catch (const std::exception& e) {
                cout << "Exception: " << e.what() << endl;
                errors++;
            }
        }
        Universe::destroy();
        return errors;
    }
}