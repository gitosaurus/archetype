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
#include <map>
#include <memory>

#include "IdIndex.h"
#include "StringIdIndex.h"
#include "Object.h"

namespace archetype {
    
    typedef IdIndex<ObjectPtr> ObjectIndex;
    typedef std::map<int, int> IdentifierMap;

    // TODO: Better name?  I like "Game" or "Universe", maybe
    class GameDefinition {
    public:
        StringIdIndex Vocabulary;
        StringIdIndex TextLiterals;
        StringIdIndex Identifiers;

        IdentifierMap TypeIdentifiers;
        IdentifierMap ObjectIdentifiers;
        
        ObjectIndex   Types;
        ObjectIndex   Objects;
        
        ObjectPtr defineNewObject(int parent_id = 0);
        void assignObjectIdentifier(const ObjectPtr& object, std::string name);

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
