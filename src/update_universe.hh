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
#include <string_view>
#include "Value.hh"
#include "Serialization.hh"

namespace archetype {

  Value dispatch_to_universe(std::string_view message);

  // The three phases of a turn, separately available so that a driver which
  // keeps the universe resident between turns -- the browser, where there is
  // no process boundary to serialize across -- can run just the middle one.
  void load_universe(Storage& in);
  std::string run_turn(std::string input, int width = 0, bool sitrep = false,
                       bool inspect = false);
  void save_universe(Storage& out);

  // Load, take one turn, save: what a stateless driver wants, and what
  // --update does.
  std::string update_universe(Storage& in, Storage& out, std::string input,
                              int width = 0, bool sitrep = false,
                              bool inspect = false);

}

#endif
