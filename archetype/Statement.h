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
    public:
        virtual ~IStatement() { }
        virtual bool make(TokenStream& t) = 0;
    };
    
    typedef std::shared_ptr<IStatement> Statement;
    
    class CompoundStatement : public IStatement {
        std::list<Statement> statements_;
    public:
        virtual bool make(TokenStream& t);
        
        const std::list<Statement> statements() const { return statements_; }
    };
    
    class ExpressionStatement : public IStatement {
        Expression expression_;
    public:
        virtual bool make(TokenStream& t);
        
        Expression expression() const { return expression_; }
    };
    
    class IfStatement : public IStatement {
        Expression condition_;
        Statement thenBranch_;
        Statement elseBranch_;
    public:
        virtual bool make(TokenStream& t);
    };
    
    class CaseStatement : public IStatement {
    public:
        struct Case {
            Expression value;
            Statement action;
        };
    private:
        Expression testExpression_;
        std::list<Case> cases_;
    public:
        virtual bool make(TokenStream& t);
        
        Expression testExpression() const { return testExpression_; }
        const std::list<Case> cases() const { return cases_; }
    };
    
    class CreateStatement : public IStatement {
        int typeId_;
        Expression target_;
    public:
        virtual bool make(TokenStream& t);
    };
    
    class DestroyStatement : public IStatement {
        Expression victim_;
    public:
        virtual bool make(TokenStream& t);
    };
    
    class ForStatement : public IStatement {
        Expression selection_;
        Statement action_;
    public:
        virtual bool make(TokenStream& t);
    };
    
    class WhileStatement : public IStatement {
        Expression condition_;
        Statement action_;
    public:
        virtual bool make(TokenStream& t);
    };
    
    class BreakStatement : public IStatement {
    public:
        virtual bool make(TokenStream& t) { return true; }
    };
    
    class OutputStatement : public IStatement {
        Keywords::Reserved_e writeType_;
        std::list<Expression> expressions_;
    public:
        OutputStatement(Keywords::Reserved_e write_type): writeType_(write_type) { }
        virtual bool make(TokenStream& t);
    };
    
    Statement make_statement(TokenStream& t);
}

#endif /* defined(__archetype__Statement__) */
