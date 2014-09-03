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

#include "SystemObject.h"
#include "Universe.h"
#include "FileStorage.h"

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
        "SAVE STATE", "LOAD STATE"
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
    
    bool SystemObject::hasMethod(int message_id) const {
        return false;
    }
    
    Value SystemObject::executeMethod(int message_id) {
        return Value{new UndefinedValue};
    }
    
    bool SystemObject::hasDefaultMethod() const {
        return true;
    }
    
    Value SystemObject::dispatch(Value message) {
        ContextScope c;
        c->messageValue = move(message);
        return executeDefaultMethod();
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
                            
                        case ROLL_CALL:
                            parser_->rollCall();
                            state_ = IDLING;
                            break;
                        case PRESENT:
                            parser_->announcePresence(Universe::instance().currentContext().selfObject->id());
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
                            break;
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
                        case NOUN_LIST:
                            cerr << __FILE__ << ":" << __LINE__ << ":" << "Cannot go to state " << state_ << " from IDLING; returning to idle" << endl;
                            state_ = IDLING;
                            break;
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
            case CLOSE_SORTER:
                cerr << __FILE__ << ":" << __LINE__ << ":" << "Unexpectedly found sorting instruction " << state_ << " at top of loop; idling" << endl;
                state_ = IDLING;
            
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
            case CLOSE_PARSER:
                cerr << __FILE__ << ":" << __LINE__ << ":" << "Unexpectedly found parsing instruction " << state_ << " at top of loop; idling" << endl;
                state_ = IDLING;
                break;

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
            case SAVE_STATE: {
                state_ = IDLING;
                Value filename_str = message->stringConversion();
                if (filename_str->isDefined()) {
                    string filename = filename_str->getString();
                    OutFileStorage save_file(filename);
                    if (save_file.ok()) {
                        save_file << Universe::instance();
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
                    }
                }
                break;
            }
                
            case NORMALIZE:
            case PARSE:
            case ROLL_CALL:
            case PRESENT:
            case NEXT_OBJECT:
                cerr << __FILE__ << ":" << __LINE__ << ":" << "Unexpectedly found interpreter instruction " << state_ << " at top of loop; idling" << endl;
                state_ = IDLING;
                break;
                
            default:
                cerr << __FILE__ << ":" << __LINE__ << ":" << "Unexpectedly found UNHANDLED state " << state_ << " at top of loop; idling" << endl;
                state_ = IDLING;
                break;
                
        }
        return Value{new UndefinedValue};
    }
}