//
//  Universe.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <cassert>

using namespace std;

#include "Universe.h"

namespace archetype {
    Universe* Universe::instance_ = nullptr;
    Universe& Universe::instance() {
        if (not instance_) {
            instance_ = new Universe();
        }
        return *instance_;
    }
    
    void Universe::destroy() {
        delete instance_;
        instance_ = nullptr;
    }
    
    Universe::Universe() {
        // TODO: better sentinel?  This feels fishy.
        // TODO: What if the original context is self:undefined, sender:undefined, message:undefined?
        // TODO: for most programs, self:main, sender:system, message:'START' (good, but maybe not the bottom)
        Context context;
        context.selfObject = ObjectPtr(new Object);
        context.senderObject = ObjectPtr(new Object);
        context.messageId = 0;
        context_.push(context);
        
        output_ = &std::cout;
    }
    
    std::ostream& Universe::output() {
        return *output_;
    }
    
    void Universe::setOutput(std::ostream &out) {
        output_ = &out;
    }
    
    ObjectPtr Universe::getObject(int object_id) const {
        // TODO:  I hate this lookup-and-find pattern.  How about a nice find() for IdIndex?
        if (objects_.hasIndex(object_id)) {
            return objects_.get(object_id);
        } else {
            return nullptr;
        }
    }
    
    ObjectPtr Universe::defineNewObject(int parent_id) {
        ObjectPtr obj(new Object(parent_id));
        int object_id = objects_.index(std::move(obj));
        obj->setId(object_id);
        return objects_.get(object_id);
    }
    
    void Universe::assignObjectIdentifier(const ObjectPtr& object, std::string name) {
        int object_id = object->id();
        int identifier_id_for_object = Identifiers.index(name);
        ObjectIdentifiers[identifier_id_for_object] = object_id;
    }
    
    SelfScope::SelfScope(ObjectPtr new_self_obj) {
        Universe::instance().pushContext(Universe::instance().currentContext());
        Universe::instance().currentContext().selfObject = new_self_obj;
    }
    
    SelfScope::~SelfScope() {
        Universe::instance().popContext();
    }
}