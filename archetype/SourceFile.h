//
//  SourceFile.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__SourceFile__
#define __archetype__SourceFile__

#include <iostream>
#include <fstream>
#include <string>

namespace archetype {
    class SourceFile {
        std::string filename_;
        std::ifstream file_;
        int fileLine_;
        std::string lineBuffer_;
        int linePos_;
        bool newlines_;
        bool consumed_;
        char lastChar_;
    public:
        SourceFile(std::string infile);
        char readChar();
        void unreadChar(char ch);
    };
}

#endif /* defined(__archetype__SourceFile__) */
