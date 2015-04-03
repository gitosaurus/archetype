//
//  UserOutput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__UserOutput__
#define __archetype__UserOutput__

#include <string>
#include <memory>

namespace archetype {
    class IUserOutput {
    public:
        IUserOutput() { }
        virtual ~IUserOutput() { }
        virtual void put(const std::string& line) = 0;
        virtual void endLine() = 0;
        virtual void resetPager() { }
        virtual void banner(char ch) = 0;
    private:
        IUserOutput(const IUserOutput&) = delete;
        IUserOutput& operator=(const IUserOutput&) = delete;
    };
    typedef std::shared_ptr<IUserOutput> UserOutput;
}

#endif /* defined(__archetype__UserOutput__) */
