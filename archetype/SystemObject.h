//
//  SystemObject.h
//  archetype
//
//  Created by Derek Jones on 4/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__SystemObject__
#define __archetype__SystemObject__

#include <iostream>
#include <memory>

#include "Value.h"
#include "Object.h"
#include "SystemSorter.h"
#include "SystemParser.h"

namespace archetype {
    class SystemObject : public Object {
    public:

        enum State_e {
            IDLING,
            INIT_SORTER, OPEN_SORTER, CLOSE_SORTER, NEXT_SORTED,
            PLAYER_CMD, NORMALIZE,
            OPEN_PARSER, VERB_LIST, NOUN_LIST, CLOSE_PARSER,
            INIT_PARSER, WHICH_OBJECT,
            ROLL_CALL, PRESENT, PARSE, NEXT_OBJECT,
            DEBUG_MESSAGES, DEBUG_EXPRESSIONS,
            DEBUG_STATEMENTS,
            SAVE_STATE, LOAD_STATE
        };
        

        SystemObject();
        
        virtual ~SystemObject() { }
        
        virtual Value executeMethod(int message_id) override;
        virtual Value executeDefaultMethod() override;
        
    private:
        State_e state_;
        std::map<int, State_e> stateByMessage_;
        
        std::unique_ptr<SystemSorter> sorter_;
        std::unique_ptr<SystemParser> parser_;

        bool figureState_(const Value& message);
    };
}

#endif /* defined(__archetype__SystemObject__) */
