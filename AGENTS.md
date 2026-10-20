# Spatial-Web agent guide

Spatial-Web is the public browser library and standalone Spatial Browser shell. It owns interpretation, discovery, manifests and web composition. ForgePipe provides engine capabilities and SpatialPipe provides offline runtime capabilities through versioned packages. Browser and content SDK extraction plus the ordinary browser shell are present; root check validates declarations only. Native interaction and release qualification are separate gates.

## Ownership and acceptance

- Keep the browser library independent of its application shell, Cockpit and privileged host providers. Consume versioned installed SDKs instead of sibling source imports.
- Public builds must exclude private source/authentication and credentials. Pure public model/schema tests must work without private engine access. Application readiness requires redistributable SDK/runtime artifacts and approved terms.
- Preserve discovery/browser fallback and URL policy. Public content cannot obtain local filesystem/process/container capabilities by naming a screen provider. Host integrations require explicit trusted adapters outside public content.
- Keep bike/game/headquarters actions in consumers rather than browser core. Prove models, SDK integration and native browser behavior separately.
- Retain inherited Apache-2.0/provenance and artifact-specific Qt/WebEngine/Chromium attribution. Private engine source and machine-specific settings never enter public build inputs. Release/publishing and license changes require separate authority.

## Working rules

- Default: `gpt-6.1-sol`, Medium reasoning. Use bounded subagents for useful independent work with explicit ownership/checks; avoid overlapping edits or expensive fanout. Escalate difficult seams only when a stronger model likely lowers total cost or rework.
- Inspect instructions and read-only Git state first. Preserve staged, unstaged, untracked and unrelated work. Git lifecycle uses runtime session actions; no raw Git mutations. Commit, push, publication, hosted dispatch and live startup require user authorization.
- Initialize the root with `dockpipe init`, preserving existing files. Run `dockpipe --workflow-file workflows/check/config.yml`; use declared root `build`/`test` workflows for builds/tests. Inspect their `workflows/<name>/config.yml`. Workflow = what, runtime = where, resolver = tool/profile, strategy = lifecycle. Keep scripts/assets workflow-owned; reuse existing helpers. Validate changed YAML with `dockpipe workflow validate <path>`.
- Follow local architecture and readable code conventions. Preserve ABI, errors/cancellation and performance contracts. Keep generated outputs ignored and credentials as references. Verify sandbox capabilities; never bypass restrictions.
- Report changed areas, checks, skips, artifacts and limits. Distinguish scaffold/local/offscreen, installed-SDK/native-interactive and hosted/release proof. Update canonical docs without duplicate guidance.

Run root workflows using `dockpipe --workflow-file workflows/<name>/config.yml` with the pinned released CLI, including on fresh CI runners. This avoids its older automatic core/source lookup.

Controlled consumers pin released dependency versions and artifact checksums; GitHub Actions use verified full commit SHAs. Upgrade the committed pin and qualify affected consumers together. Do not silently select latest, a branch tip or sibling source in release builds.
