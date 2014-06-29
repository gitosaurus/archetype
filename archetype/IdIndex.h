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

#include "Serialization.h"

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
        
        void write(Storage& out) const {
            int entries = static_cast<int>(registry_.size());
            out << entries;
            for (auto p = index_.begin(); p != index_.end(); ++p) {
                out << p->first << p->second;
            }
        }
        
        void read(Storage& in) {
            int entries;
            in >> entries;
            index_.clear();
            registry_.clear();
            for (int i = 0; i < entries; ++i) {
                T value;
                int value_index;
                in >> value >> value_index;
                index_[value] = value_index;
                registry_[value_index] = value;
            }
        }
        
    };
    
    template <class T>
    inline Storage& operator<<(Storage& out, const IdIndex<T>& index) {
        index.write(out);
        return out;
    }
    
    template <class T>
    inline Storage& operator>>(Storage& in, IdIndex<T>& index) {
        index.read(in);
        return in;
    }
    
}

#endif
