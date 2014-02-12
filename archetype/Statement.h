//
//  Statement.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Statement__
#define __archetype__Statement__

#include <memory>
#include <deque>

namespace archetype {
    class IStatement {
        
    };
    
    typedef std::shared_ptr<IStatement> Statement;
    
    class CompoundStatement {
        std::deque<Statement> statements_;
    };
}

#endif /* defined(__archetype__Statement__) */
