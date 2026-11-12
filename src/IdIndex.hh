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
#include <format>
#include <functional>
#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>
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
        // Sparse, and keyed by the id itself:  an id that remove() has taken
        // back is simply not here.  Nothing stands in for it, so no stored
        // value ever has to mean "unoccupied" -- an empty string is a
        // legitimate entry (author.arch:276) -- and a registry that has lost
        // a million entries costs nothing to hold, to write, or to read back.
        std::map<int, T> registry_;
        // One past the highest id ever handed out.  It only goes up:  an id
        // names one entry for the life of the index, and is never given to
        // another.  For the object registry that is the whole point.  A
        // reference to a destroyed object is an id held in some attribute the
        // index knows nothing about, and if the id were handed out again that
        // reference would quietly start meaning whatever was created next.
        int next_ = 0;
    public:
        static constexpr int npos = -1;

        void clear() {
            index_.clear();
            registry_.clear();
            next_ = 0;
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
                if (next_ == std::numeric_limits<int>::max()) {
                    throw std::length_error("Registry has run out of ids");
                }
                int new_index = next_++;
                registry_.emplace(new_index, T(key));
                where = index_.emplace(T(key), new_index).first;
            }
            return where->second;
        }

        // False if there was nothing at that id to remove, which is not an
        // error:  removing what is already gone leaves it gone.
        bool remove(int obj_index) {
            auto slot = registry_.find(obj_index);
            if (slot == registry_.end()) {
                return false;
            }
            index_.erase(slot->second);
            // For the object registry this lets go of the last reference to
            // the object being destroyed.
            registry_.erase(slot);
            return true;
        }

        // How many ids have ever been handed out, which is also the id the
        // next new entry will get.  Not how many entries there are now.
        int count() const {
            return next_;
        }

        // The ids in use at this moment, in ascending order.  A copy, so that
        // a caller may add and remove entries while walking it.
        std::vector<int> indices() const {
            std::vector<int> result;
            result.reserve(registry_.size());
            for (auto const& entry : registry_) {
                result.push_back(entry.first);
            }
            return result;
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
            return registry_.contains(i);
        }

        void display(std::ostream& out) const {
            for (auto const& entry : registry_) {
                out << entry.second << std::endl;
            }
        }

        void write(Storage& out) const {
            int total_entries = next_;
            int indexed_entries = static_cast<int>(registry_.size());
            out << total_entries << indexed_entries;
            // Every record carries its own id, so read() does not care what
            // order they arrive in -- but the file does.  index_ is keyed by
            // the stored value, and for the object registry that value is a
            // shared_ptr, so writing in that order wrote the file in heap
            // address order:  two universes with identical contents produced
            // different bytes.  Id order is a property of the universe rather
            // than of the allocator, and it is the order registry_ keeps.
            for (auto const& [obj_index, obj] : registry_) {
                out << obj_index << obj;
            }
        }

        void read(Storage& in) {
            // Both of these arrive from the file, and both used to be believed
            // on sight.  The record count is a promise about bytes that follow
            // and is held to it.  The total is not:  it is only where the ids
            // had got to, nothing is allocated on its say-so, and a game that
            // has destroyed more objects than its save has bytes is entitled
            // to say so.
            int total_entries = in.readInteger();
            if (total_entries < 0) {
                throw std::invalid_argument(
                    std::format("A registry size cannot be negative: {}", total_entries));
            }
            int indexed_entries = readCount(in, "registry record count");
            if (indexed_entries > total_entries) {
                throw std::invalid_argument(
                    std::format("Registry of {} slots cannot hold {} records",
                                total_entries, indexed_entries));
            }
            for (int ii = 0; ii < indexed_entries; ++ii) {
                int value_index;
                in >> value_index;
                // A record naming a slot the registry does not have was an
                // out-of-bounds write straight into the registry, reachable
                // from any file at all: thirteen crafted bytes segfaulted the
                // interpreter here.
                if (value_index < 0 or value_index >= total_entries) {
                    throw std::invalid_argument(
                        std::format("Record names slot {} in a registry of {}",
                                    value_index, total_entries));
                }
                // Read in place:  the universe sets up its reserved objects
                // before reading, and the system object in particular has to
                // be the one that reads itself back.
                T& value = registry_[value_index];
                in >> value;
                index_[value] = value_index;
            }
            // Nothing about the retired ids is written down, and nothing needs
            // to be:  they are the ones below the total that no record names.
            // The total is what has to come back, because an index that forgot
            // how far it had counted would hand a retired id out again -- and
            // a resumed game would differ from a continuous one that took the
            // same turns.
            next_ = std::max(next_, total_entries);
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
