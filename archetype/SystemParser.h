//
//  SystemParser.h
//  archetype
//
//  Created by Derek Jones on 4/26/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__SystemParser__
#define __archetype__SystemParser__

#include <iostream>
#include <string>
#include <set>

#include "Object.h"

namespace archetype {
    class SystemParser {
    public:
        enum Mode_e { VERBS, NOUNS };
        void setMode(Mode_e mode) { mode_ = mode; }
        
        // Names is a bar-separated string of synonyms, e.g. "get|take"
        void addParseable(Object sender, std::string names);
        
    private:
        Mode_e mode_;
        std::set<ObjectPtr> present_;
    };
}

#endif /* defined(__archetype__SystemParser__) */
