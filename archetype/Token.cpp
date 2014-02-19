//
//  Token.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "Token.h"

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
        out << "Token(" << t.type() << ", " << t.number() << ")";
        return out;
    }
    
    bool operator==(const Token& t1, const Token& t2) {
        return t1.type() == t2.type() && t1.number() == t2.number();
    }

}