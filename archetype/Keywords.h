//
//  Keywords.h
//  archetype
//
//  Created by Derek Jones on 2/18/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Keywords__
#define __archetype__Keywords__

#include <iostream>

#include "StringIdIndex.h"

namespace archetype {
    class Keywords {
    public:
        enum Reserved_e {
            RW_ABSENT,
            RW_FALSE,
            RW_TRUE,
            RW_UNDEFINED,
            RW_BASED,
            RW_BREAK,
            RW_CASE,
            RW_CLASS,
            RW_CREATE,
            RW_DEFAULT,
            RW_DESTROY,
            RW_DISPLAY,
            RW_DO,
            RW_EACH,
            RW_ELSE,
            RW_END,
            RW_FOR,
            RW_IF,
            RW_INCLUDE,
            RW_KEY,
            RW_KEYWORD,
            RW_MESSAGE,
            RW_METHODS,
            RW_NAMED,
            RW_OF,
            RW_ON,
            RW_READ,
            RW_SELF,
            RW_SENDER,
            RW_STOP,
            RW_THEN,
            RW_TYPE,
            RW_WHILE,
            RW_WRITE,
            RW_WRITES,
            NumReserved
        };

        enum Operators_e {
            OP_CONCAT,
            OP_C_CONCAT,
            OP_MULTIPLY,
            OP_C_MULTIPLY,
            OP_PLUS,
            OP_C_PLUS,
            OP_MINUS,
            OP_PASS,
            OP_C_MINUS,
            OP_SEND,
            OP_DOT,
            OP_DIVIDE,
            OP_C_DIVIDE,
            OP_ASSIGN,
            OP_LT,
            OP_LE,
            OP_EQ,
            OP_NE,
            OP_GT,
            OP_GE,
            OP_RANDOM,
            OP_POWER,
            OP_AND,
            OP_CHS,
            OP_LEFTFROM,
            OP_LENGTH,
            OP_NOT,
            OP_NUMERIC,
            OP_OR,
            OP_RIGHTFROM,
            OP_STRING,
            OP_WITHIN,
            NumOperators,

            // Kept outside of the range of valid operators, but
            // it functions like an operator at run-time.
            OP_LPAREN
        };

        StringIdIndex Reserved;
        StringIdIndex Operators;

        static Keywords& instance();

    private:
        static Keywords* instance_;

        Keywords();
        Keywords(const Keywords&) = delete;
        Keywords& operator=(const Keywords&) = delete;
    };
}

#endif /* defined(__archetype__Keywords__) */
