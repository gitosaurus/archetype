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
#include <list>

#include "TokenStream.h"
#include "Expression.h"

namespace archetype {
    
    class IStatement {
    };
    
    typedef std::shared_ptr<IStatement> Statement;
    
    class CompoundStatement : public IStatement {
        std::list<Statement> statements_;
    };
    
    class ExpressionStatement : public IStatement {
        Expression expression_;
    public:
        ExpressionStatement(Expression expr): expression_(expr) { }
    };
    
    class IfStatement : public IStatement {
        Expression condition_;
        Statement thenBranch_;
        Statement elseBranch_;
    public:
        IfStatement(Expression condition, Statement then_branch, Statement else_branch):
        condition_(condition),
        thenBranch_(then_branch),
        elseBranch_(else_branch)
        { }
    };
    
    class CaseStatement : public IStatement {
    public:
        struct Case {
            Expression value;
            Statement action;
        };
    private:
        Expression test_;
        std::list<Case> cases_;
    };
    
    class CreateStatement : public IStatement {
        int typeId_;
        Expression target_;
    public:
        CreateStatement(int type, Expression target): typeId_(type), target_(target) { }
    };
    
    class DestroyStatement : public IStatement {
        Expression victim_;
    public:
        DestroyStatement(Expression victim): victim_(victim) { }
    };
    
    class ForStatement : public IStatement {
        Expression selection_;
        Statement action_;
    public:
        ForStatement(Expression selection, Statement action): selection_(selection), action_(action) { }
    };
    
    class WhileStatement : public IStatement {
        Expression condition_;
        Statement action_;
    public:
        WhileStatement(Expression condition, Statement action): condition_(condition), action_(action) { }
    };
    
    class OutputStatement : public IStatement {
    protected:
        std::list<Expression> expressions_;
        OutputStatement(std::list<Expression> expressions): expressions_(expressions) { }
    };
    
    class WriteStatement : public OutputStatement {
    public:
        WriteStatement(std::list<Expression> expressions): OutputStatement(expressions) { }
    };

    class WritesStatement : public OutputStatement {
    public:
        WritesStatement(std::list<Expression> expressions): OutputStatement(expressions) { }
    };
    
    class StopStatement : public OutputStatement {
    public:
        StopStatement(std::list<Expression> expressions): OutputStatement(expressions) { }
    };
    
    Statement make_statement(TokenStream& t);
}

#endif /* defined(__archetype__Statement__) */
