//
//  wasm_api.cc
//  archetype
//
//  The browser's way in.  A page holds a universe resident in wasm memory and
//  sends it one 'UPDATE' at a time, which is the same shape the Cloud Run
//  driver has -- minus the network hop, and minus the round trip through bytes
//  that the network hop forced on every turn.  Serialization happens only when
//  somebody asks for a save.
//
//  Created by Derek Jones on 2026-08-01.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#include <exception>
#include <string>
#include <utility>
#include <vector>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "Autosave.hh"
#include "Keywords.hh"
#include "Serialization.hh"
#include "Universe.hh"
#include "Wellspring.hh"
#include "update_universe.hh"

using namespace emscripten;

namespace archetype {

  namespace {

    // The most recent failure, so that a page can say what went wrong rather
    // than only that something did.  An exception that reached the embind
    // boundary would trap the module, so nothing below is allowed to throw.
    std::string last_error;

    void tear_down_singletons() {
      // The order ~Session uses, minus TestRegistry: the test suites are not
      // part of this build.
      Autosave::destroy();
      Universe::destroy();
      Wellspring::destroy();
      Keywords::destroy();
    }

  } // namespace

  // Deserialize .acx bytes into a fresh Universe.  Because it tears down first,
  // this is also how a page restarts a game or switches to another one.
  bool arch_load(val acx_bytes) {
    try {
      last_error.clear();
      std::vector<Storage::Byte> bytes =
        convertJSArrayToNumberVector<Storage::Byte>(acx_bytes);
      if (bytes.empty()) {
        last_error = "No game data to load";
        return false;
      }
      tear_down_singletons();
      MemoryStorage in;
      in.bytes() = std::move(bytes);
      load_universe(in);
      return true;
    } catch (const std::exception& e) {
      last_error = e.what();
      return false;
    } catch (...) {
      last_error = "Unknown error loading game";
      return false;
    }
  }

  // One turn, as { status, text }.  'inputs' is everything the player has
  // supplied toward it: the command, plus an answer for each mid-turn 'read'
  // or 'key' a previous attempt ran into.  A null answer is the player
  // declining, which the interpreter reports as end-of-input.
  //
  // A turn that comes back needing more has not happened -- the universe is
  // untouched and 'text' ends with the prompt the game stopped at -- so the
  // page collects the answer, appends it, and calls again with the longer
  // list.  What finally comes back is the whole turn's narrative, prompt and
  // answer echoed in place, exactly as the native transcript would have it.
  val arch_turn(val inputs, int width) {
    val result = val::object();
    try {
      last_error.clear();
      TurnInputs items;
      const unsigned count = inputs["length"].as<unsigned>();
      items.reserve(count);
      for (unsigned i = 0; i < count; ++i) {
        val item = inputs[i];
        if (item.isNull() or item.isUndefined()) {
          items.emplace_back();
        } else {
          items.emplace_back(item.as<std::string>());
        }
      }
      TurnResult turn = run_turn_collecting(std::move(items), width);
      std::string status = "complete";
      if (turn.status == TurnResult::Status::NeedsLine) {
        status = "needs_line";
      } else if (turn.status == TurnResult::Status::NeedsKey) {
        status = "needs_key";
      }
      result.set("status", status);
      result.set("text", turn.text);
      return result;
    } catch (const std::exception& e) {
      last_error = e.what();
    } catch (...) {
      last_error = "Unknown error taking a turn";
    }
    result.set("status", std::string("error"));
    result.set("text", std::string());
    return result;
  }

  // True once the game is over.  A page must check this before every turn:
  // dispatching into an ended universe throws, and the throw is the only thing
  // standing between a finished game and a trapped module.
  bool arch_ended() {
    try {
      return Universe::instance().ended();
    } catch (...) {
      return true;
    }
  }

  // Serialize the current state to a Uint8Array -- a real .acx that the desktop
  // interpreter resumes with --perform.
  val arch_save() {
    try {
      last_error.clear();
      // A fresh one every time: MemoryStorage::write appends, so a reused
      // instance would hand back this save concatenated onto the last.
      MemoryStorage out;
      save_universe(out);
      // typed_memory_view is a window onto wasm memory, which the next
      // allocation may move; slice() copies it into a JS-owned array.
      return val(typed_memory_view(out.bytes().size(), out.bytes().data()))
        .call<val>("slice");
    } catch (const std::exception& e) {
      last_error = e.what();
      return val::null();
    } catch (...) {
      last_error = "Unknown error saving game";
      return val::null();
    }
  }

  std::string arch_last_error() {
    return last_error;
  }

  EMSCRIPTEN_BINDINGS(archetype) {
    function("arch_load", &arch_load);
    function("arch_turn", &arch_turn);
    function("arch_ended", &arch_ended);
    function("arch_save", &arch_save);
    function("arch_last_error", &arch_last_error);
  }

} // namespace archetype
