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
#include "TokenStream.h"

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

        IdentifierMap ObjectIdentifiers;
        
        Context& currentContext() { return context_.top(); }
        void pushContext(const Context& context) { context_.push(context); }
        void popContext() { context_.pop(); }
        
        std::ostream& output();
        void setOutput(std::ostream& out);
        
        ObjectPtr getObject(int object_id) const;
        ObjectPtr defineNewObject(int parent_id = 0);
        void assignObjectIdentifier(const ObjectPtr& object, std::string name);
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
        // No copying
        Universe(const Universe&);
        Universe& operator=(const Universe&);
    };
    
    class SelfScope {
    public:
        SelfScope(ObjectPtr new_self_obj);
        SelfScope(const SelfScope&) = delete;
        SelfScope& operator=(const SelfScope&) = delete;
        ~SelfScope();
    };
    
    class MessageScope {
    public:
        MessageScope(int message_id);
        MessageScope(const MessageScope&) = delete;
        MessageScope& operator=(const MessageScope&) = delete;
        ~MessageScope();
    };
}

#endif /* defined(__archetype__Universe__) */
