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
        switch (t.type()) {
            case Token::RESERVED_WORD:
                out << "reserved word '" << Keywords::instance().Reserved.get(t.number()) << "'";
                break;
            case Token::PUNCTUATION:
                out << "punctuation '" << char(t.number()) << "'";
                break;
            default:
                out << "Token(";
                switch (t.type()) {
                    case Token::RESERVED_WORD: case Token::PUNCTUATION:
                        assert(0);
                    case Token::IDENTIFIER:
                        out << "identifier";
                        break;
                    case Token::MESSAGE:
                        out << "message";
                        break;
                    case Token::OPERATOR:
                        out << "operator";
                        break;
                    case Token::TEXT_LITERAL:
                        out << "text literal";
                        break;
                    case Token::QUOTE_LITERAL:
                        out << "quote literal";
                        break;
                    case Token::NUMERIC:
                        out << "numeric";
                        break;
                    case Token::BAD_TOKEN:
                        out << "bad token";
                        break;
                    case Token::NEWLINE:
                        out << "newline";
                        break;
                }
                out << ", " << t.number() << ")";
                break;
        }
        return out;
    }

    bool operator==(const Token& t1, const Token& t2) {
        return t1.type() == t2.type() and t1.number() == t2.number();
    }

    bool operator!=(const Token& t1, const Token& t2) {
        return not (t1 == t2);
    }

}
