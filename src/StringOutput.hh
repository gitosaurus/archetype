//
//  StringOutput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__StringOutput__
#define __archetype__StringOutput__

#include <sstream>
#include <string>

#include "UserOutput.hh"

namespace archetype {
    class StringOutput : public IUserOutput {
        std::ostringstream stream_;
    public:
        virtual ~StringOutput() { }
        virtual void put(const std::string& line) override { stream_ << line; }
        virtual void endLine() override { stream_ << std::endl; }
        virtual void banner(char ch) override { for (int i = 0; i < 80; ++i) { stream_ << ch; } endLine(); }

        std::string getOutput() const { return stream_.str(); }
    };

}

#endif /* defined(__archetype__StringOutput__) */
