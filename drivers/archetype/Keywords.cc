//
//  Keywords.cpp
//  archetype
//
//  Created by Derek Jones on 2/18/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <stdexcept>

#include "Keywords.hh"

using namespace std;

namespace archetype {
    Keywords* Keywords::instance_ = nullptr;

    Keywords& Keywords::instance() {
        if (not instance_) {
            instance_ = new Keywords();
        }
        return *instance_;
    }

    void Keywords::destroy() {
        delete instance_;
        instance_ = nullptr;
    }

#define RESERVE(key, str) if (Reserved.index(str) != key) \
        throw logic_error("Reserved word " #str " did not map to " #key)
#define OPERATOR(key, str) if (Operators.index(str) != key) \
        throw logic_error("Operator " #str " did not map to " #key)

    Keywords::Keywords() {
        RESERVE(RW_ABSENT, "ABSENT");
        RESERVE(RW_FALSE, "FALSE");
        RESERVE(RW_TRUE, "TRUE");
        RESERVE(RW_UNDEFINED, "UNDEFINED");
        RESERVE(RW_BASED, "based");
        RESERVE(RW_BREAK, "break");
        RESERVE(RW_CASE, "case");
        RESERVE(RW_CLASS, "class");
        RESERVE(RW_CREATE, "create");
        RESERVE(RW_DEFAULT, "default");
        RESERVE(RW_DESTROY, "destroy");
        RESERVE(RW_DISPLAY, "display");
        RESERVE(RW_DO, "do");
        RESERVE(RW_EACH, "each");
        RESERVE(RW_ELSE, "else");
        RESERVE(RW_END, "end");
        RESERVE(RW_FOR, "for");
        RESERVE(RW_IF, "if");
        RESERVE(RW_INCLUDE, "include");
        RESERVE(RW_KEY, "key");
        RESERVE(RW_KEYWORD, "keyword");
        RESERVE(RW_MESSAGE, "message");
        RESERVE(RW_METHODS, "methods");
        RESERVE(RW_NAMED, "named");
        RESERVE(RW_OF, "of");
        RESERVE(RW_ON, "on");
        RESERVE(RW_READ, "read");
        RESERVE(RW_SELF, "self");
        RESERVE(RW_SENDER, "sender");
        RESERVE(RW_STOP, "stop");
        RESERVE(RW_THEN, "then");
        RESERVE(RW_TYPE, "type");
        RESERVE(RW_WHILE, "while");
        RESERVE(RW_WRITE, "write");
        RESERVE(RW_WRITES, "writes");

        OPERATOR(OP_PAIR, "@");
        OPERATOR(OP_CONCAT, "&");
        OPERATOR(OP_C_CONCAT, "&:=");
        OPERATOR(OP_MULTIPLY, "*");
        OPERATOR(OP_C_MULTIPLY, "*:=");
        OPERATOR(OP_PLUS, "+");
        OPERATOR(OP_C_PLUS, "+:=");
        OPERATOR(OP_MINUS, "-");
        OPERATOR(OP_PASS, "-->");
        OPERATOR(OP_C_MINUS, "-:=");
        OPERATOR(OP_SEND, "->");
        OPERATOR(OP_DOT, ".");
        OPERATOR(OP_DIVIDE, "/");
        OPERATOR(OP_C_DIVIDE, "/:=");
        OPERATOR(OP_ASSIGN, ":=");
        OPERATOR(OP_LT, "<");
        OPERATOR(OP_LE, "<=");
        OPERATOR(OP_EQ, "=");
        OPERATOR(OP_NE, "~=");
        OPERATOR(OP_GT, ">");
        OPERATOR(OP_GE, ">=");
        OPERATOR(OP_RANDOM, "?");
        OPERATOR(OP_POWER, "^");
        OPERATOR(OP_AND, "and");
        OPERATOR(OP_CHS, "chs");
        OPERATOR(OP_LEFTFROM, "leftfrom");
        OPERATOR(OP_LENGTH, "length");
        OPERATOR(OP_NOT, "not");
        OPERATOR(OP_NUMERIC, "numeric");
        OPERATOR(OP_OR, "or");
        OPERATOR(OP_RIGHTFROM, "rightfrom");
        OPERATOR(OP_STRING, "string");
        OPERATOR(OP_WITHIN, "within");
        OPERATOR(OP_HEAD, "head");
        OPERATOR(OP_TAIL, "tail");
    }

    Keywords::~Keywords() {
    }
}
