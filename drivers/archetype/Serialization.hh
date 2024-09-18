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
#include <string>
#include <vector>

#ifdef __linux__
#include <iterator>
#endif /* linux */

namespace archetype {
    class Storage {
    public:
        typedef unsigned char Byte;
        virtual ~Storage() { }
        virtual int remaining() const = 0;
        virtual int read(Byte* buf, int nbytes) = 0;
        virtual void write(const Byte* buf, int nbytes) = 0;

        int readInteger();
        void writeInteger(int value);
    };

    Storage& operator<<(Storage& out, int value);
    Storage& operator>>(Storage& in, int& value);

    Storage& operator<<(Storage& out, std::string value);
    Storage& operator>>(Storage& in, std::string& value);

    class MemoryStorage : public Storage {
        size_t seekIndex_;
        std::vector<Byte> bytes_;
    public:
        MemoryStorage();
        virtual ~MemoryStorage() { }
        std::vector<Byte>& bytes() { return bytes_; }
        virtual int remaining() const override;
        virtual int read(Byte* buf, int nbytes) override;
        virtual void write(const Byte* buf, int nbytes) override;
    };
}

#endif /* defined(__archetype__Serialization__) */
