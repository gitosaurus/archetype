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

#include "ITestSuite.hh"

namespace archetype {
    class TestSystemParser : public ITestSuite {
        void testNormalization_();
        void testBasicParsing_();
        void testPartialParsing_();
        void testProximity_();
        void testSerialization_();
    protected:
        virtual void runTests_() override;
    public:
        TestSystemParser(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestParser__) */
