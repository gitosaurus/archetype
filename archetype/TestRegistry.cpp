//
//  TestRegistry.cpp
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "TestRegistry.h"

using namespace std;

namespace archetype {
    TestRegistry* TestRegistry::instance_ = 0;
    
    TestRegistry& TestRegistry::instance() {
        if (not instance_) {
            instance_ = new TestRegistry();
        }
        return *instance_;
    }
    
    TestRegistry::TestRegistry() {
    }
    
    void TestRegistry::registerSuite(ITestSuite* suite) {
        suites_.push_back(shared_ptr<ITestSuite>(suite));
    }
    
    bool TestRegistry::runAllTestSuites(std::ostream &out) {
        bool success = true;
        for (auto suite : suites_) {
            out << "Running suite " << suite->name() << endl;
            if (not suite->runTests(out)) {
                success = false;
            }
        }
        out << "Test suites executed: " << suites_.size() << endl;
        return success;
    }
}