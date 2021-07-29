//
//  SystemObject.cpp
//  archetype
//
//  Created by Derek Jones on 4/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <sstream>

#include "SystemObject.hh"
#include "Universe.hh"
#include "FileStorage.hh"

using namespace std;

namespace archetype {

    static vector<string> SystemMessageNames = {
        "IDLING",
        "INIT SORTER", "OPEN SORTER", "CLOSE SORTER", "NEXT SORTED",
        "PLAYER CMD", "NORMALIZE",
        "OPEN PARSER", "VERB LIST", "NOUN LIST", "CLOSE PARSER",
        "INIT PARSER", "WHICH OBJECT",
        "ROLL CALL", "PRESENT", "PARSE", "NEXT OBJECT",
        "DEBUG MESSAGES", "DEBUG EXPRESSIONS",
        "DEBUG STATEMENTS",
        "SAVE STATE", "LOAD STATE",
        "BANNER"
    };

    SystemObject::SystemObject():
    state_{IDLING},
    sorter_{new SystemSorter},
    parser_{new SystemParser} {
    }

    bool SystemObject::figureState_(const Value& message) {
        if (stateByMessage_.empty()) {
            // This is done lazily and not in the constructor, because
            // the Universe constructs a SystemObject, and would result
            // in an infinite loop.
            int state_counter = 0;
            for (auto const& message_name : SystemMessageNames) {
                int message_id = Universe::instance().Messages.index(message_name);
                stateByMessage_[message_id] = State_e(state_counter++);
            }
        }
        Value message_literal = message->messageConversion();
        if (message_literal->isDefined()) {
            auto where = stateByMessage_.find(message_literal->getMessage());
            if (where != stateByMessage_.end()) {
                state_ = where->second;
                return true;
            }
        }
        return false;
    }

    Value SystemObject::executeMethod(int message_id) {
        return Value{new AbsentValue};
    }

