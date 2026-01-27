//
//  update_universe.hh
//  archetype
//
//  Created by Derek Jones on 2022-03-21
//  Copyright (c) 2022 Derek Jones. All rights reserved.
//

#ifndef __archetype__update_universe__
#define __archetype__update_universe__

#include <string>
#include "Value.hh"
#include "Serialization.hh"

namespace archetype {

  Value dispatch_to_universe(std::string message);
  std::string update_universe(Storage& in, Storage& out, std::string input, int width = 0);
  
}

#endif
