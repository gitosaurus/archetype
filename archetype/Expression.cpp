//
//  Expression.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <cmath>

#include "Expression.h"
#include "Keywords.h"
#include "Universe.h"

using namespace std;

namespace archetype {

    enum ExpressionType_e {
        RESERVED,
        UNARY,
        BINARY,
        IDENTIFIER,
        VALUE
    };

    inline Value as_boolean_value(bool value) {
        return Value{new BooleanValue{value}};
    }

    inline bool is_binary(Keywords::Operators_e op) {
        switch (op) {
            case Keywords::OP_LPAREN:
            case Keywords::OP_CHS:
            case Keywords::OP_NUMERIC:
            case Keywords::OP_NOT:
            case Keywords::OP_STRING:
            case Keywords::OP_RANDOM:
            case Keywords::OP_LENGTH:
                return false;
            default:
                return true;
        }
    }

    inline bool is_right_associative(Keywords::Operators_e op) {

        // Anything unary must be right-associative;
        // all others are  left-associative, unless
        // special-cased.
        switch (op) {
            case Keywords::OP_POWER:
            case Keywords::OP_C_MULTIPLY:
            case Keywords::OP_C_DIVIDE:
            case Keywords::OP_C_PLUS:
            case Keywords::OP_C_MINUS:
            case Keywords::OP_C_CONCAT:
            case Keywords::OP_ASSIGN:
                return true;
            default:
                return not is_binary(op);
        }
    }

    inline int precedence(Keywords::Operators_e op) {
        switch (op) {
            case Keywords::OP_LPAREN: return 14;
            case Keywords::OP_DOT: return 13;

            case Keywords::OP_CHS: return 12;
            case Keywords::OP_NUMERIC: return 12;
            case Keywords::OP_STRING: return 12;
            case Keywords::OP_RANDOM: return 12;
            case Keywords::OP_LENGTH: return 12;

            case Keywords::OP_POWER: return 11;

            case Keywords::OP_MULTIPLY: return 10;
            case Keywords::OP_DIVIDE: return 10;

            case Keywords::OP_PLUS: return 9;
            case Keywords::OP_MINUS: return 9;
            case Keywords::OP_CONCAT: return 9;

            case Keywords::OP_WITHIN: return 8;

            case Keywords::OP_LEFTFROM: return 7;
            case Keywords::OP_RIGHTFROM: return 7;

            case Keywords::OP_SEND: return 6;
            case Keywords::OP_PASS: return 6;

            case Keywords::OP_EQ: return 5;
            case Keywords::OP_NE: return 5;
            case Keywords::OP_GT: return 5;
            case Keywords::OP_LT: return 5;
            case Keywords::OP_GE: return 5;
            case Keywords::OP_LE: return 5;

            case Keywords::OP_NOT: return 4;
            case Keywords::OP_AND: return 3;
            case Keywords::OP_OR: return 2;

            case Keywords::OP_C_MULTIPLY: return 1;
            case Keywords::OP_C_DIVIDE: return 1;
            case Keywords::OP_C_PLUS: return 1;
            case Keywords::OP_C_MINUS: return 1;
            case Keywords::OP_C_CONCAT: return 1;
            case Keywords::OP_ASSIGN: return 1;

            case Keywords::NumOperators:
                throw logic_error("Attempt to look up precedence of NumOperators");
                break;
        }
    }

    Keywords::Operators_e non_assignment_equivalent(Keywords::Operators_e op) {
        switch (op) {
            case Keywords::OP_C_PLUS:
                return Keywords::OP_PLUS;
            case Keywords::OP_C_MINUS:
                return Keywords::OP_MINUS;
            case Keywords::OP_C_MULTIPLY:
                return Keywords::OP_MULTIPLY;
            case Keywords::OP_C_DIVIDE:
                return Keywords::OP_DIVIDE;
            case Keywords::OP_C_CONCAT:
                return Keywords::OP_CONCAT;
            default:
                throw logic_error("Unexpected cumulative assignment operator");
        }
    }

    void ValueExpression::write(Storage& out) const {
        out << VALUE << value_;
    }

    // This node is for those reserved words that behave like zero-argument
    // functions (sender, read, key, and so forth).
    class ReservedWordNode : public IExpression {
        Keywords::Reserved_e word_;
    public:
        ReservedWordNode(Keywords::Reserved_e word): word_(word) { }
        virtual void write(Storage& out) const override;
        virtual Value evaluate() const override;
        virtual void prefixDisplay(std::ostream& out) const override {
            out << Keywords::instance().Reserved.get(word_);
        }
    };

