//
//  PagedOutput.cc
//  archetype
//
//  Created by Derek Jones on 8/1/21.
//  Copyright © 2021 Derek Jones. All rights reserved.
//

#define _POSIX_SOURCE
#include <unistd.h>

#ifdef _XOPEN_VERSION
#  include <sys/ioctl.h>
#endif

#include "PagedOutput.hh"
#include "Universe.hh"

#include <string>

using namespace std;

namespace archetype {

    const int WrapMargin = 5;

    PagedOutput::PagedOutput(UserOutput output, int max_columns, int max_rows):
    WrappedOutput{output, max_columns},
    maxRows_{max_rows}
    {
#ifdef _XOPEN_VERSION
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) < 0) {
            perror("ioctl call to get terminal size");
        } else {
            maxRows_ = w.ws_row;
            setMaxColumns(max(0, w.ws_col - WrapMargin));
        }
#endif
    }

    PagedOutput::~PagedOutput() {
    }

    void PagedOutput::wrapWait_() {
        string prompt = "(more)...";
        string blanks(prompt.size(), ' ');
        output_->put(prompt);
        Universe::instance().input()->getKey();
        output_->put("\r" + blanks + "\r");
        rows_ = 0;
    }

    void PagedOutput::endLine() {
        output_->endLine();
        // Need at least two rows, one for text, one for the prompt,
        // otherwise there's no way to page.
        if (maxRows_ > 1  and  rows_ >= (maxRows_ - 1)) {
            wrapWait_();
        }
        rows_++;
        resetCursor();
    }

    void PagedOutput::resetPager() {
        rows_ = 0;
        resetCursor();
    }

}
