//
//  Universe.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

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
    }
    
    ObjectPtr Universe::defineNewObject(int parent_id) {
        ObjectPtr obj(new Object(parent_id));
        int object_id = Objects.index(std::move(obj));
        obj->setId(object_id);
        return Objects.get(object_id);
    }
    
    void Universe::assignObjectIdentifier(const ObjectPtr& object, std::string name) {
        int object_id = object->id();
        int identifier_id_for_object = Identifiers.index(name);
        ObjectIdentifiers[identifier_id_for_object] = object_id;
    }
    
    ObjectPtr Universe::defineNewType(int parent_id) {
        ObjectPtr type_obj(new Object(parent_id));
        int type_object_id = Types.index(std::move(type_obj));
        type_obj->setId(type_object_id);
        return Types.get(type_object_id);
    }
    
    void Universe::assignTypeIdentifier(const ObjectPtr &type_object, std::string name) {
        int type_object_id = type_object->id();
        int identifier_id_for_type = Identifiers.index(name);
        TypeIdentifiers[identifier_id_for_type] = type_object_id;
    }
    
    ObjectPtr Universe::getType(int type_object_id) const {
        if (Types.hasIndex(type_object_id)) {
            return Types.get(type_object_id);
        } else {
            return nullptr;
        }
    }
    
    SelfScope::SelfScope(ObjectPtr new_self_obj) {
        Universe::instance().pushContext(Universe::instance().currentContext());
        Universe::instance().currentContext().selfObject = new_self_obj;
    }
    
    SelfScope::~SelfScope() {
        Universe::instance().popContext();
    }
}