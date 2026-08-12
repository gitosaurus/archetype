//
//  Serialization.cc
//  archetype
//
//  Created by Derek Jones on 6/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

// For Windows
#define _SCL_SECURE_NO_WARNINGS

#include <array>
#include <span>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <format>
#include <sstream>

#include "Serialization.hh"

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
        read(span{&byte, 1});
        bool more = static_cast<bool>(byte & MoreBit);
        byte &= ~MoreBit;
        // The sign bit is the very first bit deserialized.
        // Note it for this number and shift it off.
        bool negative = (byte & SignBit);
        byte >>= 1;
        int bits = 6;
        int result = byte;
        while (more) {
            if (not read(span{&byte, 1})) {
                throw invalid_argument("End of storage in the middle of a continued integer");
            }
            int next_part = (byte & PayloadBits);
            next_part <<= bits;
            result |= next_part;
            more = static_cast<bool>(byte & MoreBit);
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
            write(span{&byte, 1});
            bits = 7;
            byte = (value & PayloadBits);
        } while (value);
    }

    Storage& operator<<(Storage& out, int value) {
        out.writeInteger(value);
        return out;
    }

    Storage& operator>>(Storage& in, int& value) {
        value = in.readInteger();
        return in;
    }

    Storage& operator<<(Storage& out, std::string_view value) {
        int size = static_cast<int>(value.size());
        out << size;
        out.write({reinterpret_cast<const Storage::Byte*>(value.data()), value.size()});
        return out;
    }

    Storage& operator>>(Storage& in, std::string& value) {
        int size = readCount(in, "string length");
        value.resize(size);
        int bytes_read = in.read({reinterpret_cast<Storage::Byte*>(value.data()), value.size()});
        if (bytes_read != size) {
            throw invalid_argument(
                format("Could not fully read string declared as {} bytes; only read {}",
                       size, bytes_read));
        }
        return in;
    }

    int readCount(Storage& in, string_view what) {
        int count = in.readInteger();
        if (count < 0) {
            throw invalid_argument(format("A {} cannot be negative: {}", what, count));
        }
        if (count > in.remaining()) {
            throw invalid_argument(
                format("A {} of {} is more than the {} bytes remaining can supply",
                       what, count, in.remaining()));
        }
        return count;
    }

    void writeFormatHeader(Storage& out) {
        out.write(FormatCookie);
        out << CurrentFormatVersion;
    }

    int readFormatHeader(Storage& in) {
        array<Storage::Byte, FormatCookie.size()> cookie{};
        if (in.peek(cookie) != static_cast<int>(cookie.size()) or
            not ranges::equal(cookie, FormatCookie)) {
            return UnversionedFormat;
        }
        in.read(cookie);
        int version = in.readInteger();
        if (version <= UnversionedFormat or version > CurrentFormatVersion) {
            throw invalid_argument(
                format("Archetype format version {} is not one this interpreter "
                       "understands; it reads up to version {}",
                       version, CurrentFormatVersion));
        }
        return version;
    }

    MemoryStorage::MemoryStorage():
    seekIndex_{0}
    { }

    int MemoryStorage::remaining() const {
        return int(bytes_.size() - seekIndex_);
    }

    int MemoryStorage::read(span<Byte> buf) {
        int bytes_read = min(static_cast<int>(buf.size()), remaining());
        auto cursor = bytes_.begin() + seekIndex_;
        copy(cursor, cursor + bytes_read, buf.begin());
        seekIndex_ += bytes_read;
        return bytes_read;
    }

    void MemoryStorage::write(span<const Byte> buf) {
        ranges::copy(buf, back_inserter(bytes_));
    }

    int MemoryStorage::peek(span<Byte> buf) {
        int bytes_read = read(buf);
        seekIndex_ -= bytes_read;
        return bytes_read;
    }

}
