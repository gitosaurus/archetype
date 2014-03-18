//
//  TestObject.h
//  archetype
//
//  Created by Derek Jones on 3/17/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestObject__
#define __archetype__TestObject__

#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestObject : public ITestSuite {
        void testObjects_();
        void testInheritance_();
    protected:
        virtual void runTests_();
    public:
        TestObject(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestObject__) */
