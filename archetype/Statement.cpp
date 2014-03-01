//
//  Statement.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <memory>

#include "Statement.h"

using namespace std;

namespace archetype {

    bool CompoundStatement::make(TokenStream& t) {
        while (t.fetch()) {
            if (t.token() == Token(Token::PUNCTUATION, '}')) {
                return true;
            } else {
                t.didNotConsume();
                Statement s = make_statement(t);
                if (s) {
                    statements_.push_back(move(s));
                } else {
                    t.errorMessage("Unfinished compound statement");
                    t.stopLooking();
                    return false;
                }
            }
        }
        return false;
    }
    
    bool ExpressionStatement::make(TokenStream& t) {
        expression_ = make_expr(t);
        return expression_ != nullptr;
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
    
    bool CaseStatement::make(TokenStream& t) {
        /* BNF:  <case_stmt> := switch (<expr>) { (<expr> <statement>)+
         [default <statement>] } */
        if (not (testExpression_ = make_expr(t))) return false;
        if (not (t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_OF)) and
                 t.insistOn(Token(Token::PUNCTUATION, '{')))) {
            return false;
        }
        shared_ptr<Case> default_ptr;
        for (;;) {
            if (not t.fetch()) {
                t.hitEOF(Token(Token::PUNCTUATION, '}'));
                return false;
            }
            
            if (t.token() == Token(Token::PUNCTUATION, '}')) {
                break;
            }
            
            shared_ptr<Case> the_case_pair(new Case);
            if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_DEFAULT)) {
                if (default_ptr.get() != nullptr) {
                    t.errorMessage("There is already a default for this case");
                    return false;
                } else {
                    default_ptr = the_case_pair;
                    default_ptr->value.reset(new ReservedConstantNode(Keywords::RW_DEFAULT));
                }
            } else {
                t.didNotConsume();
                the_case_pair->value = make_expr(t);
            }
            if (not (the_case_pair->value and
                     t.insistOn(Token(Token::PUNCTUATION, ':')) and
                     not (the_case_pair->action = make_statement(t)))) {
                return false;
            }
            if (default_ptr != the_case_pair) {
                cases_.push_back(*the_case_pair);
            }
        }
        
        if (default_ptr != nullptr) {
            cases_.push_back(*default_ptr);
        }
        return true;
    }

    bool CreateStatement::make(TokenStream& t) {
        if (not t.fetch()) {
            t.hitEOF(Token(Token::IDENTIFIER, -1));
            return false;
        }
        
        typeId_ = 0;
        if (t.token() != Token(Token::RESERVED_WORD, Keywords::RW_NULL) and
            t.token().type() != Token::IDENTIFIER) {
            t.expectGeneral("type identifier");
            return false;
        }
#ifdef NOTYET
        // TODO: Does this really belong here?  In a two-pass situation, it doesn't.
        get_meaning(t.token().number(), the_type_id, typeId_);
        if (the_type_id != TYPE_ID) {
            t.errorMessage("Require name of defined type");
            return false;
        }
#endif
        return t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_NAMED)) and (target_ = make_expr(t));
    }
    
    bool DestroyStatement::make(TokenStream& t) {
        return (victim_ = make_expr(t)) != nullptr;
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
        expressions_.push_back(the_expr);
        while (t.fetch()) {
            if (t.token() != Token(Token::PUNCTUATION, ',')) {
                t.didNotConsume();
                break;
            }
            the_expr = make_expr(t);
            if (not the_expr) return false;
            expressions_.push_back(the_expr);
        }
        return true;
    }
    
    bool ForStatement::make(TokenStream& t) {
        if (not (selection_ = make_expr(t))) return false;
        if (not t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_DO))) return false;
        action_ = make_statement(t);
        return action_ != nullptr;
    }
    
    bool WhileStatement::make(TokenStream& t) {
        if (not (condition_ = make_expr(t))) return false;
        if (not t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_DO))) return false;
        action_ = make_statement(t);
        return action_ != nullptr;
    }

    /**
     A very busy procedure.  Ensures semantic correctness of a general
     Archetype statement.
     
     @param f (IN/OUT)             -- the input .ACH file
     
     @return A pointer to the statment; or nullptr if (the statement was not syntactically
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
                case Keywords::RW_BREAK:
                    the_stmt.reset(new BreakStatement);
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
}