#!/usr/bin/env bash
set -euo pipefail
dockpipe_bin="${DOCKPIPE_BIN:-dockpipe}"
source_dir="$("$dockpipe_bin" scope source .)"
variant="${SPATIAL_WEB_VARIANT:-projection}"
case "$variant" in
  browser) composition=OFF; projection=OFF ;;
  projection) composition=ON; projection=ON ;;
  *) echo 'Unknown Spatial-Web SDK variant.' >&2; exit 1 ;;
esac
mkdir -p "$variant"
cd "$variant"
cmake -S "$source_dir" -B native -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL \
  -DSPATIAL_WEB_BUILD_COMPOSITION="$composition" -DSPATIAL_WEB_BUILD_PROJECTION="$projection"
cmake --build native --config Release --parallel 2
ctest --test-dir native -C Release --output-on-failure
cmake --install native --config Release --prefix "$PWD/sdk"
cmake -S "$source_dir/tests/consumer" -B consumer -DCMAKE_PREFIX_PATH="$PWD/sdk" -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL \
  -DSPATIAL_WEB_TEST_COMPOSITION="$composition" -DSPATIAL_WEB_TEST_PROJECTION="$projection"
cmake --build consumer --config Release --parallel 2
qt_runtime="$(cat native/qt-runtime-directory.txt)"
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software
export QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu
case "$(uname -s)" in
  Linux*)
    LD_LIBRARY_PATH="$qt_runtime${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ./consumer/spatial-web-consumer
    ;;
  Darwin*)
    qt_frameworks="$(cd "$qt_runtime/../../.." && pwd)"
    DYLD_FRAMEWORK_PATH="$qt_frameworks${DYLD_FRAMEWORK_PATH:+:$DYLD_FRAMEWORK_PATH}" ./consumer/spatial-web-consumer
    ;;
  MINGW*|MSYS*)
    PATH="$PWD/sdk/bin:$qt_runtime:$PATH" ./consumer/Release/spatial-web-consumer.exe
    ;;
  *)
    echo 'Installed SDK proof requires Linux, macOS or Windows.' >&2
    exit 1
    ;;
esac

if [[ -n "${SPATIAL_WEB_CANDIDATE_OUTPUT:-}" ]]; then
  "${SPATIAL_WEB_PYTHON:-python3}" "$source_dir/release/pack-sdk.py" --sdk "$PWD/sdk" \
    --output "$SPATIAL_WEB_CANDIDATE_OUTPUT" --variant "$variant" \
    --build-directory "$PWD/native" --platform "${SPATIAL_WEB_PLATFORM:?producer platform}" --source-sha "${GITHUB_SHA:?exact source SHA}"
fi
