---
Author: "ThemisDB Contributors"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base capabilities

This page is a brief index of source-defined base capabilities, not a claim that
every capability is enabled or linked into every build. See
[`README.md`](README.md) for source and build-registration notes.

## Role and use cases

The base APIs are for applications that load extension modules, need to inspect
dependency topology, or manage reload/sandbox/resource-pool lifecycles. Their
practical value is explicit result/error surfaces and caller-selectable
controls; they do not supply a universal plugin host policy or promise that
every control is active. The intended readers are module integrators and
maintainers, not database end users.

| Capability | Evidence | Important limit |
|---|---|---|
| Module lifecycle and verification | `include/themis/base/module_loader.h`; canonical implementation `src/themis/module_loader.cpp` | Verification behavior depends on configuration; `src/base/module_loader.cpp` is a deprecated copy. |
| Sandbox and ABI checks | `include/themis/base/module_sandbox.h`, `src/base/module_sandbox.cpp` | Controls are configurable and platform-dependent. |
| WASM validation/execution | `include/themis/base/wasm_plugin_sandbox.h`, `wasm_runtime_injector.h` | Validation can run without an engine; execution requires an injected runtime. |
| Reload and dependency graph | `include/themis/base/hot_reload_manager.h`, `plugin_dependency_graph.h` | Graph analysis is a caller surface; reload has an availability gap between unloading the candidate and loading it under the canonical key, with best-effort restoration on final-load failure. |
| Registry intake and A/B management | `include/themis/base/remote_registry_client.h`, `ab_test_manager.h` | Plain HTTP is accepted with a warning; integrations supply application-level operation/metrics surfaces. |
| Buffer and connection resource pools | `include/base/buffer_pool.h`, `resource_pool_manager.h` | Implementations are registered in both modular and monolithic builds; the connection pool manages integer slots, not live connection objects. |

Build/test registration status is summarized in [`AUDIT.md`](AUDIT.md);
planning and pending hardware evidence remain in [`ROADMAP.md`](ROADMAP.md).
