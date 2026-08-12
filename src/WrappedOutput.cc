//
//  WrappedOutput.cc
//  archetype
//
//  Created by Derek Jones on 9/20/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include "WrappedOutput.hh"

using namespace std;

namespace archetype {
    const int SafetyMargin = 3;

    WrappedOutput::WrappedOutput(UserOutput output, int max_columns):
    output_{output} {
        setMaxColumns(max_columns);
        resetCursor();
    }

    void WrappedOutput::setMaxColumns(int max_columns) {
        maxColumns_ = max_columns;
    }

    void WrappedOutput::resetCursor() {
        cursor_ = 0;
    }

    WrappedOutput::~WrappedOutput() { }

    void WrappedOutput::put(std::string_view line) {
        if (maxColumns_ == 0) {
            // Sentinel meaning "do not wrap"
            output_->put(line);
            cursor_ += line.size();
            return;
        }
        // The one output class that has to own its text:  wrapping consumes
        // the line from the front as it hands out each row.
        string s{line};

        int remaining = max(0, maxColumns_ - cursor_);
        // Keep trailing punctuation from being orphaned on the next line.
        if (not s.empty() and ispunct(static_cast<unsigned char>(s[0]))) {
            remaining += SafetyMargin;
        }

        while (int(s.size()) > remaining) {
            // Walk backward to find a breaking point.
            auto cut_p = s.begin() + remaining;
            while (not isspace(static_cast<unsigned char>(*cut_p)) and cut_p != s.begin()) {
                --cut_p;
            }

            // No breaking point within reach:  the word this string opens with
            // runs past the margin.  What to do about it turns on that word
            // alone and not on the rest of the string, which will get lines of
            // its own.  A word that would fit on an empty line is owed one.
            // Only a word too long for any line has to be broken, and that one
            // fills the line it stands on, the break being unavoidable.
            if (cut_p == s.begin()) {
                auto first_space = ranges::find_if(s, [](unsigned char c) {
                    return isspace(c);
                });
                if (first_space - s.begin() <= maxColumns_ and cursor_ != 0) {
                    endLine();
                    // What stands at the front may itself be the whitespace
                    // that a break would have fallen on -- the cursor can be
                    // right up against the margin when a separating space
                    // arrives on its own.  The break spends it; it does not
                    // belong at the head of the new line.
                    auto word_p = ranges::find_if_not(s, [](unsigned char c) {
                        return isspace(c);
                    });
                    s.erase(s.begin(), word_p);
                    remaining = maxColumns_;
                    continue;
                }
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
        resetCursor();
    }

    void WrappedOutput::center(std::string_view line) {
        if (cursor_ != 0) {
            endLine();
        }
        if (maxColumns_ > 0 and int(line.size()) < maxColumns_) {
            int pad = (maxColumns_ - int(line.size())) / 2;
            output_->put(string(pad, ' '));
        }
        output_->put(line);
        endLine();
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
