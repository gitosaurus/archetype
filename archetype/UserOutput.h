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
        virtual ~IUserOutput() { }
        virtual void put(const std::string& line) = 0;
        virtual void endLine() = 0;
    };
    typedef std::shared_ptr<IUserOutput> UserOutput;
}

#endif
