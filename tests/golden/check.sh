#!/usr/bin/env bash
#
# Compare what the interpreter compiles today against the checked-in goldens.
#
# Two oracles, deliberately of different kinds:
#
#   .ttl   the world state, as RDF/Turtle.  Text, so a failure diffs readably
#          and names the object that drifted.  This is the one to read first.
#
#   .acx   the compiled binary, byte for byte.  Says nothing about *what*
#          changed, but catches the layer the Turtle cannot see:  varint
#          encoding, field order, id assignment.
#
# Only compiled output is pinned.  A post-turn save is NOT a valid golden --
# gorreven's turn is nondeterministic by design, because the game rolls dice.
#
# Usage:  tests/golden/check.sh [path-to-archetype]
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GOLDEN="$ROOT/tests/golden"
ARCHETYPE="${1:-${ARCHETYPE:-$ROOT/build/archetype}}"

GAMES=(cherry bare)

if [ ! -x "$ARCHETYPE" ]; then
    echo "no interpreter at $ARCHETYPE -- build it first, or pass the path" >&2
    exit 2
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# A debug build compiles in main.cc's SHOW(), which narrates the include path to
# stderr on every run.  Swallow the interpreter's chatter, but keep it if the
# command actually fails -- that is when it is worth reading.
run() {
    if ! "$@" > "$work/out.log" 2> "$work/err.log"; then
        echo "FAIL  interpreter exited nonzero: $*"
        sed 's/^/      /' < "$work/err.log"
        return 1
    fi
}

failures=0

for game in "${GAMES[@]}"; do
    source_file="$ROOT/games/$game.arch"

    # Compile twice.  This is worth doing on its own:  it catches a compiler
    # that has stopped being deterministic even when the goldens are stale, and
    # it distinguishes "the output moved" from "the output is unstable".
    run "$ARCHETYPE" --silent --source="$source_file" --include="$ROOT/games" \
                     --create="$work/$game.a.acx" || { failures=$((failures + 1)); continue; }
    run "$ARCHETYPE" --silent --source="$source_file" --include="$ROOT/games" \
                     --create="$work/$game.b.acx" || { failures=$((failures + 1)); continue; }

    if ! cmp -s "$work/$game.a.acx" "$work/$game.b.acx"; then
        echo "FAIL  $game: two compiles of identical source produced different bytes."
        echo "      The compiler is not deterministic; the goldens are not the problem."
        failures=$((failures + 1))
        continue
    fi

    run "$ARCHETYPE" --silent --inspect="$work/$game.a.acx" --full \
        || { failures=$((failures + 1)); continue; }
    cp "$work/out.log" "$work/$game.ttl"

    if [ ! -f "$GOLDEN/$game.ttl" ] || [ ! -f "$GOLDEN/$game.acx" ]; then
        echo "FAIL  $game: no golden on disk.  Run tests/golden/regenerate.sh."
        failures=$((failures + 1))
        continue
    fi

    game_failed=0

    if ! diff -u "$GOLDEN/$game.ttl" "$work/$game.ttl" > "$work/$game.ttl.diff"; then
        echo "FAIL  $game: world state differs from the golden."
        sed 's/^/      /' < "$work/$game.ttl.diff" | head -40
        game_failed=1
    fi

    if ! cmp -s "$GOLDEN/$game.acx" "$work/$game.a.acx"; then
        if [ "$game_failed" -eq 0 ]; then
            # The interesting case:  same world, different bytes.  Either the
            # binary format changed, or the bytes depend on the toolchain --
            # which is exactly what running this on a second platform tests.
            echo "FAIL  $game: identical world state, but the .acx bytes differ."
            echo "      The format changed, or the bytes are not toolchain-independent."
            echo "      golden $(wc -c < "$GOLDEN/$game.acx" | tr -d ' ') bytes," \
                 "built $(wc -c < "$work/$game.a.acx" | tr -d ' ') bytes;" \
                 "first difference at $(cmp "$GOLDEN/$game.acx" "$work/$game.a.acx" 2>&1 | sed 's/.*char //')"
        else
            echo "      ...and the .acx bytes differ too, as follows from the above."
        fi
        game_failed=1
    fi

    if [ "$game_failed" -eq 0 ]; then
        echo "ok    $game: deterministic, world state and bytes match the golden."
    else
        failures=$((failures + 1))
    fi
done

echo
if [ "$failures" -ne 0 ]; then
    echo "$failures of ${#GAMES[@]} golden checks failed."
    echo "If the change was intended, regenerate with tests/golden/regenerate.sh"
    echo "and review the .ttl diff in the commit -- that is the reviewable half."
    exit 1
fi

echo "All ${#GAMES[@]} golden checks passed."
