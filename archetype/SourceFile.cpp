//
//  SourceFile.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "SourceFile.h"

namespace archetype {
    SourceFile::SourceFile(std::string infile):
    filename_(infile),
    file_(infile.c_str()),
    fileLine_(0),
    linePos_(0),
    newlines_(false),
    consumed_(true),
    lastChar_(0)
    { }
    
    char SourceFile::readChar() {
        if (lastChar_) {
            char ch = lastChar_;
            lastChar_ = 0;
            return ch;
        } else {
            linePos_++;
            while (linePos_ >= lineBuffer_.size()) {
                if (file_.eof()) {
                    return 0;
                }
                getline(file_, lineBuffer_);
                lineBuffer_ += '\n';
                fileLine_++;
                linePos_ = 0;
            }
            return lineBuffer_[linePos_];
        }
    }
    
    void SourceFile::unreadChar(char ch) {
        lastChar_ = ch;
    }
}