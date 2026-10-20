# Ordinary browser library extraction checkpoint

Spatial-Web owns the ordinary browser implementation in `browser/`. The reusable
SDK is `SpatialWebBrowser` 0.1.0 with CMake target `SpatialWeb::Browser`. Dockpipe
must continue shipping ordinary browsing as a built-in desktop feature; consuming
this SDK must not require spatial rendering, Cockpit or any game package.

This is a source/build/install checkpoint. Dockpipe still owns its admitted source
until the installed-SDK integration gate passes. The standalone `spatial-web` shell now uses this same component. This candidate
is not a CEF migration, hosted package release or native interaction qualification.
`extraction-provenance.json` identifies exact admitted source files and hashes.
Retire the originals only after the existing Dockpipe Browser tests pass through
the installed SDK and approved host adapters, with optional spatial packages absent.
No sibling-source imports are permitted in release consumers.

## Dependency map

| Area | Owner and boundary |
| --- | --- |
| Navigation, tabs, session profiles, history, permissions, downloads, capture/annotation, settings and extension installation | Spatial-Web browser library, retained from the current implementation |
| Browser icons | Neutral `BrowserHostServices::Icon` callback; browser-only Qt stroke fallback |
| Browser preferences | Neutral load/save `QJsonObject` callbacks; standalone default is bounded, locked, atomic `browser.json` under the consuming application's config directory |
| Application colors | Consuming application Qt palette; browser settings dialog derives light/dark colors itself |
| Desktop URL routing | Existing optional `BrowserLinkRouter`, explicit `ExternalOpener` callback and signals; host chooses whether to instantiate/register global handlers |
| Terminal transport, shell discovery and terminal widgets | Dockpipe/terminal components; no browser dependency despite their previous shared archive |
| Markdown/Mermaid preview | Separate document renderer; no browser-widget dependency |
| Content interpretation/discovery/media | Separate SpatialWeb::Composition SDK; optional SpatialWeb::Projection uses the installed SpatialPipe scene SDK |
| Workspaces, Git, containers, host execution, captures of native applications, games and engine rendering | Consumer/optional packages, excluded from the browser SDK |

The ordinary library links only Qt Widgets, Network, PrintSupport and WebEngine.
There is no ForgePipe, SpatialPipe, Cockpit, game, CLI or host-provider dependency.
A normal page/extension cannot obtain `BrowserHostServices` or arbitrary host access.
The service API is native application integration, never registered as a JS bridge.

## Host integration and compatibility

Configure host services on the application thread before constructing browser widgets.
The loader/saver must be supplied together; callbacks preserve failure outcomes and
must outlive their configuration. Dockpipe's consumer adapter maps the existing
`LauncherSettings` browser object and `CanvasIcons` to these neutral hooks. It must
preserve unrelated launcher settings and current browser preference values.

The existing public BrowserWidget API and signals remain for the consumer gate.
`BrowserLinkRouter::openInDockPipe` and the serialized destination value `dockpipe`
are legacy compatibility identifiers for the embedding browser destination; they
provide no Dockpipe runtime or privileged operation. Rename them only through an
explicit compatibility migration after consumers switch.

Tab moves retain the original shared session lease. Session profiles retain isolated
temporary browsing storage and no persistent cookies. Explicitly authorized extension
code/settings retain their existing durable store and approval behavior. No profile,
page, tab or teardown policy is rewritten by this checkpoint.

The installed CMake package pins the exact Qt version used to build the SDK.
Distribute matching Qt/WebEngine/Chromium runtime components and attribution through
the consumer's normal release process. The inherited source license is Apache-2.0;
Qt/WebEngine licensing remains artifact-specific and is not changed here.

## Validation and release gates

Root build/test workflows use scoped generated directories and do not install dependencies.
Tests include the inherited full browser lifecycle suite, package/CRX/store tests,
neutral host callback failures and installed-SDK consumer compilation/lifetime checks.
The independent consumer uses `find_package(SpatialWebBrowser)` from an install prefix;
it has no Dockpipe include paths, source linkage or optional spatial SDK.

An automated native X11 page-to-world fixture is qualified locally. Complete native
platform/runtime packaging proof, Dockpipe host-adapter parity, hosted binary delivery,
published catalog installation/update/removal/rollback and original source retirement
are outstanding gates. The local archive is hashed in the candidate manifest. A local offscreen test does not establish those outcomes.
