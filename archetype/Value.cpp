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
    
    string MessageValue::toString() const {
        return GameDefinition::instance().Vocabulary.get(message_);
    }
    
    string NumericValue::toString() const {
        ostringstream out;
        out << value_;
        return out.str();
    }
    
    string StringValue::toString() const {
        return value_;
    }
    
    string ReservedConstantValue::toString() const {
        return Keywords::instance().Reserved.get(word_);
    }
    
    string IdentifierValue::toString() const {
        return GameDefinition::instance().Identifiers.get(id_);
    }
}