    Expression tie_on_rside(Expression existing,
                            Keywords::Operators_e op,
                            Expression new_rside);

    class IdentifierNode : public IExpression {
        int id_;
    public:
        IdentifierNode(int id): id_{id} { }
        int id() const { return id_; }
        virtual void write(Storage& out) const override { out << IDENTIFIER << id_; }
        virtual Value evaluate() const override {
            // Closest binding:  an attribute in the current object
            ObjectPtr selfObject = Universe::instance().currentContext().selfObject;
            if (selfObject and selfObject->hasAttribute(id_)) {
                return Value(new AttributeValue(selfObject->id(), id_));
            }
            // Next:  an object in the Universe
            auto id_obj_p = Universe::instance().ObjectIdentifiers.find(id_);
            if (id_obj_p != Universe::instance().ObjectIdentifiers.end()) {
                return Value(new ObjectValue(id_obj_p->second));
            }

            // Finally:  just a keyword value
            return Value(new IdentifierValue(id_));
        }
        virtual void prefixDisplay(ostream& out) const override {
            out << Universe::instance().Identifiers.get(id_);
        }
    };

    class Operator : public IExpression {
        Keywords::Operators_e op_;
    protected:
        Operator(Keywords::Operators_e op): op_{op} { }
    public:
        Keywords::Operators_e op() const     { return op_; }

        virtual bool bindsBefore(Keywords::Operators_e other) const {
            if (precedence(other) < precedence(op())) {
                return true;
            } else if (precedence(other) > precedence(op())) {
                return false;
            } else if (is_right_associative(other)) {
                return false;
            } else {
                return true;
            }
        }

    };

    class UnaryOperator : public Operator {
        Expression right_;
    public:
        UnaryOperator(Keywords::Operators_e op, Expression right):
        Operator{op},
        right_{move(right)} {
            assert(not is_binary(op));
        }

        virtual bool verify(TokenStream& t) const override {
            return right_->verify(t);
        }

        virtual void write(Storage& out) const override {
            out << UNARY;
            int op_as_int = static_cast<int>(op());
            out << op_as_int << right_;
        }

        virtual Value evaluate() const {
            Value rv = right_->evaluate()->valueConversion();
            switch (op()) {
                case Keywords::OP_CHS: {
                    Value rv_n = rv->numericConversion();
                    if (rv_n->isDefined()) {
                        return Value{new NumericValue{-rv_n->getNumber()}};
                    } else {
                        return rv_n;
                    }
                }
                case Keywords::OP_NUMERIC:
                    return rv->numericConversion();
                case Keywords::OP_NOT:
                    return Value{new BooleanValue{not rv->isTrueEnough()}};
                case Keywords::OP_STRING:
                    return rv->stringConversion();
                case Keywords::OP_RANDOM: {
                    Value rv_n = rv->numericConversion();
                    if (rv_n->isDefined() and rv_n->getNumber() > 0) {
                        double r = drand48();
                        int r_i = static_cast<int>(r * rv_n->getNumber()) + 1;
                        return Value{new NumericValue{r_i}};
                    } else {
                        return Value{new UndefinedValue};
                    }
                }
                case Keywords::OP_LENGTH: {
                    Value rv_s = rv->stringConversion();
                    if (rv_s->isDefined()) {
                        return Value{new NumericValue{static_cast<int>(rv_s->getString().size())}};
                    } else {
                        return rv_s;
                    }
                }
                default:
                    if (is_binary(op())) {
                        throw logic_error("Attempt to do UnaryOperator evaluation on binary operator " +
                                          Keywords::instance().Operators.get(op()));
                    } else {
                        throw logic_error("No unary operator evaluation written for " +
                                          Keywords::instance().Operators.get(op()));
                    }
            }
        }

        virtual void tieOnRightSide(Keywords::Operators_e op, Expression rightSide) {
            right_ = move(tie_on_rside(move(right_), op, move(rightSide)));
        }

        virtual int nodeCount() const { return 1 + right_->nodeCount(); }
        virtual Expression anyFewerNodeEquivalent() {
            right_ = tighten(move(right_));
            return op() != Keywords::OP_LPAREN ? nullptr : move(right_);
        }

