//
//  Value.cpp
//  archetype
//
//  Created by Derek Jones on 3/5/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>
#include <cctype>
#include <cassert>

#include "Value.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    
    std::ostream& operator<<(std::ostream& out, const Value& value) {
        value->display(out);
        return out;
    }
    
    Value IValue::messageConversion() const {
        return Value(new UndefinedValue);
    }
    
    Value IValue::stringConversion() const {
        return Value(new UndefinedValue);
    }
    
    Value IValue::numericConversion() const {
        return Value(new UndefinedValue);
    }
    
    Value IValue::identifierConversion() const {
        return Value(new UndefinedValue);
    }
    
    Value IValue::objectConversion() const {
        return Value(new UndefinedValue);
    }
    
    Value IValue::attributeConversion() const {
        return Value(new UndefinedValue);
    }
    
    Value IValue::assign(Value new_value) {
        return Value(new UndefinedValue);
    }
    
    bool UndefinedValue::isSameValueAs(const Value &other) const {
        const UndefinedValue* other_p = dynamic_cast<const UndefinedValue*>(other.get());
        return other_p != nullptr;
    }
    
    void UndefinedValue::display(std::ostream &out) const {
        out << Keywords::instance().Reserved.get(Keywords::RW_UNDEFINED);
    }
    
    bool BooleanValue::isSameValueAs(const Value &other) const {
        const BooleanValue* other_p = dynamic_cast<const BooleanValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }
    
    void BooleanValue::display(std::ostream &out) const {
        if (value_) {
            out << Keywords::instance().Reserved.get(Keywords::RW_TRUE);
        } else {
            out << Keywords::instance().Reserved.get(Keywords::RW_FALSE);
        }
    }
    
    Value BooleanValue::numericConversion() const {
        return Value(new NumericValue(value_ ? 1 : 0));
    }
    
    Value BooleanValue::stringConversion() const {
        return Value(new StringValue(value_ ? "TRUE" : "FALSE"));
    }
    
    int MessageValue::getMessage() const {
        return message_;
    }
    
    Value MessageValue::stringConversion() const {
        string conversion = Universe::instance().TextLiterals.get(message_);
        return Value(new StringValue(conversion));
    }
    
    bool MessageValue::isSameValueAs(const Value &other) const {
        const MessageValue* other_p = dynamic_cast<const MessageValue*>(other.get());
        return other_p and other_p->message_ == message_;
    }
    
    void MessageValue::display(std::ostream &out) const {
        out << "'" << Universe::instance().TextLiterals.get(message_);
    }
    
    bool NumericValue::isSameValueAs(const Value &other) const {
        const NumericValue* other_p = dynamic_cast<const NumericValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }
    
    void NumericValue::display(std::ostream &out) const {
        out << value_;
    }
    
    int NumericValue::getNumber() const {
        return value_;
    }
    
    Value NumericValue::stringConversion() const {
        ostringstream out;
        out << value_;
        return Value(new StringValue(out.str()));
    }
    
    bool StringValue::isSameValueAs(const Value &other) const {
        const StringValue* other_p = dynamic_cast<const StringValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }
    
    void StringValue::display(std::ostream &out) const {
        out << '"' << value_ << '"';
    }
    
    string StringValue::getString() const {
        return value_;
    }
    
    Value StringValue::messageConversion() const {
        if (Universe::instance().TextLiterals.has(value_)) {
            return Value(new MessageValue(Universe::instance().TextLiterals.index(value_)));
        } else {
            return Value(new UndefinedValue);
        }
    }
    
    Value StringValue::numericConversion() const {
        int number = 0;
        for (char ch : value_) {
            if (not isdigit(ch)) {
                return Value(new UndefinedValue);
            }
            number *= 10;
            number += (ch - '0');
        }
        return Value(new NumericValue(number));
    }
    
    void ReservedConstantValue::display(std::ostream &out) const {
        out << Keywords::instance().Reserved.get(word_);
    }
    
    Value ReservedConstantValue::stringConversion() const {
        return Value(new StringValue(Keywords::instance().Reserved.get(word_)));
    }
    
    bool ReservedConstantValue::isTrueEnough() const {
        // TODO: How often will it happen that FALSE and UNDEFINED get to here?
        if (word_ == Keywords::RW_UNDEFINED ||
            word_ == Keywords::RW_FALSE ||
            word_ == Keywords::RW_ABSENT) {
            return false;
        } else {
            return true;
        }
    }
    
    bool ReservedConstantValue::isSameValueAs(const Value &other) const {
        const ReservedConstantValue* other_p = dynamic_cast<const ReservedConstantValue*>(other.get());
        return other_p and other_p->word_ == word_;
    }
    
    Value ReservedConstantValue::messageConversion() const {
        if (word_ == Keywords::RW_MESSAGE) {
            return Universe::instance().currentContext().messageValue->clone();
        } else {
            return Value(new UndefinedValue);
        }
    }
    
    void IdentifierValue::display(std::ostream &out) const {
        out << Universe::instance().Identifiers.get(id_);
    }
    
    int IdentifierValue::getIdentifier() const {
        return id_;
    }
    
    Value IdentifierValue::stringConversion() const {
        return attributeConversion()->stringConversion();
    }
    
    Value IdentifierValue::numericConversion() const {
        return attributeConversion()->numericConversion();
    }
    
    Value IdentifierValue::objectConversion() const {
        auto id_obj_p = Universe::instance().ObjectIdentifiers.find(id_);
        if (id_obj_p == Universe::instance().ObjectIdentifiers.end()) {
            return Value(new UndefinedValue);
        }
        return Value(new ObjectValue(id_obj_p->second));
    }
    
    Value IdentifierValue::attributeConversion() const {
        ObjectPtr selfObject = Universe::instance().currentContext().selfObject;
        if (selfObject and selfObject->hasAttribute(id_)) {
            return Value(new AttributeValue(selfObject->id(), id_));
        } else {
            return Value(new UndefinedValue);
        }
    }
    
    bool IdentifierValue::isSameValueAs(const Value &other) const {
        const IdentifierValue* other_p = dynamic_cast<const IdentifierValue*>(other.get());
        return other_p and other_p->id_ == id_;
    }
    
    bool ObjectValue::isSameValueAs(const Value &other) const {
        const ObjectValue* other_p = dynamic_cast<const ObjectValue*>(other.get());
        return other_p and other_p->objectId_ == objectId_;
    }
    
    void ObjectValue::display(std::ostream &out) const {
        int identifier = Universe::instance().ObjectIdentifiers[objectId_];
        out << Universe::instance().Identifiers.get(identifier);
    }
    
    int ObjectValue::getObject() const {
        return objectId_;
    }
    
    bool AttributeValue::isSameValueAs(const Value &other) const {
        const AttributeValue* other_p = dynamic_cast<const AttributeValue*>(other.get());
        return other_p and other_p->objectId_ == objectId_ and other_p->attributeId_ == attributeId_;
    }
    
    void AttributeValue::display(std::ostream &out) const {
        int identifier = Universe::instance().ObjectIdentifiers[objectId_];
        out
            << Universe::instance().Identifiers.get(identifier)
            << '.'
            << Universe::instance().Identifiers.get(attributeId_)
        ;
    }
    
    int AttributeValue::getIdentifier() const {
        return attributeId_;
    }
    
    Value AttributeValue::dereference_() const {
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        if (obj and obj->hasAttribute(attributeId_)) {
            ContextScope c;
            c->selfObject = obj;
            return obj->getAttributeValue(attributeId_);
        } else {
            return Value(new UndefinedValue);
        }
    }
    
    bool AttributeValue::isTrueEnough() const {
        return dereference_()->isTrueEnough();
    }
    
    Value AttributeValue::stringConversion() const {
        return dereference_()->stringConversion();
    }
    
    Value AttributeValue::numericConversion() const {
        return dereference_()->numericConversion();
    }
    
    Value AttributeValue::identifierConversion() const {
        return Value(new IdentifierValue(attributeId_));
    }
    
    Value AttributeValue::objectConversion() const {
        return dereference_()->objectConversion();
    }
    
    Value AttributeValue::assign(Value new_value) {
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        if (not obj) {
            return Value(new UndefinedValue);
        } else {
            obj->setAttribute(attributeId_, Expression(new ValueExpression(std::move(new_value))));
            return clone();
        }
    }
    
    Storage& operator<<(Storage& out, const Value& v) {
        // TODO:  finish
        return out;
    }
    
    Storage& operator>>(Storage& in, Value& v) {
        // TODO:  finish.  Will probably need another enumeration
        return in;
    }

    
}