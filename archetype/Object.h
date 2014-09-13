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
#include "Serialization.h"

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
        static const int INVALID = -1;

        Object(int parent_id = INVALID):
        parentId_{parent_id},
        id_{0},
        prototype_{false}
        { }

        Object(const Object& obj) = delete;
        Object& operator=(const Object& obj) = delete;

        virtual ~Object() { }

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

        void setMethod(int message_id, Statement stmt);

        static Value send(ObjectPtr target, Value message);
        static Value pass(ObjectPtr target, Value message);

        virtual Value dispatch();

        virtual Value executeMethod(int message_id);
        virtual Value executeDefaultMethod();

        virtual void write(Storage& out);
        virtual void read(Storage& in);
    };

    Storage& operator<<(Storage& out, const ObjectPtr& p);
    Storage& operator>>(Storage&in, ObjectPtr& p);

}

#endif /* defined(__archetype__Object__) */
