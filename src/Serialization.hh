//
//  Serialization.h
//  archetype
//
//  Created by Derek Jones on 6/15/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Serialization__
#define __archetype__Serialization__

#include <array>
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

        // Look at what read would return, without consuming it.  Deciding
        // whether a stream carries a format header means examining its first
        // bytes before knowing whether they belong to the header or to the
        // universe, and only a look that can be taken back can do that.
        virtual int peek(std::span<Byte> buf) = 0;

        int readInteger();
        void writeInteger(int value);
    };

    Storage& operator<<(Storage& out, int value);
    Storage& operator>>(Storage& in, int& value);

    // Writing reads the characters and nothing else; reading has to grow a
    // string, so it takes one to fill.
    Storage& operator<<(Storage& out, std::string_view value);
    Storage& operator>>(Storage& in, std::string& value);

    // The four bytes an Archetype binary begins with.  0x7F is neither
    // printable nor a legal start to a UTF-8 sequence, so a text file mistaken
    // for a game is rejected on its first byte.
    inline constexpr std::array<Storage::Byte, 4> FormatCookie{0x7F, 'A', 'C', 'X'};

    // The version of the *layout* that follows the cookie, which is a separate
    // thing from the interpreter's own version and deliberately so: a release
    // that moves no bytes around must leave every .acx exactly as it was, or
    // the goldens churn and byte comparison stops being an oracle.  Bump this
    // only when what operator<< writes actually changes shape.
    inline constexpr int CurrentFormatVersion = 1;

    // Version 0 is the unversioned layout that predates the header, still
    // written by no one and still readable by everyone.
    inline constexpr int UnversionedFormat = 0;

    // Read a number that counts something the stream still has to supply.
    //
    // Such a count is a promise about bytes that follow, and no stream can make
    // good on a promise bigger than what is left in it: every element, however
    // small, costs at least one byte to encode.  Checking that before believing
    // the number is the whole point -- believing it first is how a file of
    // thirteen bytes used to ask for a vector of fifty million.
    //
    // "what" names the thing being counted, for the message thrown if the count
    // is negative or larger than the stream can back.
    int readCount(Storage& in, std::string_view what);

    void writeFormatHeader(Storage& out);

    // The format version the stream declares, with the header consumed if there
    // was one.  A stream that does not begin with the cookie is left untouched
    // and reported as UnversionedFormat; deciding whether that is good enough
    // is the caller's business, since only the caller knows what the
    // unversioned layout was supposed to start with.
    //
    // Throws if the cookie is there but the version is one this interpreter
    // does not know, which is the case worth a precise complaint: the file is
    // definitely ours and definitely from the future.
    int readFormatHeader(Storage& in);

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
        virtual int peek(std::span<Byte> buf) override;
    };
}

#endif /* defined(__archetype__Serialization__) */
