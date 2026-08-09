//
//  CommandLine.hh
//  archetype
//
//  Created by Derek Jones on 2026-08-08.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__CommandLine__
#define __archetype__CommandLine__

#include <iosfwd>
#include <list>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace archetype {

    // What may follow an option's name.  This distinction is the whole reason
    // the table exists: a parser that does not know an option's arity cannot
    // tell that option's value from an unrelated argument, which is why
    // "--option=value" needs no table and "--option value" cannot do without
    // one.
    enum class Takes {
        Nothing,    // --silent
        Optional,   // --create, or --create=file.acx
        Required,   // --source=file.arch, or --source file.arch
    };

    struct Option {
        std::string_view name;
        Takes takes = Takes::Nothing;
        bool nested = false;        // listed indented, beneath what it modifies
        std::string_view argname;   // stands in for the value in the listing
        std::string_view help;      // a '\n' starts a continuation line
    };

    std::span<const Option> allOptions();

    // By value rather than by pointer: an Option is a handful of string_views
    // onto string literals, so the copy is free and cannot outlive what it
    // points at.  (C++20 has no optional<T&>, so a heavier Option would have
    // to go back to a pointer or a reference_wrapper.)
    std::optional<Option> findOption(std::string_view name);

    void usage(std::ostream& out);

    // The command line, partitioned: named options on one side, positional
    // arguments on the other.  Consuming code erases what it recognizes, so
    // whatever remains at the end was not applicable to the mode selected.
    struct CommandLine {
        std::map<std::string, std::string> opts;
        std::list<std::string> args;
        std::string error;
        bool ok() const { return error.empty(); }
    };

    CommandLine parseCommandLine(std::list<std::string> words);
}

#endif /* defined(__archetype__CommandLine__) */
