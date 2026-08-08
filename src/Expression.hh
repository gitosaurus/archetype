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

#include "Formatting.hh"
#include "Keywords.hh"
#include "TokenStream.hh"
#include "Value.hh"
#include "Serialization.hh"

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

        virtual bool bindsBefore(Keywords::Operators_e /*op*/) const { return true; }
        virtual void tieOnRightSide(Keywords::Operators_e /*op*/, Expression /*rightSide*/) { }

        virtual Expression anyFewerNodeEquivalent() { return nullptr; }
        virtual int nodeCount() const { return 1; }
        virtual bool verify(TokenStream& /*t*/) const { return true; }

        virtual void prefixDisplay(std::ostream& out) const = 0;
        virtual Value evaluate() const = 0;

        // True where this expression can be rendered as a value without either
        // changing anything or misrepresenting itself.  An attribute declared in
        // source holds the expression it was written as, not a value, and is
        // evaluated afresh on every access -- so anything that renders one
        // without the game having asked, --inspect above all, has to know which
        // it may ask.
        //
        // No operator may be asked: '->' sends a message, and a method may do
        // anything at all.  Nor may every leaf, which is the part that
        // surprises: 'read' and 'key' take a line and a keystroke from the
        // player, and 'sender', 'message' and 'each' want a dispatch context
        // that an inspection is standing outside of.
        //
        // The bar is deliberately lower than a save file's.  An .acx has to
        // round-trip a universe exactly; the RDF is a description of one, and is
        // meant to be the lesser thing.  Where an expression cannot survive the
        // trip as a value, the honest rendering is none at all -- serializing
        // the expression instead would end at serializing statements, which is
        // a different and much larger idea than describing a world.
        virtual bool isMaterializable() const { return false; }
    };

    class ValueExpression : public IExpression {
        Value value_;
    public:
        ValueExpression(Value value): value_(std::move(value)) { }
        virtual void write(Storage& out) const override;
        virtual Value evaluate() const override { return value_->clone(); }
        virtual void prefixDisplay(std::ostream& out) const override { out << value_; }
        // Already a value; evaluating it only hands back a copy.
        virtual bool isMaterializable() const override { return true; }
    };

    bool is_binary(Keywords::Operators_e op);
    bool is_right_associative(Keywords::Operators_e op);
    int precedence(Keywords::Operators_e op);

    Expression get_operand(TokenStream& t);
    Expression form_expr(TokenStream& t, int stop_precedence = 0);
    Expression tighten(Expression expr);

    bool eval_compare(Keywords::Operators_e op, const Value& lv, const Value& rv);

    [[nodiscard]] Expression make_expr(TokenStream& t);
    [[nodiscard]] Expression make_expr_from_str(std::string src_str);

    Storage& operator<<(Storage& out, const Expression& expr);
    Storage& operator>>(Storage& in, Expression& expr);
}

// Expressions render in the prefix notation used throughout the diagnostics.
// Both spellings are needed: diagnostics hold an Expression in some places and
// take a reference to the interface in others.
template <>
struct std::formatter<archetype::IExpression> : archetype::StreamedFormatter {
    auto format(const archetype::IExpression& expr, std::format_context& ctx) const {
        return render([&](std::ostream& out) { expr.prefixDisplay(out); }, ctx);
    }
};

template <>
struct std::formatter<archetype::Expression> : archetype::StreamedFormatter {
    auto format(const archetype::Expression& expr, std::format_context& ctx) const {
        return render([&](std::ostream& out) {
            if (expr) {
                expr->prefixDisplay(out);
            } else {
                out << "nullptr";
            }
        }, ctx);
    }
};

#endif /* defined(__archetype__Expression__) */
