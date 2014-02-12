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

namespace archetype {
    
    class IExpression {
    };
    
    typedef std::shared_ptr<IExpression> Expression;

    Expression load_expression(std::istream& in);
    void dump_expression(std::ostream& out, const Expression& expr);
}

#endif /* defined(__archetype__Expression__) */