        virtual void prefixDisplay(ostream& out) const {
            if (op() == Keywords::OP_LPAREN) {
                right_->prefixDisplay(out);
            } else {
                out << '(';
                out << Keywords::instance().Operators.get(int(op())) << ' ';
                right_->prefixDisplay(out);
                out << ')';
            }
        }
    };

    Value eval_ss(Keywords::Operators_e op, string lv_s, string rv_s) {
        switch (op) {
            case Keywords::OP_CONCAT:
                return Value(new StringValue(lv_s + rv_s));
            case Keywords::OP_WITHIN: {
                size_t where = rv_s.find(lv_s);
                if (where == string::npos) {
                    return Value{new UndefinedValue};
                } else {
                    return Value{new NumericValue{static_cast<int>(where+ 1)}};
                }
            }
            default:
                throw logic_error("string-op-string attempted on this operator");
        }
    }

    Value eval_sn(Keywords::Operators_e op, string lv_s, int rv_n) {
        switch (op) {
            case Keywords::OP_LEFTFROM:
                return Value(new StringValue(lv_s.substr(0, rv_n)));
            case Keywords::OP_RIGHTFROM: {
                int n = min(int(lv_s.size() + 1), max(0, rv_n));
                return Value(new StringValue(lv_s.substr(n - 1)));
            }
            default:
                throw logic_error("string-op-number attempted on this operator");
        }
    }

    Value eval_nn(Keywords::Operators_e op, int lv_n, int rv_n) {
        int result;
        switch (op) {
            case Keywords::OP_PLUS:     result = lv_n + rv_n;     break;
            case Keywords::OP_MINUS:    result = lv_n - rv_n;     break;
            case Keywords::OP_MULTIPLY: result = lv_n * rv_n;     break;
            case Keywords::OP_DIVIDE:   result = lv_n / rv_n;     break;
            case Keywords::OP_POWER:    result = pow(lv_n, rv_n); break;
            default:
                throw logic_error("number-op-number attempted on this operator");
        }
        return Value{new NumericValue{result}};
    }

    bool eval_compare(Keywords::Operators_e op, const Value& lv, const Value& rv) {
        // Quick shortcut for identity.  Will also catch (v = UNDEFINED) and (UNDEFINED = v).
        if (op == Keywords::OP_EQ and lv->isSameValueAs(rv)) {
            return true;
        }

        // If one side or the other is UNDEFINED, only ~= is possible to be true.
        if ((not lv->isDefined()) ^ (not rv->isDefined())) {
            return op == Keywords::OP_NE;
        }

        // But if they are both UNDEFINED, then they are only equal if testing for
        // that equality.
        if ((not lv->isDefined()) and (not rv->isDefined())) {
            return op == Keywords::OP_EQ;
        }

        Value lv_n = lv->numericConversion();
        Value rv_n = rv->numericConversion();
        if (lv_n->isDefined() and rv_n->isDefined()) {
            int ln = lv_n->getNumber();
            int rn = rv_n->getNumber();
            switch (op) {
                case Keywords::OP_EQ: return ln == rn;
                case Keywords::OP_NE: return ln != rn;
                case Keywords::OP_LT: return ln <  rn;
                case Keywords::OP_LE: return ln <= rn;
                case Keywords::OP_GE: return ln >= rn;
                case Keywords::OP_GT: return ln >  rn;
                default: throw logic_error("Unexpected numeric eval_compare case");
            }
        }
        Value lv_s = lv->stringConversion();
        Value rv_s = rv->stringConversion();
        if (lv_s->isDefined() and rv_s->isDefined()) {
            string ls = lv_s->getString();
            string rs = rv_s->getString();
            switch (op) {
                case Keywords::OP_EQ: return ls == rs;
                case Keywords::OP_NE: return ls != rs;
                case Keywords::OP_LT: return ls <  rs;
                case Keywords::OP_LE: return ls <= rs;
                case Keywords::OP_GE: return ls >= rs;
                case Keywords::OP_GT: return ls >  rs;
                default: throw logic_error("Unexpected numeric eval_compare case");
            }
        }
        Value lv_o = lv->objectConversion();
        Value rv_o = rv->objectConversion();
        if (lv_o->isDefined() and rv_o->isDefined()) {
            int l_obj = lv_o->getObject();
            int r_obj = rv_o->getObject();
            switch (op) {
                case Keywords::OP_EQ: return l_obj == r_obj;
                case Keywords::OP_NE: return l_obj != r_obj;
                default:
                    return false;
            }
        }
        // Special case:  if the two values aren't comparable, that is, the comparison
        // fails no matter the conversion, then they definitely aren't equal.
        if (op == Keywords::OP_NE) {
            return not lv->isSameValueAs(rv);
        } else {
            return false;
        }
    }

