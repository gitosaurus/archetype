//
//  StringInput.h
//  archetype
//
//  Created by Derek Jones on 7/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__StringInput__
#define __archetype__StringInput__

#include <sstream>

#include "UserInput.hh"

namespace archetype {
    class StringInput : public IUserInput {
        std::istringstream stream_;
    public:
        StringInput(std::string input): stream_{input} { }
        virtual ~StringInput() { }
        virtual char getKey() override {
            char key{'\0'};
            stream_ >> key;
            return key;
        }
        virtual std::string getLine() override {
            std::string result;
            // Intentionally ignore return value, since EOF is indicated by a blank string
            (void)std::getline(stream_, result);
            return result;
        }
        virtual bool atEOF() const override {
            return stream_.eof();
        }
    };
}

#endif /* defined(__archetype__StringInput__) */
