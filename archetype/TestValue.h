//
//  TestValue.h
//  archetype
//
//  Created by Derek Jones on 6/30/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestValue__
#define __archetype__TestValue__

#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestValue : public ITestSuite {
        void testSerialization_();
    protected:
        virtual void runTests_() override;
    public:
        TestValue(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestValue__) */
