//
//  IdIndex.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__IdIndex__
#define __archetype__IdIndex__

#include <iostream>
#include <map>
#include <deque>
#include <cassert>
#include <algorithm>

#include "Serialization.h"

namespace archetype {

    template <class T>
    class IdIndex {
        std::map<T, int> index_;
        std::deque<T> registry_;
        T sentinel_;
        int holes_;
    public:
        static const int npos = -1;

        IdIndex(const T& sentinel = T()):
        sentinel_(sentinel),
        holes_(0)
        { }

        int index(const T& obj) {
            auto where = index_.find(obj);
            if (where == index_.end()) {
                int new_index = static_cast<int>(registry_.size());
                if (holes_) {
                    auto last_hole = std::find(registry_.rbegin(), registry_.rend(), sentinel_);
                    assert(last_hole != registry_.rend());
                    new_index = static_cast<int>(registry_.rend() - last_hole) - 1;
                    holes_--;
                    registry_[new_index] = obj;
                } else {
                    registry_.push_back(obj);
                }
                where = index_.insert(std::make_pair(obj, new_index)).first;
            }
            return where->second;
        }

        void remove(int obj_index) {
            const T& obj = registry_.at(obj_index);
            auto where = index_.find(obj);
            assert(where != index_.end());
            index_.erase(where);
            if (obj_index == registry_.size() - 1) {
                registry_.resize(obj_index);
            } else {
                registry_[obj_index] = sentinel_;
                holes_++;
            }
        }

        int count() const {
            return static_cast<int>(registry_.size());
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
            int total_entries = static_cast<int>(registry_.size());
            int indexed_entries = static_cast<int>(index_.size());
            out << total_entries << indexed_entries;
            for (auto const& entry : index_) {
                out << entry.first << entry.second;
            }
        }

        void read(Storage& in) {
            int total_entries;
            int indexed_entries;
            in >> total_entries >> indexed_entries;
            index_.clear();
            registry_.clear();
            registry_.resize(total_entries, sentinel_);
            for (int i = 0; i < indexed_entries; ++i) {
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

#endif /* defined(__archetype__IdIndex__) */
