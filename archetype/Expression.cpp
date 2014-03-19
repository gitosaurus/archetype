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

#include "Expression.h"
#include "Keywords.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    
    bool is_binary(Keywords::Operators_e op) {
        switch (op) {
            case Keywords::OP_LPAREN:
            case Keywords::OP_CHS:
            case Keywords::OP_NUMERIC:
            case Keywords::OP_STRING:
            case Keywords::OP_RANDOM:
            case Keywords::OP_LENGTH:
                return false;
            default:
                return true;
        }
    }
    
    bool is_right_associative(Keywords::Operators_e op) {
        
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
    
    int precedence(Keywords::Operators_e op) {
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
    
    class Operator : public IExpression {
        Keywords::Operators_e op_;
    protected:
        Operator(Keywords::Operators_e op): op_(op) { }
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
        Operator(op),
        right_(move(right)) {
            assert(not is_binary(op));
        }
        
        virtual Value evaluate() const {
            throw logic_error("UnaryOperator::evaluate under construction");
            return Value(new StringValue("<unknown>"));
        }
        
        virtual void tieOnRightSide(Keywords::Operators_e op, Expression rightSide) {
            right_ = move(tie_on_rside(move(right_), op, move(rightSide)));
        }
 
        virtual int nodeCount() const { return 1 + right_->nodeCount(); }
        virtual Expression anyFewerNodeEquivalent() {
            right_ = tighten(move(right_));
            return op() != Keywords::OP_LPAREN ? nullptr : move(right_);
        }
        
        virtual void prettyPrint(std::ostream& out, std::string indent) const {
            if (op() != Keywords::OP_LPAREN) {
                out << indent << Keywords::instance().Operators.get(int(op())) << endl;
            }
            right_->prettyPrint(out, indent + " ");
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
            case Keywords::OP_WITHIN:
                return Value(new BooleanValue(rv_s.find(lv_s) != string::npos));
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
            case Keywords::OP_PLUS:     result = lv_n + rv_n; break;
            case Keywords::OP_MINUS:    result = lv_n - rv_n; break;
            case Keywords::OP_MULTIPLY: result = lv_n * rv_n; break;
            case Keywords::OP_DIVIDE:   result = lv_n / rv_n; break;
            default:
                throw logic_error("number-op-number attempted on this operator");
        }
        return Value(new NumericValue(result));
    }
    
    bool eval_compare(Keywords::Operators_e op, const Value& lv, const Value& rv) {
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
        return false;
    }

    class BinaryOperator : public Operator {
        Expression left_;
        Expression right_;
    public:
        BinaryOperator(Expression left, Keywords::Operators_e op, Expression right):
        Operator(op),
        left_(move(left)),
        right_(move(right))
        {
            assert(is_binary(op));
        }
        
        virtual Value evaluate() const {
            Value lv = left_->evaluate();
            Value rv = right_->evaluate();
            // Sort evaluations by "signature"
            switch (op()) {
                case Keywords::OP_CONCAT:
                case Keywords::OP_WITHIN: {
                    Value lv_s = lv->stringConversion();
                    Value rv_s = rv->stringConversion();
                    if (lv_s->isDefined() and rv_s->isDefined()) {
                        return eval_ss(op(), lv_s->getString(), rv_s->getString());
                    } else {
                        return Value(new UndefinedValue);
                    }
                }
                case Keywords::OP_LEFTFROM:
                case Keywords::OP_RIGHTFROM: {
                    Value lv_s = lv->stringConversion();
                    Value rv_n = rv->numericConversion();
                    if (lv_s->isDefined() and rv_n->isDefined()) {
                        return eval_sn(op(), lv_s->getString(), rv_n->getNumber());
                    } else {
                        return Value(new UndefinedValue);
                    }
                }
                case Keywords::OP_PLUS:
                case Keywords::OP_MINUS:
                case Keywords::OP_MULTIPLY:
                case Keywords::OP_DIVIDE: {
                    Value lv_n = lv->numericConversion();
                    Value rv_n = rv->numericConversion();
                    if (lv_n->isDefined() and rv_n->isDefined()) {
                        return eval_nn(op(), lv_n->getNumber(), rv_n->getNumber());
                    } else {
                        return Value(new UndefinedValue);
                    }
                }
                case Keywords::OP_EQ:
                    if (lv->isSameValueAs(rv)) return Value(new BooleanValue(true));
                    // Otherwise, intentional fall-through
                case Keywords::OP_NE:
                case Keywords::OP_LT:
                case Keywords::OP_LE:
                case Keywords::OP_GE:
                case Keywords::OP_GT:
                    return Value(new BooleanValue(eval_compare(op(), lv, rv)));

                case Keywords::OP_ASSIGN: {
                    Value lv_a = lv->attributeConversion();
                    return lv_a->assign(std::move(rv));
                }
                    
                case Keywords::OP_DOT: {
                    Value lv_o = lv->objectConversion();
                    Value rv_a = rv->identifierConversion();
                    if (lv_o->isDefined() and rv_a->isDefined()) {
                        int object_id = lv_o->getObject();
                        int attribute_id = rv_a->getIdentifier();
                        return Value(new AttributeValue(object_id, attribute_id));
                    } else {
                        return Value(new UndefinedValue);
                    }
                }
                    
                case Keywords::OP_SEND: {
                    Value rv_o = rv->objectConversion();
                    if (rv_o->isDefined()) {
                        ObjectPtr recipient = Universe::instance().Objects.get(rv_o->getObject());
                        if (not recipient) {
                            return Value(new UndefinedValue);
                        } else {
                            SelfScope s(recipient);
                            // TODO:  Need evaluate() to go to an ostream!
                            return recipient->send(std::move(lv), std::cout);
                        }
                    } else {
                        return Value(new UndefinedValue);
                    }
                }
                    
                case Keywords::OP_PASS: {
                    // Like OP_SEND but doesn't push the new object's context
                    // Also, in this case, the RHS can be a type and not an instance
                    return Value(new UndefinedValue);
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
            throw logic_error("BinaryOperator::evaluate under construction");
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
        
        virtual void prettyPrint(ostream& out, string indent) const {
            out << indent << Keywords::instance().Operators.get(int(op())) << endl;
            left_->prettyPrint(out, indent + " ");
            right_->prettyPrint(out, indent + " ");
        }
        virtual void prefixDisplay(ostream& out) const {
            out << '(' << Keywords::instance().Operators.get(int(op())) << ' ';
            left_->prefixDisplay(out);
            out << ' ';
            right_->prefixDisplay(out);
            out << ')';
        }
    };
    
    class LiteralNode : public ScalarNode {
        int index_;
    protected:
        LiteralNode(int index): index_(index) { }
    public:
        int index() const { return index_; }
    };
    
    class NumericLiteralNode : public LiteralNode {
    public:
        NumericLiteralNode(int number): LiteralNode(number) { }
        virtual Value evaluate() const { return Value(new NumericValue(index())); }
        virtual void prefixDisplay(ostream& out) const {
            out << index();
        }
    };
    
    class MessageNode : public LiteralNode {
    public:
        MessageNode(int index): LiteralNode(index) { }
        virtual Value evaluate() const { return Value(new MessageValue(index())); }
        virtual void prefixDisplay(ostream& out) const {
            out << "'" << Universe::instance().Vocabulary.get(index()) << "'";
        }
    };
    
    class TextLiteralNode : public LiteralNode {
    public:
        TextLiteralNode(int index): LiteralNode{index} { }
        virtual Value evaluate() const {
            return Value(new StringValue(Universe::instance().TextLiterals.get(index())));
        }
        virtual void prefixDisplay(ostream& out) const {
            out << '"' << Universe::instance().TextLiterals.get(index()) << '"';
        }
    };
    
    class QuoteLiteralNode : public LiteralNode {
    public:
        QuoteLiteralNode(int index): LiteralNode{index} { }
        virtual Value evaluate() const {
            return Value(new StringValue(Universe::instance().TextLiterals.get(index())));
        }
        virtual void prefixDisplay(ostream& out) const {
            out << "<<" << Universe::instance().TextLiterals.get(index()) << ">>";
        }
    };
    
    class IdentifierNode : public ScalarNode {
        int id_;
    public:
        IdentifierNode(int id): id_{id} { }
        virtual Value evaluate() const {
            ObjectPtr selfObject = Universe::instance().currentContext().selfObject;
            if (selfObject and selfObject->hasAttribute(id_)) {
                return Value(new AttributeValue(selfObject->id(), id_));
            } else {
                return Value(new IdentifierValue(id_));
            }
        }
        virtual void prefixDisplay(ostream& out) const {
            out << Universe::instance().Identifiers.get(id_);
        }
    };
    
    Expression get_scalar(TokenStream& t) {
        Expression scalar;
        switch (t.token().type()) {
            case Token::MESSAGE:
                scalar.reset(new MessageNode(t.token().number()));
                break;
            case Token::TEXT_LITERAL:
                scalar.reset(new TextLiteralNode(t.token().number()));
                break;
            case Token::QUOTE_LITERAL:
                scalar.reset(new QuoteLiteralNode(t.token().number()));
                break;
            case Token::NUMERIC:
                scalar.reset(new NumericLiteralNode(t.token().number()));
                break;
            case Token::IDENTIFIER:
                scalar.reset(new IdentifierNode(t.token().number()));
                break;
            case Token::RESERVED_WORD: {
                Keywords::Reserved_e word = Keywords::Reserved_e(t.token().number());
                switch (word) {
                    case Keywords::RW_NULL:
                    case Keywords::RW_UNDEFINED:
                    case Keywords::RW_ABSENT:
                    case Keywords::RW_EACH:
                    case Keywords::RW_SELF: case Keywords::RW_SENDER: case Keywords::RW_MESSAGE:
                    case Keywords::RW_READ: case Keywords::RW_KEY:
                    case Keywords::RW_TRUE: case Keywords::RW_FALSE:
                        scalar.reset(new ReservedConstantNode(word));
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
                switch (op_name) {
                    case Keywords::OP_PLUS:
                    case Keywords::OP_NUMERIC:
                    case Keywords::OP_CONCAT:
                        // Special cases, operators that can be unary in this context
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
    }  /* tie_on_rside */

    Expression form_expr(TokenStream& t, int stop_precedence) {
        Expression expr = get_operand(t);
        if (not expr) return nullptr;
        while (t.fetch()) {
            if ((t.token().type() != Token::OPERATOR) or
                (not is_binary(Keywords::Operators_e(t.token().number())))) {
                if (t.token() != Token(Token::PUNCTUATION, ')') and stop_precedence == 0) {
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
        Expression t = expr->anyFewerNodeEquivalent();
        return move(t != nullptr ? t : expr);
    }
    
    Expression make_expr(TokenStream& t) {
        t.considerNewline();
        Expression expr = tighten(form_expr(t));
        t.restoreNewlineSignificance();
        // TODO:  will also verify the expression
        return expr;
    }
    
} // archetype