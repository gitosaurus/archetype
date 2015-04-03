//
//  ConsoleOutput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__ConsoleOutput__
#define __archetype__ConsoleOutput__

#include <iostream>

#include "UserOutput.h"

namespace archetype {
    class ConsoleOutput : public IUserOutput {
    public:
        virtual ~ConsoleOutput() { }
        virtual void put(const std::string& line) override { std::cout << line; std::cout.flush(); }
        virtual void endLine() override { std::cout << std::endl; }
        virtual void banner(char ch) override { for (int i = 0; i < 80; ++i) std::cout << ch; endLine(); }
    };
}

#endif /* defined(__archetype__ConsoleOutput__) */