    class BinaryOperator : public Operator {
        Expression left_;
        Expression right_;
    public:
        BinaryOperator(Expression left, Keywords::Operators_e op, Expression right):
        Operator{op},
        left_{move(left)},
        right_{move(right)}
        {
            assert(is_binary(op));
        }

        virtual bool verify(TokenStream& t) const override {
            if (not (left_->verify(t) and right_->verify(t))) {
                return false;
            }

            switch (op()) {
                case Keywords::OP_DOT:
                    if (not dynamic_cast<const IdentifierNode*>(right_.get())) {
                        t.errorMessage("Right-hand side of \".\" must be an identifier");
                        return false;
                    }
                    break;
                case Keywords::OP_ASSIGN:
                case Keywords::OP_C_CONCAT:
                case Keywords::OP_C_DIVIDE:
                case Keywords::OP_C_MINUS:
                case Keywords::OP_C_MULTIPLY:
                case Keywords::OP_C_PLUS:
                    if (dynamic_cast<const IdentifierNode*>(left_.get())) {
                        return true;
                    } else if (const BinaryOperator* binary = dynamic_cast<const BinaryOperator*>(left_.get())) {
                        if (binary->op() != Keywords::OP_DOT) {
                            t.errorMessage("Left-hand side of assignment must be an attribute");
                            return false;
                        }
                    } else {
                        return false;
                    }
                    break;
                default:
                    return true;
            }
            return true;
        }

        virtual void write(Storage& out) const override {
            out << BINARY;
            int op_as_int = static_cast<int>(op());
            out << op_as_int << left_ << right_;
        }

