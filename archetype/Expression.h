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

namespace archetype {
    
    class IExpression;
    typedef std::shared_ptr<IExpression> Expression;
    
    class IExpression {
    public:
        virtual bool bindsBefore(Keywords::Operators_e op) const { return true; }
        virtual Expression left() const  { return nullptr; }
        virtual Expression right() const { return nullptr; }
        virtual void setRight(Expression rightSide) { throw std::logic_error("No right side to set"); }
        virtual void prettyPrint(std::ostream& out, std::string indent = "") const = 0;
        virtual void prefixDisplay(std::ostream& out) const = 0;
    };
    
    bool is_binary(Keywords::Operators_e op);
    bool is_right_associative(Keywords::Operators_e op);
    int precedence(Keywords::Operators_e op);
    
    Expression get_operand(TokenStream& t);
    Expression form_expr(TokenStream& t, int stop_precedence = 0);
}

#endif /* defined(__archetype__Expression__) */
