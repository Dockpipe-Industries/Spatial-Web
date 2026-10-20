# Binary SDK candidates

Browser SDK qualification is independent of Scene, ForgePipe, private source and
private credentials. `.github/workflows/browser-sdk.yml` builds native Qt-only
binary SDKs for Linux amd64/arm64, macOS Intel/ARM and Windows amd64. It pins aqt
3.3.0, Qt 6.10.3 and committed native Dockpipe 0.6.11 binary checksums. Windows uses
MSVC 2022 Release and the dynamic /MD runtime matching the desktop/MSI toolchain.
The root browser variant runs inherited tests, installs, packages regular files,
then configures/builds/runs a relocated archive consumer before writing its receipt.
Only Linux amd64 has local qualification evidence; the other runners remain a
hosted qualification gate until the matrix executes successfully.

Browser assets use platform-prefixed archive, `.files.json` and `.release.json`
names to coexist in one candidate. Projection uses `sdk-files.json`,
`sdk-release.json` and `SpatialWebProjectionSDK-0.1.0-linux-amd64.tar.gz` for the
existing bounded SDK resolver. Both metadata schemas include source commit, exact
Qt version, actual CMake compiler ID/version, C++17 ABI, archive hash/bytes and
file inventory entries with path/bytes/SHA-256. Archive aliases are materialized
as regular files so bounded SDK consumers do not need link extraction.

Projection qualification is Linux amd64 only on the admitted Ubuntu 22.04/GNU 11
ABI. `.github/workflows/candidate.yml` explicitly skips qualification/publication,
with a recorded pending notice, until a separate approved PR adds
`release/projection-dependencies.lock.json`. That lock has schema 1, platform
`linux-amd64`, qt_version `6.10.3`, and a `dependencies` map with exactly
SpatialPipe, SpatialPipeScene and ForgePipe. Each entry binds version `0.1.0`, a
public immutable GitHub release archive URL and its lowercase SHA-256. No private
source fallback, authentication or latest-version lookup is available.

Both workflows publish only after an explicit `workflow_dispatch` with publish
selected from dev. Publisher jobs attest the exact archive/inventory/receipt bytes,
validate the complete expected asset set and source identity, create a source-bound
prerelease candidate with latest=false, and read back tag/assets. Existing bytes
are never replaced. PR runs have no publisher path. No workflow has been dispatched
or remote candidate published by this local task.

Qt/WebEngine renderer, resources, plugins and transitive runtime SDKs are separate
qualified artifacts; SDK archives are not self-contained application installers.
Native application packaging and catalog installation/update/disable/removal/
rollback remain downstream gates. Licenses and runtime attribution remain attached
to the corresponding distributed artifact.
