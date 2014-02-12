//
//  TestRegistry.h
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TestRegistry__
#define __archetype__TestRegistry__

#include <iostream>
#include <deque>
#include <memory>

#include "ITestSuite.h"

namespace archetype {
    class TestRegistry {
        static TestRegistry* instance_;
        std::deque<std::shared_ptr<ITestSuite>> suites_;
        
        TestRegistry();
        // No copying
        TestRegistry(const TestRegistry&);
        TestRegistry& operator=(const TestRegistry);
    public:
        static TestRegistry& instance();
        void registerSuite(ITestSuite* suite);
        bool runAllTestSuites(std::ostream& out);
    };
}

#define ARCHETYPE_TEST_REGISTER(TNAME) \
static class register_##TNAME { \
public: register_##TNAME() { \
TestRegistry::instance().registerSuite(new TNAME(#TNAME)); \
} \
} stub_register_##TNAME

#endif /* defined(__archetype__TestRegistry__) */
