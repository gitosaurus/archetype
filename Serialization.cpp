//
//  Serialization.cpp
//  archetype
//
//  Created by Derek Jones on 6/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <stdexcept>
#include <algorithm>

#include "Serialization.h"

using namespace std;

namespace archetype {
    const Storage::Byte SignBit = 0x01;
    const Storage::Byte MoreBit = 0x80;
    const Storage::Byte PayloadBits = 0x7F;
    const Storage::Byte FirstBytePayloadBits = 0x3F;
    
    int Storage::readInteger() {
        int bytes_left = remaining();
        if (not bytes_left) {
            throw invalid_argument("No more bytes remaining; cannot read an integer");
        }
        Byte byte;
        read(&byte, sizeof(byte));
        bool more = (byte & MoreBit);
        byte &= ~MoreBit;
        // The sign bit is the very first bit deserialized.
        // Note it for this number and shift if off.
        bool negative = (byte & SignBit);
        byte >>= 1;
        int bits = 6;
        int result = byte;
        while (more) {
            if (not read(&byte, sizeof(byte))) {
                throw invalid_argument("End of storage in the middle of a continued integer");
            }
            int next_part = (byte & PayloadBits);
            next_part <<= bits;
            result |= next_part;
            more = (byte & MoreBit);
            bits += 7;
        }
        return negative ? -result : result;
    }
    
    void Storage::writeInteger(int value) {
        bool negative = value < 0;
        if (negative) {
            value = -value;
        }
        int bits = 6;
        Byte byte = (value & FirstBytePayloadBits);
        byte <<= 1;
        if (negative) {
            byte |= SignBit;
        } else {
            byte &= ~SignBit;
        }
        do {
            value >>= bits;
            if (value) {
                byte |= MoreBit;
            }
            write(&byte, sizeof(byte));
            bits = 7;
            byte = (value & PayloadBits);
        } while (value);
    }
    
    MemoryStorage::MemoryStorage():
    seekIndex_{0}
    { }
    
    int MemoryStorage::remaining() const {
        return int(bytes_.size() - seekIndex_);
    }
    
    int MemoryStorage::read(Byte *buf, int nbytes) {
        int bytes_read = min(nbytes, remaining());
        auto cursor = bytes_.begin() + seekIndex_;
        copy(cursor, cursor + bytes_read, buf);
        seekIndex_ += bytes_read;
        return bytes_read;
    }
    
    void MemoryStorage::write(const Byte *buf, int nbytes) {
        copy(buf, buf + nbytes, back_inserter(bytes_));
    }
    
}