//
//  TokenStream.cpp
//  archetype
//
//  Created by Derek Jones on 2/16/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <set>
#include <deque>
#include <sstream>
#include <cctype>

#include "TokenStream.hh"
#include "Universe.hh"
#include "Keywords.hh"

using namespace std;

namespace archetype {

    static class TypeChecker {
        set<char> longOpers_;
        set<char> opers_;
    public:
        TypeChecker();
        bool isWhite(char c) const          { return isspace(c); }
        bool isLiteral(char c) const        { return c == '\'' or c == '"'; }
        bool isIDStart(char c) const        { return isalpha(c) or c == '_'; }
        bool isIDChar(char c) const         { return isIDStart(c) or isDigit(c); }
        bool isDigit(char c) const          { return isdigit(c); }
        bool isOperator(char c) const       { return opers_.count(c) > 0; }
        bool isLongOperator(char c) const   { return longOpers_.count(c) > 0; }
    } TypeCheck;

    TypeChecker::TypeChecker():
    longOpers_({'<', '>', ':', '+', '-', '*', '/', '&', '~'}),
    opers_({'=', '.', '^', '?', '@'})
    {
        for (auto oper : longOpers_) {
            opers_.insert(oper);
        }
    }

    TokenStream::TokenStream(SourceFilePtr source):
    source_(source),
    consumed_(true),
    keepLooking_(true)
    {
        newlineIsToken_.push_front(false);
    }

    void TokenStream::hitEOF(Token required) {
        if (keepLooking_) {
            stopLooking();
            cout << "Found end of file; expected " << required << endl;
        }
    }

    bool TokenStream::insistOn(Token required) {
        if (not fetch()) {
            hitEOF(required);
            return false;
        }

        if (required == token_) {
            return true;
        }

        expected(required);
        stopLooking();
        return false;
    }

    void TokenStream::expectGeneral(std::string required) {
        if (keepLooking_) {
            source_->showPosition(cout);
            cout << "Expected " << required << "; found " << token_ << endl;
        }
    }

    void TokenStream::expected(Token required) {
        if (keepLooking_) {
            source_->showPosition(cout);
            cout << "Expected " << required << "; found " << token_ << endl;
        }

    }

    void TokenStream::errorMessage(std::string message) {
        if (keepLooking_) {
            source_->showPosition(cout);
            cout << message << endl;
        }
    }

    void TokenStream::stopLooking() {
        keepLooking_ = false;
    }

