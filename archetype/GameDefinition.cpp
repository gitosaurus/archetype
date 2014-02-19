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
}