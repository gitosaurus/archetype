//
//  Expression.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <stdexcept>
#include <cassert>

#include "Expression.h"
#include "Keywords.h"
#include "GameDefinition.h"

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
        void setOp(Keywords::Operators_e op) { op_ = op; }
        
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
        UnaryOperator(Keywords::Operators_e op): Operator(op) { assert(not is_binary(op)); }
 
        virtual Expression right() const     { return right_; }
        virtual void setRight(Expression r)  { right_ = r; };
        
        virtual int nodeCount() const { return 1 + right_->nodeCount(); }
        virtual Expression anyFewerNodeEquivalent() {
            right_ = tighten(right_);
            return op() != Keywords::OP_LPAREN ? nullptr : right();
        }
        
        virtual void prettyPrint(std::ostream& out, std::string indent) const {
            if (op() != Keywords::OP_LPAREN) {
                out << indent << Keywords::instance().Operators.get(int(op())) << endl;
            }
            right()->prettyPrint(out, indent + " ");
        }
        virtual void prefixDisplay(ostream& out) const {
            if (op() == Keywords::OP_LPAREN) {
                right()->prefixDisplay(out);
            } else {
                out << '(';
                out << Keywords::instance().Operators.get(int(op())) << ' ';
                right()->prefixDisplay(out);
                out << ')';
            }
        }
    };
    
    class BinaryOperator : public Operator {
        Expression left_;
        Expression right_;
    public:
        BinaryOperator(Expression left, Keywords::Operators_e op, Expression right):
        Operator(op),
        left_(left),
        right_(right)
        {
            assert(is_binary(op));
        }
        
        virtual Expression right() const        { return right_; }
        
        virtual void setRight(Expression right) { right_ = right; }
        
        virtual int nodeCount() const { return 1 + left_->nodeCount() + right_->nodeCount(); }
        virtual Expression anyFewerNodeEquivalent() {
            left_ = tighten(left_);
            right_ = tighten(right_);
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
        virtual void prefixDisplay(ostream& out) const {
            out << index();
        }
    };
    
    class MessageNode : public LiteralNode {
    public:
        MessageNode(int index): LiteralNode(index) { }
        virtual void prefixDisplay(ostream& out) const {
            out << "'" << GameDefinition::instance().Vocabulary.get(index()) << "'";
        }
    };
    
    class TextLiteralNode : public LiteralNode {
    public:
        TextLiteralNode(int index): LiteralNode(index) { }
        virtual void prefixDisplay(ostream& out) const {
            out << '"' << GameDefinition::instance().TextLiterals.get(index()) << '"';
        }
    };
    
    class QuoteLiteralNode : public LiteralNode {
    public:
        QuoteLiteralNode(int index): LiteralNode(index) { }
        virtual void prefixDisplay(ostream& out) const {
            out << "<<" << GameDefinition::instance().TextLiterals.get(index()) << ">>";
        }
    };
    
    class IdentifierNode : public ScalarNode {
        int id_;
    public:
        IdentifierNode(int id): id_(id) { }
        virtual void prefixDisplay(ostream& out) const {
            out << GameDefinition::instance().Identifiers.get(id_);
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
    
    Expression get_operand(TokenStream& t) {
        unique_ptr<UnaryOperator> the_operand(new UnaryOperator(Keywords::OP_LPAREN));
        bool more = t.fetch();
        while (more and (t.token().type() == Token::NEWLINE))
            more = t.fetch();
        if (not more) {
            t.didNotConsume();
            return Expression();
        }
        
        switch (t.token().type()) {
            case Token::PUNCTUATION:
                switch (t.token().number()) {
                    case ')':
                        t.didNotConsume();
                        return Expression();
                        break;
                    case '(':
                        the_operand->setRight(form_expr(t));
                        if (not the_operand->right()) {
                            return Expression();
                        }
                        break;
                    default:
                        return Expression();
                }  /* switch t.token().number() */
                break; // PUNCTUATION
            case Token::OPERATOR: {
                Keywords::Operators_e op_name = Keywords::Operators_e(t.token().number());
                switch (op_name) {
                    case Keywords::OP_PLUS:
                    case Keywords::OP_NUMERIC:
                    case Keywords::OP_CONCAT:
                        the_operand->setOp(op_name);
                        break;
                    default:
                        if (not is_binary(op_name))
                            the_operand->setOp(op_name);
                        else {
                            t.expectGeneral("unary operator");
                            t.stopLooking();
                            the_operand.reset();
                        }
                        break;
                }  /* switch t.token().number() */
                if (the_operand.get() != nullptr) {
                    Expression r = form_expr(t, precedence(op_name));
                    if (r != nullptr) {
                        the_operand->setRight(r);
                    }
                }
                break;
            }
            default: { /* some constant or keyword */
                Expression scalar = get_scalar(t);
                if (scalar != nullptr) {
                    the_operand->setRight(scalar);
                } else {
                    the_operand.reset();
                }
            }                              /* some constant */
                break;
        }  /* switch t.token().type() */
        
        if (not the_operand.get()) {
            t.didNotConsume();
        }
        return Expression(the_operand.release());
    }

    Expression tie_on_rside(Expression existing,
                            Keywords::Operators_e op,
                            Expression new_rside) {
        if (existing->bindsBefore(op)) {
            Expression new_oper(new BinaryOperator(existing, op, new_rside));
            return new_oper;
        } else {
            existing->setRight(tie_on_rside(existing->right(), op, new_rside));
            return existing;
        }
    }  /* tie_on_rside */

    Expression form_expr(TokenStream& t, int stop_precedence) {
        bool done = false;
        Expression expr = get_operand(t);
        if (expr != nullptr) {
            do {
                if (not t.fetch()) {
                    done = true;
                } else if ((t.token().type() != Token::OPERATOR) or
                         (not is_binary(Keywords::Operators_e(t.token().number())))) {
                    if (not ((t.token().type() == Token::PUNCTUATION) and (t.token().number() == ')') and
                             (stop_precedence == 0)))
                        t.didNotConsume();
                    done = true;
                } else {
                    Keywords::Operators_e the_operator = Keywords::Operators_e(t.token().number());
                    if (precedence(the_operator) < stop_precedence) {
                        t.didNotConsume();
                        done = true;
                    } else {
                        Expression rside = get_operand(t);
                        if (rside != nullptr)
                            expr = tie_on_rside(expr, the_operator, rside);
                        else {
                            t.errorMessage("Empty expression or unbalanced parentheses");
                            expr.reset();
                            done = true;
                        }
                    }
                }
            } while (not done);
        }
        return expr;
    } // form_expr
    
    Expression tighten(Expression expr) {
        Expression t = expr->anyFewerNodeEquivalent();
        return t != nullptr ? t : expr;
    }
    
    Expression make_expr(TokenStream& t) {
        t.considerNewline();
        Expression expr = tighten(form_expr(t));
        t.restoreNewlineSignificance();
        // TODO:  will also verify the expression
        return expr;
    }
    
} // archetype