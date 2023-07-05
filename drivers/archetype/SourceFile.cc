//
//  SourceFile.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <sstream>

#include "SourceFile.hh"

using namespace std;

namespace archetype {
    SourceFile::SourceFile(std::string source, stream_ptr& in):
    filename_{source},
    file_{std::move(in)},
    fileLine_{0},
    linePos_{0},
    lastChar_{0}
    { }

    char SourceFile::readChar() {
        if (lastChar_) {
            char ch = lastChar_;
            lastChar_ = 0;
            return ch;
        } else {
            linePos_++;
            while (linePos_ >= int(lineBuffer_.size())) {
                if (file_->eof()) {
                    return 0;
                }
                getline(*file_, lineBuffer_);
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

    void SourceFile::showPosition(std::ostream &out) {
        out << "At " << filename_ << ", line " << fileLine_ << ", column " << (linePos_ + 1) << ":" << endl;
        out << lineBuffer_; // has the newline built in, always
        for (int i = 0; i < linePos_; ++i) {
            out << ' ';
        }
        out << '^' << endl;
    }

    SourceFilePtr make_source_from_str(string name, string src_str) {
        stream_ptr in(new istringstream(src_str));
        return SourceFilePtr{make_shared<SourceFile>(name, in)};
    }

}
