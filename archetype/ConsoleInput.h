//
//  ConsoleInput.h
//  archetype
//
//  Created by Derek Jones on 9/3/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__ConsoleInput__
#define __archetype__ConsoleInput__

#include <iostream>

#include "UserInput.h"

namespace archetype {
    class ConsoleInput : public IUserInput {
    public:
        virtual ~ConsoleInput() { }
        virtual char getKey() override {
            // TODO:  Totally not good enough; befouls "animal.ach"
            char key;
            std::cin >> key;
            return key;
        }
        virtual std::string getLine() override {
            std::string line;
            std::getline(std::cin, line);
            return line;
        }
        virtual bool atEOF() const override {
            return cin.eof();
        }
    };
}

#endif /* defined(__archetype__ConsoleInput__) */
