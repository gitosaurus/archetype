#!/usr/bin/env bash
#
# Rewrite the goldens from the interpreter as it stands right now.
#
# Run this only when a change to the compiler or the binary format was
# intended.  The .ttl diff is the part a reviewer can actually read, so commit
# it alongside the .acx and say in the message what moved and why.
#
# Usage:  tests/golden/regenerate.sh [path-to-archetype]
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

for game in "${GAMES[@]}"; do
    # stderr is swallowed on purpose:  a debug build's SHOW() narrates the
    # include path, and it is not part of what is being pinned.
    "$ARCHETYPE" --silent --source="$ROOT/games/$game.arch" --include="$ROOT/games" \
                 --create="$GOLDEN/$game.acx" >/dev/null 2>/dev/null
    "$ARCHETYPE" --silent --inspect="$GOLDEN/$game.acx" --full > "$GOLDEN/$game.ttl" 2>/dev/null
    echo "regenerated $game: $(wc -c < "$GOLDEN/$game.acx" | tr -d ' ') bytes," \
         "$(wc -l < "$GOLDEN/$game.ttl" | tr -d ' ') lines of Turtle"
done

echo
echo "Now run tests/golden/check.sh to confirm, then review 'git diff' on the .ttl."
