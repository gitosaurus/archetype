//
//  Wellspring.cpp
//  archetype
//
//  Created by Derek Jones on 6/4/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <fstream>

#include "Wellspring.h"

namespace archetype {
    
    SourceFilePtr Wellspring::open(std::string source_name) {
        auto result = sources_.find(source_name);
        if (result != sources_.end()) {
            everBeenOpened_.insert(source_name);
            return result->second;
        }
        // Take the source name and compose it on the end of each path
        // Can do automatic extension here
        // Add an extension of SourceFile which contains its own ifstream
        return nullptr;
    }
    
    bool Wellspring::hasNeverBeenOpened(std::string source_name) const {
        return everBeenOpened_.count(source_name) == 0;
    }
    
    void Wellspring::put(std::string source_name, SourceFilePtr source) {
        sources_.insert(std::make_pair(source_name, source));
    }
    
    void Wellspring::close(std::string source_name) {
        sources_.erase(sources_.find(source_name));
    }
    
    void Wellspring::closeAll() {
        sources_.clear();
    }
    
    Wellspring* Wellspring::instance_ = 0;
    
    Wellspring::Wellspring() {
    }
    
    Wellspring& Wellspring::instance() {
        if (not instance_) {
            instance_ = new Wellspring;
        }
        return *instance_;
    }
}