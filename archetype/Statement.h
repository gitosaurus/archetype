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
#include "Serialization.h"

namespace archetype {
    
    class IStatement {
    protected:
        IStatement() { }
    public:
        IStatement(const IStatement&) = delete;
        IStatement& operator=(const IStatement&) = delete;
        virtual ~IStatement() { }
        
        virtual bool make(TokenStream& t) = 0;
        virtual Value execute(std::ostream& out) const;
    };
    
    typedef std::unique_ptr<IStatement> Statement;
    
    class CompoundStatement : public IStatement {
        std::list<Statement> statements_;
    public:
        virtual bool make(TokenStream& t) override;
        virtual Value execute(std::ostream& out) const override;
        
        const std::list<Statement>& statements() const { return statements_; }
    };
    
    class ExpressionStatement : public IStatement {
        Expression expression_;
    public:
        virtual bool make(TokenStream& t) override;
        virtual Value execute(std::ostream& out) const override;

        const Expression& expression() const { return expression_; }
    };
    
    class IfStatement : public IStatement {
        Expression condition_;
        Statement thenBranch_;
        Statement elseBranch_;
    public:
        virtual bool make(TokenStream& t) override;
        virtual Value execute(std::ostream& out) const override;
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
        Statement defaultCase_;
    public:
        virtual bool make(TokenStream& t) override;
        
        const Expression& testExpression() const { return testExpression_; }
        const std::list<Case>& cases() const { return cases_; }
    };
    
    class CreateStatement : public IStatement {
        int typeId_;
        Expression target_;
    public:
        virtual bool make(TokenStream& t) override;
    };
    
    class DestroyStatement : public IStatement {
        Expression victim_;
    public:
        virtual bool make(TokenStream& t) override;
    };
    
    class ForStatement : public IStatement {
        Expression selection_;
        Statement action_;
    public:
        virtual bool make(TokenStream& t) override;
    };
    
    class WhileStatement : public IStatement {
        Expression condition_;
        Statement action_;
    public:
        virtual bool make(TokenStream& t) override;
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
        virtual bool make(TokenStream& t) override;
        virtual Value execute(std::ostream& out) const override;
    };
    
    Statement make_statement(TokenStream& t);
    
    Storage& operator<<(Storage& out, const Statement& stmt);
    Storage& operator>>(Storage& in, Statement& stmt);
}

#endif /* defined(__archetype__Statement__) */
