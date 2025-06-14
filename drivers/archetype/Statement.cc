//
//  Statement.cc
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <memory>
#include <stdexcept>
#include <sstream>
#include <cctype>

#include "Statement.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {

    enum StatementType_e {
        COMPOUND,
        EXPRESSION,
        IF,
        CASE,
        CREATE,
        DESTROY,
        FOR,
        WHILE,
        OUTPUT,
        PARAGRAPH_OUTPUT
    };

    bool IStatement::Debug = false;

    void CompoundStatement::read(Storage& in) {
        int count;
        in >> count;
        statements_.clear();
        for (int i = 0; i < count; ++i) {
            Statement stmt;
            in >> stmt;
            statements_.push_back(std::move(stmt));
        }
    }

    void CompoundStatement::write(Storage& out) const {
        out << COMPOUND;
        int count = static_cast<int>(statements_.size());
        out << count;
        for (auto const& stmt : statements_) {
            out << stmt;
        }
    }

    bool CompoundStatement::make(TokenStream& t) {
        while (t.fetch()) {
            if (t.token() == Token(Token::PUNCTUATION, '}')) {
                return true;
            } else {
                t.didNotConsume();
                if (Statement s = make_statement(t)) {
                    statements_.push_back(std::move(s));
                } else {
                    t.errorMessage("Unfinished compound statement");
                    t.stopLooking();
                    return false;
                }
            }
        }
        return false;
    }

    void CompoundStatement::display(std::ostream &out) const {
        out << "{";
        for (auto stmt_p = statements_.begin(); stmt_p != statements_.end(); ++stmt_p) {
            if (stmt_p != statements_.begin()) {
                out << "; ";
            }
            (*stmt_p)->display(out);
        }
        out << "}";
    }

    Value CompoundStatement::execute() const {
        Value break_v{new BreakValue};
        Value result{new UndefinedValue};
        for (auto const& stmt : statements_) {
            result = stmt->execute();
            if (result->isSameValueAs(break_v)) {
                // Break statements stop a compound statement, but the result must be propagated up
                // to the containing loop, which consumes it.
                break;
            }
        }
        return result;
    }

    void ExpressionStatement::read(Storage& in) {
        in >> expression_;
    }

    void ExpressionStatement::write(Storage& out) const {
        out << EXPRESSION << expression_;
    }

    bool ExpressionStatement::make(TokenStream& t) {
        expression_ = make_expr(t);
        return expression_ != nullptr;
    }

    void ExpressionStatement::display(std::ostream &out) const {
        expression_->prefixDisplay(out);
    }

    Value ExpressionStatement::execute() const {
        if (auto val_expr = dynamic_cast<ValueExpression*>(expression_.get())) {
            Value val = val_expr->evaluate();
            if (dynamic_cast<MessageValue*>(val.get())) {
                return Object::pass(Universe::instance().currentContext().selfObject, std::move(val));
            }
        }
        return expression_->evaluate()->valueConversion();
    }

    void IfStatement::read(Storage& in) {
        in >> condition_ >> thenBranch_;
        int has_else;
        in >> has_else;
        if (has_else) {
            in >> elseBranch_;
        }
    }

    void IfStatement::write(Storage& out) const {
        out << IF;
        out << condition_ << thenBranch_;
        if (not elseBranch_) {
            out << 0;
        } else {
            out << 1 << elseBranch_;
        }
    }

    bool IfStatement::make(TokenStream& t) {
        /* BNF:  <if_stmt> := if (<expr>) <statement> [else <statement>] */
        if (not (condition_ = make_expr(t))) return false;
        if (not t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_THEN))) return false;
        if (not (thenBranch_ = make_statement(t))) return false;
        if (t.fetch() and t.token() == Token(Token::RESERVED_WORD, Keywords::RW_ELSE)) {
            if (not (elseBranch_ = make_statement(t))) return false;
        } else {
            t.didNotConsume();
        }
        return true;
    }

    void IfStatement::display(std::ostream &out) const {
        out << "if ";
        condition_->prefixDisplay(out);
        out << " then ";
        thenBranch_->display(out);
        if (elseBranch_) {
            out << " else ";
            elseBranch_->display(out);
        }
    }

    Value IfStatement::execute() const {
        Value condition_value = condition_->evaluate();
        bool true_enough = condition_value->isTrueEnough();
        if (IStatement::Debug) {
            ostringstream out;
            out << "if ";
            condition_->prefixDisplay(out);
            out << " => ";
            condition_value->display(out);
            if (true_enough) {
                out << "then-branch";
            } else {
                out << "else-branch";
            }
            Universe::instance().output()->put(out.str());
            Universe::instance().output()->endLine();
        }
        Value result;
        if (true_enough) {
            result = thenBranch_->execute();
        } else if (elseBranch_){
            result = elseBranch_->execute();
        } else {
            result = Value{new UndefinedValue};
        }
        if (IExpression::Debug) {
            ostringstream out;
            out << "if-result => ";
            result->display(out);
            Universe::instance().output()->put(out.str());
            Universe::instance().output()->endLine();
        }
        return result;
    }

    void CaseStatement::read(Storage& in) {
        in >> testExpression_;
        int entries;
        in >> entries;
        cases_.clear();
        for (int i = 0; i < entries; ++i) {
            Case case_pair;
            in >> case_pair.match >> case_pair.action;
            cases_.push_back(std::move(case_pair));
        }
        int default_exists;
        in >> default_exists;
        if (default_exists) {
            in >> defaultCase_;
        } else {
            defaultCase_.reset();
        }
    }

    void CaseStatement::write(Storage& out) const {
        out << CASE;
        out << testExpression_;
        int entries = static_cast<int>(cases_.size());
        out << entries;
        for (auto const& case_pair : cases_) {
            out << case_pair.match << case_pair.action;
        }
        if (not defaultCase_) {
            out << 0;
        } else {
            out << 1 << defaultCase_;
        }
    }

    bool CaseStatement::make(TokenStream& t) {
        /* BNF:  <case_stmt> := switch (<expr>) { (<expr> <statement>)+
         [default <statement>] } */
        if (not (testExpression_ = make_expr(t))) return false;
        if (not (t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_OF)) and
                 t.insistOn(Token(Token::PUNCTUATION, '{')))) {
            return false;
        }
        for (;;) {
            if (not t.fetch()) {
                t.hitEOF(Token(Token::PUNCTUATION, '}'));
                return false;
            }

            if (t.token() == Token(Token::PUNCTUATION, '}')) {
                break;
            }

            if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_DEFAULT)) {
                if (defaultCase_) {
                    t.errorMessage("There is already a default for this case");
                    return false;
                } else {
                    if (not t.insistOn(Token(Token::PUNCTUATION, ':'))) return false;
                    defaultCase_ = make_statement(t);
                    if (not defaultCase_) return false;
                }
            } else {
                t.didNotConsume();
                cases_.push_back(Case());
                cases_.back().match = make_expr(t);
                if (not cases_.back().match) return false;
                if (not t.insistOn(Token(Token::PUNCTUATION, ':'))) return false;
                cases_.back().action = make_statement(t);
                if (not cases_.back().action) return false;
            }
        }
        return true;
    }

    void CaseStatement::display(std::ostream &out) const {
        out << "case ";
        testExpression_->prefixDisplay(out);
        out << " of {";
        for (const auto& c : cases_) {
            c.match->prefixDisplay(out);
            out << " : ";
            c.action->display(out);
            out << "; ";
        }
        if (defaultCase_) {
            out << "default : ";
            defaultCase_->display(out);
        }
        out << "}";
    }

    Value CaseStatement::execute() const {
        Value test_value = testExpression_->evaluate()->valueConversion();
        for (auto const& case_pair : cases_) {
            Value case_value = case_pair.match->evaluate()->valueConversion();
            if (eval_compare(Keywords::OP_EQ, test_value, case_value)) {
                if (IStatement::Debug) {
                    ostringstream out;
                    test_value->display(out);
                    out << " matched case ";
                    case_value->display(out);
                    Universe::instance().output()->put(out.str());
                    Universe::instance().output()->endLine();
                }
                return case_pair.action->execute();
            }
        }
        if (defaultCase_) {
            if (IStatement::Debug) {
                ostringstream out;
                out << "default case; nothing matched ";
                test_value->display(out);
                Universe::instance().output()->put(out.str());
                Universe::instance().output()->endLine();
            }
            return defaultCase_->execute();
        }
        return Value{new UndefinedValue};
    }

    void CreateStatement::read(Storage& in) {
        in >> typeId_ >> target_;
    }

    void CreateStatement::write(Storage& out) const {
        out << CREATE << typeId_ << target_;
    }

    bool CreateStatement::make(TokenStream& t) {
        if (not t.fetch()) {
            t.hitEOF(Token(Token::IDENTIFIER, -1));
            return false;
        }

        typeId_ = 0;
        if (t.token().type() != Token::IDENTIFIER) {
            t.expectGeneral("type identifier");
            return false;
        }

        int identifier = t.token().number();
        auto where = Universe::instance().ObjectIdentifiers.find(identifier);
        if (where == Universe::instance().ObjectIdentifiers.end()) {
            t.expectGeneral("defined type identifier");
            return false;
        }
        ObjectPtr typeObject = Universe::instance().getObject(where->second);
        if (not (typeObject and typeObject->isPrototype())) {
            t.expectGeneral("identifier of defined type, not instance");
            return false;
        }
        typeId_ = typeObject->id();
        return t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_NAMED)) and (target_ = make_expr(t));
    }

    void CreateStatement::display(std::ostream &out) const {
        out << "create <something> named ";
        target_->prefixDisplay(out);
    }

    Value CreateStatement::execute() const {
        ObjectPtr object{Universe::instance().defineNewObject(typeId_)};
        Value object_v{new ObjectValue{object->id()}};
        Value result{object_v->clone()};
        Value target{target_->evaluate()->attributeConversion()};
        target->assign(std::move(object_v));
        if (IStatement::Debug) {
            ostringstream out;
            out << "created new instance ";
            result->display(out);
            out << " assigned to ";
            target->display(out);
            Universe::instance().output()->put(out.str());
            Universe::instance().output()->endLine();
        }
        // Intentionally return a clone of the object value, not the target, in order not to
        // leak an l-value out to the caller.
        return result;
    }

    void DestroyStatement::read(Storage& in) {
        in >> victim_;
    }

    void DestroyStatement::write(Storage& out) const {
        out << DESTROY << victim_;
    }

    bool DestroyStatement::make(TokenStream& t) {
        return (victim_ = make_expr(t)) != nullptr;
    }

    void DestroyStatement::display(std::ostream &out) const {
        victim_->prefixDisplay(out);
    }

    Value DestroyStatement::execute() const {
        Value victim_v{victim_->evaluate()->objectConversion()};
        if (victim_v->isDefined()) {
            if (IStatement::Debug) {
                ostringstream out;
                out << "destroyed ";
                victim_v->display(out);
                Universe::instance().output()->put(out.str());
                Universe::instance().output()->endLine();
            }
            Universe::instance().destroyObject(victim_v->getObject());
        }
        return Value{new UndefinedValue};
    }

    void OutputStatement::read(Storage& in) {
        int write_type_as_int;
        in >> write_type_as_int;
        writeType_ = static_cast<Keywords::Reserved_e>(write_type_as_int);
        int entries;
        in >> entries;
        expressions_.clear();
        for (int i = 0; i < entries; ++i) {
            Expression expr;
            in >> expr;
            expressions_.push_back(std::move(expr));
        }
    }

    void OutputStatement::write(Storage& out) const {
        out << OUTPUT;
        int write_type_as_int = static_cast<int>(writeType_);
        out << write_type_as_int;
        int entries = static_cast<int>(expressions_.size());
        out << entries;
        for (auto const& expr : expressions_) {
            out << expr;
        }
    }

    bool OutputStatement::make(TokenStream& t) {
        /* If the token immediately following the write statement is NEWLINE, then
         the write was intended to be a null write - that is, no message, only
         the action. */
        t.considerNewline();
        bool found_newline = t.fetch() and (t.token().type() == Token::NEWLINE);
        t.restoreNewlineSignificance();
        if (found_newline) return true;

        t.didNotConsume();
        Expression the_expr = make_expr(t);
        if (not the_expr) return false;
        expressions_.push_back(std::move(the_expr));
        while (t.fetch()) {
            if (t.token() != Token(Token::PUNCTUATION, ',')) {
                t.didNotConsume();
                break;
            }
            the_expr = make_expr(t);
            if (not the_expr) return false;
            expressions_.push_back(std::move(the_expr));
        }
        return true;
    }

    void OutputStatement::display(std::ostream& out) const {
        out << Keywords::instance().Reserved.get(writeType_);
        if (not expressions_.empty()) {
            out << ' ';
            for (auto expr_p = expressions_.begin(); expr_p != expressions_.end(); ++expr_p) {
                if (expr_p != expressions_.begin()) {
                    out << ", ";
                }
                (*expr_p)->prefixDisplay(out);
            }
        }
    }

    Value OutputStatement::execute() const {
        Value last_value(new UndefinedValue);
        for (auto const& expr : expressions_) {
            last_value = expr->evaluate();
            if (writeType_ == Keywords::RW_DISPLAY) {
                ostringstream out;
                out << '[';
                last_value->display(out);
                out << ']';
                Universe::instance().output()->put(out.str());
            }
            Value v_s = last_value->stringConversion();
            if (v_s->isDefined()) {
                Universe::instance().output()->put(v_s->getString());
            }
        }
        if (writeType_ != Keywords::RW_WRITES) {
            Universe::instance().output()->endLine();
        }
        if (writeType_ == Keywords::RW_STOP) {
            throw QuitGame();
        }
        return last_value;
    }

    void ParagraphOutputStatement::read(Storage& in) {
        int entries;
        in >> entries;
        vector<int> v(entries);
        for (int i = 0; i < entries; ++i) {
            int q;
            in >> q;
            v[i] = q;
        }
        quoteLiterals_.swap(v);
    }

    void ParagraphOutputStatement::write(Storage& out) const {
        out << PARAGRAPH_OUTPUT;
        int entries = static_cast<int>(quoteLiterals_.size());
        out << entries;
        for (int i = 0; i < entries; ++i) {
            int q = quoteLiterals_[i];
            out << q;
        }
    }

    bool ParagraphOutputStatement::make(TokenStream& t) {
        quoteLiterals_.clear();
        while (t.fetch()) {
            if (t.token().type() != Token::QUOTE_LITERAL) {
                t.didNotConsume();
                break;
            }
            quoteLiterals_.push_back(t.token().number());
        }
        vector<int>(quoteLiterals_).swap(quoteLiterals_);
        return true;
    }

    void ParagraphOutputStatement::display(std::ostream& out) const {
        for (int q : quoteLiterals_) {
            out << ">>" << Universe::instance().TextLiterals.get(q) << endl;
        }
    }

    Value ParagraphOutputStatement::execute() const {
        string prev;
        string line;
        bool in_paragraph = false;
        for (int q : quoteLiterals_) {
            line = Universe::instance().TextLiterals.get(q);
            bool was_in_paragraph = in_paragraph;
            in_paragraph = line.size() > 0 and not isspace(*(line.begin()));
            if (was_in_paragraph) {
                if (in_paragraph) {
                    // Prevent a trimmed end-of-wiki line from being glued closely onto a next line.
                    auto final = *(prev.rbegin());
                    switch (final) {
                        case '.': case ':': case '!': case '?':
                            Universe::instance().output()->put("  ");
                            break;
                        default:
                            if (not isspace(final)) {
                                Universe::instance().output()->put(" ");
                            }
                            break;
                    } // switch
                } else {
                    Universe::instance().output()->endLine();
                }
            }
            Universe::instance().output()->put(line);
            if (not in_paragraph) {
                Universe::instance().output()->endLine();
            }
            prev = line;
        }
        // This is the end of the series of quote-lines.  Close out any dangling paragraph.
        if (in_paragraph) {
            Universe::instance().output()->endLine();
        }
        return Value{new StringValue{line}};
    }

    void ForStatement::read(Storage& in) {
        in >> selection_ >> action_;
    }

    void ForStatement::write(Storage& out) const {
        out << FOR << selection_ << action_;
    }

    bool ForStatement::make(TokenStream& t) {
        if (not (selection_ = make_expr(t))) return false;
        if (not t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_DO))) return false;
        action_ = make_statement(t);
        return action_ != nullptr;
    }

    void ForStatement::display(std::ostream &out) const {
        out << "for ";
        selection_->prefixDisplay(out);
        out << " do ";
        action_->display(out);
    }

    Value ForStatement::execute() const {
        Value break_v{new BreakValue};
        Value result{new UndefinedValue};
        int object_count = Universe::instance().objectCount();
        for (int object_id = Universe::UserObjectsBeginAt; object_id < object_count; ++object_id) {
            ObjectPtr each_object = Universe::instance().getObject(object_id);
            if (not each_object or each_object->id() == Object::INVALID or each_object->isPrototype()) {
                continue;
            }
            ContextScope c;
            c->eachObject = each_object;
            Value selectionValue = selection_->evaluate();
            if (selectionValue->isTrueEnough()) {
                result = action_->execute();
                if (IStatement::Debug) {
                    ostringstream out;
                    out << "for each = ";
                    ObjectValue each_value{object_id};
                    each_value.display(out);
                    out << "; ";
                    selection_->prefixDisplay(out);
                    out << " => ";
                    selectionValue->display(out);
                    Universe::instance().output()->put(out.str());
                    Universe::instance().output()->endLine();
                }
                if (result->isSameValueAs(break_v)) {
                    if (IExpression::Debug) {
                        Universe::instance().output()->put("break for");
                        Universe::instance().output()->endLine();
                    }
                    // The for-loop "consumes" the break so it doesn't keep breaking outer loops
                    result.reset(new UndefinedValue);
                    break;
                }
            }
        }
        if (IStatement::Debug) {
            ostringstream out;
            out << "for-result => ";
            result->display(out);
            Universe::instance().output()->put(out.str());
            Universe::instance().output()->endLine();
        }
        return result;
    }

    void WhileStatement::read(Storage &in) {
        in >> condition_ >> action_;
    }

    void WhileStatement::write(Storage& out) const {
        out << WHILE << condition_ << action_;
    }

    bool WhileStatement::make(TokenStream& t) {
        if (not (condition_ = make_expr(t))) return false;
        if (not t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_DO))) return false;
        action_ = make_statement(t);
        return action_ != nullptr;
    }

    void WhileStatement::display(std::ostream &out) const {
        out << "while ";
        condition_->prefixDisplay(out);
        out << " do ";
        action_->display(out);
    }

    Value WhileStatement::execute() const {
        Value break_v{new BreakValue};
        Value result{new UndefinedValue};
        for (;;) {
            Value condition_value = condition_->evaluate();
            bool true_enough = condition_value->isTrueEnough();
            if (IStatement::Debug) {
                ostringstream out;
                out << "while ";
                condition_->prefixDisplay(out);
                out << " => ";
                condition_value->display(out);
                out << "; ";
                if (true_enough) {
                    out << "continuing";
                } else {
                    out << "leaving";
                }
                Universe::instance().output()->put(out.str());
                Universe::instance().output()->endLine();
            }
            if (not true_enough) {
                break;
            }
            result = action_->execute();
            if (result->isSameValueAs(break_v)) {
                if (IExpression::Debug) {
                    Universe::instance().output()->put("break while");
                    Universe::instance().output()->endLine();
                }
                // The while-loop "consumes" the break so it doesn't keep breaking outer loops
                result.reset(new UndefinedValue);
                break;
            }
        }
        if (IStatement::Debug) {
            ostringstream out;
            out << "while-result => ";
            result->display(out);
            Universe::instance().output()->put(out.str());
            Universe::instance().output()->endLine();
        }
        return result;
    }

    /**
     @param t (IN/OUT)             -- the token stream

     @return A pointer to the statement; or nullptr if (the statement was not syntactically
     correct.

     BNF:
     <statement> := <compound> | <single>

     <compound> := <left brace> <single>+ <right brace>

     <single> := <if_stmt> | <case_stmt> | <any_stmt> |
     <write_stmt> | <send_stmt> | <for_stmt>

     */

    Statement make_statement(TokenStream& t) {
        Statement the_stmt;
        if (not t.fetch()) {
            t.errorMessage("Expected Archetype statement, found end of file");
            t.stopLooking();
            return the_stmt;
        }

        if (t.token() == Token(Token::PUNCTUATION, '{')) {
            the_stmt.reset(new CompoundStatement);
        } else if (t.token().type() == Token::QUOTE_LITERAL) {
            t.didNotConsume();
            the_stmt.reset(new ParagraphOutputStatement);
        } else if (t.token().type() != Token::RESERVED_WORD) {
            t.didNotConsume();
            the_stmt.reset(new ExpressionStatement);
        } else {
            Keywords::Reserved_e word = Keywords::Reserved_e(t.token().number());
            switch (word) {
                case Keywords::RW_IF:
                    the_stmt.reset(new IfStatement);
                    break;
                case Keywords::RW_CASE:
                    the_stmt.reset(new CaseStatement);
                    break;
                case Keywords::RW_CREATE:
                    the_stmt.reset(new CreateStatement);
                    break;
                case Keywords::RW_DESTROY:
                    the_stmt.reset(new DestroyStatement);
                    break;
                case Keywords::RW_DISPLAY:
                case Keywords::RW_WRITE:
                case Keywords::RW_WRITES:
                case Keywords::RW_STOP:
                    the_stmt.reset(new OutputStatement(word));
                    break;
                case Keywords::RW_FOR:
                    the_stmt.reset(new ForStatement);
                    break;
                case Keywords::RW_WHILE:
                    the_stmt.reset(new WhileStatement);
                    break;
                default:
                    /* Default:  an expression that may begin with a reserved word */
                    t.didNotConsume();
                    the_stmt.reset(new ExpressionStatement);
                    break;
            }  /* switch (t.token().number()) */
        }

        if (the_stmt->make(t))
            return the_stmt;
        else
            return nullptr;

    }  /* make_statement */

    Storage& operator<<(Storage& out, const Statement& stmt) {
        // Each statement writes its own type out first before writing the rest
        // of its implementation.
        stmt->write(out);
        return out;
    }

    Storage& operator>>(Storage& in, Statement& stmt) {
        int stmt_type_as_int;
        in >> stmt_type_as_int;
        StatementType_e type = static_cast<StatementType_e>(stmt_type_as_int);
        switch (type) {
            case COMPOUND:
                stmt.reset(new CompoundStatement);
                break;
            case EXPRESSION:
                stmt.reset(new ExpressionStatement);
                break;
            case IF:
                stmt.reset(new IfStatement);
                break;
            case CASE:
                stmt.reset(new CaseStatement);
                break;
            case CREATE:
                stmt.reset(new CreateStatement);
                break;
            case DESTROY:
                stmt.reset(new DestroyStatement);
                break;
            case FOR:
                stmt.reset(new ForStatement);
                break;
            case WHILE:
                stmt.reset(new WhileStatement);
                break;
            case OUTPUT:
                stmt.reset(new OutputStatement);
                break;
            case PARAGRAPH_OUTPUT:
                stmt.reset(new ParagraphOutputStatement);
                break;
        }
        stmt->read(in);
        return in;
    }

    Statement make_stmt_from_str(string src_str) {
        stream_ptr in(new istringstream(src_str));
        SourceFilePtr src{make_shared<SourceFile>("test", in)};
        TokenStream token_stream(src);
        Statement stmt = make_statement(token_stream);
        return stmt;
    }

}
