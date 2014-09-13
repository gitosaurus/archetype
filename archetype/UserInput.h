//
//  IUserInput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef archetype_UserInput_h
#define archetype_UserInput_h

#include <memory>
#include <string>

namespace archetype {
    class IUserInput {
    public:
        IUserInput() { }
        virtual ~IUserInput() { }
        virtual char getKey() = 0;
        virtual std::string getLine() = 0;
        virtual bool atEOF() const = 0;
    private:
        IUserInput(const IUserInput&) = delete;
        IUserInput& operator=(const IUserInput&) = delete;
    };

    typedef std::shared_ptr<IUserInput> UserInput;
}

#endif
