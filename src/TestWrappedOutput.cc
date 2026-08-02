//
//  TestWrappedOutput.cc
//  archetype
//
//  Created by Derek Jones on 9/21/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <string>
#include <sstream>
#include <deque>
#include <iterator>
#include <algorithm>

#include "TestWrappedOutput.hh"
#include "TestRegistry.hh"
#include "StringOutput.hh"
#include "WrappedOutput.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestWrappedOutput);

    void TestWrappedOutput::testBasicWrap_() {
        UserOutput user_soutput = make_shared<StringOutput>();
        StringOutput& strout(*dynamic_cast<StringOutput*>(user_soutput.get()));
        UserOutput user_output = make_shared<WrappedOutput>(user_soutput);
        WrappedOutput& wrout(*dynamic_cast<WrappedOutput*>(user_output.get()));
        wrout.setMaxColumns(10);
        string utterance = "Now is the time for all good men to come to the aid of their country.";
        user_output->put(utterance);
        user_output->endLine();
        string result = strout.getOutput();
        out() << result;
        istringstream istr(result);
        string line;
        deque<string> lines;
        while (getline(istr, line)) {
            ARCHETYPE_TEST(line.size() <= 10);
            lines.push_back(line);
        }
        ARCHETYPE_TEST(lines.size() > 1);
        // Paste it back together and make sure it matches the original
        ostringstream back_out;
        copy(lines.begin(), lines.end(), ostream_iterator<string>(back_out, " "));
        string back_out_s = back_out.str();
        back_out_s.resize(back_out_s.size() - 1);
        ARCHETYPE_TEST_EQUAL(back_out_s, utterance);
        out() << "TestWrappedOutput finished." << endl;
    }

    void TestWrappedOutput::testCenter_() {
        UserOutput user_soutput = make_shared<StringOutput>();
        StringOutput& strout(*dynamic_cast<StringOutput*>(user_soutput.get()));
        UserOutput user_output = make_shared<WrappedOutput>(user_soutput);
        WrappedOutput& wrout(*dynamic_cast<WrappedOutput*>(user_output.get()));

        auto delta = [&](size_t& mark) {
            string all = strout.getOutput();
            string d = all.substr(mark);
            mark = all.size();
            return d;
        };
        size_t mark = 0;

        wrout.setMaxColumns(20);
        user_output->center("Hello");
        // (20 - 5) / 2 = 7 leading spaces
        ARCHETYPE_TEST_EQUAL(delta(mark), string("       Hello\n"));

        // A line at the column count gets no padding.
        wrout.setMaxColumns(5);
        user_output->center("Hello");
        ARCHETYPE_TEST_EQUAL(delta(mark), string("Hello\n"));

        // A line wider than the column count gets no padding.
        user_output->center("Hello, world!");
        ARCHETYPE_TEST_EQUAL(delta(mark), string("Hello, world!\n"));

        // Zero columns means indeterminate width: emit unpadded.
        wrout.setMaxColumns(0);
        user_output->center("X");
        ARCHETYPE_TEST_EQUAL(delta(mark), string("X\n"));

        out() << "TestWrappedOutput::testCenter_ finished." << endl;
    }

    void TestWrappedOutput::runTests_() {
        testBasicWrap_();
        testCenter_();
    }
}
