//
//  TestInspectUniverse.hh
//  archetype
//
//  Created by Derek Jones on 2026-03-18.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestInspectUniverse__
#define __archetype__TestInspectUniverse__

#include <iostream>

#include "ITestSuite.hh"

namespace archetype {
    class TestInspectUniverse : public ITestSuite {
        void testNullParentType_();
        void testVocabSyntax_();
        void testProximateSyntax_();
        void testParserBlock_();
        void testMaterializable_();
        void testDeclaredAttributes_();
    protected:
        virtual void runTests_() override;
    public:
        TestInspectUniverse(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestInspectUniverse__) */
