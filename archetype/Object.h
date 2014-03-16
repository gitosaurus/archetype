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

#include "Expression.h"
#include "Statement.h"

namespace archetype {
    class Object {
        int parentId_;
        int id_;
        std::map<int, Expression> attributes_;
        std::map<int, Statement> methods_;
    public:
        Object(int parent_id = 0): parentId_(parent_id), id_(0) { }
        
        int id() const { return id_; }
        void setId(int id) { id_ = id; }
        
        bool hasAttribute(int attribute_id) const;
        const Expression& attribute(int attribute_id) const;
        void setAttribute(int attribute_id, Expression expr);
        
        bool hasMethod(int message_id) const;
        const Statement& method(int message_id) const;
    };
    
    typedef std::shared_ptr<Object> ObjectPtr;
}

#endif /* defined(__archetype__Object__) */
