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
    typedef std::unique_ptr<std::istream> stream_ptr;
    class SourceFile {
        std::string filename_;
        stream_ptr file_;
        int fileLine_;
        std::string lineBuffer_;
        int linePos_;
        char lastChar_;
    public:
        SourceFile(std::string source, stream_ptr& in);
        SourceFile(const SourceFile&) = delete;
        SourceFile& operator=(const SourceFile&) = delete;
        virtual ~SourceFile() { }
        char readChar();
        void unreadChar(char ch);
        void showPosition(std::ostream& out);
    };

    typedef std::shared_ptr<SourceFile> SourceFilePtr;

    SourceFilePtr make_source_from_str(std::string name, std::string src_str);
}

#endif /* defined(__archetype__SourceFile__) */
