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
#include <concepts>
#include <functional>
#include <map>
#include <deque>
#include <set>
#include <vector>
#include <cassert>

#include "Serialization.hh"

namespace archetype {

    // A key the index can be asked about without first being handed a T:  a
    // std::string_view or a string literal, against a registry of std::string.
    // std::less<> is what makes the lookup heterogeneous, and this is what
    // keeps the door open only to keys it can actually weigh.
    template <class K, class T>
    concept IndexKey = std::strict_weak_order<std::less<>, const K&, const T&>;

    template <class T>
    class IdIndex {
        std::map<T, int, std::less<>> index_;
        std::deque<T> registry_;
        // Slots that remove() gave back.  A free slot still holds a T, because
        // a deque slot must hold something, and T{} is the natural nothing --
        // nullptr for the object registry, "" for the string ones.  But which
        // slots are free is recorded here rather than inferred from what they
        // hold:  an empty string is a legitimate entry (author.arch:276), so a
        // value can never be trusted to mean "unoccupied".
        std::set<int> free_;
    public:
        static constexpr int npos = -1;

        void clear() {
            index_.clear();
            registry_.clear();
            free_.clear();
        }

        // Takes any key std::less<> can weigh against a T, so that indexing a
        // name spelled as a literal costs nothing when it hits -- which is the
        // common case, since a name is looked up far more often than it is
        // first seen.  A miss still materializes a T, which is exactly the
        // moment a T is wanted.
        template <IndexKey<T> K>
            requires std::constructible_from<T, const K&>
        int index(const K& key) {
            auto where = index_.find(key);
            if (where == index_.end()) {
                int new_index;
                if (free_.empty()) {
                    new_index = static_cast<int>(registry_.size());
                    registry_.emplace_back(key);
                } else {
                    // The lowest free slot, which keeps ids small and leaves
                    // the free ones bunched near the end of the registry --
                    // which is where remove()'s trim can give them back.
                    new_index = *free_.begin();
                    free_.erase(free_.begin());
                    registry_[new_index] = T(key);
                }
                where = index_.insert(std::make_pair(T(key), new_index)).first;
            }
            return where->second;
        }

        void remove(int obj_index) {
            assert(not free_.contains(obj_index));
            const T& obj = registry_.at(obj_index);
            auto where = index_.find(obj);
            assert(where != index_.end());
            index_.erase(where);
            // Let go of the value:  for the object registry this is the last
            // reference to the object being destroyed.
            registry_[obj_index] = T{};
            free_.insert(obj_index);
            // A hole at the end is not a hole, it is a shorter registry.
            while (not registry_.empty() and
                   free_.contains(static_cast<int>(registry_.size()) - 1)) {
                free_.erase(static_cast<int>(registry_.size()) - 1);
                registry_.pop_back();
            }
        }

        int count() const {
            return static_cast<int>(registry_.size());
        }

        template <IndexKey<T> K>
        int find(const K& key) const {
            auto where = index_.find(key);
            if (where == index_.end()) {
                return npos;
            } else {
                return where->second;
            }
        }

        const T& get(int obj_index) const {
            return registry_.at(obj_index);
        }

        template <IndexKey<T> K>
        bool has(const K& key) const {
            return index_.contains(key);
        }

        bool hasIndex(int i) const {
            return i >= 0 and size_t(i) < registry_.size();
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
            // Every record carries its own index, so read() does not care what
            // order they arrive in -- but the file does.  index_ is keyed by
            // the stored value, and for the object registry that value is a
            // shared_ptr, so writing in map order wrote the file in heap
            // address order:  two universes with identical contents produced
            // different bytes.  Registry order is a property of the universe
            // rather than of the allocator, so write in registry order.
            //
            // Which slots are occupied is what index_ claims, not what the
            // stored value happens to look like -- see free_.
            std::vector<const T*> occupant(total_entries, nullptr);
            for (auto const& [obj, obj_index] : index_) {
                occupant[obj_index] = &obj;
            }
            for (int ii = 0; ii < total_entries; ++ii) {
                if (occupant[ii]) {
                    out << ii << *occupant[ii];
                }
            }
        }

        void read(Storage& in) {
            int total_entries;
            int indexed_entries;
            in >> total_entries >> indexed_entries;
            registry_.resize(total_entries, T{});
            // Nothing about the holes is written down, and nothing needs to be.
            // Each record names the slot it belongs in, so the occupied slots
            // are exactly the ones the file mentions and the free ones are the
            // rest; total_entries is what makes a run of free slots at the end
            // recoverable, since no record would mention them.
            //
            // Reading this back matters:  an index that came back believing it
            // was full would append where it should have reused, and a resumed
            // game would hand out different ids than a continuous one that took
            // the same turns.
            std::vector<bool> occupied(total_entries, false);
            for (int ii = 0; ii < indexed_entries; ++ii) {
                int value_index;
                in >> value_index;
                in >> registry_[value_index];
                index_[registry_[value_index]] = value_index;
                occupied[value_index] = true;
            }
            free_.clear();
            for (int ii = 0; ii < total_entries; ++ii) {
                if (not occupied[ii]) {
                    free_.insert(ii);
                }
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
