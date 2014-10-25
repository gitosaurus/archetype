//
//  UserOutput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef archetype_UserOutput_h
#define archetype_UserOutput_h

#include <string>
#include <memory>

namespace archetype {
    class IUserOutput {
    public:
        IUserOutput() { }
        virtual ~IUserOutput() { }
        virtual void put(const std::string& line) = 0;
        virtual void endLine() = 0;

        // TODO:  Better plumbing?
        virtual void resetPager() { }
    private:
        IUserOutput(const IUserOutput&) = delete;
        IUserOutput& operator=(const IUserOutput&) = delete;
    };
    typedef std::shared_ptr<IUserOutput> UserOutput;
}

#endif
