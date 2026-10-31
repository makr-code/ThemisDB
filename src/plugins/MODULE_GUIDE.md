---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Plugins Module Guide

## Purpose and audience

Use this guide when implementing or integrating a plugin against ThemisDB's
runtime. It covers the host contract and review path; a plugin's domain
interface, dependencies, model assets, and business logic remain with that
plugin's owning module or repository.

## Implement a plugin

1. **Choose the host contract.** Implement `IThemisPlugin` from
   [`plugin_interface.h`](../../include/plugins/plugin_interface.h). Also
   implement the relevant domain interface when one exists. Keep plugin name,
   version, type, and reported capabilities stable for the loaded instance.
2. **Export the lifecycle entry points.** The manager resolves `createPlugin`
   and `destroyPlugin` from the shared library. Use
   `THEMIS_PLUGIN_IMPL(YourPlugin)` from the public interface header rather
   than inventing symbol names.
3. **Provide a manifest.** Include a valid plugin `name`, `version`, and
   platform `binary` object. Declare dependencies and capabilities where
   needed. `auto_load` defaults to false. An empty `allowed_editions` list
   means no edition restriction in the manager; an empty `license_feature`
   means no license feature is required.
4. **Make initialization failure-safe.** `initialize()` returns false on
   failure. Release resources in `shutdown()`. If state must survive reload,
   implement `IStatefulPlugin`; `restoreState()` failure is surfaced to the
   reload operation.
5. **Validate compatibility and trust.** Test the target edition, dependency
   resolution, artifact path, required symbols, manifest checks, and binary
   security policy in the same build mode used for deployment. Do not equate
   the `.sig` manifest hash check with the pinned-key Ed25519 catalog.

Use the manifest-backed `loadPlugin(name)` path when relying on manifest
edition/license restrictions. `loadPluginFromPath()` and the OCI convenience
path do not read `plugin.json` and therefore do not apply those manifest
restrictions; their binary and digest checks are not a substitute.

## Integration decision points

- Use `PluginManager` when the host must discover and own a dynamic library
  lifecycle.
- Use `PluginRegistry` only for typed factory registration/creation; it is not
  a substitute for loading, validating, or unloading a shared library.
- Use `loadPluginFromOci()` only when registry transport and artifact digest
  verification are part of the deployment. Registry access can fail before
  plugin validation begins.
- Use the WASM bridge only in a build with WASM support and a permitted
  runtime/license configuration. A valid hash is required but does not by
  itself authenticate the source.
- For an image backend, see
  [`image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h)
  and [`EMBEDDING.md`](./EMBEDDING.md). This separate interface is not a
  dedicated embedding implementation in the generic manager; the plugin core
  does not define a universal embedding dimension or quality threshold.

## Test and review checklist

Add focused coverage under `tests/plugins/` for the relevant contract:

- successful load and cleanup, missing exports, null factory, and failed
  initialization;
- malformed names/manifests, digest/signature mismatch, edition/license denial,
  and missing or cyclic dependencies;
- concurrent registry access or lifecycle transition conflicts if the change
  touches shared state;
- reload rollback and state restore behavior when implementing
  `IStatefulPlugin`;
- runtime-specific tests for OCI, health monitoring, hot-plug, or WASM when
  those paths are changed.

Useful current suites include
`test_plugin_lifecycle_state_machine.cpp`,
`test_plugin_error_handling_phase3.cpp`,
`test_registry_concurrency_hardening.cpp`,
`test_plugin_manifest_edition_gates_focused.cpp`, and
`test_plugin_boundary_enforcement_focused.cpp`. Check the active build's test
registration before assuming a source file is executable.

See [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) for load ordering,
[`API.md`](./API.md) for call contracts, and [`TRUST.md`](./TRUST.md) for
security boundaries.
