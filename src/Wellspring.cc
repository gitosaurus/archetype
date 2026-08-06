//
//  Wellspring.cc
//  archetype
//
//  Created by Derek Jones on 6/4/14.
//  Copyright (c) 2014, 2022 Derek Jones. All rights reserved.
//

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <cstdlib>
#include <stdexcept>

#include "Wellspring.hh"

using namespace std;
namespace fs = std::filesystem;

namespace archetype {
    SourceFilePtr Wellspring::primarySource(string_view file_path) {
        fs::path source_path{file_path};
        fs::path directory = source_path.parent_path();
        paths_.push_front(directory.empty() ? "." : directory.string());
        // Run the filename alone through the regular search now, since that
        // will also supply the default extension.
        return open(source_path.filename().string());
    }

    void Wellspring::addSearchPath(std::string directory_path) {
        paths_.push_back(std::move(directory_path));
    }

    SourceFilePtr Wellspring::open(string_view source_name) {
        if (auto result = sources_.find(source_name); result != sources_.end()) {
            everBeenOpened_.emplace(source_name);
            return result->second;
        }
        for (const auto& p : paths_) {
            fs::path try_path = fs::path{p} / source_name;
            if (try_path.extension().empty()) {
                try_path += ".arch";
            }
            auto input = make_unique<ifstream>(try_path);
            if (input->is_open()) {
                // Now that it's been tested for openness, move it to a higher abstraction
                stream_ptr source_stream{input.release()};
                SourceFilePtr source{make_shared<SourceFile>(try_path.string(), source_stream)};
                sources_[try_path.string()] = source;
                // Record the name that was asked for, not the path it resolved
                // to.  hasNeverBeenOpened is asked the question the way the
                // includer wrote it, and sources_ is keyed by the resolved
                // path -- and emptied again as soon as the file has been read
                // -- so this set is the only lasting record that the question
                // has already been answered once.
                everBeenOpened_.emplace(source_name);
                return source;
            }
        }
        return nullptr;
    }

    bool Wellspring::hasNeverBeenOpened(std::string_view source_name) const {
        return not everBeenOpened_.contains(source_name);
    }

    void Wellspring::put(std::string source_name, SourceFilePtr source) {
        sources_.insert(std::make_pair(std::move(source_name), std::move(source)));
    }

    void Wellspring::close(SourceFilePtr source) {
        erase_if(sources_, [&source](auto const& entry) { return entry.second == source; });
    }

    void Wellspring::closeAll() {
        sources_.clear();
        everBeenOpened_.clear();
    }

    Wellspring* Wellspring::instance_ = nullptr;

    Wellspring::Wellspring() {
    }

    Wellspring::~Wellspring() {
        closeAll();
    }

    Wellspring& Wellspring::instance() {
        if (not instance_) {
            instance_ = new Wellspring;
        }
        return *instance_;
    }

    void Wellspring::destroy() {
        delete instance_;
        instance_ = nullptr;
    }
}
