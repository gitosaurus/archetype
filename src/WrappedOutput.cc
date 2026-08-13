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

    namespace {
        // A mark that clings to the word in front of it:  the "--" and "---"
        // that stand in for an em dash, and the "..." of an ellipsis.  A lone
        // hyphen belongs to the word it joins and a lone period ends a
        // sentence, so neither of those needs a rule here.
        bool isClingingMark(char c) {
            return c == '-' or c == '.';
        }

        // Does the text at p open with such a mark?  No break is chosen that
        // would put one at the head of a line.  A break forced for want of
        // anywhere else to put it is another matter:  the margin is held to
        // in that case, the same as for a word too long to fit.
        bool opensWithMark(std::string::const_iterator p,
                           std::string::const_iterator end) {
            return end - p >= 2 and isClingingMark(p[0]) and p[1] == p[0];
        }

        // Would a line ending at p end with such a mark, and the next one
        // open with a word?  This is the one position inside a word where a
        // break may fall:  "the chair---" | "sort of".  The word on the far
        // side is the point of the test -- breaking "am I..." from "?" would
        // strand the question mark, which is the very thing being avoided.
        bool followsMark(std::string::const_iterator begin,
                         std::string::const_iterator p) {
            return p - begin >= 2 and
                   isClingingMark(p[-1]) and p[-2] == p[-1] and
                   isalnum(static_cast<unsigned char>(*p));
        }
    }

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
        // The concession is meant for a stub -- a period, a comma, a closing
        // quote -- and not for a whole clause that merely opens with a mark,
        // which is what a fragment beginning with a dash looks like from
        // here.  Handing those three columns to fifty characters of prose
        // just prints a line past the margin.
        if (not s.empty() and ispunct(static_cast<unsigned char>(s[0]))) {
            auto first_space = ranges::find_if(s, [](unsigned char c) {
                return isspace(c);
            });
            if (first_space - s.begin() <= SafetyMargin) {
                remaining += SafetyMargin;
            }
        }

        while (int(s.size()) > remaining) {
            // Whether a break may fall at p.  Whitespace is the usual place;
            // the far side of a dash or an ellipsis is the other.  Neither
            // will do if what follows opens with such a mark, because a line
            // may not begin with one:  the mark belongs to the word behind it
            // and has to travel with it.
            auto marksNextLine = [&s](string::const_iterator p) {
                while (p != s.cend() and isspace(static_cast<unsigned char>(*p))) {
                    ++p;
                }
                return opensWithMark(p, s.cend());
            };
            auto breakable = [&](string::const_iterator p) {
                if (p == s.cbegin()) {
                    return false;
                }
                if (not isspace(static_cast<unsigned char>(*p)) and
                    not followsMark(s.cbegin(), p)) {
                    return false;
                }
                return not marksNextLine(p);
            };

            // Walk backward to find a breaking point.
            auto cut_p = s.begin() + remaining;
            while (cut_p != s.begin() and not breakable(cut_p)) {
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
