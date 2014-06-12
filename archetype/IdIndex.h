//
//  IdIndex.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef archetype_IdIndex_h
#define archetype_IdIndex_h

#include <iostream>
#include <map>
#include <deque>

namespace archetype {
    template <class T>
    class IdIndex {
        std::map<T, int> index_;
        std::deque<T> registry_;
    public:
        static const int npos = -1;
        int index(const T& obj) {
            auto where = index_.find(obj);
            if (where == index_.end()) {
                where = index_.insert(std::make_pair(obj, registry_.size())).first;
                registry_.push_back(obj);
            }
            return where->second;
        }
        
        int find(const T& obj) const {
            auto where = index_.find(obj);
            if (where == index_.end()) {
                return npos;
            } else {
                return where->second;
            }
        }
        
        const T& get(int obj_index) const {
            return registry_.at(obj_index);
        }
        
        bool has(const T& obj) const {
            return index_.count(obj);
        }
        
        bool hasIndex(int i) const {
            return i >= 0 and i < registry_.size();
        }
        
        void display(std::ostream& out) const {
            for (const T& obj : registry_) {
                out << obj << std::endl;
            }
        }
    };
    
}

#endif
