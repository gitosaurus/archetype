//
//  Token.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Token__
#define __archetype__Token__

#include <iostream>

#include "Formatting.hh"
#include "Keywords.hh"

namespace archetype {
    class Token {
    public:
        enum Type_e {
            RESERVED_WORD,
            IDENTIFIER,
            MESSAGE,
            OPERATOR,
            TEXT_LITERAL,
            QUOTE_LITERAL,
            NUMERIC,
            PUNCTUATION,
            BAD_TOKEN,
            NEWLINE
        };

        Token();
        Token(Type_e type, int number);

        // A reserved word or an operator already implies its token type, so
        // these spellings cannot pair a keyword with the wrong kind of token.
        Token(Keywords::Reserved_e word);
        Token(Keywords::Operators_e op);
        Type_e type() const     { return type_; }
        int number() const      { return number_; }

        bool operator==(const Token&) const = default;
    private:
        Type_e type_;
        int number_;
    };

    std::ostream& operator<<(std::ostream& out, const Token& t);
}

template <>
struct std::formatter<archetype::Token> : archetype::StreamedFormatter {
    auto format(const archetype::Token& token, std::format_context& ctx) const {
        return render([&](std::ostream& out) { out << token; }, ctx);
    }
};

#endif /* defined(__archetype__Token__) */
