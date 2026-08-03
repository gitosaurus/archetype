//
//  Random.hh
//  archetype
//
//  Created by Derek Jones on 2026-08-03.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__Random__
#define __archetype__Random__

#include <cstdint>

namespace archetype {

  // splitmix64, written out rather than reached for.  std::mt19937 is fully
  // specified by the standard, but std::uniform_int_distribution is not:
  // libc++ and libstdc++ can draw different numbers from identical engine
  // state, and a transcript that depends on which platform produced it is no
  // transcript at all.  Eight bytes of state, so a universe can carry it in a
  // save without anyone noticing.
  class Random {
  public:
    Random() { }
    explicit Random(std::uint64_t state): state_{state} { }

    std::uint64_t state() const { return state_; }
    void setState(std::uint64_t state) { state_ = state; }

    std::uint64_t next() {
      state_ += 0x9E3779B97F4A7C15ULL;
      std::uint64_t z = state_;
      z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
      return z ^ (z >> 31);
    }

    // A number in [1, bound], the way Archetype's '?' has always counted.
    // Rejection rather than a bare modulo: the low residues would otherwise
    // come up slightly more often, which a die roll would eventually show.
    int upTo(int bound) {
      std::uint64_t range = static_cast<std::uint64_t>(bound);
      std::uint64_t limit = UINT64_MAX - (UINT64_MAX % range);
      std::uint64_t drawn;
      do {
        drawn = next();
      } while (drawn >= limit);
      return static_cast<int>(drawn % range) + 1;
    }

  private:
    std::uint64_t state_ = 0;
  };

}

#endif /* defined(__archetype__Random__) */