    Value SystemObject::executeDefaultMethod() {
        Value message = Universe::instance().currentContext().messageValue->clone();
        switch (state_) {
            case IDLING:
                if (figureState_(message)) {
                    switch (state_) {
                        case INIT_SORTER:
                            sorter_.reset(new SystemSorter);
                            state_ = OPEN_SORTER;
                            break;
                        case OPEN_SORTER:
                            // Nothing new; remain in this state
                            break;
                        case NEXT_SORTED:
                            state_ = IDLING;
                            return sorter_->nextSorted();

                        case INIT_PARSER:
                            parser_.reset(new SystemParser);
                            state_ = OPEN_PARSER;
                            break;
                        case OPEN_PARSER:
                            // Nothing new; remain in this state
                            break;

                        case PLAYER_CMD:
                            // Nothing new; remain in this state
                            break;

                        case BANNER:
                            // Nothing new; remain in this state (for banner character)
                            break;

                        case ROLL_CALL:
                            parser_->rollCall();
                            state_ = IDLING;
                            break;
                        case PRESENT:
                            parser_->announcePresence(Universe::instance().currentContext().senderObject->id());
                            state_ = IDLING;
                            break;
                        case PARSE:
                            // In this implementation, PARSE is a no-op, since the parsing happened during
                            // the PLAYER_CMD message.
                            state_ = IDLING;
                            break;
                        case NORMALIZE:
                            state_ = IDLING;
                            return Value(new StringValue(parser_->normalized()));
                        case NEXT_OBJECT:
                            state_ = IDLING;
                            return parser_->nextObject();
                        case WHICH_OBJECT:
                        case SAVE_STATE:
                        case LOAD_STATE:
                            // Nothing new; remain in this state for the argument
                            break;

                        case IDLING:
                            break;

                        case CLOSE_SORTER:
                        case CLOSE_PARSER:
                        case VERB_LIST:
                        case NOUN_LIST: {
                            ostringstream out;
                            out << "Cannot go to state " << state_ << " from IDLING; returning to idle" << endl;
                            Universe::instance().output()->put(out.str());
                            state_ = IDLING;
                            break;
                        }

                        case DEBUG_MESSAGES:
                            Object::Debug = not Object::Debug;
                            state_ = IDLING;
                            return Value{new BooleanValue{Object::Debug}};
                        case DEBUG_EXPRESSIONS:
                            IExpression::Debug = not IExpression::Debug;
                            state_ = IDLING;
                            return Value{new BooleanValue{IExpression::Debug}};
                        case DEBUG_STATEMENTS:
                            IStatement::Debug = not IStatement::Debug;
                            state_ = IDLING;
                            return Value{new BooleanValue{IStatement::Debug}};
                    }
                }
                break;

            case OPEN_SORTER:
                if (figureState_(message)) {
                    if (state_ == CLOSE_SORTER) {
                        state_ = IDLING;
                    } else {
                        // Ignore and remain in this state
                        state_ = OPEN_SORTER;
                    }
                } else {
                    Value message_string = message->stringConversion();
                    if (message_string->isDefined()) {
                        sorter_->add(message_string->getString());
                        return message_string;
                    }
                }
                break;

            case INIT_SORTER:
            case NEXT_SORTED:
            case CLOSE_SORTER: {
                ostringstream out;
                out << "Unexpectedly found sorting instruction " << state_ << " at top of loop; idling" << endl;
                Universe::instance().output()->put(out.str());
                state_ = IDLING;
                break;
            }

            case OPEN_PARSER:
                if (figureState_(message)) {
                    if (state_ == CLOSE_PARSER) {
                        parser_->close();
                        state_ = IDLING;
                    } else if (state_ == VERB_LIST) {
                        parser_->setMode(SystemParser::VERBS);
                        state_ = OPEN_PARSER;
                    } else if (state_ == NOUN_LIST) {
                        parser_->setMode(SystemParser::NOUNS);
                        state_ = OPEN_PARSER;
                    } else {
                        // Ignore and remain in this state
                        state_ = OPEN_PARSER;
                    }
                } else {
                    Value message_str = message->stringConversion();
                    if (message_str->isDefined()) {
                        int sender = Universe::instance().currentContext().senderObject->id();
                        parser_->addParseable(sender, message_str->getString());
                    }
                }
                break;
            case INIT_PARSER:
            case VERB_LIST:
            case NOUN_LIST:
            case CLOSE_PARSER: {
                ostringstream out;
                out << "Unexpectedly found parsing instruction " << state_ << " at top of loop; idling" << endl;
                Universe::instance().output()->put(out.str());
                state_ = IDLING;
                break;
            }

            // Interpreter states
            case PLAYER_CMD: {
                state_ = IDLING;
                Value message_str = message->stringConversion();
                if (message_str->isDefined()) {
                    parser_->parse(message_str->getString());
                }
                break;
            }
            case WHICH_OBJECT: {
                state_ = IDLING;
                Value message_str = message->stringConversion();
                if (message_str->isDefined()) {
                    return parser_->whichObject(message_str->getString());
                }
                break;
            }
            case BANNER: {
                state_ = IDLING;
                char banner_ch = '-';
                Value message_str = message->stringConversion();
                if (message_str->isDefined()) {
                    string s = message_str->getString();
                    if (not s.empty()) {
                        banner_ch = s[0];
                    }
                }
                Universe::instance().output()->banner(banner_ch);
                break;
            }
            case SAVE_STATE: {
                state_ = IDLING;
                Value filename_str = message->stringConversion();
                if (filename_str->isDefined()) {
                    string filename = filename_str->getString();
                    OutFileStorage save_file(filename);
                    if (save_file.ok()) {
                        save_file << Universe::instance();
                        return Value{new BooleanValue{true}};
                    } else {
                        return Value{new BooleanValue{false}};
                    }
                }
                break;
            }
            case LOAD_STATE: {
                state_ = IDLING;
                Value filename_str = message->stringConversion();
                if (filename_str->isDefined()) {
                    string filename = filename_str->getString();
                    InFileStorage load_file(filename);
                    if (load_file.ok()) {
                        load_file >> Universe::instance();
                        resetSystem_();
                        return Value{new BooleanValue{true}};
                    } else {
                        return Value{new BooleanValue{false}};
                    }
                }
                break;
            }

            case NORMALIZE:
            case PARSE:
            case ROLL_CALL:
            case PRESENT:
            case NEXT_OBJECT: {
                ostringstream out;
                out << "Unexpectedly found interpreter instruction " << state_ << " at top of loop; idling" << endl;
                Universe::instance().output()->put(out.str());
                state_ = IDLING;
                break;
            }

            default: {
                ostringstream out;
                out << "Unexpectedly found UNHANDLED state " << state_ << " at top of loop; idling" << endl;
                Universe::instance().output()->put(out.str());
                state_ = IDLING;
                break;
            }

        }
        return Value{new UndefinedValue};
    }

    void SystemObject::resetSystem_() {
        stateByMessage_.clear();
        sorter_.reset(new SystemSorter);
        parser_.reset(new SystemParser);
        state_ = IDLING;
    }


    void SystemObject::write(Storage& out) {
        int state_int = static_cast<int>(state_);
        out << state_int << *sorter_ << *parser_;
    }

    void SystemObject::read(Storage& in) {
        resetSystem_();
        int state_int;
        in >> state_int;
        state_ = static_cast<State_e>(state_int);
        in >> *sorter_ >> *parser_;
    }

}
