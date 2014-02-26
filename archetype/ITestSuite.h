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
        std::string name_;
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
        
        void checkCondition_(std::string filename, int lineno, std::string expr, bool success) {
            if (not success) {
                *out_ << filename << ":" << lineno << " ";
                *out_ << "{" << expr << "} was not true" << std::endl;
                errorCount_++;
            }
        }
        
        virtual void runTests_(std::ostream& out) = 0;
        
        ITestSuite(std::string name):
        name_(name),
        out_(nullptr),
        errorCount_(0)
        { }

    public:
        std::string name() const { return name_; }
        bool runTests(std::ostream& out) {
            errorCount_ = 0;
            out_ = &out;
            runTests_(out);
            return errorCount_ == 0;
        }
    };
}

#define ARCHETYPE_TEST_EQUAL(actual, expected) checkCondition_(__FILE__, __LINE__, #actual, (actual), expected)
#define ARCHETYPE_TEST(expr) checkCondition_(__FILE__, __LINE__, #expr, (expr));

#endif
