//
//  Expression.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Expression__
#define __archetype__Expression__

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "Keywords.h"
#include "TokenStream.h"
#include "Value.h"
#include "Serialization.h"

namespace archetype {

    class IExpression;
    typedef std::unique_ptr<IExpression> Expression;

    class IExpression {
    protected:
        IExpression() { }
    public:
        static bool Debug;

        IExpression(const IExpression&) = delete;
        IExpression& operator=(const IExpression&) = delete;
        virtual ~IExpression() { }

        virtual void write(Storage& out) const = 0;

        virtual bool bindsBefore(Keywords::Operators_e op) const { return true; }
        virtual void tieOnRightSide(Keywords::Operators_e op, Expression rightSide) { }

        virtual Expression anyFewerNodeEquivalent() { return nullptr; }
        virtual int nodeCount() const { return 1; }
        virtual bool verify(TokenStream& t) const { return true; }

        virtual void prefixDisplay(std::ostream& out) const = 0;
        virtual Value evaluate() const = 0;
    };

    class ValueExpression : public IExpression {
        Value value_;
    public:
        ValueExpression(Value value): value_(std::move(value)) { }
        virtual void write(Storage& out) const override;
        virtual Value evaluate() const override { return value_->clone(); }
        virtual void prefixDisplay(std::ostream& out) const override { out << value_; }
    };

    bool is_binary(Keywords::Operators_e op);
    bool is_right_associative(Keywords::Operators_e op);
    int precedence(Keywords::Operators_e op);

    Expression get_operand(TokenStream& t);
    Expression form_expr(TokenStream& t, int stop_precedence = 0);
    Expression tighten(Expression expr);

    bool eval_compare(Keywords::Operators_e op, const Value& lv, const Value& rv);

    Expression make_expr(TokenStream& t);
    Expression make_expr_from_str(std::string src_str);

    Storage& operator<<(Storage& out, const Expression& expr);
    Storage& operator>>(Storage& in, Expression& expr);
}

#endif /* defined(__archetype__Expression__) */