        virtual Value evaluate() const {
            // Sort evaluations by "signature"
            switch (op()) {
                case Keywords::OP_CONCAT:
                case Keywords::OP_WITHIN: {
                    Value lv_s = left_->evaluate()->stringConversion();
                    Value rv_s = right_->evaluate()->stringConversion();
                    if (lv_s->isDefined() and rv_s->isDefined()) {
                        return eval_ss(op(), lv_s->getString(), rv_s->getString());
                    } else {
                        return Value{new UndefinedValue};
                    }
                }
                case Keywords::OP_LEFTFROM:
                case Keywords::OP_RIGHTFROM: {
                    Value lv_s = left_->evaluate()->stringConversion();
                    Value rv_n = right_->evaluate()->numericConversion();
                    if (lv_s->isDefined() and rv_n->isDefined()) {
                        return eval_sn(op(), lv_s->getString(), rv_n->getNumber());
                    } else {
                        return Value{new UndefinedValue};
                    }
                }
                case Keywords::OP_AND: {
                    Value lv = left_->evaluate();
                    Value rv = right_->evaluate();
                    return Value{new BooleanValue{lv->isTrueEnough() and rv->isTrueEnough()}};
                }
                case Keywords::OP_OR: {
                    Value lv = left_->evaluate();
                    Value rv = right_->evaluate();
                    return Value{new BooleanValue{lv->isTrueEnough() or rv->isTrueEnough()}};
                }
                case Keywords::OP_PLUS:
                case Keywords::OP_MINUS:
                case Keywords::OP_MULTIPLY:
                case Keywords::OP_DIVIDE:
                case Keywords::OP_POWER: {
                    Value lv_n = left_->evaluate()->numericConversion();
                    Value rv_n = right_->evaluate()->numericConversion();
                    if (lv_n->isDefined() and rv_n->isDefined()) {
                        return eval_nn(op(), lv_n->getNumber(), rv_n->getNumber());
                    } else {
                        return Value{new UndefinedValue};
                    }
                }

                case Keywords::OP_C_PLUS:
                case Keywords::OP_C_MINUS:
                case Keywords::OP_C_MULTIPLY:
                case Keywords::OP_C_DIVIDE: {
                    Value lv = left_->evaluate();
                    Value rv = right_->evaluate();
                    Value lv_a = lv->attributeConversion();
                    Value lv_n = lv->numericConversion();
                    Value rv_n = rv->numericConversion();
                    Value rv_c;
                    if (lv_n->isDefined() and rv_n->isDefined()) {
                        rv_c = eval_nn(non_assignment_equivalent(op()), lv_n->getNumber(), rv_n->getNumber());
                    } else {
                        rv_c = Value{new UndefinedValue};
                    }
                    return lv_a->assign(std::move(rv_c));
                }

                case Keywords::OP_C_CONCAT: {
                    Value lv = left_->evaluate();
                    Value rv = right_->evaluate();
                    Value lv_a = lv->attributeConversion();
                    Value lv_s = lv->stringConversion();
                    Value rv_s = rv->stringConversion();
                    Value rv_c;
                    if (lv_s->isDefined() and rv_s->isDefined()) {
                        rv_c = eval_ss(non_assignment_equivalent(op()), lv_s->getString(), rv_s->getString());
                    } else {
                        rv_c = Value{new UndefinedValue};
                    }
                    return lv_a->assign(std::move(rv_c));

                }

                case Keywords::OP_EQ:
                case Keywords::OP_NE:
                case Keywords::OP_LT:
                case Keywords::OP_LE:
                case Keywords::OP_GE:
                case Keywords::OP_GT:
                    return as_boolean_value(eval_compare(op(),
                                                         left_->evaluate()->valueConversion(),
                                                         right_->evaluate()->valueConversion()));

                case Keywords::OP_ASSIGN: {
                    Value lv_a = left_->evaluate()->attributeConversion();
                    Value rv_v = right_->evaluate()->valueConversion();
                    return lv_a->assign(std::move(rv_v));
                }

                case Keywords::OP_DOT: {
                    Value lv_o = left_->evaluate()->objectConversion();
                    if (not lv_o->isDefined()) {
                        return Value{new UndefinedValue};
                    } else {
                        int object_id = lv_o->getObject();
                        const IdentifierNode* id_node = dynamic_cast<const IdentifierNode*>(right_.get());
                        if (id_node) {
                            int attribute_id = id_node->id();
                            return Value(new AttributeValue(object_id, attribute_id));
                        } else {
                            throw logic_error("Non-identifier node on right-hand-side of OP_DOT");
                        }
                    }
                }

                case Keywords::OP_SEND:
                case Keywords::OP_PASS: {
                    Value lv_v = left_->evaluate()->valueConversion();
                    Value rv_o = right_->evaluate()->objectConversion();
                    if (not rv_o->isDefined()) {
                        return rv_o;
                    }
                    ObjectPtr recipient = Universe::instance().getObject(rv_o->getObject());
                    if (not recipient) {
                        return Value{new UndefinedValue};
                    } else if (op() == Keywords::OP_PASS or recipient->isPrototype()) {
                        return Object::pass(recipient, std::move(lv_v));
                    } else {
                        return Object::send(recipient, std::move(lv_v));
                    }
                }

                default:
                    if (is_binary(op())) {
                        throw logic_error("No binary operator evaluation written for " +
                                          Keywords::instance().Operators.get(op()));
                    } else {
                        throw logic_error("Attempt to do BinaryOperator evaluation on unary operator " +
                                          Keywords::instance().Operators.get(op()));
                    }
            }
        }

        virtual void tieOnRightSide(Keywords::Operators_e op, Expression rightSide) {
            right_ = move(tie_on_rside(move(right_), op, move(rightSide)));
        }

        virtual int nodeCount() const { return 1 + left_->nodeCount() + right_->nodeCount(); }
        virtual Expression anyFewerNodeEquivalent() {
            left_ = move(tighten(move(left_)));
            right_ = move(tighten(move(right_)));
            return nullptr;
        }

        virtual void prefixDisplay(ostream& out) const {
            out << '(' << Keywords::instance().Operators.get(int(op())) << ' ';
            if (left_) {
                left_->prefixDisplay(out);
            } else {
                out << "nullptr";
            }
            out << ' ';
            if (right_) {
                right_->prefixDisplay(out);
            } else {
                out << "nullptr";
            }
            out << ')';
        }
    };

    void ReservedWordNode::write(Storage& out) const {
        out << RESERVED;
        int word_as_int = static_cast<int>(word_);
        out << word_as_int;
    }

