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
        std::deque<std::unique_ptr<ITestSuite>> suites_;
    public:
        static TestRegistry& instance();
        static void destroy();

        void registerSuite(ITestSuite* suite);
        bool runAllTestSuites(std::ostream& out);
    private:
        static TestRegistry* instance_;

        TestRegistry();
        TestRegistry(const TestRegistry&) = delete;
        TestRegistry& operator=(const TestRegistry) = delete;
        ~TestRegistry();
    };
}

#define ARCHETYPE_TEST_REGISTER(TNAME) \
static class register_##TNAME { \
public: register_##TNAME() { \
TestRegistry::instance().registerSuite(new TNAME(#TNAME)); \
} \
} stub_register_##TNAME

#endif /* defined(__archetype__TestRegistry__) */
