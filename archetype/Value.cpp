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
    
    enum Type_e {
        UNDEFINED,
        ABSENT,
        BREAK,
        BOOLEAN,
        MESSAGE,
        NUMERIC,
        STRING,
        IDENTIFIER,
        OBJECT,
        ATTRIBUTE
    };
        
    std::ostream& operator<<(std::ostream& out, const Value& value) {
        value->display(out);
        return out;
    }
    
    Value IValue::messageConversion() const {
        return Value{new UndefinedValue};
    }
    
    Value IValue::stringConversion() const {
        return Value{new UndefinedValue};
    }
    
    Value IValue::numericConversion() const {
        return Value{new UndefinedValue};
    }
    
    Value IValue::identifierConversion() const {
        return Value{new UndefinedValue};
    }
    
    Value IValue::objectConversion() const {
        return Value{new UndefinedValue};
    }
    
    Value IValue::attributeConversion() const {
        return Value{new UndefinedValue};
    }
    
    Value IValue::assign(Value new_value) {
        return Value{new UndefinedValue};
    }
    
    bool UndefinedValue::isSameValueAs(const Value &other) const {
        const UndefinedValue* other_p = dynamic_cast<const UndefinedValue*>(other.get());
        return other_p != nullptr;
    }
    
    void UndefinedValue::display(std::ostream &out) const {
        out << Keywords::instance().Reserved.get(Keywords::RW_UNDEFINED);
    }
    
    void UndefinedValue::write(Storage& out) const {
        out << UNDEFINED;
    }
    
    bool AbsentValue::isSameValueAs(const Value &other) const {
        const AbsentValue* other_p = dynamic_cast<const AbsentValue*>(other.get());
        return other_p != nullptr;
    }
    
    void AbsentValue::display(std::ostream &out) const {
        out << Keywords::instance().Reserved.get(Keywords::RW_ABSENT);
    }
    
    void AbsentValue::write(Storage& out) const {
        out << ABSENT;
    }
    
    bool BreakValue::isSameValueAs(const Value &other) const {
        const BreakValue* other_p = dynamic_cast<const BreakValue*>(other.get());
        return other_p != nullptr;
    }
    
    void BreakValue::display(std::ostream &out) const {
        out << Keywords::instance().Reserved.get(Keywords::RW_BREAK);
    }
    
    void BreakValue::write(Storage& out) const {
        out << BREAK;
    }
    
    bool BooleanValue::isSameValueAs(const Value &other) const {
        const BooleanValue* other_p = dynamic_cast<const BooleanValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }
    
    void BooleanValue::display(std::ostream &out) const {
        out << Keywords::instance().Reserved.get(value_ ?
                                                 Keywords::RW_TRUE :
                                                 Keywords::RW_FALSE);
    }
    
    void BooleanValue::write(Storage& out) const {
        out << BOOLEAN << static_cast<int>(value_);
    }
    
    Value BooleanValue::numericConversion() const {
        return Value(new NumericValue(value_ ? 1 : 0));
    }
    
    Value BooleanValue::stringConversion() const {
        string bool_str = Keywords::instance().Reserved.get(value_ ?
                                                            Keywords::RW_TRUE :
                                                            Keywords::RW_FALSE);
        return Value(new StringValue(bool_str));
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
        out << "'" << Universe::instance().TextLiterals.get(message_) << "'";
    }
    
    void MessageValue::write(Storage& out) const {
        out << MESSAGE << message_;
    }
    
    bool NumericValue::isSameValueAs(const Value &other) const {
        const NumericValue* other_p = dynamic_cast<const NumericValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }
    
    void NumericValue::display(std::ostream &out) const {
        out << value_;
    }
    
    void NumericValue::write(Storage& out) const {
        out << NUMERIC << value_;
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
    
    void StringValue::write(Storage& out) const {
        out << STRING;
        int text_length = static_cast<int>(value_.size());
        out << text_length;
        const Storage::Byte* buffer = reinterpret_cast<const Storage::Byte*>(value_.data());
        out.write(buffer, text_length);
    }
    
    string StringValue::getString() const {
        return value_;
    }
    
    Value StringValue::messageConversion() const {
        if (Universe::instance().TextLiterals.has(value_)) {
            return Value(new MessageValue(Universe::instance().TextLiterals.index(value_)));
        } else {
            return Value{new UndefinedValue};
        }
    }
    
    Value StringValue::numericConversion() const {
        int number = 0;
        for (char ch : value_) {
            if (not isdigit(ch)) {
                return Value{new UndefinedValue};
            }
            number *= 10;
            number += (ch - '0');
        }
        return Value(new NumericValue(number));
    }
    
    void IdentifierValue::display(std::ostream &out) const {
        out << Universe::instance().Identifiers.get(id_);
    }
    
    int IdentifierValue::getIdentifier() const {
        return id_;
    }
    
    bool IdentifierValue::isSameValueAs(const Value &other) const {
        const IdentifierValue* other_p = dynamic_cast<const IdentifierValue*>(other.get());
        return other_p and other_p->id_ == id_;
    }
    
    void IdentifierValue::write(Storage& out) const {
        out << IDENTIFIER << id_;
    }
    
    bool ObjectValue::isSameValueAs(const Value &other) const {
        const ObjectValue* other_p = dynamic_cast<const ObjectValue*>(other.get());
        return other_p and other_p->objectNameId_ == objectNameId_;
    }
    
    void ObjectValue::display(std::ostream &out) const {
        out << Universe::instance().Identifiers.get(objectNameId_);
    }
    
    void ObjectValue::write(Storage& out) const {
        out << OBJECT << objectNameId_;
    }
    
    Value ObjectValue::identifierConversion() const {
        return Value{new IdentifierValue{objectNameId_}};
    }
    
    int ObjectValue::getObject() const {
        return objectNameId_;
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
    
    void AttributeValue::write(Storage& out) const {
        out << ATTRIBUTE << objectId_ << attributeId_;
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
            return Value{new UndefinedValue};
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
        return Value{new IdentifierValue{attributeId_}};
    }
    
    Value AttributeValue::objectConversion() const {
        return dereference_()->objectConversion();
    }
    
    Value AttributeValue::assign(Value new_value) {
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        if (not obj) {
            return Value{new UndefinedValue};
        } else {
            obj->setAttribute(attributeId_, Expression{new ValueExpression{std::move(new_value)}});
            return clone();
        }
    }
    
    Storage& operator<<(Storage& out, const Value& v) {
        v->write(out);
        return out;
    }
    
    Storage& operator>>(Storage& in, Value& v) {
        
        int type_as_int;
        in >> type_as_int;
        Type_e type = static_cast<Type_e>(type_as_int);
        switch (type) {
            case UNDEFINED:
                v = Value{new UndefinedValue};
                break;
            case ABSENT:
                v = Value{new AbsentValue};
                break;
            case BREAK:
                v = Value{new BreakValue};
                break;
            case BOOLEAN: {
                int bool_as_int;
                in >> bool_as_int;
                v = Value(new BooleanValue(static_cast<bool>(bool_as_int)));
                break;
            }
            case MESSAGE: {
                int message_id;
                in >> message_id;
                v = Value(new MessageValue(message_id));
                break;
            }
            case NUMERIC: {
                int number;
                in >> number;
                v = Value(new NumericValue(number));
                break;
            }
            case STRING: {
                int text_size;
                in >> text_size;
                string text;
                text.resize(text_size);
                Storage::Byte* buffer = reinterpret_cast<Storage::Byte*>(&text[0]);
                in.read(buffer, text_size);
                v = Value(new StringValue(text));
                break;
            }
            case IDENTIFIER: {
                int id;
                in >> id;
                v = Value(new IdentifierValue(id));
                break;
            }
            case OBJECT: {
                int object_id;
                in >> object_id;
                v = Value(new ObjectValue(object_id));
                break;
            }
            case ATTRIBUTE: {
                int object_id, attribute_id;
                in >> object_id >> attribute_id;
                v = Value(new AttributeValue(object_id, attribute_id));
                break;
            }
        }
        return in;
    }

    
}