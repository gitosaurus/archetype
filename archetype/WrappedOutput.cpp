//
//  WrappedOutput.cpp
//  archetype
//
//  Created by Derek Jones on 9/20/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#include "WrappedOutput.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    const int SafetyMargin = 3;

    WrappedOutput::WrappedOutput(UserOutput output):
    output_{output},
    maxRows_{24},
    maxColumns_{75},
    rows_{0},
    cursor_{0} {
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0) {
            perror("ioctl call to get terminal size");
        } else {
            maxRows_ = w.ws_row;
            maxColumns_ = w.ws_col;
        }
    }

    WrappedOutput::~WrappedOutput() { }

    void WrappedOutput::wrapWait_() {
        string prompt = "Hit any key to continue...";
        string blanks(prompt.size(), ' ');
        output_->put(prompt);
        Universe::instance().input()->getKey();
        output_->put("\r" + blanks + "\r");
        rows_ = 0;
    }

    void WrappedOutput::put(const std::string& line) {
        if (maxColumns_ == 0) {
            // Sentinel meaning "do not wrap"
            output_->put(line);
            cursor_ += line.size();
            return;
        }
        string s = line;
        /* "thisline" starts out as the maximum number of characters that can be
         written before a newline; it gets trimmed back to being the number of
         characters from the string that are actually written on this line. */

        int maxchars = maxColumns_ - cursor_;

        const static string punctuation = ".,:;)-\"";
        if (punctuation.find(s[0]) != string::npos)
            maxchars += SafetyMargin;

        int thisline = maxchars;
        while (thisline < s.size()) {
            // TODO:  Note the inability to deal with hard tabs
            while ((thisline > 0) and (s[thisline] != ' ')) {
                thisline--;
            }

            /*
             If we were unable to find a wrapping point, it means one of two
             things:  a) the string is too long to fit on one line, and must be
             split unnaturally; or b) we are near the end of a line and must wrap
             the entire string; i.e. print nothing, finish the line and go on.
             */

            if (thisline == 0 and s.size() > maxColumns_) {
                thisline = maxchars + 1;
            }

            output_->put(s.substr(0, thisline));
            output_->endLine();
            rows_++;
            if (rows_ >= maxRows_) {
                wrapWait_();
            }
            int startnext = thisline;
            while (s[startnext] == ' ') {
                startnext++;
            }
            s = s.substr(startnext);
            cursor_ = 0;
            thisline = maxColumns_ - cursor_;
        }
        output_->put(s);
        cursor_ += s.size();
    }

    void WrappedOutput::endLine() {
        output_->endLine();
        rows_++;
        // Zero is a sentinel value meaning "never page".
        if (maxRows_ > 0  and  rows_ >= maxRows_) wrapWait_();
        cursor_ = 0;
    }

    void WrappedOutput::resetPager() {
        rows_ = 0;
    }
}