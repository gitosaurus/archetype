//
//  Object.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <limits>
#include <cassert>

#include "Object.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    
    const int DefaultMethod = numeric_limits<int>::max();
    
    ObjectPtr Object::parent() const {
        if (parentId_ < 0) {
            return nullptr;
        } else {
            ObjectPtr obj = Universe::instance().getObject(parentId_);
            return (obj and obj->isPrototype()) ? obj : nullptr;
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
    
    Value Object::executeMethod(int message_id) const {
        ObjectPtr p = parent();
        auto where = methods_.find(message_id);
        if (where != methods_.end()) {
            return where->second->execute(Universe::instance().output());
        } else if (p and p->hasMethod(message_id)) {
            return p->executeMethod(message_id);
        } else if (hasDefaultMethod()) {
            return executeDefaultMethod();
        } else if (p and p->hasDefaultMethod()) {
            return p->executeDefaultMethod();
        } else {
            return Value(new ReservedConstantValue(Keywords::RW_ABSENT));
        }
    }
    
    bool Object::hasDefaultMethod() const {
        return methods_.size() > 0  and  methods_.rbegin()->first == DefaultMethod;
    }
    
    Value Object::executeDefaultMethod() const {
        assert(not methods_.empty());
        auto defaultMethod = methods_.rbegin();
        assert(defaultMethod->first == DefaultMethod);
        return defaultMethod->second->execute(Universe::instance().output());
    }
    
    void Object::setMethod(int message_id, Statement stmt) {
        methods_[message_id] = std::move(stmt);
    }
    
    Value Object::dispatch(Value message) {
        Value defined_message = message->messageConversion();
        if (defined_message->isDefined()) {
            int message_id = defined_message->getMessage();
            ContextScope c;
            c->messageValue = std::move(defined_message);
            return executeMethod(message_id);
        }

        // If the message isn't defined, then the only place to which it can be delivered
        // is the default method, if one exists.
        if (hasDefaultMethod()) {
            ContextScope c;
            c->messageValue = std::move(message);
            return executeDefaultMethod();
        }
        ObjectPtr p = parent();
        if (p and p->hasDefaultMethod()) {
            ContextScope c;
            c->messageValue = std::move(message);
            return p->executeDefaultMethod();
        }
        
        return Value(new UndefinedValue);
    }
    
}