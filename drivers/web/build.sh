#!/bin/sh
#
# Assemble the browser build into build-wasm/web.
#
# Two compilers are involved and the order matters: the native interpreter is
# what turns games/*.arch into the .acx files the page fetches, so it has to be
# built first even though nothing native ends up being served.
#
# Requires emcc on PATH (see https://emscripten.org/docs/getting_started).

set -eu

root=$(cd "$(dirname "$0")/../.." && pwd)
out="$root/build-wasm/web"

cd "$root"

if ! command -v emcmake > /dev/null 2>&1; then
    echo "build.sh: emcmake not found; source your emsdk_env.sh first" >&2
    exit 1
fi

echo "==> native interpreter"
cmake -S src -B build
cmake --build build --parallel

echo "==> wasm interpreter"
emcmake cmake -S src -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm --parallel

echo "==> page"
mkdir -p "$out"
cp drivers/web/index.html drivers/web/play.js drivers/web/games.json "$out/"
cp build-wasm/archetype.js build-wasm/archetype.wasm "$out/"

echo "==> games"
# Each .arch named in games.json, compiled to the .acx the page fetches.
python3 - "$out" <<'PY'
import json, pathlib, subprocess, sys

out = pathlib.Path(sys.argv[1])
manifest = json.loads(pathlib.Path('drivers/web/games.json').read_text())
for game in manifest['games']:
    target = out / game['acx']
    subprocess.run(
        ['./build/archetype', '--silent',
         '--source=' + game['source'],
         '--include=games',
         '--create=' + str(target)],
        check=True)
    print('    {} -> {} ({:,} bytes)'.format(
        game['source'], game['acx'], target.stat().st_size))
PY

echo
echo "Built $out"
echo "Serve it with:  (cd $out && python3 -m http.server 8000)"
