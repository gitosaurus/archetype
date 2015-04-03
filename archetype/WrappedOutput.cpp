//
//  WrappedOutput.cpp
//  archetype
//
//  Created by Derek Jones on 9/20/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <cctype>
#include <sys/ioctl.h>
#include <unistd.h>

#include "WrappedOutput.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    const int WrapMargin = 5;
    const int SafetyMargin = 3;

    WrappedOutput::WrappedOutput(UserOutput output):
    output_{output},
    maxRows_{24},
    maxColumns_{80},
    rows_{0},
    cursor_{0} {
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0) {
            perror("ioctl call to get terminal size");
        } else {
            maxRows_ = w.ws_row;
            maxColumns_ = w.ws_col;
        }
        maxColumns_ = max(0, maxColumns_ - WrapMargin);
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

        int remaining = max(0, maxColumns_ - cursor_);
        // Keep trailing punctuation from being orphaned on the next line.
        if (not s.empty() and ispunct(s[0])) {
            remaining += SafetyMargin;
        }

        while (int(s.size()) > remaining) {
            // Walk backward to find a breaking point.
            auto cut_p = s.begin() + remaining;
            while (not isspace(*cut_p) and cut_p != s.begin()) {
                --cut_p;
            }

            /*
             If we were unable to find a wrapping point, it means one of two
             things:  a) the string is too long to fit on one line, and must be
             split unnaturally; or b) we are near the end of a line and must wrap
             the entire string; i.e. print nothing, finish the line and go on.
             */

            if (cut_p == s.begin() and int(s.size()) > maxColumns_) {
                cut_p = s.begin() + remaining;
            }

            string written(s.begin(), cut_p);
            output_->put(written);
            endLine();
            while (cut_p != s.end() and isspace(*cut_p)) {
                ++cut_p;
            }
            s.erase(s.begin(), cut_p);
            remaining = maxColumns_;
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
        cursor_ = 0;
    }

    void WrappedOutput::banner(char ch) {
        if (cursor_ != 0) {
            endLine();
        }
        string banner_str(maxColumns_, ch);
        output_->put(banner_str);
        endLine();
    }
}
