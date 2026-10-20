# Browser, content and projection SDKs

SpatialWebBrowser 0.1.0 exports `SpatialWeb::Browser`, an ordinary browser library
with Qt 6.10.3 as its only external SDK. Set `SPATIAL_WEB_BUILD_COMPOSITION=OFF`
(and leave Projection disabled) to build and install it and the ordinary browser
shell without ForgePipe or SpatialPipe. BrowserWidget retains isolated profiles,
URL policy, permissions, extensions, tabs, downloads and session teardown.

SpatialWebComposition 0.1.0 exports `SpatialWeb::Composition`, a separate static
library containing bounded documents/graphs, illustrative animal models, anonymous
site policy/fetch/crawl/cache/explorer and browser media control. It consumes the
installed Browser and exact SpatialPipe 0.1.0 SDKs and transitive ForgePipe contract.

SpatialWebProjection 0.1.0 exports `SpatialWeb::Projection`, an optional static
library containing the inherited web world planner/compiler/architecture, civic
cinema/city spaces, connected passages, SpatialWebBrowser UI/search and optional
local model enhancement. It consumes Composition and exact SpatialPipeScene 0.1.0
Geometry/Scene targets. SpatialWebView adds web status/fade over SpatialWorldView;
renderer, navigation, terrain and geometry stay owned by the installed Scene SDK.
The Projection SDK owns SpatialWebDocument::room(), so callers link Projection when
using that method. Public class names remain for consumer compatibility.

The standalone `spatial-web` shell offers Browser and Spatial Web tabs when built
with Projection. A browser-only build requires no optional SDK. `--spatial URL`
selects spatial browsing. The public browser has a neutral start page and return
signal. Headquarters/game portals, host providers, filesystem/process/container/Git
operations and native desktop capture are excluded. Consumer applications own any
explicit host actions. Generic illustrative vehicle metadata/geometry remains;
vehicle sessions require an explicit consumer adapter and do not claim paid-game
behavior. Optional enhancement invokes fixed package-owned scripts only through
native user controls; no webpage process provider or JS host bridge exists. Model
installation supports Linux and was not invoked during qualification.

The local candidate includes source/build/install, bounded model/navigation tests,
shared-profile isolation, local HTTP DOM-to-world projection and live/world-state
preservation. An automated native X11 fixture also verifies the local page and generated world
using software scene rendering. User-driven native interaction, full consumer parity,
release runtime bundling,
hosted CI and catalog installation/update/disable/removal/rollback remain separate
gates. Qt renderer/resources/plugins and the transitive runtime SDKs are separately
packaged dependencies. Licenses and Qt/WebEngine/Chromium terms remain attached to
their distributed artifacts; private source/authentication is never a build input.
