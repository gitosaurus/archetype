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
#include <string>
#include <memory>

namespace archetype {
    class SourceFile {
        std::string filename_;
        std::istream& file_;
        int fileLine_;
        std::string lineBuffer_;
        int linePos_;
        char lastChar_;
    public:
        SourceFile(std::string source, std::istream& in);
        virtual ~SourceFile() { }
        char readChar();
        void unreadChar(char ch);
        void showPosition(std::ostream& out);
    private:
        SourceFile(const SourceFile&) = delete;
        SourceFile& operator=(const SourceFile&) = delete;
    };

    typedef std::shared_ptr<SourceFile> SourceFilePtr;
}

#endif /* defined(__archetype__SourceFile__) */
