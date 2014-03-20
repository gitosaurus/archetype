//
//  Object.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Object__
#define __archetype__Object__

#include <map>
#include <memory>
#include <iostream>

#include "Expression.h"
#include "Statement.h"
#include "Value.h"

namespace archetype {

    class Object;
    typedef std::shared_ptr<Object> ObjectPtr;

    class Object {
        int parentId_;
        int id_;
        bool prototype_;
        std::map<int, Expression> attributes_;
        std::map<int, Statement> methods_;
    public:
        Object(int parent_id = -1):
        parentId_{parent_id},
        id_{0},
        prototype_{false}
        { }

        Object(const Object& obj) = delete;
        Object& operator=(const Object& obj) = delete;
        
        int id() const { return id_; }
        void setId(int id) { id_ = id; }
        
        int parentId() const { return parentId_; }
        void setParentId(int parent_id) { parentId_ = parent_id; }
        
        bool isPrototype() const { return prototype_; }
        void setPrototype(bool prototype) { prototype_ = prototype; }
        
        ObjectPtr parent() const;
        
        bool hasAttribute(int attribute_id) const;
        Value getAttributeValue(int attribute_id) const;
        void setAttribute(int attribute_id, Expression expr);
        void setAttribute(int attribute_id, Value val);
        
        bool hasMethod(int message_id) const;
        Value executeMethod(int message_id, std::ostream& out) const;
        void setMethod(int message_id, Statement stmt);
        
        Value send(Value message, std::ostream& out);
    };
    
}

#endif /* defined(__archetype__Object__) */
