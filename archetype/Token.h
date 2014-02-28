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
        Type_e type() const     { return type_; }
        int number() const      { return number_; }
    private:
        Type_e type_;
        int number_;
    };
    
    bool operator==(const Token& t1, const Token& t2);
    bool operator!=(const Token& t1, const Token& t2);
    std::ostream& operator<<(std::ostream& out, const Token& t);
}

#endif /* defined(__archetype__Token__) */
