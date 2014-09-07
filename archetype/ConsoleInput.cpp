//
//  ConsoleInput.cpp
//  archetype
//
//  Created by Derek Jones on 9/6/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

#include "ConsoleInput.h"

using namespace std;

namespace archetype {
    char ConsoleInput::getKey() {
        cout.flush();
        struct termios term;
        if (tcgetattr(0, &term) < 0) {
            throw runtime_error("Could not get terminal settings: " + string(strerror(errno)));
        }
        struct termios prev = term;
        term.c_lflag &= ~(ICANON | ISIG | IEXTEN);
        if (tcsetattr(0, TCSANOW, &term) < 0) {
            throw runtime_error("Could not set terminal: " + string(strerror(errno)));
        }
        char key;
        size_t read_stat = read(0, &key, sizeof(key));
        if (tcsetattr(0, TCSANOW, &prev) < 0) {
            throw runtime_error("Could not restore terminal: " + string(strerror(errno)));
        }
        // This is an odd convention to have for 'key', but the Animal game appears to have
        // depended on it.
        cout << endl;
        if (read_stat != sizeof(key)) {
            throw runtime_error("Could not even read key from terminal");
        } else {
            return key;
        }
    }
    
    string ConsoleInput::getLine() {
        string line;
        getline(cin, line);
        return line;
    }
    
    bool ConsoleInput::atEOF() const {
        return cin.eof();
    }
}