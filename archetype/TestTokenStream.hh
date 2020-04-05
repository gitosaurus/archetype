//
//  TestTokenStream.h
//  archetype
//
//  Created by Derek Jones on 2/19/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestTokenStream__
#define __archetype__TestTokenStream__

#include <iostream>

#include "ITestSuite.hh"

namespace archetype {
    class TestTokenStream : public ITestSuite {
    protected:
        virtual void runTests_() override;
    public:
        TestTokenStream(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestTokenStream__) */