    Value ReservedWordNode::evaluate() const {
        switch (word_) {
            case Keywords::RW_SELF:
                return Value{new ObjectValue{Universe::instance().currentContext().selfObject->id()}};
            case Keywords::RW_SENDER:
                return Value{new ObjectValue{Universe::instance().currentContext().senderObject->id()}};
            case Keywords::RW_MESSAGE:
                return Universe::instance().currentContext().messageValue->clone();
            case Keywords::RW_EACH:
                return Value{new ObjectValue{Universe::instance().currentContext().eachObject->id()}};
            case Keywords::RW_READ:
                return Value{new StringValue{Universe::instance().input()->getLine()}};
            case Keywords::RW_KEY: {
                char key = Universe::instance().input()->getKey();
                if (key == '\0') {
                    return Value{new StringValue{""}};
                } else {
                    return Value{new StringValue{string{key}}};
                }
            }
            default:
                throw logic_error("Attempt to evaluate reserved word which is not a lambda");
        }
    }

    Expression get_scalar(TokenStream& t) {
        Expression scalar;
        switch (t.token().type()) {
            case Token::TEXT_LITERAL:
            case Token::QUOTE_LITERAL:
                scalar.reset(new ValueExpression{Value{new TextLiteralValue{t.token().number()}}});
                break;
            case Token::MESSAGE:
                scalar.reset(new ValueExpression{Value{new MessageValue{t.token().number()}}});
                break;
            case Token::NUMERIC:
                scalar.reset(new ValueExpression{Value{new NumericValue{t.token().number()}}});
                break;
            case Token::IDENTIFIER:
                scalar.reset(new IdentifierNode{t.token().number()});
                break;
            case Token::RESERVED_WORD: {
                // Some reserved words are like zero-argument functions, others are constant values
                Keywords::Reserved_e word = Keywords::Reserved_e(t.token().number());
                switch (word) {
                    case Keywords::RW_UNDEFINED:
                        scalar.reset(new ValueExpression{Value{new UndefinedValue}});
                        break;
                    case Keywords::RW_ABSENT:
                        scalar.reset(new ValueExpression{Value{new AbsentValue}});
                        break;
                    case Keywords::RW_BREAK:
                        scalar.reset(new ValueExpression{Value{new BreakValue}});
                        break;
                    case Keywords::RW_TRUE:
                        scalar.reset(new ValueExpression{Value{new BooleanValue{true}}});
                        break;
                    case Keywords::RW_FALSE:
                        scalar.reset(new ValueExpression{Value{new BooleanValue{false}}});
                        break;
                    case Keywords::RW_READ:
                    case Keywords::RW_KEY:
                    case Keywords::RW_EACH:
                    case Keywords::RW_SELF:
                    case Keywords::RW_SENDER:
                    case Keywords::RW_MESSAGE:
                        scalar.reset(new ReservedWordNode(word));
                        break;
                    default:
                        scalar.reset();
                        break;
                }  /* the "identifier" is a reserved keyword */
                break;
            }
            default:
                scalar.reset();
                break;
        }  /* switch t.token().type() */
        return scalar;
    }

    Expression get_operand_node(TokenStream& t) {
        switch (t.token().type()) {
            case Token::PUNCTUATION:
                switch (t.token().number()) {
                    case ')':
                        return nullptr;
                    case '(':
                        if (Expression expr = form_expr(t)) {
                            Expression result(new UnaryOperator(Keywords::OP_LPAREN, move(expr)));
                            return result;
                        } else {
                            return nullptr;
                        }
                    default:
                        return nullptr;
                }  /* switch t.token().number() */
                break;
            case Token::OPERATOR: {
                Keywords::Operators_e op_name = Keywords::Operators_e(t.token().number());
                // Check for special cases, operators that can be unary in this context
                switch (op_name) {
                    case Keywords::OP_PLUS:
                        op_name = Keywords::OP_NUMERIC;
                        break;
                    case Keywords::OP_MINUS:
                        op_name = Keywords::OP_CHS;
                        break;
                    case Keywords::OP_CONCAT:
                        op_name = Keywords::OP_STRING;
                        break;
                    default:
                        if (is_binary(op_name)) {
                            t.expectGeneral("unary operator");
                            t.stopLooking();
                            return nullptr;
                        }
                        break;
                }  /* switch op_name */
                if (Expression r = form_expr(t, precedence(op_name))) {
                    return Expression(new UnaryOperator(op_name, move(r)));
                } else {
                    return nullptr;
                }
            }
            default: { /* some constant or keyword */
                if (Expression scalar = get_scalar(t)) {
                    return Expression(new UnaryOperator(Keywords::OP_LPAREN, move(scalar)));
                } else {
                    return nullptr;
                }
            }                              /* some constant */
                break;
        }  /* switch t.token().type() */
    }

