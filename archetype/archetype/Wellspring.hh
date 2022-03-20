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

#include "SourceFile.hh"

namespace archetype {

    // The "source of sources."
    class Wellspring {
        std::list<std::string> paths_;
        std::set<std::string> everBeenOpened_;
        std::map<std::string, SourceFilePtr> sources_;
    public:
        static Wellspring& instance();
        static void destroy();

        // Given the path to a source file, prioritize the path to the directory
        // in which it resides, searching it before all other search paths,
        // including any which have been added so far through addSearchPath.
        // The given file must exist or invalid_argument will be thrown.
        // If it does exist, open and return.
        SourceFilePtr primarySource(std::string file_path);

        // Adds the given path to the list of paths to search for include files.
        // It will be searched after all the paths that have been added via this call so far.
        void addSearchPath(std::string directory_path);

        bool hasNeverBeenOpened(std::string source_name) const;
        void put(std::string source_name, SourceFilePtr source);
        SourceFilePtr open(std::string source_name);
        void close(SourceFilePtr source);
        void closeAll();
    private:
        static Wellspring* instance_;

        Wellspring();
        Wellspring(const Wellspring&) = delete;
        Wellspring& operator=(const Wellspring&) = delete;
        ~Wellspring();
    };
}

#endif /* defined(__archetype__Wellspring__) */
