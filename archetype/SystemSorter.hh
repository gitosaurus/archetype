//
//  SystemSorter.h
//  archetype
//
//  Created by Derek Jones on 4/26/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__SystemSorter__
#define __archetype__SystemSorter__

#include <iostream>
#include <string>
#include <set>

#include "Value.hh"

namespace archetype {
    class SystemSorter {
    public:
    private:
        std::multiset<std::string> sortedStrings_;
        friend Storage& operator<<(Storage& out, const SystemSorter& p);
        friend Storage& operator>>(Storage&in, SystemSorter& p);
    public:
        void add(std::string s);
        Value nextSorted();
    };

    Storage& operator<<(Storage& out, const SystemSorter& p);
    Storage& operator>>(Storage&in, SystemSorter& p);

}

#endif /* defined(__archetype__SystemSorter__) */
