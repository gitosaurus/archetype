//
//  TestSourceFile.h
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestSourceFile__
#define __archetype__TestSourceFile__

#include <string>
#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestSourceFile : public ITestSuite {
    protected:
        virtual void runTests_() override;
    public:
        TestSourceFile(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestSourceFile__) */
