//
//  TestParser.h
//  archetype
//
//  Created by Derek Jones on 5/25/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestParser__
#define __archetype__TestParser__

#include <iostream>

#include "ITestSuite.h"

namespace archetype {
    class TestParser : public ITestSuite {
        void testNormalization_();
        void testBasicParsing_();
    protected:
        virtual void runTests_() override;
    public:
        TestParser(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestParser__) */
