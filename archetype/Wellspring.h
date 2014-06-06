//
//  Wellspring.h
//  archetype
//
//  Created by Derek Jones on 6/4/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Wellspring__
#define __archetype__Wellspring__

#include <iostream>
#include <map>
#include <list>
#include <set>
#include <string>
#include <memory>

#include "SourceFile.h"

namespace archetype {
    
    // The "source of sources."
    class Wellspring {
        std::list<std::string> paths_;
        std::set<std::string> everBeenOpened_;
        std::map<std::string, SourceFilePtr> sources_;
    public:
        static Wellspring& instance();
        
        void addSearchPath(std::string path) {
            paths_.push_back(path);
        }
        
        bool hasNeverBeenOpened(std::string source_name) const;
        void put(std::string source_name, SourceFilePtr);
        SourceFilePtr open(std::string source_name);
        void close(std::string source_name);
        void closeAll();
    private:
        static Wellspring* instance_;

        Wellspring();
        Wellspring(const Wellspring&) = delete;
        Wellspring& operator=(const Wellspring&) = delete;
    };
}

#endif /* defined(__archetype__Wellspring__) */
