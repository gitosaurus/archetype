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
#include <iterator> // for std::ostream_iterator

#include "Serialization.hh"

namespace archetype {

    void inspect_universe(Storage& in, std::ostream& out);

}

#endif // __archetype__inspect_universe__
