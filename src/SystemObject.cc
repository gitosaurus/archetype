//
//  SystemObject.cc
//  archetype
//
//  Created by Derek Jones on 4/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <format>
#include <sstream>

#include "SystemObject.hh"
#include "Universe.hh"
#include "Autosave.hh"
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
    sorter_{make_unique<SystemSorter>()},
    parser_{make_unique<SystemParser>()} {
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
            if (auto where = stateByMessage_.find(message_literal->getMessage()); where != stateByMessage_.end()) {
                state_ = where->second;
                return true;
            }
        }
        return false;
    }

    Value SystemObject::executeMethod(int /*message_id*/) {
        return make_unique<AbsentValue>();
    }

    Value SystemObject::executeDefaultMethod() {
        const Value& sent = Universe::instance().currentContext().messageValue;
        // A list message is the staged dance in one send:  the head first,
        // then each element of the tail in order, with the reply of the whole
        // being the reply of the last.  So ['LOAD STATE' "file.acx"] keeps
        // its Boolean, and no send can be caught mid-dance by a save.  An
        // atom is the head of itself with an undefined tail, so a bare
        // message walks the same walk in one step.
        Value result = interpret_(sent->head());
        for (Value node = sent->tail(); node->isDefined(); node = node->tail()) {
            result = interpret_(node->head());
        }
        return result;
    }

    Value SystemObject::interpret_(const Value& message) {
        switch (state_) {
            case IDLING:
                if (figureState_(message)) {
                    switch (state_) {
                        case INIT_SORTER:
                            sorter_ = make_unique<SystemSorter>();
                            state_ = OPEN_SORTER;
                            break;
                        case OPEN_SORTER:
                            // Nothing new; remain in this state
                            break;
                        case NEXT_SORTED:
                            state_ = IDLING;
                            return sorter_->nextSorted();

                        case INIT_PARSER:
                            parser_ = make_unique<SystemParser>();
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
                            return make_unique<StringValue>(parser_->normalized());
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
                            Universe::instance().output()->put(format("Cannot go to state {} from IDLING; returning to idle\n", static_cast<int>(state_)));
                            state_ = IDLING;
                            break;
                        }

                        case DEBUG_MESSAGES:
                            Object::Debug = not Object::Debug;
                            state_ = IDLING;
                            return make_unique<BooleanValue>(Object::Debug);
                        case DEBUG_EXPRESSIONS:
                            IExpression::Debug = not IExpression::Debug;
                            state_ = IDLING;
                            return make_unique<BooleanValue>(IExpression::Debug);
                        case DEBUG_STATEMENTS:
                            IStatement::Debug = not IStatement::Debug;
                            state_ = IDLING;
                            return make_unique<BooleanValue>(IStatement::Debug);
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
                Universe::instance().output()->put(format("Unexpectedly found sorting instruction {} at top of loop; idling\n", static_cast<int>(state_)));
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
                Universe::instance().output()->put(format("Unexpectedly found parsing instruction {} at top of loop; idling\n", static_cast<int>(state_)));
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
                    // Written atomically so that overwriting an existing save
                    // cannot destroy it if this one fails part way through.  No
                    // backup: an explicit save should not litter the player's
                    // directory with .bak files.
                    string error;
                    bool saved = writeUniverseAtomically(filename, /* keep_backup = */ false, error);
                    return make_unique<BooleanValue>(saved);
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
                        // The whole object graph has just been replaced, so the
                        // ids autosave watches for are no longer the right ones.
                        Autosave::instance().rearm();
                        return make_unique<BooleanValue>(true);
                    } else {
                        return make_unique<BooleanValue>(false);
                    }
                }
                break;
            }

            case NORMALIZE:
            case PARSE:
            case ROLL_CALL:
            case PRESENT:
            case NEXT_OBJECT: {
                Universe::instance().output()->put(format("Unexpectedly found interpreter instruction {} at top of loop; idling\n", static_cast<int>(state_)));
                state_ = IDLING;
                break;
            }

            default: {
                Universe::instance().output()->put(format("Unexpectedly found UNHANDLED state {} at top of loop; idling\n", static_cast<int>(state_)));
                state_ = IDLING;
                break;
            }

        }
        return make_unique<UndefinedValue>();
    }

    void SystemObject::resetSystem_() {
        stateByMessage_.clear();
        sorter_ = make_unique<SystemSorter>();
        parser_ = make_unique<SystemParser>();
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
