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
#include <vector>

#include "TokenStream.hh"
#include "Expression.hh"
#include "Serialization.hh"

namespace archetype {

    class IStatement {
    protected:
        IStatement() { }
    public:
        static bool Debug;

        IStatement(const IStatement&) = delete;
        IStatement& operator=(const IStatement&) = delete;
        virtual ~IStatement() { }

        virtual void read(Storage& in) = 0;
        virtual void write(Storage& out) const = 0;
        virtual bool make(TokenStream& t) = 0;
        virtual void display(std::ostream& out) const = 0;
        virtual Value execute() const = 0;
    };

    typedef std::unique_ptr<IStatement> Statement;

    Storage& operator<<(Storage& out, const Statement& stmt);
    Storage& operator>>(Storage& in, Statement& stmt);

    class CompoundStatement : public IStatement {
        std::list<Statement> statements_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;

        const std::list<Statement>& statements() const { return statements_; }
    };

    class ExpressionStatement : public IStatement {
        Expression expression_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;

        const Expression& expression() const { return expression_; }
    };

    class IfStatement : public IStatement {
        Expression condition_;
        Statement thenBranch_;
        Statement elseBranch_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;
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
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;

        const Expression& testExpression() const { return testExpression_; }
        const std::list<Case>& cases() const { return cases_; }
    };

    class CreateStatement : public IStatement {
        int typeId_;
        Expression target_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;
    };

    class DestroyStatement : public IStatement {
        Expression victim_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;
    };

    class ForStatement : public IStatement {
        Expression selection_;
        Statement action_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;
    };

    class WhileStatement : public IStatement {
        Expression condition_;
        Statement action_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;
    };

    class OutputStatement : public IStatement {
        Keywords::Reserved_e writeType_;
        std::list<Expression> expressions_;
    public:
        OutputStatement(Keywords::Reserved_e write_type = Keywords::RW_WRITE): writeType_(write_type) { }
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;
    };

    class ParagraphOutputStatement : public IStatement {
        std::vector<int> quoteLiterals_;
    public:
        virtual void read(Storage& in) override;
        virtual void write(Storage& out) const override;
        virtual bool make(TokenStream& t) override;
        virtual void display(std::ostream& out) const override;
        virtual Value execute() const override;
    };

    Statement make_statement(TokenStream& t);
    Statement make_stmt_from_str(std::string src_str);
}

#endif /* defined(__archetype__Statement__) */
