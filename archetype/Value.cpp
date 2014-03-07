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
    
    Value MessageValue::stringConversion() const {
        string conversion = GameDefinition::instance().Vocabulary.get(message_);
        return Value(new StringValue(conversion));
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
    
    Value IdentifierValue::stringConversion() const {
        return Value(new StringValue(GameDefinition::instance().Identifiers.get(id_)));
    }
}