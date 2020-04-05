//
//  ConsoleInput.cpp
//  archetype
//
//  Created by Derek Jones on 9/6/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#define _POSIX_SOURCE
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <cstring>

#ifdef _XOPEN_VERSION
#  include <termios.h>
#endif

#include "Universe.hh"
#include "ConsoleInput.hh"

using namespace std;

namespace archetype {
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
        if (tcsetattr(0, TCSANOW, &term) < 0) {
            throw runtime_error("Could not set terminal: " + string(strerror(errno)));
        }
        char key;
        size_t read_stat = read(0, &key, sizeof(key));
        if (tcsetattr(0, TCSANOW, &prev) < 0) {
            throw runtime_error("Could not restore terminal: " + string(strerror(errno)));
        }
        if (read_stat != sizeof(key)) {
            throw runtime_error("Could not even read key from terminal");
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
        getline(cin, line);
        Universe::instance().output()->resetPager();
        return line;
    }

    bool ConsoleInput::atEOF() const {
        return cin.eof();
    }
}
