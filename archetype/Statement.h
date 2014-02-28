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
        virtual ~CompoundStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    class ExpressionStatement : public IStatement {
        Expression expression_;
    public:
        virtual ~ExpressionStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    class IfStatement : public IStatement {
        Expression condition_;
        Statement thenBranch_;
        Statement elseBranch_;
    public:
        virtual ~IfStatement() { }
        virtual bool make(TokenStream& t);
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
    public:
        virtual ~CaseStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    class CreateStatement : public IStatement {
        int typeId_;
        Expression target_;
    public:
        virtual ~CreateStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    class DestroyStatement : public IStatement {
        Expression victim_;
    public:
        virtual ~DestroyStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    class ForStatement : public IStatement {
        Expression selection_;
        Statement action_;
    public:
        virtual ~ForStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    class WhileStatement : public IStatement {
        Expression condition_;
        Statement action_;
    public:
        virtual ~WhileStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    class BreakStatement : public IStatement {
    public:
        virtual ~BreakStatement() { }
        virtual bool make(TokenStream& t) { return true; }
    };
    
    class OutputStatement : public IStatement {
        Keywords::Reserved_e writeType_;
        std::list<Expression> expressions_;
        OutputStatement(std::list<Expression> expressions): expressions_(expressions) { }
    public:
        OutputStatement(Keywords::Reserved_e write_type): writeType_(write_type) { }
        virtual ~OutputStatement() { }
        virtual bool make(TokenStream& t);
    };
    
    Statement make_statement(TokenStream& t);
}

#endif /* defined(__archetype__Statement__) */
