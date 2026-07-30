//
//  Object.cc
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <cassert>
#include <format>
#include <sstream>

#include "Object.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {

    bool Object::Debug = false;

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
        return attributes_.contains(attribute_id) or (p and p->hasAttribute(attribute_id));
    }

    bool Object::hasLocalAttribute(int attribute_id) const {
        return attributes_.contains(attribute_id);
    }

    Value Object::getAttributeValue(int attribute_id) const {
        if (auto where = attributes_.find(attribute_id); where != attributes_.end()) {
            return where->second->evaluate();
        }
        ObjectPtr p = parent();
        if (p) {
            return p->getAttributeValue(attribute_id);
        } else {
            return make_unique<UndefinedValue>();
        }
    }

    void Object::setAttribute(int attribute_id, Expression expr) {
        attributes_[attribute_id] = std::move(expr);
    }

    void Object::setAttribute(int attribute_id, Value val) {
        attributes_[attribute_id] = make_unique<ValueExpression>(std::move(val));
    }

    Value Object::send(ObjectPtr target, Value message) {
        ContextScope c;
        c->senderObject = c->selfObject;
        c->selfObject = target;
        c->messageValue = std::move(message);
        return target->dispatch();
    }

    Value Object::pass(ObjectPtr target, Value message) {
        ContextScope c;
        c->messageValue = std::move(message);
        return target->dispatch();
    }

    Value Object::dispatch() {
        Value defined_message = Universe::instance().currentContext().messageValue->messageConversion();
        Value absence = make_unique<AbsentValue>();
        Value result = make_unique<AbsentValue>();
        if (defined_message->isDefined()) {
            if (Debug) {
                Value target = make_unique<ObjectValue>(id());
                Universe::instance().output()->put(format("dispatching {} to {}",
                                                          defined_message, target));
                Universe::instance().output()->endLine();
            }
            int message_id = defined_message->getMessage();
            result = executeMethod(message_id);
        }
        if (result->isSameValueAs(absence)) {
            if (Debug) {
                Value target = make_unique<ObjectValue>(id());
                Universe::instance().output()->put(format("dispatching default method to {}", target));
                Universe::instance().output()->endLine();
            }
            result = executeDefaultMethod();
        }
        return result;
    }

    Value Object::executeMethod(int message_id) {
        if (auto where = methods_.find(message_id); where != methods_.end()) {
            return where->second->execute();
        }
        ObjectPtr p = parent();
        if (p) {
            return p->executeMethod(message_id);
        } else {
            return make_unique<AbsentValue>();
        }
    }

    Value Object::executeDefaultMethod() {
        Value obj = make_unique<ObjectValue>(id());
        if (methods_.size() > 0  and  methods_.rbegin()->first == DefaultMethod) {
            auto defaultMethod = methods_.rbegin();
            return defaultMethod->second->execute();
        }
        ObjectPtr p = parent();
        if (p) {
            return p->executeDefaultMethod();
        } else {
            return make_unique<AbsentValue>();
        }
    }

    void Object::setMethod(int message_id, Statement stmt) {
        methods_[message_id] = std::move(stmt);
    }

    void Object::write(Storage& out) {
        out << parentId_ << id_ << static_cast<int>(prototype_);
        out << static_cast<int>(attributes_.size());
        for (auto const& [attribute_id, expr] : attributes_) {
            out << attribute_id << expr;
        }
        out << static_cast<int>(methods_.size());
        for (auto const& [message_id, stmt] : methods_) {
            out << message_id << stmt;
        }
    }

    void Object::read(Storage& in) {
        int is_prototype;
        in >> parentId_ >> id_ >> is_prototype;
        prototype_ = static_cast<bool>(is_prototype);

        int attribute_entries;
        in >> attribute_entries;
        attributes_.clear();
        for (int i = 0; i < attribute_entries; ++i) {
            int attribute_id;
            Expression expr;
            in >> attribute_id >> expr;
            attributes_[attribute_id] = std::move(expr);
        }

        int method_entries;
        in >> method_entries;
        methods_.clear();
        for (int i = 0; i < method_entries; ++i) {
            int message_id;
            Statement stmt;
            in >> message_id >> stmt;
            methods_[message_id] = std::move(stmt);
        }
    }

    Storage& operator<<(Storage& out, const ObjectPtr& p) {
        if (!p) {
            out << 0;
        } else {
            out << 1;
            p->write(out);
        }
        return out;
    }

    Storage& operator>>(Storage& in, ObjectPtr& p) {
        int non_null;
        in >> non_null;
        if (non_null) {
            if (not p.get()) {
                p = make_shared<Object>();
            }
            p->read(in);
        } else {
            p.reset();
        }
        return in;
    }

}
