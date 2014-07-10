//
//  ConsoleOutput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef archetype_ConsoleOutput_h
#define archetype_ConsoleOutput_h

#include <iostream>

#include "UserOutput.h"

namespace archetype {
    class ConsoleOutput : public IUserOutput {
    public:
        virtual ~ConsoleOutput() { }
        virtual void put(const std::string& line) { cout << line; }
        virtual void endLine() { cout << endl; }
    };
}

#endif
