#!/usr/bin/env bash
set -euo pipefail
dockpipe_bin="${DOCKPIPE_BIN:-dockpipe}"
source_dir="$("$dockpipe_bin" scope source .)"
: "${SPATIAL_WEB_FLATPAK_OUTPUT:?An isolated output directory is required}"
python3 -B "$source_dir/.github/scripts/build-flatpak-browser-sdk.py" --output "$SPATIAL_WEB_FLATPAK_OUTPUT"
