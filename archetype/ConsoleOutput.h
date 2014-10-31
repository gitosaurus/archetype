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
        virtual void put(const std::string& line) override { cout << line; cout.flush(); }
        virtual void endLine() override { cout << endl; }
    };
}

#endif /* defined(__archetype__ConsoleOutput__) */
