//
//  Capture.h
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Capture__
#define __archetype__Capture__

#include <iostream>

#include "UserOutput.hh"

namespace archetype {
    class Capture {
        UserOutput previous_;
        UserOutput output_;
    public:
        Capture();
        ~Capture();

        Capture(const Capture&) = delete;
        Capture& operator=(const Capture&) = delete;

        std::string getCapture() const;
    };
}

#endif /* defined(__archetype__Capture__) */
