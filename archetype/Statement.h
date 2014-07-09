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
        enum Type_e {
            COMPOUND,
            EXPRESSION,
            IF,
            CASE,
            CREATE,
            DESTROY,
            FOR,
            WHILE,
            OUTPUT
        };
        IStatement(const IStatement&) = delete;
        IStatement& operator=(const IStatement&) = delete;
        virtual ~IStatement() { }
        
        virtual Type_e type() const = 0;
        virtual void read(Storage& in) = 0;
        virtual void write(Storage& out) const = 0;
        virtual bool make(TokenStream& t) = 0;
        virtual void display(std::ostream& out) const = 0;
        virtual Value execute(std::ostream& out) const = 0;
    };
    
    typedef std::unique_ptr<IStatement> Statement;
    
    Storage& operator<<(Storage& out, const Statement& stmt);
    Storage& operator>>(Storage& in, Statement& stmt);
    
    class CompoundStatement : public IStatement {
        std::list<Statement> statements_;
    public:
        virtual Type_e type() const override { return COMPOUND; }
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
        
        const std::list<Statement>& statements() const { return statements_; }
    };
    
    class ExpressionStatement : public IStatement {
        Expression expression_;
    public:
        virtual Type_e type() const override { return EXPRESSION; }
        virtual void read(Storage& in) override { in >> expression_; }
        virtual void write(Storage& out) const override { out << expression_; }
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;

        const Expression& expression() const { return expression_; }
    };
    
    class IfStatement : public IStatement {
        Expression condition_;
        Statement thenBranch_;
        Statement elseBranch_;
    public:
        virtual Type_e type() const override { return IF; }
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
    };
    
    class CaseStatement : public IStatement {
    public:
        struct Case {
            Expression match;
            Statement action;
        };
    private:
        Expression testExpression_;
        std::list<Case> cases_;
        Statement defaultCase_;
    public:
        virtual Type_e type() const override { return CASE; }
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
        
        const Expression& testExpression() const { return testExpression_; }
        const std::list<Case>& cases() const { return cases_; }
    };
    
    class CreateStatement : public IStatement {
        int typeId_;
        Expression target_;
    public:
        virtual Type_e type() const override { return CREATE; }
        virtual void read(Storage& in) override { in >> typeId_ >> target_; }
        virtual void write(Storage& out) const override { out << typeId_ << target_; }
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
    };
    
    class DestroyStatement : public IStatement {
        Expression victim_;
    public:
        virtual Type_e type() const override { return DESTROY; }
        virtual void read(Storage& in) override { in >> victim_; }
        virtual void write(Storage& out) const override { out << victim_; }
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
    };
    
    class ForStatement : public IStatement {
        Expression selection_;
        Statement action_;
    public:
        virtual Type_e type() const override { return FOR; }
        virtual void read(Storage& in) override { in >> selection_ >> action_; }
        virtual void write(Storage& out) const override { out << selection_ << action_; }
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
    };
    
    class WhileStatement : public IStatement {
        Expression condition_;
        Statement action_;
    public:
        virtual Type_e type() const override { return WHILE; }
        virtual void read(Storage& in) override { in >> condition_ >> action_; }
        virtual void write(Storage& out) const override { out << condition_ << action_; }
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
    };
    
    class OutputStatement : public IStatement {
        Keywords::Reserved_e writeType_;
        std::list<Expression> expressions_;
    public:
        OutputStatement(Keywords::Reserved_e write_type = Keywords::RW_WRITE): writeType_(write_type) { }
        virtual Type_e type() const override { return OUTPUT; }
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute(std::ostream& out) const override;
    };
    
    Statement make_statement(TokenStream& t);
}

#endif /* defined(__archetype__Statement__) */
