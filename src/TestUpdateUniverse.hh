//
//  TestUpdateUniverse.hh
//  archetype
//
//  Created by Derek Jones on 2026-04-19.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestUpdateUniverse__
#define __archetype__TestUpdateUniverse__

#include <iostream>

#include "ITestSuite.hh"

namespace archetype {
    class TestUpdateUniverse : public ITestSuite {
        void testPlainUpdate_();
        void testSitrepAppendsParserRdf_();
        void testSitrepUnpacksPairs_();
        void testInspectAppendsStateRdf_();
        void testTurnAsksForInput_();
        void testDeclinedInputEndsTheTurn_();
        void testSeedBetweenLoadAndTurn_();
    protected:
        virtual void runTests_() override;
    public:
        TestUpdateUniverse(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestUpdateUniverse__) */
