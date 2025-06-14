//
//  Expression.cc
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
#include <random>
#include <stack>

#include "Expression.hh"
#include "Keywords.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {

    bool IExpression::Debug = false;

    enum ExpressionType_e {
        RESERVED,
        UNARY,
        BINARY,
        IDENTIFIER,
        VALUE
    };

    static void debug_expr(const IExpression& expr, const Value& result) {
        ostringstream out;
        expr.prefixDisplay(out);
        out << " => ";
        result->display(out);
        Universe::instance().output()->put(out.str());
        Universe::instance().output()->endLine();
    }

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
            case Keywords::OP_HEAD:
            case Keywords::OP_TAIL:
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
            case Keywords::OP_PAIR:
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

            // TODO:  What's the right precedence here?  LISP never has to decide
            case Keywords::OP_PAIR: return 2;
            case Keywords::OP_HEAD: return 2;
            case Keywords::OP_TAIL: return 2;

            case Keywords::OP_C_MULTIPLY: return 1;
            case Keywords::OP_C_DIVIDE: return 1;
            case Keywords::OP_C_PLUS: return 1;
            case Keywords::OP_C_MINUS: return 1;
            case Keywords::OP_C_CONCAT: return 1;
            case Keywords::OP_ASSIGN: return 1;

            default:
                throw logic_error("Attempt to look up precedence of nonexistent operator");
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
            Value result;
            // Closest binding:  an attribute in the current object
            ObjectPtr selfObject = Universe::instance().currentContext().selfObject;
            if (selfObject and selfObject->hasAttribute(id_)) {
                result = Value(new AttributeValue(selfObject->id(), id_));
            } else {
                // Next:  an object in the Universe
                auto id_obj_p = Universe::instance().ObjectIdentifiers.find(id_);
                if (id_obj_p != Universe::instance().ObjectIdentifiers.end()) {
                    result = Value{new ObjectValue{id_obj_p->second}};
                }
            }
            // Finally:  just a keyword value
            if (not result) {
                result = Value(new IdentifierValue(id_));
            }
            if (IExpression::Debug) {
                debug_expr(*this, result);
            }
            assert(result);
            return result;
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
        right_{std::move(right)} {
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

        virtual Value evaluate() const override {
            Value rv = right_->evaluate()->valueConversion();
            Value result;
            switch (op()) {
                case Keywords::OP_CHS: {
                    Value rv_n = rv->numericConversion();
                    if (rv_n->isDefined()) {
                        result = Value{new NumericValue{-rv_n->getNumber()}};
                    } else {
                        result = std::move(rv_n);
                    }
                    break;
                }
                case Keywords::OP_NUMERIC:
                    result = rv->numericConversion();
                    break;
                case Keywords::OP_NOT:
                    result = Value{new BooleanValue{not rv->isTrueEnough()}};
                    break;
                case Keywords::OP_STRING:
                    result = rv->stringConversion();
                    break;
                case Keywords::OP_RANDOM: {
                    Value rv_n = rv->numericConversion();
                    if (rv_n->isDefined() and rv_n->getNumber() > 0) {
                        random_device rd;
                        mt19937 gen(rd());
                        uniform_int_distribution<> dis(1, rv_n->getNumber());
                        int r_i = dis(gen);
                        result = Value{new NumericValue{r_i}};
                    } else {
                        result = Value{new UndefinedValue};
                    }
                    break;
                }
                case Keywords::OP_LENGTH: {
                    Value rv_s = rv->stringConversion();
                    if (rv_s->isDefined()) {
                        result = Value{new NumericValue{static_cast<int>(rv_s->getString().size())}};
                    } else {
                        result = std::move(rv_s);
                    }
                    break;
                }
                case Keywords::OP_HEAD: {
                    Value rv_v = rv->valueConversion();
                    result = rv_v->head();
                    break;
                }
                case Keywords::OP_TAIL: {
                    Value rv_v = rv->valueConversion();
                    result = rv_v->tail();
                    break;
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
            if (IExpression::Debug) {
                debug_expr(*this, result);
            }
            assert(result);
            return result;
        }

        virtual void tieOnRightSide(Keywords::Operators_e op, Expression rightSide) override {
            right_ = tie_on_rside(std::move(right_), op, std::move(rightSide));
        }

        virtual int nodeCount() const override { return 1 + right_->nodeCount(); }
        virtual Expression anyFewerNodeEquivalent() override {
            right_ = tighten(std::move(right_));
            return op() != Keywords::OP_LPAREN ? nullptr : std::move(right_);
        }

        virtual void prefixDisplay(ostream& out) const override {
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
                if (lv_s.empty()  or  where == string::npos) {
                    return Value{new UndefinedValue};
                } else {
                    return Value{new NumericValue{static_cast<int>(where + 1)}};
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
            case Keywords::OP_POWER:    result = static_cast<int>(pow(lv_n, rv_n)); break;
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
        if ((not lv->isDefined()) xor (not rv->isDefined())) {
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
        left_{std::move(left)},
        right_{std::move(right)}
        {
            assert(is_binary(op));
        }

        virtual bool verify(TokenStream& t) const override {
            if (not (left_->verify(t) and right_->verify(t))) {
                return false;
            }
            bool result = true;
            switch (op()) {
                case Keywords::OP_DOT:
                    if (auto id_node = dynamic_cast<const IdentifierNode*>(right_.get())) {
                        Universe::instance().classify(t, id_node->id(), REFERENCED_ATTRIBUTE_ID);
                    } else {
                        t.errorMessage("Right-hand side of \".\" must be an identifier");
                        result = false;
                    }
                    break;
                case Keywords::OP_ASSIGN:
                case Keywords::OP_C_CONCAT:
                case Keywords::OP_C_DIVIDE:
                case Keywords::OP_C_MINUS:
                case Keywords::OP_C_MULTIPLY:
                case Keywords::OP_C_PLUS: {
                    if (auto id_node = dynamic_cast<const IdentifierNode*>(left_.get())) {
                        Universe::instance().classify(t, id_node->id(), REFERENCED_ATTRIBUTE_ID);
                    } else if (auto binary = dynamic_cast<const BinaryOperator*>(left_.get())) {
                        if (binary->op() != Keywords::OP_DOT) {
                            t.errorMessage("Left-hand side of assignment must be an attribute");
                            result = false;
                        }
                    } else {
                        result = false;
                    }
                    break;
                }
                default:
                    result = true;
            }
            return result;
        }

        virtual void write(Storage& out) const override {
            out << BINARY;
            int op_as_int = static_cast<int>(op());
            out << op_as_int << left_ << right_;
        }

        virtual Value evaluate() const override {
            Value result;
            // Sort evaluations by "signature"
            switch (op()) {
                case Keywords::OP_PAIR: {
                    Value lv_v = left_->evaluate()->valueConversion();
                    Value rv_v = right_->evaluate()->valueConversion();
                    result = Value{new PairValue{std::move(lv_v), std::move(rv_v)}};
                    break;
                }
                case Keywords::OP_CONCAT:
                case Keywords::OP_WITHIN: {
                    Value lv_s = left_->evaluate()->stringConversion();
                    Value rv_s = right_->evaluate()->stringConversion();
                    if (lv_s->isDefined() and rv_s->isDefined()) {
                        result = eval_ss(op(), lv_s->getString(), rv_s->getString());
                    } else {
                        result = Value{new UndefinedValue};
                    }
                    break;
                }
                case Keywords::OP_LEFTFROM:
                case Keywords::OP_RIGHTFROM: {
                    Value lv_s = left_->evaluate()->stringConversion();
                    Value rv_n = right_->evaluate()->numericConversion();
                    if (lv_s->isDefined() and rv_n->isDefined()) {
                        result = eval_sn(op(), lv_s->getString(), rv_n->getNumber());
                    } else {
                        result = Value{new UndefinedValue};
                    }
                    break;
                }
                case Keywords::OP_AND: {
                    Value lv = left_->evaluate();
                    Value rv = right_->evaluate();
                    result = Value{new BooleanValue{lv->isTrueEnough() and rv->isTrueEnough()}};
                    break;
                }
                case Keywords::OP_OR: {
                    Value lv = left_->evaluate();
                    Value rv = right_->evaluate();
                    result = Value{new BooleanValue{lv->isTrueEnough() or rv->isTrueEnough()}};
                    break;
                }
                case Keywords::OP_PLUS:
                case Keywords::OP_MINUS:
                case Keywords::OP_MULTIPLY:
                case Keywords::OP_DIVIDE:
                case Keywords::OP_POWER: {
                    Value lv_n = left_->evaluate()->numericConversion();
                    Value rv_n = right_->evaluate()->numericConversion();
                    if (lv_n->isDefined() and rv_n->isDefined()) {
                        result = eval_nn(op(), lv_n->getNumber(), rv_n->getNumber());
                    } else {
                        result = Value{new UndefinedValue};
                    }
                    break;
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
                    result = lv_a->assign(std::move(rv_c));
                    break;
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
                    result = lv_a->assign(std::move(rv_c));
                    break;
                }

                case Keywords::OP_EQ:
                case Keywords::OP_NE:
                case Keywords::OP_LT:
                case Keywords::OP_LE:
                case Keywords::OP_GE:
                case Keywords::OP_GT:
                    result = as_boolean_value(eval_compare(op(),
                                                         left_->evaluate()->valueConversion(),
                                                         right_->evaluate()->valueConversion()));
                    break;

                case Keywords::OP_ASSIGN: {
                    Value lv_a = left_->evaluate()->attributeConversion();
                    Value rv_v = right_->evaluate()->valueConversion();
                    result = lv_a->assign(std::move(rv_v));
                    break;
                }

                case Keywords::OP_DOT: {
                    Value lv_o = left_->evaluate()->objectConversion();
                    if (not lv_o->isDefined()) {
                        result = Value{new UndefinedValue};
                    } else {
                        int object_id = lv_o->getObject();
                        const IdentifierNode* id_node = dynamic_cast<const IdentifierNode*>(right_.get());
                        if (id_node) {
                            int attribute_id = id_node->id();
                            result = Value(new AttributeValue(object_id, attribute_id));
                        } else {
                            throw logic_error("Non-identifier node on right-hand-side of OP_DOT");
                        }
                    }
                    break;
                }

                case Keywords::OP_SEND:
                case Keywords::OP_PASS: {
                    Value lv_v = left_->evaluate()->valueConversion();
                    Value rv_o = right_->evaluate()->objectConversion();
                    if (not rv_o->isDefined()) {
                        result = std::move(rv_o);
                    } else {
                        ObjectPtr recipient = Universe::instance().getObject(rv_o->getObject());
                        if (not recipient) {
                            result = Value{new UndefinedValue};
                        } else if (op() == Keywords::OP_PASS or recipient->isPrototype()) {
                            result = Object::pass(recipient, std::move(lv_v));
                        } else {
                            result = Object::send(recipient, std::move(lv_v));
                        }
                    }
                    break;
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
            if (IExpression::Debug) {
                debug_expr(*this, result);
            }
            assert(result);
            return result;
        }

        virtual void tieOnRightSide(Keywords::Operators_e op, Expression rightSide) override {
            right_ = tie_on_rside(std::move(right_), op, std::move(rightSide));
        }

        virtual int nodeCount() const override { return 1 + left_->nodeCount() + right_->nodeCount(); }
        virtual Expression anyFewerNodeEquivalent() override {
            left_ = tighten(std::move(left_));
            right_ = tighten(std::move(right_));
            return nullptr;
        }

        virtual void prefixDisplay(ostream& out) const override {
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
        Value result;
        switch (word_) {
            case Keywords::RW_SELF:
                result = Value{new ObjectValue{Universe::instance().currentContext().selfObject->id()}};
                break;
            case Keywords::RW_SENDER:
                result = Value{new ObjectValue{Universe::instance().currentContext().senderObject->id()}};
                break;
            case Keywords::RW_MESSAGE:
                result = Universe::instance().currentContext().messageValue->clone();
                break;
            case Keywords::RW_EACH:
                result = Value{new ObjectValue{Universe::instance().currentContext().eachObject->id()}};
                break;
            case Keywords::RW_READ: {
                string line = Universe::instance().input()->getLine();
                if (line.empty()  and  Universe::instance().input()->atEOF()) {
                    result = Value{new UndefinedValue};
                } else {
                    result = Value{new StringValue{line}};
                }
                break;
            }
            case Keywords::RW_KEY: {
                char key = Universe::instance().input()->getKey();
                if (key == '\4'  ||  key == '\0') {
                    // Consider it UNDEFINED if the user hit ^D (to immediately cause EOF)
                    // or if the stream truly is exhausted, which will return NUL ('\0').
                    result = Value{new UndefinedValue};
                } else {
                    result = Value{new StringValue{string{key}}};
                }
                break;
            }
            default:
                throw logic_error("Attempt to evaluate reserved word which is not a lambda");
        }
        if (IExpression::Debug) {
            debug_expr(*this, result);
        }
        assert(result);
        return result;
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
            case Token::IDENTIFIER: {
                int id = t.token().number();
                Universe::instance().classify(t, id, UNKNOWN_ID);
                scalar.reset(new IdentifierNode{id});
                break;
            }
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

    Expression form_list_expr(TokenStream& t) {
        // Called when the list has already begun, with an opening '{'.
        stack<Expression> elements;
        while (t.fetch() and t.token() != Token(Token::PUNCTUATION, '}')) {
            if (t.token() != Token(Token::PUNCTUATION, ';')) {
                t.didNotConsume();
            }
            if (Expression element = form_expr(t)) {
                elements.push(tighten(std::move(element)));
            } else {
                return nullptr;
            }
        }
        Expression list_expr{new ValueExpression{Value{new UndefinedValue}}};
        while (not elements.empty()) {
            list_expr = Expression{new BinaryOperator{std::move(elements.top()), Keywords::OP_PAIR, std::move(list_expr)}};
            elements.pop();
        }
        // Ensure that everything inside the braces is grouped together
        return Expression{new UnaryOperator{Keywords::OP_LPAREN, std::move(list_expr)}};
    }

    Expression get_operand_node(TokenStream& t) {
        switch (t.token().type()) {
            case Token::PUNCTUATION:
                switch (t.token().number()) {
                    case ')':
                        return nullptr;
                    case '(':
                        if (Expression expr = form_expr(t)) {
                            Expression result(new UnaryOperator(Keywords::OP_LPAREN, std::move(expr)));
                            return result;
                        } else {
                            return nullptr;
                        }
                    case '{':
                        return form_list_expr(t);
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
                    return Expression(new UnaryOperator(op_name, std::move(r)));
                } else {
                    return nullptr;
                }
            }
            default: { /* some constant or keyword */
                if (Expression scalar = get_scalar(t)) {
                    return Expression(new UnaryOperator(Keywords::OP_LPAREN, std::move(scalar)));
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
            return Expression(new BinaryOperator(std::move(existing), op, std::move(new_rside)));
        } else {
            existing->tieOnRightSide(op, std::move(new_rside));
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
                    expr = tie_on_rside(std::move(expr), the_operator, std::move(rside));
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
        return std::move(t != nullptr ? t : expr);
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
                expr.reset(new UnaryOperator(op, std::move(right_side)));
                break;
            }
            case BINARY: {
                int op_as_int;
                in >> op_as_int;
                Keywords::Operators_e op = static_cast<Keywords::Operators_e>(op_as_int);
                Expression left_side;
                Expression right_side;
                in >> left_side >> right_side;
                expr.reset(new BinaryOperator(std::move(left_side), op, std::move(right_side)));
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
                expr.reset(new ValueExpression(std::move(value)));
                break;
            }
        }
        return in;
    }

    Expression make_expr_from_str(string src_str) {
        stream_ptr in{new istringstream{src_str}};
        SourceFilePtr src{make_shared<SourceFile>("test", in)};
        TokenStream token_stream(src);
        Expression expr = make_expr(token_stream);
        return expr;
    }

} // archetype
