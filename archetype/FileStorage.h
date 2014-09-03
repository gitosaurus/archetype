//
//  FileStorage.h
//  archetype
//
//  Created by Derek Jones on 9/2/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__FileStorage__
#define __archetype__FileStorage__

#include <iostream>
#include <fstream>
#include <string>

#include "Serialization.h"

namespace archetype {
    class InFileStorage : public Storage {
    public:
        InFileStorage(std::string filename);
        bool ok() const { return stream_ ? true : false; }
        virtual ~InFileStorage() { }
        virtual int remaining() const override;
        virtual int read(Byte* buf, int nbytes) override;
        virtual void write(const Byte* buf, int nbytes) override;
    private:
        std::ifstream stream_;
        int remaining_;
        
        InFileStorage(const InFileStorage&) = delete;
        InFileStorage& operator=(const InFileStorage&) = delete;
    };
    
    class OutFileStorage : public Storage {
    public:
        OutFileStorage(std::string filename);
        bool ok() const { return stream_ ? true : false; }
        virtual ~OutFileStorage() { }
        virtual int remaining() const override;
        virtual int read(Byte* buf, int nbytes) override;
        virtual void write(const Byte* buf, int nbytes) override;
    private:
        std::ofstream stream_;
        
        OutFileStorage(const OutFileStorage&) = delete;
        OutFileStorage& operator=(const OutFileStorage&) = delete;
    };
}

#endif /* defined(__archetype__FileStorage__) */
