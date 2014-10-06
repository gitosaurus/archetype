//
//  TestWrappedOutput.cpp
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

#include "TestWrappedOutput.h"
#include "TestRegistry.h"
#include "StringOutput.h"
#include "WrappedOutput.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestWrappedOutput);

    void TestWrappedOutput::testBasicWrap_() {
        UserOutput user_soutput{new StringOutput};
        StringOutput& strout{*dynamic_cast<StringOutput*>(user_soutput.get())};
        UserOutput user_output{new WrappedOutput{user_soutput}};
        WrappedOutput& wrout{*dynamic_cast<WrappedOutput*>(user_output.get())};
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

    void TestWrappedOutput::runTests_() {
        testBasicWrap_();
    }
}