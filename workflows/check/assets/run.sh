#!/usr/bin/env bash
set -euo pipefail
dockpipe_bin="${DOCKPIPE_BIN:-dockpipe}"
test -s AGENTS.md
test -s README.md
test -s dockpipe.config.json
for config in workflows/*/config.yml; do
  "$dockpipe_bin" workflow validate "$config"
done
printf '%s\n' 'Repository scaffold and workflow declarations passed; application/build proof is separate.'
