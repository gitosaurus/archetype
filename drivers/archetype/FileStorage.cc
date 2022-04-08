//
//  FileStorage.cpp
//  archetype
//
//  Created by Derek Jones on 9/2/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <string>

#include "FileStorage.hh"

using namespace std;

namespace archetype {
    InFileStorage::InFileStorage(std::string filename)
    {
        stream_.open(filename.c_str(), ios::in | ios::binary);
        stream_.seekg(0, ios::end);
        remaining_ = static_cast<int>(stream_.tellg());
        stream_.seekg(0, ios::beg);
    }

    bool InFileStorage::ok() const {
        return stream_.is_open();
    }

    int InFileStorage::remaining() const {
        return remaining_;
    }

    int InFileStorage::read(Byte *buf, int nbytes) {
        stream_.read(reinterpret_cast<char *>(buf), nbytes);
        int bytes_read = static_cast<int>(stream_.gcount());
        remaining_ -= bytes_read;
        return bytes_read;
    }

    void InFileStorage::write(const Byte *buf, int nbytes) {
    }

    OutFileStorage::OutFileStorage(std::string filename)
    {
        stream_.open(filename.c_str(), ios::out | ios::binary);
    }

    bool OutFileStorage::ok() const {
        return stream_.is_open();
    }

    int OutFileStorage::remaining() const {
        return 0;
    }

    int OutFileStorage::read(Byte *buf, int nbytes) {
        return 0;
    }

    void OutFileStorage::write(const Byte *buf, int nbytes) {
        stream_.write(reinterpret_cast<const char*>(buf), nbytes);
    }
}
