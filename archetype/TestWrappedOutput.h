//
//  TestWrappedOutput.h
//  archetype
//
//  Created by Derek Jones on 9/21/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestWrappedOutput__
#define __archetype__TestWrappedOutput__

#include "ITestSuite.h"

namespace archetype {
    class TestWrappedOutput : public ITestSuite {
        void testBasicWrap_();
    protected:
        virtual void runTests_() override;
    public:
        TestWrappedOutput(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestWrappedOutput__) */
