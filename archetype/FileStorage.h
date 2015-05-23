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
        InFileStorage(const InFileStorage&) = delete;
        InFileStorage& operator=(const InFileStorage&) = delete;

        bool ok() const;
        virtual ~InFileStorage() { }
        virtual int remaining() const override;
        virtual int read(Byte* buf, int nbytes) override;
        virtual void write(const Byte* buf, int nbytes) override;
    private:
        std::ifstream stream_;
        int remaining_;
    };

    class OutFileStorage : public Storage {
    public:
        OutFileStorage(std::string filename);
        OutFileStorage(const OutFileStorage&) = delete;
        OutFileStorage& operator=(const OutFileStorage&) = delete;

        bool ok() const;
        virtual ~OutFileStorage() { }
        virtual int remaining() const override;
        virtual int read(Byte* buf, int nbytes) override;
        virtual void write(const Byte* buf, int nbytes) override;
    private:
        std::ofstream stream_;
    };
}

#endif /* defined(__archetype__FileStorage__) */
