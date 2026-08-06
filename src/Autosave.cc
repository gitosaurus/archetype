//
//  Autosave.cc
//  archetype
//

#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "Autosave.hh"
#include "FileStorage.hh"
#include "Object.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {

    Autosave* Autosave::instance_ = nullptr;
    bool Autosave::watchingTurns_ = false;

    Autosave::Autosave():
    armed_{false},
    cadence_{Cadence::Turn},
    keepBackup_{true},
    mainObjectId_{-1},
    updateMessageId_{-1}
    { }

    Autosave& Autosave::instance() {
        if (not instance_) {
            instance_ = new Autosave();
        }
        return *instance_;
    }

    void Autosave::destroy() {
        delete instance_;
        instance_ = nullptr;
        watchingTurns_ = false;
    }

    string Autosave::deriveTarget(string_view game_path) {
        constexpr string_view SaveSuffix = ".save.acx";
        if (game_path.ends_with(SaveSuffix)) {
            return string{game_path};
        }
        auto last_slash = game_path.find_last_of('/');
        auto last_dot = game_path.rfind('.');
        bool has_extension = last_dot != string_view::npos and
                             (last_slash == string_view::npos or last_dot > last_slash);
        string_view stem = has_extension ? game_path.substr(0, last_dot) : game_path;
        string target{stem};
        target += SaveSuffix;
        return target;
    }

    void Autosave::resolveIds_() {
        // find rather than index: index() inserts on a miss, and an 'UPDATE'
        // entry conjured up here would be written into every save that follows.
        updateMessageId_ = Universe::instance().Messages.find("UPDATE");
        mainObjectId_ = -1;
        if (ObjectPtr main_object = Universe::instance().getObject("main")) {
            mainObjectId_ = main_object->id();
        }
    }

    bool Autosave::turnBoundaryResolved_() const {
        return mainObjectId_ >= 0 and updateMessageId_ >= 0;
    }

    void Autosave::updateWatch_() {
        watchingTurns_ = armed_ and cadence_ == Cadence::Turn and turnBoundaryResolved_();
    }

    void Autosave::arm(string target, Cadence cadence, bool keep_backup) {
        target_ = std::move(target);
        cadence_ = cadence;
        keepBackup_ = keep_backup;
        armed_ = true;
        resolveIds_();
        if (cadence_ == Cadence::Turn and not turnBoundaryResolved_()) {
            cerr << "Cannot autosave every turn: this game has no 'main' object "
                    "answering 'UPDATE'.  Autosaving at exit instead." << endl;
            cadence_ = Cadence::Exit;
        }
        updateWatch_();
    }

    void Autosave::rearm() {
        if (not armed_) {
            return;
        }
        // Quietly: a game-level 'load' happens mid-play, and a warning here
        // would land in the middle of the narrative.  Losing the turn boundary
        // degrades to an exit-time save rather than to nothing.
        resolveIds_();
        updateWatch_();
    }

    void Autosave::disarm() {
        armed_ = false;
        watchingTurns_ = false;
    }

    void Autosave::noteDispatch(int object_id, int message_id) {
        if (object_id == mainObjectId_ and message_id == updateMessageId_) {
            checkpoint();
        }
    }

    bool Autosave::checkpoint() {
        if (not armed_) {
            return false;
        }
        string error;
        if (not writeUniverseAtomically(target_, keepBackup_, error)) {
            return fail_(error);
        }
        return true;
    }

    bool Autosave::fail_(string_view reason) {
        cerr << format("Autosave to {} failed: {}", target_, reason) << endl;
        cerr << "Autosave is off for the rest of this session." << endl;
        disarm();
        return false;
    }

}
