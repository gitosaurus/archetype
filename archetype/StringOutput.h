//
//  StringOutput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef archetype_StringOutput_h
#define archetype_StringOutput_h

#include <sstream>
#include <string>

#include "UserOutput.h"

namespace archetype {
    class StringOutput : public IUserOutput {
        std::ostringstream stream_;
    public:
        virtual ~StringOutput() { }
        virtual void put(const std::string& line) override { stream_ << line; }
        virtual void endLine() override { stream_ << std::endl; }
        
        std::string getOutput() const { return stream_.str(); }
    };
    
}

#endif
