//
//  TestCommandLine.hh
//  archetype
//
//  Created by Derek Jones on 2026-08-08.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestCommandLine__
#define __archetype__TestCommandLine__

#include "ITestSuite.hh"

namespace archetype {
    class TestCommandLine : public ITestSuite {
        void testEqualsForm_();
        void testSeparateWordForm_();
        void testStandaloneOptionsNeedEquals_();
        void testPositionalArguments_();
        void testErrors_();
        void testUsageCoversTable_();
    protected:
        virtual void runTests_() override;
    public:
        TestCommandLine(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestCommandLine__) */
