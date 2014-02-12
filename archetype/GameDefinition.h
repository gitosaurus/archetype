//
//  GameDefinition.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__GameDefinition__
#define __archetype__GameDefinition__

#include <string>

#include "IdIndex.h"
#include "Object.h"

namespace archetype {
    
    typedef IdIndex<std::string> StringIdIndex;
    typedef IdIndex<Object> ObjectIndex;

    class GameDefinition {
        StringIdIndex messages_;
        StringIdIndex textLiterals;
        StringIdIndex identifiers_;
        ObjectIndex types_;
        ObjectIndex objects_;
    };
}

#endif /* defined(__archetype__GameDefinition__) */