    Expression get_operand(TokenStream& t) {
        bool more = t.fetch();
        while (more and (t.token().type() == Token::NEWLINE)) {
            more = t.fetch();
        }
        if (more) {
            if (Expression node = get_operand_node(t)) {
                return node;
            }
        }
        t.didNotConsume();
        return nullptr;
    }

    Expression tie_on_rside(Expression existing,
                            Keywords::Operators_e op,
                            Expression new_rside) {
        if (existing->bindsBefore(op)) {
            return Expression(new BinaryOperator(move(existing), op, move(new_rside)));
        } else {
            existing->tieOnRightSide(op, move(new_rside));
            return existing;
        }
    }

    Expression form_expr(TokenStream& t, int stop_precedence) {
        Expression expr = get_operand(t);
        if (not expr) return nullptr;
        while (t.fetch()) {
            // Proceed only if the next token is a binary operator.
            // If this token we have just taken is a right-hand parenthesis,
            // only consume it if we're at level 0.
            if ((t.token().type() != Token::OPERATOR) or
                (not is_binary(Keywords::Operators_e(t.token().number())))) {
                if (not (t.token() == Token(Token::PUNCTUATION, ')') and stop_precedence == 0)) {
                    t.didNotConsume();
                }
                break;
            } else {
                Keywords::Operators_e the_operator = Keywords::Operators_e(t.token().number());
                if (precedence(the_operator) < stop_precedence) {
                    t.didNotConsume();
                    break;
                } else if (Expression rside = get_operand(t)) {
                    expr = tie_on_rside(move(expr), the_operator, move(rside));
                } else {
                    t.errorMessage("Empty expression or unbalanced parentheses");
                    return nullptr;
                }
            }
        }
        return expr;
    } // form_expr

    Expression tighten(Expression expr) {
        if (not expr) {
            return nullptr;
        }
        Expression t = expr->anyFewerNodeEquivalent();
        return move(t != nullptr ? t : expr);
    }

    bool verify_expr(const Expression& expr, TokenStream& t) {
        if (expr) {
            return expr->verify(t);
        } else {
            return true;
        }
    }

    Expression make_expr(TokenStream& t) {
        t.considerNewline();
        Expression expr = tighten(form_expr(t));
        t.restoreNewlineSignificance();
        if (not verify_expr(expr, t)) {
            return nullptr;
        } else {
            return expr;
        }
    }

    Storage& operator<<(Storage& out, const Expression& expr) {
        expr->write(out);
        return out;
    }

    Storage& operator>>(Storage& in, Expression& expr) {
        int node_type_as_int;
        in >> node_type_as_int;
        ExpressionType_e node_type = static_cast<ExpressionType_e>(node_type_as_int);
        switch (node_type) {
            case RESERVED: {
                int word_as_int;
                in >> word_as_int;
                Keywords::Reserved_e word = static_cast<Keywords::Reserved_e>(word_as_int);
                expr.reset(new ReservedWordNode(word));
                break;
            }
            case UNARY: {
                int op_as_int;
                in >> op_as_int;
                Keywords::Operators_e op = static_cast<Keywords::Operators_e>(op_as_int);
                Expression right_side;
                in >> right_side;
                expr.reset(new UnaryOperator(op, move(right_side)));
                break;
            }
            case BINARY: {
                int op_as_int;
                in >> op_as_int;
                Keywords::Operators_e op = static_cast<Keywords::Operators_e>(op_as_int);
                Expression left_side;
                Expression right_side;
                in >> left_side >> right_side;
                expr.reset(new BinaryOperator(move(left_side), op, move(right_side)));
                break;
            }
            case IDENTIFIER: {
                int id;
                in >> id;
                expr.reset(new IdentifierNode(id));
                break;
            }
            case VALUE: {
                Value value;
                in >> value;
                expr.reset(new ValueExpression(move(value)));
                break;
            }
        }
        return in;
    }

    Expression make_expr_from_str(string src_str) {
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr src(new SourceFile("test", in));
        TokenStream token_stream(src);
        Expression expr = make_expr(token_stream);
        return expr;
    }

} // archetype
