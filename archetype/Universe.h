//
//  Universe.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Universe__
#define __archetype__Universe__

#include <string>
#include <map>
#include <memory>
#include <stack>

#include "IdIndex.h"
#include "StringIdIndex.h"
#include "Object.h"

namespace archetype {
    
    typedef IdIndex<ObjectPtr> ObjectIndex;
    typedef std::map<int, int> IdentifierMap;

    class Universe {
    public:
        
        struct Context {
            ObjectPtr selfObject;
            ObjectPtr senderObject;
            int messageId;
        };

        StringIdIndex Vocabulary;
        StringIdIndex TextLiterals;
        StringIdIndex Identifiers;

        IdentifierMap TypeIdentifiers;
        IdentifierMap ObjectIdentifiers;
        
        ObjectIndex   Types;
        ObjectIndex   Objects;
        
        Context& currentContext() { return context_.top(); }
        void pushContext(const Context& context) { context_.push(context); }
        void popContext() { context_.pop(); }
        
        ObjectPtr defineNewObject(int parent_id = 0);
        void assignObjectIdentifier(const ObjectPtr& object, std::string name);
        
        ObjectPtr getType(int type_object_id) const;
        ObjectPtr defineNewType(int parent_id = 0);
        void assignTypeIdentifier(const ObjectPtr& type_object, std::string name);

        static Universe& instance();
        static void destroy();
        
    private:
        std::stack<Context> context_;
        
        static Universe* instance_;
        Universe();
        // No copying
        Universe(const Universe&);
        Universe& operator=(const Universe&);
    };
    
    class SelfScope {
    public:
        SelfScope(ObjectPtr new_self_obj);
        SelfScope(const SelfScope& other) = delete;
        SelfScope& operator=(const SelfScope&) = delete;
        ~SelfScope();
    };
}

#endif /* defined(__archetype__Universe__) */
