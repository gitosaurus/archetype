//
//  GameDefinition.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "GameDefinition.h"

namespace archetype {
    GameDefinition* GameDefinition::instance_ = nullptr;
    GameDefinition& GameDefinition::instance() {
        if (not instance_) {
            instance_ = new GameDefinition();
        }
        return *instance_;
    }
    
    void GameDefinition::destroy() {
        delete instance_;
        instance_ = nullptr;
    }
    
    GameDefinition::GameDefinition() {
    }
    
    ObjectPtr GameDefinition::defineNewObject(int parent_id) {
        ObjectPtr obj(new Object(parent_id));
        int object_id = Objects.index(std::move(obj));
        obj->setId(object_id);
        return Objects.get(object_id);
    }
    
    void GameDefinition::assignObjectIdentifier(const ObjectPtr& object, std::string name) {
        int object_id = object->id();
        int identifier_id_for_object = Identifiers.index(name);
        ObjectIdentifiers[identifier_id_for_object] = object_id;
    }
}