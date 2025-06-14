//
//  ReadEvalPrintLoop.cc
//  archetype
//
//  Created by Derek Jones on 9/3/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <sstream>
#include <string>

#include "ReadEvalPrintLoop.hh"
#include "ConsoleInput.hh"
#include "ConsoleOutput.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Statement.hh"

using namespace std;

namespace archetype {
    inline TokenStream make_tokens_from_string(string name, string src_str) {
        stream_ptr in{new istringstream{src_str}};
        SourceFilePtr source{make_shared<SourceFile>(name, in)};
        return TokenStream{source};
    }

    inline bool is_object_declaration(const Token& t) {
        if (t == Token{Token::RESERVED_WORD, Keywords::RW_TYPE} ||
            t == Token{Token::RESERVED_WORD, Keywords::RW_CLASS}) {
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
        for (;;) {
            try {
                UserInput in = Universe::instance().input();
                UserOutput out = Universe::instance().output();
                out->put("> ");
                string command = in->getLine();
                if (in->atEOF() or command == "exit") {
                    out->put("Goodbye.");
                    out->endLine();
                    break;
                }
                TokenStream input_tokens = make_tokens_from_string("input", command);
                while (input_tokens.fetch()) {
                    if (is_object_declaration(input_tokens.token())) {
                        input_tokens.didNotConsume();
                        if (Universe::instance().make(input_tokens)) {
                            out->put("Added to Universe.");
                            out->endLine();
                        } else {
                            out->put("Could not compile as an object declaration.");
                            out->endLine();
                            break;
                        }
                    } else {
                        input_tokens.didNotConsume();
                        Statement stmt = make_statement(input_tokens);
                        if (not stmt) {
                            out->put("Could not compile as a statement.");
                            out->endLine();
                            break;
                        }
                        Value result = stmt->execute();
                        ostringstream sout;
                        sout << "[";
                        result->display(sout);
                        sout << "]";
                        Value result_str = result->stringConversion();
                        if (result_str->isDefined()) {
                            sout << " " << result_str->getString();
                        }
                        sout << endl;
                        out->put(sout.str());
                    }
                }
            } catch (const std::exception& e) {
                cout << "Exception: " << e.what() << endl;
                errors++;
            }
        }
        return errors;
    }
}
