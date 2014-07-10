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
        virtual ~IUserInput() { }
        virtual char getKey() const = 0;
        virtual std::string getLine() const = 0;
    private:
        IUserInput& operator=(const IUserInput&) = delete;
        IUserInput(const IUserInput&) = delete;
    };
    
    typedef std::shared_ptr<IUserInput> UserInput;
}

#endif
