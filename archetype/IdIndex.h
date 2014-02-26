//
//  IdIndex.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef archetype_IdIndex_h
#define archetype_IdIndex_h

#include <map>
#include <deque>

namespace archetype {
    template <class T>
    class IdIndex {
        std::map<T, int> index_;
        std::deque<T> registry_;
    public:
        int index(T obj) {
            auto where = index_.find(obj);
            if (where == index_.end()) {
                where = index_.insert(std::make_pair(obj, registry_.size())).first;
                registry_.push_back(obj);
            }
            return where->second;
        }
        
        T get(int obj_index) const {
            return registry_.at(obj_index);
        }
        
        bool has(T obj) const {
            return index_.count(obj);
        }
    };
    
}

#endif
