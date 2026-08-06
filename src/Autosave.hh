//
//  Autosave.hh
//  archetype
//
//  Keeps a .acx file current during ordinary play, so that the state of a game
//  is always on disk rather than only at the moments a player remembers to type
//  "save".  The explicit save verb remains, for named checkpoints.
//

#ifndef __archetype__Autosave__
#define __archetype__Autosave__

#include <string>
#include <string_view>

namespace archetype {

    class Autosave {
    public:
        enum class Cadence {
            Turn,   // after every completed turn, and again at exit
            Exit    // only as the interpreter is going away
        };

        static Autosave& instance();
        static void destroy();

        // gorreven.acx -> gorreven.save.acx, so that the pristine binary a game
        // is distributed as is never the thing being overwritten.  A file that
        // is already a save keeps its own name, so resuming a save goes on
        // updating that same file rather than making a save of a save.
        static std::string deriveTarget(std::string_view game_path);

        // Only meaningful once a universe has been loaded or compiled: arming
        // resolves the ids that identify a turn boundary.  A game with no
        // 'main' object or no 'UPDATE' message cannot be watched per turn, and
        // falls back to Cadence::Exit with a warning.
        void arm(std::string target, Cadence cadence, bool keep_backup);

        // Re-resolve those ids after a game-level 'load' has replaced the whole
        // object graph underneath us.
        void rearm();

        void disarm();

        bool armed() const { return armed_; }
        const std::string& target() const { return target_; }
        Cadence cadence() const { return cadence_; }

        // The dispatch hook's fast path: a static bool, so that a game running
        // without autosave pays one load and one predictable branch per message
        // send rather than a singleton lookup.
        static bool watchingTurns() { return watchingTurns_; }

        // Called from Object::dispatch once watchingTurns() is known true.
        void noteDispatch(int object_id, int message_id);

        // Write the current state to the target.  On failure, reports once and
        // disarms: an autosave that has quietly stopped working is worse than
        // one that says so, and complaining every turn would be unusable.
        bool checkpoint();

    private:
        bool armed_;
        Cadence cadence_;
        bool keepBackup_;
        std::string target_;
        int mainObjectId_;
        int updateMessageId_;

        static Autosave* instance_;
        static bool watchingTurns_;

        Autosave();
        Autosave(const Autosave&) = delete;
        Autosave& operator=(const Autosave&) = delete;

        void resolveIds_();
        bool turnBoundaryResolved_() const;
        void updateWatch_();
        bool fail_(std::string_view reason);
    };

}

#endif /* defined(__archetype__Autosave__) */
