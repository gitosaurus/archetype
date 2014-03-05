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
    
    Expression get_operand_node(TokenStream& t) {
        switch (t.token().type()) {
            case Token::PUNCTUATION:
                switch (t.token().number()) {
                    case ')':
                        return nullptr;
                    case '(':
                        if (Expression expr = form_expr(t)) {
                            Expression result(new UnaryOperator(Keywords::OP_LPAREN));
                            result->setRight(expr);
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
                Expression the_operand(new UnaryOperator(op_name));
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
                    the_operand->setRight(r);
                    return the_operand;
                } else {
                    return nullptr;
                }
            }
            default: { /* some constant or keyword */
                if (Expression scalar = get_scalar(t)) {
                    Expression result(new UnaryOperator(Keywords::OP_LPAREN));
                    result->setRight(scalar);
                    return result;
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
            return Expression(new BinaryOperator(existing, op, new_rside));
        } else {
            existing->setRight(tie_on_rside(existing->right(), op, new_rside));
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
                    expr = tie_on_rside(expr, the_operator, rside);
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