//
//  Object.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include "Object.h"

namespace archetype {
    
    bool Object::hasAttribute(int attribute_id) const {
        // TODO: must search up parent tree
        return attributes_.count(attribute_id) > 0;
    }
    
    const Expression& Object::attribute(int attribute_id) const {
        // TODO: must search up parent tree
        return attributes_.find(attribute_id)->second;
    }
    
    void Object::setAttribute(int attribute_id, Expression expr) {
        attributes_.insert(make_pair(attribute_id, std::move(expr)));
    }
    
    bool Object::hasMethod(int message_id) const {
        // TODO: must search up parent tree
        return methods_.count(message_id) > 0;
    }
    
    const Statement& Object::method(int message_id) const {
        // TODO: must search up parent tree
        return methods_.find(message_id)->second;
    }
    
}