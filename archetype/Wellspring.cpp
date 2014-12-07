//
//  Wellspring.cpp
//  archetype
//
//  Created by Derek Jones on 6/4/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <fstream>
#include <string>
#include <cassert>
#include <cstdlib>
#include <stdexcept>

#include "Wellspring.h"

using namespace std;

namespace archetype {
    SourceFilePtr Wellspring::primarySource(string file_path) {
        string::size_type last_slash = file_path.rfind('/');
        string basename = file_path.substr(0, last_slash + 1);
        if (basename.empty()) {
            basename = "./";
        }
        paths_.push_front(basename);
        // Run the filename alone through the regular search now, since that
        // will also supply the default extension.
        return open(file_path.substr(last_slash + 1));
    }

    SourceFilePtr Wellspring::open(string source_name) {
        auto result = sources_.find(source_name);
        if (result != sources_.end()) {
            everBeenOpened_.insert(source_name);
            return result->second;
        }
        for (auto p : paths_) {
            string try_path = p;
            assert(!try_path.empty());
            if (try_path.rfind('/') != (try_path.size() - 1)) {
                try_path += '/';
            }
            try_path += source_name;
            if (source_name.find('.') == string::npos) {
                try_path += ".ach";
            }
            unique_ptr<ifstream> input(new ifstream(try_path.c_str()));
            if (input->is_open()) {
                // Now that it's been tested for openness, move it to a higher abstraction
                stream_ptr source_stream{input.release()};
                SourceFilePtr source{new SourceFile{try_path, source_stream}};
                sources_[try_path] = source;
                return source;
            }
        }
        return nullptr;
    }

    bool Wellspring::hasNeverBeenOpened(std::string source_name) const {
        return everBeenOpened_.count(source_name) == 0;
    }

    void Wellspring::put(std::string source_name, SourceFilePtr source) {
        sources_.insert(std::make_pair(source_name, source));
    }

    void Wellspring::close(SourceFilePtr source) {
        for (auto p = sources_.begin(); p != sources_.end(); ++p) {
            if (p->second == source) {
                sources_.erase(p);
                return;
            }
        }
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
