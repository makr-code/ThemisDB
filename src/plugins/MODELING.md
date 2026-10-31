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

# Plugin Runtime Model

## Purpose and scope

This document describes the lifecycle and capability model implemented by the
plugin manager. It is for maintainers changing state transitions, discovery,
or capability checks. It is a software state model, not an ML or embedding
model.

## Lifecycle state

`PluginLifecycleState` in
[`plugin_interface.h`](../../include/plugins/plugin_interface.h) is the state
tracked by `PluginManager::PluginEntry`.

| Current state | Allowed next state | Meaning |
|---|---|---|
| `UNLOADED` | `LOADING` | A load attempt begins. |
| `LOADING` | `LOADED` | Initialization succeeds. |
| `LOADING` | `UNLOADED` | Validation, library loading, construction, or initialization fails and the attempt is rolled back. |
| `LOADED` | `UNLOADING` | An unload begins. |
| `LOADED` | `LOADED` | A reload may replace the active instance without exposing an intermediate state to callers. |
| `UNLOADING` | `UNLOADED` | Shutdown and unload complete. |

Transitions are synchronous in the manager. A request conflicting with an
in-progress transition is an error, not permission to skip a state. `UNKNOWN`
and any unlisted transition are invalid. Do not confuse this enum with the
separate `PluginState` values in `plugins_api_contract.h`.

## Activation decision

For manifest-based loading, the manager's validation path checks required
`name` and `version`, the plugin-name character/path rules, current-edition and
license-feature eligibility, manifest-side `.sig` policy, and the binary
security policy before activation. A failure returns an error and should leave
the plugin inactive. The implementation's `.sig` behavior is described
precisely in [`TRUST.md`](./TRUST.md); it is not equivalent to the pinned-key
Ed25519 catalog.

Dependencies are resolved before dependent activation. Missing or cyclic
dependencies prevent a valid load order. Optional manifest fields include
`dependencies`, `capabilities`, `auto_load`, `allowed_editions`, and
`license_feature`; the loader applies defaults when they are absent.

## Capability model

`PluginCapabilities` is a set of boolean declarations. The manager snapshots
the plugin's reported capabilities after initialization. An explicit
`checkCapabilityEscalation()` compares the live values to that snapshot; a new
true flag beyond the snapshot marks the plugin restricted and returns an
error. This check is not implicitly run on every plugin call.

`PluginCapabilityNegotiator` evaluates each requested capability and optional
inclusive version range. Unknown capability names do not match. An empty
version is accepted only when the requested range is unconstrained.

## Registry distinction

`PluginManager` tracks manifest-backed runtime instances and their load state.
`PluginRegistry` is a separate type-indexed factory registry for creating
interface-specific objects. Its `registerFactory<T>` replaces an existing
factory with the same name and interface type; `create<T>` throws when a
factory is absent or the registered type does not match. Do not treat these
two registries as one lifecycle store.

## Validation evidence and failure cases

- `tests/plugins/test_plugin_lifecycle_state_machine.cpp` covers legal and
  illegal state transitions.
- `tests/plugins/test_plugin_error_handling_phase3.cpp` covers transition
  conflicts, initialization failure, partial recovery, and retry scenarios.
- `tests/plugins/test_registry_concurrency_hardening.cpp` covers concurrent
  factory registration and lookup.
- `include/plugins/plugins_api_contract.h` defines the error-code vocabulary;
  actual return types and call behavior are declared in the corresponding
  interfaces.

When changing a transition, update the interface contract, manager behavior,
focused tests, [`API.md`](./API.md), and [`RUNBOOK.md`](./RUNBOOK.md) together.
