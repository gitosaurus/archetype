//
//  ITestSuite.h
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__ITestSuite__
#define __archetype__ITestSuite__

#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace archetype {
    class ITestSuite {
        std::string name_;
        std::ostream* out_;
        int errorCount_;
    protected:

        void reportLocation_(const std::source_location& where) {
            out() << where.file_name() << ":" << where.line()
                  << " in " << where.function_name() << " ";
        }

        // The location defaults to wherever the check was written, so the test
        // macros no longer pass __FILE__ and __LINE__ by hand.  They do still
        // exist, since only a macro can stringify the expression being checked.
        template <class T>
        void checkCondition_(std::string_view expr, T actual, T expected,
                             std::source_location where = std::source_location::current()) {
            if (actual != expected) {
                reportLocation_(where);
                out() << "{" << expr << "} -> {" << actual << "}; expected {" << expected << "}" << std::endl;
                errorCount_++;
            }
        }

        void checkCondition_(std::string_view expr, bool success,
                             std::source_location where = std::source_location::current()) {
            if (not success) {
                reportLocation_(where);
                out() << "{" << expr << "} was not true" << std::endl;
                errorCount_++;
            }
        }

        virtual void runTests_() = 0;

        ITestSuite(std::string name):
        name_(std::move(name)),
        out_(nullptr),
        errorCount_(0)
        { }

    public:
        ITestSuite(const ITestSuite&) = delete;
        ITestSuite& operator=(const ITestSuite&) = delete;
        virtual ~ITestSuite() {
            out_ = nullptr;
        }

        std::ostream& out() { return *out_; }
        std::string name() const { return name_; }
        bool runTests(std::ostream& output_for_suite) {
            errorCount_ = 0;
            out_ = &output_for_suite;
            runTests_();
            if (errorCount_) {
                out() << "Errors encountered during " << name_ << ": " << errorCount_ << std::endl;
            }
            return errorCount_ == 0;
        }
    };
}

#define ARCHETYPE_TEST_EQUAL(actual, expected) checkCondition_(#actual, (actual), (expected))
#define ARCHETYPE_TEST(expr) checkCondition_(#expr, (expr))

#endif /* defined(__archetype__ITestSuite__) */
