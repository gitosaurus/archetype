//
//  update_universe.hh
//  archetype
//
//  Created by Derek Jones on 2022-03-21
//  Copyright (c) 2022 Derek Jones. All rights reserved.
//

#ifndef __archetype__update_universe__
#define __archetype__update_universe__

#include <optional>
#include <string>
#include <string_view>
#include <vector>
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

  // Everything the player has supplied toward one turn: the command, then an
  // answer for each mid-turn 'read' or 'key' a previous attempt ran into.  An
  // absent item is the player declining to answer.
  using TurnInputs = std::vector<std::optional<std::string>>;

  // What one turn came back with.  A turn that stopped to ask for input has
  // not happened -- the universe is exactly as it was -- and 'text' is
  // everything the game wrote before it asked, which ends with the prompt.
  struct TurnResult {
    enum class Status { Complete, NeedsLine, NeedsKey };
    Status status = Status::Complete;
    std::string text;
  };

  // One turn for a driver that cannot block waiting for input.  Where run_turn
  // treats the end of its input as EOF, this treats it as a question, and
  // hands the question back for the driver to answer and call again.
  TurnResult run_turn_collecting(TurnInputs inputs, int width = 0);

}

#endif
