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
    
    void TestRegistry::destroy() {
        delete instance_;
        instance_ = nullptr;
    }
    
    TestRegistry::TestRegistry() {
    }
    
    void TestRegistry::registerSuite(ITestSuite* suite) {
        suites_.push_back(unique_ptr<ITestSuite>(suite));
    }
    
    bool TestRegistry::runAllTestSuites(std::ostream &out) {
        bool success = true;
        for (auto suite = suites_.begin(); suite != suites_.end(); ++suite) {
            out << "Running suite " << (*suite)->name() << endl;
            if (not (*suite)->runTests(out)) {
                success = false;
            }
        }
        out << "Test suites executed: " << suites_.size() << endl;
        return success;
    }
}