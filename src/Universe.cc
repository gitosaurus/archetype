//
//  Universe.cc
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <limits>

using namespace std;

#include "Universe.hh"
#include "Wellspring.hh"
#include "SystemObject.hh"
#include "ConsoleInput.hh"
#include "ConsoleOutput.hh"
#include "PagedOutput.hh"

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

    void Universe::createReservedObjects_() {
        nullObject_ = defineNewObject();
        assignObjectIdentifier(nullObject_, "null");
        nullObject_->setPrototype(true);
        // If the "null" object had a parent ID of zero, it would be its own parent.
        // But it's a special object, one that has no parent.
        nullObject_->setParentId(Object::INVALID);
        assert(nullObject_->id() == NullObjectId);
        kinds_[nullObject_->id()] = OBJECT_ID;

        systemObject_ = ObjectPtr{new SystemObject};
        int system_id = objects_.index(systemObject_);
        systemObject_->setId(system_id);
        assert(system_id == SystemObjectId);
        assignObjectIdentifier(systemObject_, "system");
        kinds_[systemObject_->id()] = OBJECT_ID;
    }

    Universe::Universe() :
    ended_(false),
    input_{new ConsoleInput},
    output_{new PagedOutput{UserOutput{new ConsoleOutput}}}
    {
        createReservedObjects_();
        Context context;
        context.selfObject = nullObject_;
        context.senderObject = nullObject_;
        context.messageValue = Value{new UndefinedValue};
        context_.push(context);
    }

    Universe::~Universe() {
    }

    ostream& operator<< (ostream& out, IdentifierKind_e kind) {
        switch (kind) {
            case OBJECT_ID:
                out << "an object";
                break;
            case DEFINED_ATTRIBUTE_ID:
                out << "a defined attribute";
                break;
            case REFERENCED_ATTRIBUTE_ID:
                out << "an attribute in an expression";
                break;
            case KEYWORD_ID:
                out << "a keyword";
                break;
            case UNKNOWN_ID:
                out << "an unknown identifier";
                break;
        }
        return out;
    }

    void Universe::classify(TokenStream& t, int identifier, IdentifierKind_e kind) {
        auto existing_p = kinds_.find(identifier);
        if (existing_p == kinds_.end()) {
            kinds_[identifier] = kind;
        } else if (kind != UNKNOWN_ID and existing_p->second == UNKNOWN_ID) {
            kinds_[identifier] = kind;
        } else if (existing_p->second == REFERENCED_ATTRIBUTE_ID and kind == DEFINED_ATTRIBUTE_ID) {
            kinds_[identifier] = kind;
        } else if (existing_p->second == DEFINED_ATTRIBUTE_ID and kind == REFERENCED_ATTRIBUTE_ID) {
            // no-op
        } else if (kind != UNKNOWN_ID and existing_p->second != kind) {
            ostringstream out;
            out << "Identifier '" << Identifiers.get(identifier);
            out << "' is already the name of " << existing_p->second << " but is used here as " << kind;
            t.errorMessage(out.str());
        }
    }

    void Universe::reportUndefinedIdentifiers() const {
        for (auto const& p : kinds_) {
            if (p.second == UNKNOWN_ID or p.second == REFERENCED_ATTRIBUTE_ID) {
                ostringstream out;
                out << "Identifier \"";
                out << Identifiers.get(p.first);
                out << "\" was never defined";
                output()->put(out.str());
                output()->endLine();
            }
        }
    }

    int Universe::objectCount() const {
        return objects_.count();
    }

    ObjectPtr Universe::getObject(int object_id) const {
        // Because deserialization tramples on null and (especially) system,
        // dispatch to the originals every time.
        switch (object_id) {
            case NullObjectId:
                return nullObject_;
            case SystemObjectId:
                return systemObject_;
            default:
                if (objects_.hasIndex(object_id)) {
                    return objects_.get(object_id);
                } else {
                    return nullptr;
                }
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
        ObjectPtr obj{make_shared<Object>(parent_id)};
        int object_id = objects_.index(obj);
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
            Universe::instance().classify(t, attribute_id, DEFINED_ATTRIBUTE_ID);
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
                if (t.token() != Token(Token::RESERVED_WORD, Keywords::RW_DEFAULT) and
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

    bool Universe::identifierIsAssignedAs(int identifier_id, int object_id) const {
        auto p = ObjectIdentifiers.find(identifier_id);
        return p != ObjectIdentifiers.end() and p->second == object_id;
    }

    static ObjectPtr instantiate(TokenStream& t, ObjectPtr parent = nullptr) {
        if (not t.fetch() or t.token().type() != Token::IDENTIFIER) {
            t.expectGeneral("name of new object");
            return nullptr;
        }
        Universe::instance().classify(t, t.token().number(), OBJECT_ID);
        ObjectPtr obj = Universe::instance().defineNewObject();
        // "null" is allowed as an instance identifier which allows the instance to be
        // anonymous.  Don't bind the identifier in this case.
        if (not Universe::instance().identifierIsAssignedAs(t.token().number(), Universe::NullObjectId)) {
            Universe::instance().assignObjectIdentifier(obj, t.token().number());
        }
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
        Universe::instance().classify(t, t.token().number(), OBJECT_ID);
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
                        if (not t.fetch() or t.token().type() != Token::IDENTIFIER) {
                            t.errorMessage("Must follow \"keyword\" with one or more identifiers");
                            return false;
                        } else {
                            classify(t, t.token().number(), KEYWORD_ID);
                            while (t.token().type() == Token::IDENTIFIER and
                                   t.fetch() and t.token() == Token(Token::PUNCTUATION, ',')) {
                                if (not t.fetch()) {
                                    t.expectGeneral("identifier of keyword");
                                    return false;
                                }
                                classify(t, t.token().number(), KEYWORD_ID);
                            }
                            t.didNotConsume();
                        }
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
                            Wellspring::instance().close(source);
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
        out << static_cast<int>(u.ended_);
        out << u.Messages << u.TextLiterals << u.Identifiers << u.ObjectIdentifiers;
        out << u.objects_;
        return out;
    }

    Storage& operator>>(Storage& in, Universe& u) {
        int ended;
        in >> ended;
        u.ended_ = static_cast<bool>(ended);
        u.Messages.clear();
        u.TextLiterals.clear();
        u.Identifiers.clear();
        u.ObjectIdentifiers.clear();
        in >> u.Messages >> u.TextLiterals >> u.Identifiers >> u.ObjectIdentifiers;
        u.objects_.clear();
        u.createReservedObjects_();
        in >> u.objects_;
        return in;
    }

}
