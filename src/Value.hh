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

#include "Formatting.hh"
#include "Keywords.hh"
#include "Serialization.hh"

namespace archetype {

    class IValue;
    typedef std::unique_ptr<IValue> Value;

    std::ostream& operator<<(std::ostream& out, const Value& value);

    class IValue {
    protected:
        IValue() = default;

        // Available to derived classes so that Cloneable can copy-construct them,
        // but not to the outside world, which must duplicate values via clone().
        IValue(const IValue&) = default;
    public:
        IValue& operator=(const IValue&) = delete;
        virtual ~IValue() { }

        virtual bool isDefined() const        { return true; }

        virtual bool isSameValueAs(const Value& other) const = 0;
        virtual Value clone() const = 0;
        virtual void display(std::ostream& out) const = 0;
        virtual std::string asRDF() const = 0;
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
        virtual Value valueConversion() const { return clone(); }

        virtual Value head() const;
        virtual Value tail() const;

        // Not every question about a value is answerable by conversion.  An
        // operator whose meaning depends on what a value is made of -- how many
        // things are in it, whether it can be ordered against another -- has to
        // ask, and these are what it asks with.
        virtual bool isList() const           { return false; }
        virtual int listLength() const        { return 0; }

        virtual Value assign(Value new_value);
    };

    // Supplies the clone() that every concrete value type would otherwise have to
    // spell out for itself.  A type derives from Cloneable<itself>; clone() then
    // copy-constructs that type, which makes the copy constructor the single place
    // where a value says how it duplicates itself.  Types whose members copy
    // correctly on their own (that is, all of them but PairValue) need say nothing.
    template <class Derived>
    class Cloneable : public IValue {
    public:
        virtual Value clone() const override {
            return std::make_unique<Derived>(static_cast<const Derived&>(*this));
        }
    };

    class UndefinedValue : public Cloneable<UndefinedValue> {
    public:
        UndefinedValue() { }
        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual bool isDefined()   const override { return false; }
        virtual bool isTrueEnough() const override { return false; }
    };

    class AbsentValue : public Cloneable<AbsentValue> {
    public:
        AbsentValue() { }
        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual bool isDefined()   const override { return true; }
        virtual bool isTrueEnough() const override { return false; }
    };

    class BreakValue : public Cloneable<BreakValue> {
    public:
        BreakValue() { }
        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual bool isDefined()   const override { return true; }
    };

    class BooleanValue : public Cloneable<BooleanValue> {
        bool value_;
    public:
        BooleanValue(bool value): value_(value) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual bool isTrueEnough() const override { return value_; }
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override;
    };

    class MessageValue : public Cloneable<MessageValue> {
        int message_;
    public:
        MessageValue(int message): message_(message) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual int getMessage() const override;

        virtual Value messageConversion() const override { return clone(); }
        virtual Value stringConversion() const override;
    };

    class TextLiteralValue : public Cloneable<TextLiteralValue> {
        int textLiteral_;
    public:
        TextLiteralValue(int text_literal): textLiteral_(text_literal) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual std::string getString() const override;

        virtual Value messageConversion() const override;
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override;
    };

    class NumericValue : public Cloneable<NumericValue> {
        int value_;
    public:
        NumericValue(int value): value_(value) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual int getNumber() const override;

        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override { return clone(); }
    };

    class StringValue : public Cloneable<StringValue> {
        std::string value_;
    public:
        StringValue(std::string value): value_(std::move(value)) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual std::string getString() const override;

        virtual Value messageConversion() const override;
        virtual Value stringConversion() const override { return clone(); }
        virtual Value numericConversion() const override;
    };

    class IdentifierValue : public Cloneable<IdentifierValue> {
        int id_;
    public:
        IdentifierValue(int id): id_(id) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual int getIdentifier() const override;

        virtual Value identifierConversion() const override { return clone(); }
    };

    class ObjectValue : public Cloneable<ObjectValue> {
        int objectId_;
    public:
        ObjectValue(int object_id): objectId_(object_id) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual int getObject() const override;

        virtual Value identifierConversion() const override;
        virtual Value objectConversion() const override { return clone(); }
    };

    // The purpose of this Value type is to track a writable reference to an object attribute.
    // If Archetype had any other types of left-hand values, there would be a parent
    // of this class called LeftHandValue.
    class AttributeValue : public Cloneable<AttributeValue> {
        int objectId_;
        int attributeId_;

        Value dereference_() const;
    public:
        AttributeValue(int object_id, int attribute_id): objectId_(object_id), attributeId_(attribute_id) { }

        virtual bool isSameValueAs(const Value& other) const override;
        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

        virtual int getIdentifier() const override;

        virtual bool isTrueEnough() const override;
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override;
        virtual Value identifierConversion() const override;
        virtual Value objectConversion() const override;
        virtual Value attributeConversion() const override { return clone(); }
        virtual Value valueConversion() const override;

        virtual Value assign(Value new_value) override;
    };

    class PairValue : public Cloneable<PairValue> {
        Value head_;
        Value tail_;
    public:
        PairValue(Value head, Value tail):
        head_(std::move(head)),
        tail_(std::move(tail))
        { }

        // A pair owns its head and tail outright, so duplicating one has to go
        // all the way down.  This is the only value type that copies by hand.
        PairValue(const PairValue& other):
        Cloneable<PairValue>{other},
        head_{other.head_->clone()},
        tail_{other.tail_->clone()}
        { }

        virtual bool isSameValueAs(const Value& other) const override;

        virtual Value head() const override;
        virtual Value tail() const override;

        virtual bool isList() const override { return true; }
        virtual int listLength() const override;

        virtual Value stringConversion() const override;

        virtual void display(std::ostream& out) const override;
        virtual std::string asRDF() const override;
        virtual void write(Storage& out) const override;

    };

    Storage& operator<<(Storage& out, const Value& v);
    Storage& operator>>(Storage& in, Value& v);

}

template <>
struct std::formatter<archetype::Value> : archetype::StreamedFormatter {
    auto format(const archetype::Value& value, std::format_context& ctx) const {
        return render([&](std::ostream& out) { out << value; }, ctx);
    }
};

#endif /* defined(__archetype__Value__) */
