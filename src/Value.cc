//
//  Value.cc
//  archetype
//
//  Created by Derek Jones on 3/5/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>
#include <string_view>
#include <optional>
#include <memory>
#include <cctype>
#include <cassert>

#include "Value.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {
    using enum Keywords::Reserved_e;

    // Escape a string for safe embedding in a quoted literal.
    static std::string escape_string(std::string_view s) {
        std::string result;
        result.reserve(s.size() + 2);
        result += '"';
        for (char ch : s) {
            switch (ch) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                default:   result += ch;     break;
            }
        }
        result += '"';
        return result;
    }

    // Look up the identifier name bound to an object, if there is one.
    static std::optional<std::string> identifier_of(int object_id) {
        int identifier_id = Universe::instance().identifierForObject(object_id);
        if (identifier_id < 0) return std::nullopt;
        return Universe::instance().Identifiers.get(identifier_id);
    }


    enum ValueType_e {
        UNDEFINED,
        ABSENT,
        BREAK,
        BOOLEAN,
        MESSAGE,
        TEXT_LITERAL,
        NUMERIC,
        STRING,
        IDENTIFIER,
        OBJECT,
        ATTRIBUTE,
        PAIR
    };

    std::ostream& operator<<(std::ostream& out, const Value& value) {
        if (value) {
            value->display(out);
        } else {
            out << "nullptr";
        }
        return out;
    }

    inline Value numeric_value_from_string(string str) {
        int number = 0;
        for (char ch : str) {
            if (not isdigit(ch)) {
                return make_unique<UndefinedValue>();
            }
            number *= 10;
            number += (ch - '0');
        }
        return make_unique<NumericValue>(number);
    }

    Value IValue::messageConversion() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::stringConversion() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::numericConversion() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::identifierConversion() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::objectConversion() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::attributeConversion() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::head() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::tail() const {
        return make_unique<UndefinedValue>();
    }

    Value IValue::assign(Value /*new_value*/) {
        return make_unique<UndefinedValue>();
    }

    bool UndefinedValue::isSameValueAs(const Value &other) const {
        const UndefinedValue* other_p = dynamic_cast<const UndefinedValue*>(other.get());
        return other_p != nullptr;
    }

    void UndefinedValue::display(std::ostream &out) const {
        out << Keywords::instance().reservedWord(RW_UNDEFINED);
    }

    std::string UndefinedValue::asRDF() const {
        return "archetype:UNDEFINED";
    }

    void UndefinedValue::write(Storage& out) const {
        out << UNDEFINED;
    }

    bool AbsentValue::isSameValueAs(const Value &other) const {
        const AbsentValue* other_p = dynamic_cast<const AbsentValue*>(other.get());
        return other_p != nullptr;
    }

    void AbsentValue::display(std::ostream &out) const {
        out << Keywords::instance().reservedWord(RW_ABSENT);
    }

    std::string AbsentValue::asRDF() const {
        return "archetype:ABSENT";
    }

    void AbsentValue::write(Storage& out) const {
        out << ABSENT;
    }

    bool BreakValue::isSameValueAs(const Value &other) const {
        const BreakValue* other_p = dynamic_cast<const BreakValue*>(other.get());
        return other_p != nullptr;
    }

    void BreakValue::display(std::ostream &out) const {
        out << Keywords::instance().reservedWord(RW_BREAK);
    }

    std::string BreakValue::asRDF() const {
        return "archetype:BREAK";
    }

    void BreakValue::write(Storage& out) const {
        out << BREAK;
    }

    bool BooleanValue::isSameValueAs(const Value &other) const {
        const BooleanValue* other_p = dynamic_cast<const BooleanValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }

    void BooleanValue::display(std::ostream &out) const {
        out << Keywords::instance().reservedWord(value_ ? RW_TRUE : RW_FALSE);
    }

    std::string BooleanValue::asRDF() const {
        return value_ ? "\"true\"^^xsd:boolean" : "\"false\"^^xsd:boolean";
    }

    void BooleanValue::write(Storage& out) const {
        out << BOOLEAN << static_cast<int>(value_);
    }

    Value BooleanValue::numericConversion() const {
        return make_unique<NumericValue>(value_ ? 1 : 0);
    }

    Value BooleanValue::stringConversion() const {
        string bool_str = Keywords::instance().reservedWord(value_ ? RW_TRUE : RW_FALSE);
        return make_unique<StringValue>(bool_str);
    }

    int MessageValue::getMessage() const {
        return message_;
    }

    Value MessageValue::stringConversion() const {
        string conversion = Universe::instance().Messages.get(message_);
        return make_unique<StringValue>(conversion);
    }

    bool MessageValue::isSameValueAs(const Value &other) const {
        const MessageValue* other_p = dynamic_cast<const MessageValue*>(other.get());
        return other_p and other_p->message_ == message_;
    }

    void MessageValue::display(std::ostream &out) const {
        out << "'" << Universe::instance().Messages.get(message_) << "'";
    }

    std::string MessageValue::asRDF() const {
        return escape_string(Universe::instance().Messages.get(message_)) + "^^archetype:message";
    }

    void MessageValue::write(Storage& out) const {
        out << MESSAGE << message_;
    }

    string TextLiteralValue::getString() const {
        return Universe::instance().TextLiterals.get(textLiteral_);
    }

    Value TextLiteralValue::messageConversion() const {
        string value = getString();
        if (Universe::instance().Messages.has(value)) {
            return make_unique<MessageValue>(Universe::instance().Messages.index(value));
        } else {
            return make_unique<UndefinedValue>();
        }
    }

    Value TextLiteralValue::stringConversion() const {
        return make_unique<StringValue>(getString());
    }

    Value TextLiteralValue::numericConversion() const {
        return numeric_value_from_string(getString());
    }

    bool TextLiteralValue::isSameValueAs(const Value &other) const {
        const TextLiteralValue* other_p = dynamic_cast<const TextLiteralValue*>(other.get());
        return other_p and other_p->textLiteral_ == textLiteral_;
    }

    void TextLiteralValue::display(std::ostream &out) const {
        out << '"' << Universe::instance().TextLiterals.get(textLiteral_) << '"';
    }

    std::string TextLiteralValue::asRDF() const {
        return escape_string(getString());
    }

    void TextLiteralValue::write(Storage& out) const {
        out << TEXT_LITERAL << textLiteral_;
    }

    bool NumericValue::isSameValueAs(const Value &other) const {
        const NumericValue* other_p = dynamic_cast<const NumericValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }

    void NumericValue::display(std::ostream &out) const {
        out << value_;
    }

    std::string NumericValue::asRDF() const {
        return std::to_string(value_);
    }

    void NumericValue::write(Storage& out) const {
        out << NUMERIC << value_;
    }

    int NumericValue::getNumber() const {
        return value_;
    }

    Value NumericValue::stringConversion() const {
        return make_unique<StringValue>(std::to_string(value_));
    }

    bool StringValue::isSameValueAs(const Value &other) const {
        const StringValue* other_p = dynamic_cast<const StringValue*>(other.get());
        return other_p and other_p->value_ == value_;
    }

    void StringValue::display(std::ostream &out) const {
        out << '"' << value_ << '"';
    }

    std::string StringValue::asRDF() const {
        return escape_string(value_);
    }

    void StringValue::write(Storage& out) const {
        out << STRING;
        out << static_cast<int>(value_.size());
        out.write({reinterpret_cast<const Storage::Byte*>(value_.data()), value_.size()});
    }

    string StringValue::getString() const {
        return value_;
    }

    Value StringValue::messageConversion() const {
        if (Universe::instance().Messages.has(value_)) {
            return make_unique<MessageValue>(Universe::instance().Messages.index(value_));
        } else {
            return make_unique<UndefinedValue>();
        }
    }

    Value StringValue::numericConversion() const {
        return numeric_value_from_string(value_);
    }

    void IdentifierValue::display(std::ostream &out) const {
        out << Universe::instance().Identifiers.get(id_);
    }

    std::string IdentifierValue::asRDF() const {
        return "archetype:" + Universe::instance().Identifiers.get(id_);
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
        return other_p and other_p->objectId_ == objectId_;
    }

    void ObjectValue::display(std::ostream &out) const {
        int identifier_id = Universe::instance().identifierForObject(objectId_);
        if (identifier_id >= 0) {
            out << Universe::instance().Identifiers.get(identifier_id);
            return;
        }
        out << "<object " << objectId_ << ", type ";
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        ObjectPtr parent = obj->parent();
        if (parent) {
            ObjectValue parentValue{parent->id()};
            parentValue.display(out);
        } else {
            out << "null";
        }
        out << '>';
    }

    std::string ObjectValue::asRDF() const {
        auto name = identifier_of(objectId_);
        if (not name) {
            return "_:object_" + std::to_string(objectId_);
        }
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        std::string prefix = obj->isPrototype() ? "type:" : "obj:";
        return prefix + *name;
    }

    void ObjectValue::write(Storage& out) const {
        out << OBJECT << objectId_;
    }

    Value ObjectValue::identifierConversion() const {
        int identifier_id = Universe::instance().identifierForObject(objectId_);
        if (identifier_id >= 0) {
            return make_unique<IdentifierValue>(identifier_id);
        }
        return make_unique<UndefinedValue>();
    }

    int ObjectValue::getObject() const {
        return objectId_;
    }

    bool AttributeValue::isSameValueAs(const Value &other) const {
        const AttributeValue* other_p = dynamic_cast<const AttributeValue*>(other.get());
        return other_p and other_p->objectId_ == objectId_ and other_p->attributeId_ == attributeId_;
    }

    std::string AttributeValue::asRDF() const {
        return dereference_()->asRDF();
    }

    void AttributeValue::display(std::ostream &out) const {
        Value obj_v = make_unique<ObjectValue>(objectId_);
        obj_v->display(out);
        out << '.';
        out << Universe::instance().Identifiers.get(attributeId_);
        out << "(";
        dereference_()->display(out);
        out << ")";
    }

    void AttributeValue::write(Storage& out) const {
        out << ATTRIBUTE << objectId_ << attributeId_;
    }

    int AttributeValue::getIdentifier() const {
        return attributeId_;
    }

    Value AttributeValue::dereference_() const {
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        if (not obj or not obj->hasAttribute(attributeId_)) {
            return make_unique<UndefinedValue>();
        }

        ContextScope c;
        c->selfObject = obj;
        return obj->getAttributeValue(attributeId_);
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
        return make_unique<IdentifierValue>(attributeId_);
    }

    Value AttributeValue::objectConversion() const {
        return dereference_()->objectConversion();
    }

    Value AttributeValue::valueConversion() const {
        return dereference_();
    }

    Value AttributeValue::assign(Value new_value) {
        ObjectPtr obj = Universe::instance().getObject(objectId_);
        if (not obj) {
            return make_unique<UndefinedValue>();
        } else {
            obj->setAttribute(attributeId_, make_unique<ValueExpression>(std::move(new_value)));
            return clone();
        }
    }

    bool PairValue::isSameValueAs(const Value &other) const {
        if (const PairValue* other_p = dynamic_cast<const PairValue*>(other.get())) {
            return head_->isSameValueAs(other_p->head_) and tail_->isSameValueAs(other_p->tail_);
        } else {
            return false;
        }
    }

    Value PairValue::head() const {
        return head_->clone();
    }

    Value PairValue::tail() const {
        return tail_->clone();
    }

    void PairValue::display(ostream &out) const {
        const PairValue* tail_p = dynamic_cast<const PairValue*>(tail_.get());
        if (tail_->isDefined() and not tail_p) {
            out << '(';
            head_->display(out);
            out << " @ ";
            tail_->display(out);
            out << ')';
        } else {
            out << '[';
            head_->display(out);
            while (tail_p) {
                out << ' ';
                tail_p->head_->display(out);
                if (const PairValue* next_p = dynamic_cast<const PairValue*>(tail_p->tail_.get())) {
                    tail_p = next_p;
                } else {
                    if (tail_p->tail_->isDefined()) {
                        out << " @ ";
                        tail_p->tail_->display(out);
                    }
                    break;
                }
            }
            out << ']';
        }
    }

    std::string PairValue::asRDF() const {
        // Render as a Turtle collection: ( item1 item2 ... )
        std::string result = "( ";
        result += head_->asRDF();
        const PairValue* p = dynamic_cast<const PairValue*>(tail_.get());
        while (p) {
            result += " ";
            result += p->head_->asRDF();
            const PairValue* next = dynamic_cast<const PairValue*>(p->tail_.get());
            if (not next and p->tail_->isDefined()) {
                result += " ";
                result += p->tail_->asRDF();
            }
            p = next;
        }
        result += " )";
        return result;
    }

    void PairValue::write(Storage &out) const {
        out << PAIR << head_ << tail_;
    }

    Storage& operator<<(Storage& out, const Value& v) {
        v->write(out);
        return out;
    }

    Storage& operator>>(Storage& in, Value& v) {

        int type_as_int;
        in >> type_as_int;
        ValueType_e type = static_cast<ValueType_e>(type_as_int);
        switch (type) {
            case UNDEFINED:
                v = make_unique<UndefinedValue>();
                break;
            case ABSENT:
                v = make_unique<AbsentValue>();
                break;
            case BREAK:
                v = make_unique<BreakValue>();
                break;
            case BOOLEAN: {
                int bool_as_int;
                in >> bool_as_int;
                v = make_unique<BooleanValue>(static_cast<bool>(bool_as_int));
                break;
            }
            case MESSAGE: {
                int message_id;
                in >> message_id;
                v = make_unique<MessageValue>(message_id);
                break;
            }
            case TEXT_LITERAL: {
                int text_literal;
                in >> text_literal;
                v = make_unique<TextLiteralValue>(text_literal);
                break;
            }
            case NUMERIC: {
                int number;
                in >> number;
                v = make_unique<NumericValue>(number);
                break;
            }
            case STRING: {
                int text_size;
                in >> text_size;
                string text;
                text.resize(text_size);
                in.read({reinterpret_cast<Storage::Byte*>(text.data()), text.size()});
                v = make_unique<StringValue>(text);
                break;
            }
            case IDENTIFIER: {
                int id;
                in >> id;
                v = make_unique<IdentifierValue>(id);
                break;
            }
            case OBJECT: {
                int object_id;
                in >> object_id;
                v = make_unique<ObjectValue>(object_id);
                break;
            }
            case ATTRIBUTE: {
                int object_id, attribute_id;
                in >> object_id >> attribute_id;
                v = make_unique<AttributeValue>(object_id, attribute_id);
                break;
            }
            case PAIR: {
                Value head, tail;
                in >> head >> tail;
                v = make_unique<PairValue>(std::move(head), std::move(tail));
                break;
            }
        }
        return in;
    }


}
