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

#include "UserOutput.h"

namespace archetype {
    class Capture {
        UserOutput previous_;
        UserOutput output_;
        Capture(const Capture&) = delete;
        Capture& operator=(const Capture&) = delete;
    public:
        Capture();
        ~Capture();
        std::string getCapture() const;
    };
}

#endif /* defined(__archetype__Capture__) */
