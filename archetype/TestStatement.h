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
        void testConstruction_();
        void testExecution_();
        void testLoopBreaks_();
        void testSerialization_();
    protected:
        virtual void runTests_() override;
    public:
        TestStatement(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestStatement__) */
