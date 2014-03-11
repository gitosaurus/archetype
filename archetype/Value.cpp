//
//  Value.cpp
//  archetype
//
//  Created by Derek Jones on 3/5/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>

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
    
    bool UndefinedValue::isSameValueAs(const Value &other) const {
        const UndefinedValue* other_p = dynamic_cast<UndefinedValue*>(other.get());
        return other_p != nullptr;
    }
    
    bool BooleanValue::isSameValueAs(const Value &other) const {
        const BooleanValue* other_p = dynamic_cast<BooleanValue*>(other.get());
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
        const MessageValue* other_p = dynamic_cast<MessageValue*>(other.get());
        return other_p and other_p->message_ == message_;
    }
    
    bool NumericValue::isSameValueAs(const Value &other) const {
        const NumericValue* other_p = dynamic_cast<NumericValue*>(other.get());
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
        const StringValue* other_p = dynamic_cast<StringValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }
    
    string StringValue::getString() const {
        return value_;
    }
    
    Value StringValue::stringConversion() const {
        return Value(new StringValue(value_));
    }
    
    Value StringValue::numericConversion() const {
        istringstream in(value_);
        int number = 0;
        in >> number;
        if (in.good()) {
            return Value(new NumericValue(number));
        } else {
            return Value(new UndefinedValue);
        }
    }
    
    Value ReservedConstantValue::stringConversion() const {
        return Value(new StringValue(Keywords::instance().Reserved.get(word_)));
    }
    
    bool ReservedConstantValue::isSameValueAs(const Value &other) const {
        const ReservedConstantValue* other_p = dynamic_cast<ReservedConstantValue*>(other.get());
        return other_p and other_p->word_ == word_;
    }
        
    Value IdentifierValue::stringConversion() const {
        return Value(new StringValue(GameDefinition::instance().Identifiers.get(id_)));
    }
    
    bool IdentifierValue::isSameValueAs(const Value &other) const {
        const IdentifierValue* other_p = dynamic_cast<IdentifierValue*>(other.get());
        return other_p and other_p->id_ == id_;
    }
}