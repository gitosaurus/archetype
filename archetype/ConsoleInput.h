//
//  ConsoleInput.h
//  archetype
//
//  Created by Derek Jones on 9/6/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__ConsoleInput__
#define __archetype__ConsoleInput__

#include "UserInput.h"

namespace archetype {
    class ConsoleInput : public IUserInput {
    public:
        virtual ~ConsoleInput() { }
        virtual char getKey() override;
        virtual std::string getLine() override;
        virtual bool atEOF() const override;
    };
}

#endif /* defined(__archetype__ConsoleInput__) */
