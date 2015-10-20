//
//  ConsoleInput.cpp
//  archetype
//
//  Created by Derek Jones on 9/6/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <stdexcept>
#include <cstring>

#ifdef _XOPEN_VERSION
#  include <termios.h>
#  include <unistd.h>
#endif

#include "Universe.h"
#include "ConsoleInput.h"

using namespace std;

namespace archetype {
    char ConsoleInput::getKey() {
        cout.flush();
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
        } else {
            return key;
        }
#else
		char key;
		cin >> key;
#endif
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
