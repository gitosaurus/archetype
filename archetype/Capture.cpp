//
//  Capture.cpp
//  archetype
//
//  Created by Derek Jones on 7/9/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "Capture.h"
#include "StringOutput.h"
#include "Universe.h"

namespace archetype {
    Capture::Capture():
    output_{new StringOutput}
    {
        previous_ = Universe::instance().output();
        Universe::instance().setOutput(output_);
    }
    
    Capture::~Capture()
    {
        Universe::instance().setOutput(previous_);
    }
    
    std::string Capture::getCapture() const {
        return dynamic_cast<const StringOutput*>(output_.get())->getOutput();
    }
}