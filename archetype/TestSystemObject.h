//
//  TestSystemObject.h
//  archetype
//
//  Created by Derek Jones on 4/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestSystemObject__
#define __archetype__TestSystemObject__

#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestSystemObject : public ITestSuite {
        void testSorting_();
    protected:
        virtual void runTests_() override;
    public:
        TestSystemObject(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestSystemObject__) */
