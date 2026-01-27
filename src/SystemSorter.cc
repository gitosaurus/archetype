//
//  SystemSorter.cc
//  archetype
//
//  Created by Derek Jones on 4/26/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>

#include "SystemSorter.hh"

using namespace std;

namespace archetype {
    void SystemSorter::add(std::string s) {
        sortedStrings_.insert(s);
    }

    Value SystemSorter::nextSorted() {
        if (not sortedStrings_.empty()) {
            string result = *sortedStrings_.begin();
            sortedStrings_.erase(sortedStrings_.begin());
            return Value{new StringValue{result}};
        }
        return Value{new UndefinedValue};
    }

    Storage& operator<<(Storage& out, const SystemSorter& ss) {
        int items = static_cast<int>(ss.sortedStrings_.size());
        out << items;
        for (auto const& s : ss.sortedStrings_) {
            out << s;
        }
        return out;
    }

    Storage& operator>>(Storage&in, SystemSorter& ss) {
        int items;
        in >> items;
        ss.sortedStrings_.clear();
        for (int ii = 0; ii < items; ++ii) {
            string str;
            in >> str;
            ss.sortedStrings_.insert(str);
        }
        return in;
    }

}
