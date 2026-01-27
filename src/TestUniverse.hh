//
//  TestUniverse.h
//  archetype
//
//  Created by Derek Jones on 3/22/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestUniverse__
#define __archetype__TestUniverse__

#include <iostream>

#include "ITestSuite.hh"

namespace archetype {
    class TestUniverse : public ITestSuite {
        void testBasicObjects_();
        void testNullIsNull_();
        void testDynamicObjects_();
        void testInclusion_();
        void testDefaultMethods_();
        void testMessagingKeywords_();
        void testSerialization_();
    protected:
        virtual void runTests_() override;
    public:
        TestUniverse(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestUniverse__) */
