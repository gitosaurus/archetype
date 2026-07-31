//
//  FileStorage.cc
//  archetype
//
//  Created by Derek Jones on 9/2/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <span>
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

    int InFileStorage::read(span<Byte> buf) {
        stream_.read(reinterpret_cast<char*>(buf.data()), static_cast<streamsize>(buf.size()));
        int bytes_read = static_cast<int>(stream_.gcount());
        remaining_ -= bytes_read;
        return bytes_read;
    }

    void InFileStorage::write(span<const Byte>) {
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

    int OutFileStorage::read(span<Byte>) {
        return 0;
    }

    void OutFileStorage::write(span<const Byte> buf) {
        stream_.write(reinterpret_cast<const char*>(buf.data()), static_cast<streamsize>(buf.size()));
    }
}
