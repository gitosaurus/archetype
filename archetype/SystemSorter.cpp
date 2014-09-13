//
//  SystemSorter.cpp
//  archetype
//
//  Created by Derek Jones on 4/26/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <string>

#include "SystemSorter.h"

using namespace std;

namespace archetype {
    void SystemSorter::add(std::string s) {
        sortedStrings_.insert(s);
    }

    Value SystemSorter::nextSorted() {
        if (not sortedStrings_.empty()) {
            string result = *sortedStrings_.begin();
            sortedStrings_.erase(sortedStrings_.begin());
            return Value(new StringValue(result));
        }
        return Value{new UndefinedValue};
    }
}
