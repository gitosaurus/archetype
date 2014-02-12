//
//  ITestSuite.h
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef archetype_ITestSuite_h
#define archetype_ITestSuite_h

#include <iostream>
#include <string>

namespace archetype {
    class ITestSuite {
        std::ostream* out_;
        int errorCount_;
    protected:
        template <class T>
        void checkCondition_(std::string filename, int lineno, std::string expr, T actual, T expected) {
            if (actual != expected) {
                *out_ << filename << ":" << lineno << " ";
                *out_ << "{" << expr << "} -> {" << actual << "}; expected {" << expected << "}" << std::endl;
                errorCount_++;
            }
        }
        virtual void runTests_(std::ostream& out) = 0;
        
        ITestSuite():
        out_(nullptr),
        errorCount_(0)
        { }

    public:
        bool runTests(std::ostream& out) {
            errorCount_ = 0;
            out_ = &out;
            runTests_(out);
            return errorCount_ != 0;
        }
    };
}

#define ARCHETYPE_TEST_EQUAL(actual, expected) checkCondition_(__FILE__, __LINE__, #actual, (actual), expected)

#endif
