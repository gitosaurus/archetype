//
//  UserOutput.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__UserOutput__
#define __archetype__UserOutput__

#include <string_view>
#include <memory>

namespace archetype {
    class IUserOutput {
    public:
        IUserOutput() { }
        IUserOutput(const IUserOutput&) = delete;
        IUserOutput& operator=(const IUserOutput&) = delete;
        virtual ~IUserOutput() { }

        // Text on its way out is only ever read, and never has to be
        // NUL-terminated to be written, so a view is all an implementation is
        // owed.  Most of the literal prompts in the interpreter -- "> ",
        // "(more)...", "Goodbye." -- reach here without allocating anything.
        virtual void put(std::string_view line) = 0;
        virtual void endLine() = 0;
        virtual void resetPager() { }
        virtual void banner(char ch) = 0;
        virtual void center(std::string_view line) { put(line); endLine(); }
    };
    typedef std::shared_ptr<IUserOutput> UserOutput;
}

#endif /* defined(__archetype__UserOutput__) */
