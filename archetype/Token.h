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
        int type_;
        int number_;
    public:
        Token(int type, int number): type_(type), number_(number) { }
        int type() const   { return type_; }
        int number() const { return number_; }
    };
}

#endif /* defined(__archetype__Token__) */
