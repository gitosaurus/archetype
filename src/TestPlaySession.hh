//
//  TestPlaySession.hh
//  archetype
//
//  Created by Derek Jones on 2026-08-01.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestPlaySession__
#define __archetype__TestPlaySession__

#include <iostream>

#include "ITestSuite.hh"

namespace archetype {
    class TestPlaySession : public ITestSuite {
        void testResidentMatchesStateless_();
        void testSaveResumesResidentSession_();
        void testEndedUniverseRefusesAnotherTurn_();
    protected:
        virtual void runTests_() override;
    public:
        TestPlaySession(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestPlaySession__) */
