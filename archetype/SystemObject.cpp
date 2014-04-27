//
//  SystemObject.cpp
//  archetype
//
//  Created by Derek Jones on 4/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <vector>
#include <string>

#include "SystemObject.h"
#include "Universe.h"

using namespace std;

namespace archetype {
    
    static vector<string> SystemMessageNames = {
        "IDLING",
        "INIT SORTER", "OPEN SORTER", "CLOSE SORTER", "NEXT SORTED",
        "PLAYER CMD", "NORMALIZE", "ABBR",
        "OPEN PARSER", "VERB LIST", "NOUN LIST", "CLOSE PARSER",
        "INIT PARSER", "WHICH OBJECT",
        "ROLL CALL", "PRESENT", "PARSE", "NEXT OBJECT",
        "DEBUG MESSAGES", "DEBUG EXPRESSIONS",
        "DEBUG STATEMENTS",
        "DEBUG MEMORY", "FREE MEMORY",
        "SAVE STATE", "LOAD STATE"
    };
    
    SystemObject::SystemObject():
    state_(IDLING),
    sorter_(new SystemSorter),
    parser_(new SystemParser) {
        // Build up the known messages to which we respond
        int state_counter = 0;
        for (auto message_name : SystemMessageNames) {
            int message_id = Universe::instance().Vocabulary.index(message_name);
            stateByMessage_[message_id] = State_e(state_counter++);
        }
    }
    
    bool SystemObject::figureState_(const Value& message) {
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
    
    Value SystemObject::send(Value message) {
        switch (state_) {
            case IDLING:
                if (figureState_(message)) {
                    switch (state_) {
                        case INIT_SORTER:
                            sorter_.reset(new SystemSorter);
                            state_ = OPEN_SORTER;
                            break;
                        case OPEN_SORTER:
                            // Remain
                            break;
                        case NEXT_SORTED:
                            state_ = IDLING;
                            return sorter_->nextSorted();
                            
                        case INIT_PARSER:
                            parser_.reset(new SystemParser);
                            state_ = OPEN_PARSER;
                            break;
                        case OPEN_PARSER:
                            // Remain
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
                
            case OPEN_PARSER:
                if (figureState_(message)) {
                    if (state_ == CLOSE_PARSER) {
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
                    
                    // TODO: add the sender to the parser, with attached synonyms
                }
                break;
                
        }
        return Value(new UndefinedValue);
    }
}