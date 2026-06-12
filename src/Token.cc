//
//  Token.cc
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "Token.hh"
#include "Keywords.hh"

namespace archetype {
    Token::Token():
    type_(BAD_TOKEN),
    number_(0)
    { }

    Token::Token(Type_e type, int number):
    type_(type),
    number_(number)
    { }

    std::ostream& operator<<(std::ostream& out, const Token& t) {
        using enum Token::Type_e;
        switch (t.type()) {
            case RESERVED_WORD:
                out << "reserved word '" << Keywords::instance().Reserved.get(t.number()) << "'";
                break;
            case PUNCTUATION:
                out << "punctuation '" << char(t.number()) << "'";
                break;
            default:
                out << "Token(";
                switch (t.type()) {
                    case RESERVED_WORD: case PUNCTUATION:
                        assert(0);
                    case IDENTIFIER:
                        out << "identifier";
                        break;
                    case MESSAGE:
                        out << "message";
                        break;
                    case OPERATOR:
                        out << "operator";
                        break;
                    case TEXT_LITERAL:
                        out << "text literal";
                        break;
                    case QUOTE_LITERAL:
                        out << "quote literal";
                        break;
                    case NUMERIC:
                        out << "numeric";
                        break;
                    case BAD_TOKEN:
                        out << "bad token";
                        break;
                    case NEWLINE:
                        out << "newline";
                        break;
                }
                out << ", " << t.number() << ")";
                break;
        }
        return out;
    }

}
