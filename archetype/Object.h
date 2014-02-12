//
//  Object.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Object__
#define __archetype__Object__

#include <map>

#include "Expression.h"
#include "Statement.h"

namespace archetype {
    class Object {
        int parent_;
        std::map<int, Expression> attributes_;
        std::map<int, Statement> methods_;
    };
}

#endif /* defined(__archetype__Object__) */
