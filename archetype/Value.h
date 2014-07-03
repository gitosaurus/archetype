//
//  Value.h
//  archetype
//
//  Created by Derek Jones on 3/5/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Value__
#define __archetype__Value__

#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>

#include "Keywords.h"
#include "Serialization.h"

namespace archetype {

    class IValue;
    typedef std::unique_ptr<IValue> Value;
    
    std::ostream& operator<<(std::ostream& out, const Value& value);
    
    class IValue {
    public:
        enum Type_e {
            UNDEFINED,
            ABSENT,
            BOOLEAN,
            MESSAGE,
            NUMERIC,
            STRING,
            IDENTIFIER,
            OBJECT,
            ATTRIBUTE
        };
        
        IValue() { }
        IValue(const IValue&) = delete;
        IValue& operator=(const IValue&) = delete;
    
        virtual bool isDefined() const        { return true; }

        virtual bool isSameValueAs(const Value& other) const = 0;
        virtual Value clone() const = 0;
        virtual void display(std::ostream& out) const = 0;
        virtual Type_e type() const = 0;
        virtual void write(Storage& out) const = 0;

        virtual bool isTrueEnough() const     { return true; }
        virtual int getMessage() const        { throw std::logic_error("Value is not a defined message"); }
        virtual std::string getString() const { throw std::logic_error("Value is not a string"); }
        virtual int getNumber() const         { throw std::logic_error("Value is not a number"); }
        virtual int getObject() const         { throw std::logic_error("Value is not an object reference"); }
        virtual int getIdentifier() const     { throw std::logic_error("Value does not have an identifier"); }
        
        virtual Value messageConversion() const;
        virtual Value stringConversion() const;
        virtual Value numericConversion() const;
        virtual Value identifierConversion() const;
        virtual Value objectConversion() const;
        virtual Value attributeConversion() const;
        
        virtual Value assign(Value new_value);
    };
    
    class UndefinedValue : public IValue {
    public:
        UndefinedValue() { }
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value{new UndefinedValue}; }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return UNDEFINED; }
        virtual void write(Storage& out) const override { }

        virtual bool isDefined()   const override { return false; }
        virtual bool isTrueEnough() const override { return false; }
    };
    
    class AbsentValue : public IValue {
    public:
        AbsentValue() { }
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value{new AbsentValue}; }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return ABSENT; }
        virtual void write(Storage& out) const override { }

        virtual bool isDefined()   const override { return true; }
        virtual bool isTrueEnough() const override { return false; }
    };
    
    class BooleanValue : public IValue {
        bool value_;
    public:
        BooleanValue(bool value): value_(value) { }
        
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value(new BooleanValue(value_)); }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return BOOLEAN; }
        virtual void write(Storage& out) const override;

        virtual bool isTrueEnough() const override { return value_; }
        virtual Value stringConversion() const;
        virtual Value numericConversion() const;
    };
    
    class MessageValue : public IValue {
        int message_;
    public:
        MessageValue(int message): message_(message) { }
        
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value(new MessageValue(message_)); }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return MESSAGE; }
        virtual void write(Storage& out) const override { out << message_; }
       
        virtual int getMessage() const override;
        
        virtual Value messageConversion() const override { return clone(); }
        virtual Value stringConversion() const override;
    };
    
    class NumericValue : public IValue {
        int value_;
    public:
        NumericValue(int value): value_(value) { }
        
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value(new NumericValue(value_)); }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return NUMERIC; }
        virtual void write(Storage& out) const override { out << value_; }
        
        virtual int getNumber() const override;
        
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override { return clone(); }
    };
    
    class StringValue : public IValue {
        std::string value_;
    public:
        StringValue(std::string value): value_(value) { }
        
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value(new StringValue(value_)); }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return STRING; }
        virtual void write(Storage& out) const override;
        
        virtual std::string getString() const override;
        
        virtual Value messageConversion() const override;
        virtual Value stringConversion() const override { return clone(); }
        virtual Value numericConversion() const override;
    };
    
    class IdentifierValue : public IValue {
        int id_;
    public:
        IdentifierValue(int id): id_(id) { }
        
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value(new IdentifierValue(id_)); }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return IDENTIFIER; }
        virtual void write(Storage& out) const override { out << id_; }
        
        virtual int getIdentifier() const override;
        
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override;
        virtual Value identifierConversion() const override { return clone(); }
        virtual Value objectConversion() const override;
        virtual Value attributeConversion() const override;
    };
    
    class ObjectValue : public IValue {
        int objectId_;
    public:
        ObjectValue(int object_id): objectId_(object_id) { }
        
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value(new ObjectValue(objectId_)); }
        virtual Type_e type() const override { return OBJECT; }
        virtual void display(std::ostream& out) const override;
        virtual void write(Storage& out) const override { out << objectId_; }
        
        virtual int getObject() const override;

        virtual Value objectConversion() const override { return clone(); }
    };
    
    // The purpose of this Value type is to track a writable reference to an object attribute.
    // If Archetype had any other types of left-hand values, there would be a parent
    // of this class called LeftHandValue.
    class AttributeValue : public IValue {
        int objectId_;
        int attributeId_;

        Value dereference_() const;
    public:
        AttributeValue(int object_id, int attribute_id): objectId_(object_id), attributeId_(attribute_id) { }
        
        virtual bool isSameValueAs(const Value& other) const override;
        virtual Value clone() const override { return Value(new AttributeValue(objectId_, attributeId_)); }
        virtual void display(std::ostream& out) const override;
        virtual Type_e type() const override { return ATTRIBUTE; }
        virtual void write(Storage& out) const override { out << objectId_ << attributeId_; }
        
        virtual int getIdentifier() const override;
        
        virtual bool isTrueEnough() const override;
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override;
        virtual Value identifierConversion() const override;
        virtual Value objectConversion() const override;
        virtual Value attributeConversion() const override { return clone(); }
        
        virtual Value assign(Value new_value) override;
    };
    
    Storage& operator<<(Storage& out, const Value& v);
    Storage& operator>>(Storage& in, Value& v);

}

#endif /* defined(__archetype__Value__) */
