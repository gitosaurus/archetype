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

  // One turn.  Returns the narrative text, which includes the prompt and the
  // echoed command, exactly as the native interpreter's transcript would.
  std::string arch_turn(std::string input, int width) {
    try {
      last_error.clear();
      return run_turn(input, width);
    } catch (const std::exception& e) {
      last_error = e.what();
      return std::string();
    } catch (...) {
      last_error = "Unknown error taking a turn";
      return std::string();
    }
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
