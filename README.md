# Spatial-Web

Spatial-Web owns a reusable ordinary browser, a separate spatial content discovery
and composition SDK, and the standalone ordinary browser shell.

- `SpatialWebBrowser` 0.1.0: installed CMake target `SpatialWeb::Browser`.
- `SpatialWebComposition` 0.1.0: installed CMake target `SpatialWeb::Composition`.
- `SpatialWebProjection` 0.1.0: installed CMake target `SpatialWeb::Projection`.
- `spatial-web`: ordinary and spatial browser shell using the shared components.

Build through `workflows/build/config.yml` and test through
`workflows/test/config.yml` using the pinned Dockpipe CLI. Supply the retained Qt
6.10.3 and released SpatialPipe/SpatialPipeScene/ForgePipe prefixes through CMAKE_PREFIX_PATH.
For an ordinary-browser-only build, configure with
`-DSPATIAL_WEB_BUILD_COMPOSITION=OFF`; no spatial SDK is needed.

See [composition boundaries](docs/composition-boundary.md),
[browser extraction](docs/browser-extraction.md), LICENSE and NOTICE.
Local candidate artifacts are not a hosted release or native interaction proof.

The separate `flatpak-kde-6.10` Browser ABI profile exports
`SpatialWebFlatpakBrowser` and the same `SpatialWeb::Browser` target. It binds the
official KDE Core/Gui/Widgets/Network/PrintSupport 6.10.3 distribution and BaseApp
WebEngineCore/WebEngineWidgets 6.10.2, GNU 15.2.0, and the three immutable OSTree
commits in `release/flatpak-stack.lock.json`. It cannot substitute for the five
native Qt 6.10.3 SDKs. Configure it explicitly with
`-DSPATIAL_WEB_ABI_PROFILE=flatpak-kde-6.10`; composition and projection are excluded.

`workflows/sdk.flatpak/config.yml` builds in this retained stack, installs and
relocates the standalone SDK, then exports and installs a private proof app. Its
native X11 check loads real local HTTP content and evaluates DOM JavaScript with
the default Chromium sandbox. Only build-container tests disable the inner sandbox.
The SDK archive contains Spatial-Web headers and library, without Qt or Flatpak
runtimes. `flatpak-browser-sdk.yml` attests all three platform-prefixed assets;
publication is an explicit dispatch from accepted `dev`, never a pull request.
Local proof receipts cannot pass the immutable candidate publisher.

The isolated runtime qualification additionally pins a Mesa software driver in
`release/flatpak-proof-driver.lock.json` and records the actual llvmpipe renderer.
This is a reproducible test input, separate from the SDK Qt/C++ ABI contract.
