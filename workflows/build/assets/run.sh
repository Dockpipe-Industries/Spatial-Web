#!/usr/bin/env bash
set -euo pipefail
dockpipe_bin="${DOCKPIPE_BIN:-dockpipe}"
source_dir="$("$dockpipe_bin" scope source .)"
cmake -S "$source_dir" -B native -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build native --config Release --parallel 2
cmake --install native --config Release --prefix "$PWD/sdk"
