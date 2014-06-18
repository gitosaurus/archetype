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
#include "Value.h"
#include "TokenStream.h"

namespace archetype {
    
    typedef IdIndex<ObjectPtr> ObjectIndex;
    typedef std::map<int, int> IdentifierMap;

    class Universe {
    public:
        static const int NullObjectId = 0;
        static const int SystemObjectId = 1;
        
        struct Context {
            ObjectPtr selfObject;
            ObjectPtr senderObject;
            Value messageValue;
            
            Context();
            Context(const Context& c);
            Context& operator=(const Context& c);
            ~Context();
        };

        StringIdIndex Vocabulary;
        StringIdIndex TextLiterals;
        StringIdIndex Identifiers;

        IdentifierMap ObjectIdentifiers;
        
        Context& currentContext() { return context_.top(); }
        void pushContext(const Context& context) { context_.push(context); }
        void popContext() { context_.pop(); }
        
        std::ostream& output();
        void setOutput(std::ostream& out);
        
        ObjectPtr getObject(int object_id) const;
        ObjectPtr getObject(std::string identifier) const;
        ObjectPtr defineNewObject(int parent_id = 0);
        void assignObjectIdentifier(const ObjectPtr& object, std::string identifier);
        void assignObjectIdentifier(const ObjectPtr& object, int identifier_id);
        
        bool make(TokenStream& t);
        
        static Universe& instance();
        static void destroy();
        
    private:
        ObjectIndex   objects_;
        std::stack<Context> context_;
        std::ostream* output_;
        
        static Universe* instance_;

        Universe();
        Universe(const Universe&) = delete;
        Universe& operator=(const Universe&) = delete;
    };
    
    class ContextScope {
    public:
        ContextScope();
        ContextScope(const ContextScope&) = delete;
        ContextScope& operator=(const ContextScope&) = delete;
        ~ContextScope();
        
        Universe::Context* operator->()
        { return &(Universe::instance().currentContext()); }
    };
    
}

#endif /* defined(__archetype__Universe__) */
