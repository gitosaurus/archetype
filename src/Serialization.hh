//
//  Serialization.h
//  archetype
//
//  Created by Derek Jones on 6/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Serialization__
#define __archetype__Serialization__

#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace archetype {
    class Storage {
    public:
        typedef unsigned char Byte;
        virtual ~Storage() { }
        virtual int remaining() const = 0;

        // The buffer carries its own length, so a caller cannot hand over a
        // pointer and a size that disagree.  read returns the count actually
        // transferred, which may be less than the span for a truncated source.
        virtual int read(std::span<Byte> buf) = 0;
        virtual void write(std::span<const Byte> buf) = 0;

        int readInteger();
        void writeInteger(int value);
    };

    Storage& operator<<(Storage& out, int value);
    Storage& operator>>(Storage& in, int& value);

    // Writing reads the characters and nothing else; reading has to grow a
    // string, so it takes one to fill.
    Storage& operator<<(Storage& out, std::string_view value);
    Storage& operator>>(Storage& in, std::string& value);

    class MemoryStorage : public Storage {
        size_t seekIndex_;
        std::vector<Byte> bytes_;
    public:
        MemoryStorage();
        virtual ~MemoryStorage() { }
        std::vector<Byte>& bytes() { return bytes_; }
        virtual int remaining() const override;
        virtual int read(std::span<Byte> buf) override;
        virtual void write(std::span<const Byte> buf) override;
    };
}

#endif /* defined(__archetype__Serialization__) */
