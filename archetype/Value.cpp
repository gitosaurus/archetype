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
    
    bool BooleanValue::isSameValueAs(const Value &other) const {
        const BooleanValue* other_p = dynamic_cast<const BooleanValue*>(other.get());
        return other_p and other_p->value_ == value_;
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
        string conversion = Universe::instance().Vocabulary.get(message_);
        return Value(new StringValue(conversion));
    }
    
    bool MessageValue::isSameValueAs(const Value &other) const {
        const MessageValue* other_p = dynamic_cast<const MessageValue*>(other.get());
        return other_p and other_p->message_ == message_;
    }
    
    bool NumericValue::isSameValueAs(const Value &other) const {
        const NumericValue* other_p = dynamic_cast<const NumericValue*>(other.get());
        return other_p and other_p->value_ == value_;
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
    
    string StringValue::getString() const {
        return value_;
    }
    
    Value StringValue::messageConversion() const {
        if (Universe::instance().Vocabulary.has(value_)) {
            return Value(new MessageValue(Universe::instance().Vocabulary.index(value_)));
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
    
    Value ReservedConstantValue::stringConversion() const {
        return Value(new StringValue(Keywords::instance().Reserved.get(word_)));
    }
    
    bool ReservedConstantValue::isTrueEnough() const {
        // TODO: How often does it happen that FALSE and UNDEFINED get to here?
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
            int message_id = Universe::instance().currentContext().messageId;
            return Value(new MessageValue(message_id));
        } else {
            return Value(new UndefinedValue);
        }
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
    
    int ObjectValue::getObject() const {
        return objectId_;
    }
    
    bool AttributeValue::isSameValueAs(const Value &other) const {
        const AttributeValue* other_p = dynamic_cast<const AttributeValue*>(other.get());
        return other_p and other_p->objectId_ == objectId_ and other_p->attributeId_ == attributeId_;
    }
    
    int AttributeValue::getIdentifier() const {
        return attributeId_;
    }
    
    Value AttributeValue::dereference_() const {
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        if (obj and obj->hasAttribute(attributeId_)) {
            SelfScope s(obj);
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
    
}