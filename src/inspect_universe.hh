//
//  inspect_universe.hh
//  archetype
//
//  Created by Derek Jones on 2023-07-08
//  Copyright (c) 2023 Derek Jones. All rights reserved.
//
#ifndef __archetype__inspect_universe__
#define __archetype__inspect_universe__

#include <iostream>

#include "Serialization.hh"

namespace archetype {

    // Deserialize `in` into the Universe, then dump its full RDF/Turtle.
    void inspect_universe(Storage& in, std::ostream& out, bool include_methods = false);

    // Dump the current Universe as RDF/Turtle (no deserialization step).
    void dump_universe_rdf(std::ostream& out, bool include_methods = false);

    // Emit the parser's current state as a Turtle block describing
    // archetype:parser, an instance of archetype:SystemParser.  If
    // `with_prefixes` is true, emits the @base/@prefix preamble first so the
    // output is a self-contained Turtle fragment.
    void write_parser_rdf(std::ostream& out, bool with_prefixes = false);

}

#endif // __archetype__inspect_universe__
