//
//  Object.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>

#include "Object.h"
#include "Universe.h"

namespace archetype {
    
    ObjectPtr Object::parent() const {
        if (parentId_ < 0) {
            return nullptr;
        } else {
            return Universe::instance().getType(parentId_);
        }
    }
    
    bool Object::hasAttribute(int attribute_id) const {
        ObjectPtr p = parent();
        return attributes_.count(attribute_id) > 0 or (p and p->hasAttribute(attribute_id));
    }
    
    Value Object::getAttributeValue(int attribute_id) const {
        ObjectPtr p = parent();
        auto where = attributes_.find(attribute_id);
        if (where != attributes_.end()) {
            return where->second->evaluate();
        } else if (p and p->hasAttribute(attribute_id)) {
            return p->getAttributeValue(attribute_id);
        } else {
            return Value(new UndefinedValue);
        }
    }
    
    void Object::setAttribute(int attribute_id, Expression expr) {
        attributes_[attribute_id] = std::move(expr);
    }
    
    void Object::setAttribute(int attribute_id, Value val) {
        attributes_[attribute_id] = Expression(new ValueExpression(std::move(val)));
    }
    
    bool Object::hasMethod(int message_id) const {
        ObjectPtr p = parent();
        return methods_.count(message_id) > 0 or (p and p->hasMethod(message_id));
    }
    
    Value Object::executeMethod(int message_id, std::ostream& out) const {
        ObjectPtr p = parent();
        auto where = methods_.find(message_id);
        if (where != methods_.end()) {
            return where->second->execute(out);
        } else if (p->hasMethod(message_id)) {
            return p->executeMethod(message_id, out);
        } else {
            return Value(new ReservedConstantValue(Keywords::RW_ABSENT));
        }
    }
    
    void Object::setMethod(int message_id, Statement stmt) {
        methods_[message_id] = std::move(stmt);
    }
    
    Value Object::send(Value message, std::ostream& out) {
        // TODO: If this is the system object, dispatch to system
        Value defined_message = message->messageConversion();
        if (defined_message->isDefined()) {
            int message_id = defined_message->getMessage();
            return executeMethod(message_id, out);
        } else {
            return Value(new UndefinedValue);
        }
    }
    
}