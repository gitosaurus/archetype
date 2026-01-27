//
//  IUserInput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__UserInput__
#define __archetype__UserInput__

#include <memory>
#include <string>

namespace archetype {
    class IUserInput {
    public:
        IUserInput() { }
        IUserInput(const IUserInput&) = delete;
        IUserInput& operator=(const IUserInput&) = delete;
        virtual ~IUserInput() { }

        virtual char getKey() = 0;
        virtual std::string getLine() = 0;
        virtual bool atEOF() const = 0;
    };

    typedef std::shared_ptr<IUserInput> UserInput;
}

#endif /* defined(__archetype__UserInput__) */
