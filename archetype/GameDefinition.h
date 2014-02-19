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
#include "StringIdIndex.h"
#include "Object.h"

namespace archetype {
    
    typedef IdIndex<Object> ObjectIndex;

    class GameDefinition {
    public:
        StringIdIndex Vocabulary;
        StringIdIndex TextLiterals;
        StringIdIndex Identifiers;
        ObjectIndex   Types;
        ObjectIndex   Objects;

        static GameDefinition& instance();
        static void destroy();
    private:
        static GameDefinition* instance_;
        GameDefinition();
        // No copying
        GameDefinition(const GameDefinition&);
        GameDefinition& operator=(const GameDefinition&);
    };
}

#endif /* defined(__archetype__GameDefinition__) */
