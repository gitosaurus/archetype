//
//  TestIdIndex.h
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestIdIndex__
#define __archetype__TestIdIndex__

#include <string>
#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestIdIndex : public ITestSuite {
    protected:
        virtual void runTests_();
    public:
        TestIdIndex(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestIdIndex__) */
