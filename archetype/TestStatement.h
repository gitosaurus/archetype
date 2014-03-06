//
//  TestStatement.h
//  archetype
//
//  Created by Derek Jones on 2/28/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestStatement__
#define __archetype__TestStatement__

#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestStatement : public ITestSuite {
    protected:
        virtual void runTests_();
    public:
        TestStatement(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestStatement__) */
