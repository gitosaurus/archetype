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
#include "GameDefinition.h"

using namespace std;

namespace archetype {
    
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
    
    Value MessageValue::stringConversion() const {
        string conversion = GameDefinition::instance().Vocabulary.get(message_);
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
    
    Value NumericValue::numericConversion() const {
        return Value(new NumericValue(value_));
    }
    
    bool StringValue::isSameValueAs(const Value &other) const {
        const StringValue* other_p = dynamic_cast<const StringValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }
    
    string StringValue::getString() const {
        return value_;
    }
    
    Value StringValue::stringConversion() const {
        return Value(new StringValue(value_));
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
    
    int IdentifierValue::getIdentifier() const {
        return id_;
    }
        
    Value IdentifierValue::stringConversion() const {
        return Value(new StringValue(GameDefinition::instance().Identifiers.get(id_)));
    }
    
    Value IdentifierValue::identifierConversion() const {
        return Value(new IdentifierValue(id_));
    }
    
    Value IdentifierValue::objectConversion() const {
        auto id_obj_p = GameDefinition::instance().ObjectIdentifiers.find(id_);
        if (id_obj_p == GameDefinition::instance().ObjectIdentifiers.end()) {
            return Value(new UndefinedValue);
        }
        return Value(new ObjectValue(id_obj_p->second));
    }
    
    Value IdentifierValue::attributeConversion() const {
        ObjectPtr current = GameDefinition::instance().CurrentObject;
        if (current and current->hasAttribute(id_)) {
            return Value(new AttributeValue(current->id(), id_));
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
    
    Value ObjectValue::objectConversion() const {
        return Value(new ObjectValue(objectId_));
    }
    
    bool AttributeValue::isSameValueAs(const Value &other) const {
        const AttributeValue* other_p = dynamic_cast<const AttributeValue*>(other.get());
        return other_p and other_p->objectId_ == objectId_ and other_p->attributeId_ == attributeId_;
    }
    
    int AttributeValue::getIdentifier() const {
        return attributeId_;
    }
    
    Value AttributeValue::evaluate() const {
        assert(GameDefinition::instance().Objects.hasIndex(objectId_));
        ObjectPtr obj = GameDefinition::instance().Objects.get(objectId_);
        if (not obj->hasAttribute(attributeId_)) {
            return Value(new UndefinedValue);
        }
        return obj->attribute(attributeId_)->evaluate();
    }
    
    bool AttributeValue::isTrueEnough() const {
        return evaluate()->isTrueEnough();
    }
    
    Value AttributeValue::stringConversion() const {
        return evaluate()->stringConversion();
    }
    
    Value AttributeValue::numericConversion() const {
        return evaluate()->numericConversion();
    }
    
    Value AttributeValue::identifierConversion() const {
        return Value(new IdentifierValue(attributeId_));
    }
    
    Value AttributeValue::objectConversion() const {
        return evaluate()->objectConversion();
    }
    
    Value AttributeValue::attributeConversion() const {
        return Value(new AttributeValue(objectId_, attributeId_));
    }
    
}