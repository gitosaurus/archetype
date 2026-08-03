//
//  TurnInput.hh
//  archetype
//
//  Created by Derek Jones on 2026-08-03.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__TurnInput__
#define __archetype__TurnInput__

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "UserInput.hh"

namespace archetype {

  // Thrown when a turn asks for input the driver did not supply.  Deliberately
  // not derived from std::exception: the blanket handlers at the embind
  // boundary and around SITREP would otherwise swallow it, and report a turn
  // that merely stopped to ask a question as one that failed.
  struct NeedsInput {
    enum class Want { Line, Key };
    Want want;
  };

  // One turn's input, handed over up front, for a driver with nowhere to wait.
  // A browser page cannot block on a keystroke, so instead of waiting it runs
  // the turn again with one more item in hand; running out of items is
  // therefore not end-of-input but a request for another one.
  //
  // An absent item is the player declining to answer -- Escape in a browser,
  // ^D at a console -- and reports EOF exactly as ConsoleInput does, so a game
  // testing 'read = UNDEFINED' behaves the same either way.  Declining is
  // absorbing: every later read in the turn reports EOF too, which is what
  // guarantees a driver can always bring a turn to an end.
  class TurnInput : public IUserInput {
  public:
    using Item = std::optional<std::string>;

    explicit TurnInput(std::vector<Item> items):
      items_{std::move(items)}
    { }

    virtual ~TurnInput() { }

    virtual char getKey() override {
      if (declined_) return '\0';
      if (exhausted()) throw NeedsInput{NeedsInput::Want::Key};
      const Item& item = items_[next_++];
      if (not item) {
        declined_ = true;
        return '\0';
      }
      // A keystroke arrives as a one-character item; an empty one is how a page
      // reports that 'press any key' was answered with the space bar.
      return item->empty() ? ' ' : item->front();
    }

    virtual std::string getLine() override {
      if (declined_) return std::string();
      if (exhausted()) throw NeedsInput{NeedsInput::Want::Line};
      const Item& item = items_[next_++];
      if (not item) {
        declined_ = true;
        return std::string();
      }
      return *item;
    }

    // Only declining is EOF.  Merely having run out never surfaces here,
    // because reading past the end throws before anyone can ask.
    virtual bool atEOF() const override { return declined_; }

  private:
    bool exhausted() const { return next_ >= items_.size(); }

    std::vector<Item> items_;
    std::size_t next_ = 0;
    bool declined_ = false;
  };

}

#endif /* defined(__archetype__TurnInput__) */
