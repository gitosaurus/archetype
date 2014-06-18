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
    
    class MemoryStorage : public Storage {
        size_t seekIndex_;
        std::vector<Byte> bytes_;
    public:
        MemoryStorage();
        virtual ~MemoryStorage() { }
        virtual int remaining() const;
        virtual int read(Byte* buf, int nbytes);
        virtual void write(const Byte* buf, int nbytes);
    };
}

#endif /* defined(__archetype__Serialization__) */
