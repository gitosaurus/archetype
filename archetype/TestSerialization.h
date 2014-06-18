//
//  TestSerialization.h
//  archetype
//
//  Created by Derek Jones on 6/17/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestSerialization__
#define __archetype__TestSerialization__

#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestSerialization : public ITestSuite {
    protected:
        virtual void runTests_() override;
    public:
        TestSerialization(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestSerialization__) */