    bool TokenStream::fetch() {
        enum StateType_e {START, STOP, DECIDE, WHITE, COMMENT, QUOTE,
            LITERAL, IDENTIFIER, NUMBER, OPERATOR};

        StateType_e state;
        char bracket;
        char next_ch;
        string s;

        // Check for old token.  newlines_ may have changed while (an old
        // token was unconsumed, so if the unconsumed token was a NEWLINE
        // and newlines_ is false, we must continue and get another token;
        // otherwise we jump out with what we have.

        if (not consumed_) {
            consumed_ = true;
            if (not ((token_.type() == Token::NEWLINE) and (not isNewlineSignificant()))) {
                return true;
            }
        }

        state = START;
        s     = "";

        while (state != STOP) {

            switch (state) {

                case START:
                    if ((next_ch = source_->readChar()))
                        state = DECIDE;
                    else {
                        state = STOP;
                    }
                    break;

                case DECIDE:
                    if (not next_ch)
                        state = STOP;
                    else if (TypeCheck.isWhite(next_ch))
                        state = WHITE;
                    else if (TypeCheck.isLiteral(next_ch))
                        state = LITERAL;
                    else if (TypeCheck.isIDStart(next_ch))
                        state = IDENTIFIER;
                    else if (TypeCheck.isDigit(next_ch))
                        state = NUMBER;
                    else if (TypeCheck.isOperator(next_ch))
                        state = OPERATOR;
                    else                    // a single-character token
                        switch (next_ch) {
                            case '#':
                                state = COMMENT;
                                break;
                            case ';':
                                if (not isNewlineSignificant())
                                    state = START;
                                else {
                                    token_ = Token(Token::NEWLINE, '\n');
                                    state = STOP;
                                }
                                break;
                            default:
                                token_ = Token(Token::PUNCTUATION, next_ch);
                                state = STOP;
                                break;
                        } // switch
                    break; // case

                case WHITE:
                    while (state == WHITE and TypeCheck.isWhite(next_ch)) {
                        if (next_ch == '\n' and isNewlineSignificant()) {
                            token_ = Token(Token::NEWLINE, '\n');
                            state  = STOP;
                        }
                        else
                            next_ch = source_->readChar();
                    }
                    if (state == WHITE) {
                        if (next_ch) {         // decide on new non-white character
                            state = DECIDE;
                        } else {
                            state = STOP;
                        }
                    }
                    break;

                case COMMENT:
                case QUOTE:
                    s = "";
                    while ((next_ch = source_->readChar()) != '\n') {
                        s += next_ch;
                    }
                    if (state == COMMENT) {
                        if (next_ch)
                            state = START;
                        else
                            state = STOP;
                    }
                    else {                        // quoted literal
                        source_->unreadChar(next_ch);           // leave \n for the next guy
                        token_ = Token(Token::QUOTE_LITERAL,
                                       Universe::instance().TextLiterals.index(s));
                        state  = STOP;
                    }
                    break;

                case LITERAL: {

                    bracket = next_ch;
                    s = "";
                    while ((next_ch = source_->readChar()) and
                           (next_ch != '\n') and (next_ch != bracket)) {
                        if (next_ch == '\\') {
                            next_ch = source_->readChar();
                            switch (next_ch) {
                                case 't' : next_ch = '\t'; break;
                                case 'b' : next_ch = '\b'; break;
                                case 'e' : next_ch = '\033'; break;  // escape
                                case 'n' : next_ch = '\n'; break;
                                case '"' : next_ch = '"'; break;
                                default: {
                                    ostringstream out;
                                    out << "Unknown escape character \\" << next_ch;
                                    errorMessage(out.str());
                                    break;
                                }
                            }  // switch
                        }
                        s += next_ch;
                    };  // while

                    if (next_ch != bracket) {
                        source_->showPosition(cout);
                        throw std::runtime_error("Unterminated literal");
                    }
                    else {

                        switch (bracket) {
                            case '"':
                                token_ = Token(Token::TEXT_LITERAL,
                                               Universe::instance().TextLiterals.index(s));
                                break;
                            case '\'':
                                token_ = Token(Token::MESSAGE,
                                               Universe::instance().Messages.index(s));
                                break;
                            default:
                                cout << "Programmer error: unknown literal type" << endl;
                                break;
                        }  // switch

                        state = STOP;

                    }  // else

                }  // LITERAL
                    break;

                case IDENTIFIER: {
                    s = "";
                    while (TypeCheck.isIDChar(next_ch)) {
                        s += next_ch;
                        next_ch = source_->readChar();
                    }
                    if (not (TypeCheck.isIDChar(next_ch)))
                        source_->unreadChar(next_ch);
                    // Check for reserved words or operators
                    if (Keywords::instance().Reserved.has(s)) {
                        token_ = Token(Token::RESERVED_WORD,
                                       Keywords::instance().Reserved.index(s));
                    } else if (Keywords::instance().Operators.has(s)) {
                        token_ = Token(Token::OPERATOR,
                                       Keywords::instance().Operators.index(s));
                    } else {
                        int tnum = Universe::instance().Identifiers.index(s);
                        token_ = Token(Token::IDENTIFIER, tnum);
                    }
                    state = STOP;
                }
                    break;

                case NUMBER:
                {
                    s = "";
                    while (next_ch and (TypeCheck.isDigit(next_ch))) {
                        s += next_ch;
                        next_ch = source_->readChar();
                    }
                    if (not (TypeCheck.isDigit(next_ch)))
                        source_->unreadChar(next_ch);
                    int tnum;
                    istringstream in(s);
                    in >> tnum;
                    token_ = Token(Token::NUMERIC, tnum);
                    state = STOP;
                }
                    break;

                case OPERATOR:
                {
                    s = "";
                    while (next_ch and
                           (TypeCheck.isLongOperator(next_ch)) and
                           (s != ">>")                 // have to stop short with >>
                           ) {
                        s += next_ch;
                        next_ch = source_->readChar();
                    }
                    if (s == ">>") {
                        source_->unreadChar(next_ch);
                        state = QUOTE;
                    }
                    else {
                        if (not (TypeCheck.isOperator(next_ch)))
                            source_->unreadChar(next_ch);
                        else
                            s += next_ch;
                        state = STOP;
                        if (s == ":") {
                            token_ = Token(Token::PUNCTUATION, ':');
                        }
                        else if (not Keywords::instance().Operators.has(s)) {
                            source_->showPosition(cout);
                            cout << "Unknown operator: " << s << endl;
                            terminate();
                        }
                        else {
                            token_ = Token(Token::OPERATOR,
                                           Keywords::instance().Operators.index(s));
                        }
                    }     // all cases which are not >>
                }         // OPERATOR
                    break;

                case STOP:
                    break;
            } // switch
        } // while - primary state machine loop

        return next_ch != '\0';
    }

    void TokenStream::didNotConsume() {
        consumed_ = false;
    }

}
