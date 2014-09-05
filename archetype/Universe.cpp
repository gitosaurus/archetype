//
//  Universe.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <cassert>
#include <limits>

using namespace std;

#include "Universe.h"
#include "Wellspring.h"
#include "SystemObject.h"
#include "ConsoleInput.h"
#include "ConsoleOutput.h"

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
    
    Universe::Context::Context():
    selfObject(nullptr),
    senderObject(nullptr),
    messageValue(new UndefinedValue)
    { }
    
    Universe::Context::Context(const Context& c):
    selfObject{c.selfObject},
    senderObject{c.senderObject},
    messageValue{c.messageValue->clone()},
    eachObject{c.eachObject}
    { }
    
    Universe::Context& Universe::Context::operator=(const Universe::Context& c) {
        selfObject = c.selfObject;
        senderObject = c.senderObject;
        messageValue = c.messageValue->clone();
        eachObject = c.eachObject;
        return *this;
    }
    
    Universe::Context::~Context()
    { }
    
    Universe::Universe() :
    input_{new ConsoleInput},
    output_{new ConsoleOutput}
    {
        ObjectPtr nullObject = defineNewObject();
        assignObjectIdentifier(nullObject, "null");
        nullObject->setPrototype(true);
        // If the "null" object had a parent ID of zero, it would be its own parent.
        // But it's a special object, one that has no parent.
        nullObject->setParentId(Object::INVALID);
        assert(nullObject->id() == NullObjectId);
        
        ObjectPtr system_obj(new SystemObject);
        int system_id = objects_.index(std::move(system_obj));
        system_obj->setId(system_id);
        assert(system_id == SystemObjectId);
        assignObjectIdentifier(system_obj, "system");
        
        Context context;
        context.selfObject = nullObject;
        context.senderObject = nullObject;
        context.messageValue = Value{new UndefinedValue};
        context_.push(context);
    }
    
    ObjectPtr Universe::getObject(int object_id) const {
        if (objects_.hasIndex(object_id)) {
            return objects_.get(object_id);
        } else {
            return nullptr;
        }
    }
    
    ObjectPtr Universe::getObject(std::string identifier) const {
        int name_id = Identifiers.find(identifier);
        if (name_id != StringIdIndex::npos) {
            auto which = ObjectIdentifiers.find(name_id);
            if (which != ObjectIdentifiers.end()) {
                int object_id = which->second;
                return getObject(object_id);
            }
        }
        return nullptr;
    }
    
    ObjectPtr Universe::defineNewObject(int parent_id) {
        ObjectPtr obj(new Object(parent_id));
        int object_id = objects_.index(std::move(obj));
        obj->setId(object_id);
        return objects_.get(object_id);
    }
    
    void Universe::destroyObject(int object_id) {
        ObjectPtr existing = objects_.get(object_id);
        // Debugging sentinel, noting that the object is now invalid.
        existing->setId(Object::INVALID);
        objects_.remove(object_id);
    }
    
    void Universe::assignObjectIdentifier(const ObjectPtr& object, std::string identifier) {
        int identifier_id_for_object = Identifiers.index(identifier);
        assignObjectIdentifier(object, identifier_id_for_object);
    }
    
    void Universe::assignObjectIdentifier(const ObjectPtr& object, int identifier_id) {
        int object_id = object->id();
        ObjectIdentifiers[identifier_id] = object_id;
    }
    
    static ObjectPtr declare_object(TokenStream& t, ObjectPtr obj) {
        while (t.fetch()) {
            if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_END)) {
                return obj;
            }
            if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_METHODS)) {
                break;
            }
            if (t.token().type() != Token::IDENTIFIER) {
                t.expectGeneral("attribute identifier");
                return nullptr;
            }
            int attribute_id = t.token().number();
            t.insistOn(Token(Token::PUNCTUATION, ':'));
            Expression expr = make_expr(t);
            if (not expr) {
                return nullptr;
            }
            obj->setAttribute(attribute_id, std::move(expr));
        }
        if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_METHODS)) {
            while (t.fetch()) {
                if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_END)) {
                    return obj;
                }
                if (t.token() != Token(Token::RESERVED_WORD, Keywords::RW_DEFAULT) &&
                    t.token().type() != Token::MESSAGE) {
                    t.expectGeneral("message literal or 'default'");
                    return nullptr;
                }
                int message_id = (t.token().type() == Token::MESSAGE) ? t.token().number() : numeric_limits<int>::max();
                t.insistOn(Token(Token::PUNCTUATION, ':'));
                Statement stmt = make_statement(t);
                if (not stmt) {
                    return nullptr;
                }
                obj->setMethod(message_id, std::move(stmt));
            }
        }
        return nullptr;
    }
    
    static ObjectPtr instantiate(TokenStream& t, ObjectPtr parent = nullptr) {
        if (not t.fetch() or t.token().type() != Token::IDENTIFIER) {
            t.expectGeneral("name of new object");
            return nullptr;
        }
        ObjectPtr obj = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(obj, t.token().number());
        if (parent) {
            obj->setParentId(parent->id());
        }
        return declare_object(t, obj);
    }
    
    static ObjectPtr define_type(TokenStream& t) {
        if (not (t.fetch() and t.token().type() == Token::IDENTIFIER)) {
            t.expectGeneral("name of new type");
            return nullptr;
        }
        ObjectPtr obj = Universe::instance().defineNewObject();
        obj->setPrototype(true);
        Universe::instance().assignObjectIdentifier(obj, t.token().number());
        t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_BASED));
        t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_ON));
        if (not t.fetch()) {
            t.expectGeneral("name of a previously defined type");
            return nullptr;
        }
        if (t.token().type() == Token::IDENTIFIER) {
            auto parent_id_p = Universe::instance().ObjectIdentifiers.find(t.token().number());
            if (parent_id_p == Universe::instance().ObjectIdentifiers.end()) {
                t.expectGeneral("name of a previously defined type");
                return nullptr;
            }
            ObjectPtr parent = Universe::instance().getObject(parent_id_p->second);
            if (not parent->isPrototype()) {
                t.expectGeneral("name of a previously defined type, not the name of an instance");
                return nullptr;
            }
            obj->setParentId(parent->id());
        } else {
            t.expectGeneral("name of a previously defined type");
            return nullptr;
        }
        return declare_object(t, obj);
    }
    
    bool Universe::make(TokenStream& t) {
        while (t.fetch()) {
            if (t.token().type() == Token::RESERVED_WORD) {
                switch (Keywords::Reserved_e(t.token().number())) {
                    case Keywords::RW_TYPE:
                    case Keywords::RW_CLASS:
                        if (not define_type(t)) {
                            return false;
                        } else {
                            break;
                        }
                    case Keywords::RW_KEYWORD:
                        // TODO:  Return here to complete 'keyword' feature
                        // TODO:  Allows users to catch spelling errors
                        break;
                    case Keywords::RW_INCLUDE: {
                        if (not t.fetch() or  t.token().type() != Token::TEXT_LITERAL) {
                            t.errorMessage("Must follow \"include\" with string file name");
                            return false;
                        }
                        string source_file = TextLiterals.get(t.token().number());
                        if (Wellspring::instance().hasNeverBeenOpened(source_file)) {
                            SourceFilePtr source = Wellspring::instance().open(source_file);
                            if (not source) {
                                t.errorMessage("Cannot open source file \"" + source_file + "\"");
                                return false;
                            }
                            TokenStream included_tokens(source);
                            if (not make(included_tokens)) {
                                return false;
                            }
                            Wellspring::instance().close(source_file);
                        }
                        break;
                    }
                    default:
                        t.expected(Token(Token::RESERVED_WORD, Keywords::RW_TYPE));
                        return false;
                }
            } else if (t.token().type() == Token::IDENTIFIER) {
                int id_number = t.token().number();
                auto which = Universe::instance().ObjectIdentifiers.find(id_number);
                if (which == Universe::instance().ObjectIdentifiers.end()) {
                    t.errorMessage("Require name of defined type");
                    return false;
                } else {
                    ObjectPtr type_object = Universe::instance().getObject(which->second);
                    if (type_object and type_object->isPrototype()) {
                        if (not instantiate(t, type_object)) {
                            return false;
                        }
                    }
                }
            } else {
                t.expectGeneral("Need a type declaration or object instantiation");
                return false;
            }
        }
        return true;
    }
    
    ContextScope::ContextScope() {
        Universe::instance().pushContext(Universe::instance().currentContext());
    }
    
    ContextScope::~ContextScope() {
        Universe::instance().popContext();
    }
    
    Storage& operator<<(Storage& out, const IdentifierMap& m) {
        int entries = static_cast<int>(m.size());
        out << entries;
        for (auto const& p : m) {
            out << p.first << p.second;
        }
        return out;
    }
    
    Storage& operator>>(Storage&in, IdentifierMap& m) {
        m.clear();
        int entries;
        in >> entries;
        for (int i = 0; i < entries; ++i) {
            int first, second;
            in >> first >> second;
            m[first] = second;
        }
        return in;
    }
    
    Storage& operator<<(Storage& out, const Universe& u) {
        out << u.Messages << u.TextLiterals << u.Identifiers << u.ObjectIdentifiers;
        out << u.objects_;
        return out;
    }
    
    Storage& operator>>(Storage& in, Universe& u) {
        in >> u.Messages >> u.TextLiterals >> u.Identifiers >> u.ObjectIdentifiers;
        in >> u.objects_;
        return in;
    }
    
}