//
//  TestExpression.h
//  archetype
//
//  Created by Derek Jones on 2/25/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestExpression__
#define __archetype__TestExpression__

#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestExpression : public ITestSuite {
        void testTranslation_();
        void testEvaluation_();
        void testSerialization_();
    protected:
        virtual void runTests_() override;
    public:
        TestExpression(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestExpression__) */
