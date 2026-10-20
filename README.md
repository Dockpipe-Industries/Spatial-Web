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
