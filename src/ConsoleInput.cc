//
//  ConsoleInput.cc
//  archetype
//
//  Created by Derek Jones on 9/6/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#define _POSIX_SOURCE
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>

#ifdef _XOPEN_VERSION
#  include <termios.h>
#endif

#include "Universe.hh"
#include "ConsoleInput.hh"

using namespace std;

namespace archetype {
#ifdef _XOPEN_VERSION
    // ISIG is off while a key is awaited, so the suspend character arrives as
    // an ordinary byte rather than stopping us.  A terminal need not have one;
    // it can be disabled, and then no byte should be taken for it.
    static bool isSuspendKey(char key, const struct termios& settings) {
        cc_t suspend = settings.c_cc[VSUSP];
        return suspend != _POSIX_VDISABLE and
               static_cast<unsigned char>(key) == suspend;
    }
#endif

    char ConsoleInput::getKey() {
        cout.flush();
        Universe::instance().output()->resetPager();
#ifdef _XOPEN_VERSION
        struct termios term;
        if (tcgetattr(0, &term) < 0) {
            throw runtime_error("Could not get terminal settings: " + string(strerror(errno)));
        }
        struct termios prev = term;
        term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        char key;
        for (;;) {
            if (tcsetattr(0, TCSANOW, &term) < 0) {
                throw runtime_error("Could not set terminal: " + string(strerror(errno)));
            }
            ssize_t read_stat;
            do {
                read_stat = read(0, &key, sizeof(key));
            } while (read_stat < 0 and errno == EINTR); // e.g. resuming from a stop
            if (tcsetattr(0, TCSANOW, &prev) < 0) {
                throw runtime_error("Could not restore terminal: " + string(strerror(errno)));
            }
            if (read_stat != static_cast<ssize_t>(sizeof(key))) {
                throw runtime_error("Could not even read key from terminal");
            }
            if (not isSuspendKey(key, prev)) {
                break;
            }
            // Do by hand what the line discipline would have done.  The
            // terminal is already back the way the shell expects to find it;
            // the top of the loop makes it raw again when we are resumed.
            cout << endl;
            raise(SIGTSTP);
        }
#else
        char key;
        cin >> key;
#endif
        if (key == 3) {
            throw runtime_error("^C Interrupt");
        }
        if (key == 28) {
            throw runtime_error("^\\ Quit");
        }
        return key;
    }

    string ConsoleInput::getLine() {
        string line;
        while (not getline(cin, line)) {
            // Suspending the interpreter interrupts the pending read, and
            // returning to the foreground finds cin failed and the error flag
            // set on stdin underneath it.  Neither clears itself, so without
            // this every later turn would see an end of input that never came.
            if (feof(stdin) or errno != EINTR) {
                at_eof_ = true;
                break;
            }
            clearerr(stdin);
            cin.clear();
            line.clear();
        }
        Universe::instance().output()->resetPager();
        return line;
    }

    bool ConsoleInput::atEOF() const {
        return at_eof_;
    }
}
