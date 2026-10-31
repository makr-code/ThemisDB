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

# Plugin Runtime Research

## Purpose and scope

This note frames engineering evaluation of extension loading for plugin authors
and maintainers. It is not a report of benchmark results and does not select a
plugin runtime or model on behalf of a deployment. The subject is the runtime
and trust boundary in `src/plugins`, not the business behavior of third-party
plugins.

## Question and assumptions

The design question is how to add a capability while preserving a reviewable
activation path, predictable failure behavior, and an operator-visible lifecycle.
The source provides two materially different execution paths:

- Native shared libraries are opened through the platform loader in
  `plugin_manager.cpp`. They execute in the ThemisDB process; the plugin
  subsystem does not sandbox native code.
- WASM loading is an optional build/runtime path in `wasm_plugin_loader.cpp`.
  It requires an expected SHA-256 value, an enabled runtime build, and the
  `enterprise_plugins` license feature.

OCI is a distribution transport, not an execution isolation mechanism. Its
downloaded binary is checked against the manifest layer digest before it is
passed to the path loader. That path does not read `plugin.json`, so OCI loading
does not apply the manager's manifest edition/license restrictions.

## Evaluation method

Use the existing test and benchmark sources as reproducible evidence:

1. Exercise lifecycle transitions and validation failures with
   `tests/plugins/test_plugin_lifecycle_state_machine.cpp`,
   `tests/plugins/test_validation_contract_hardening.cpp` (where present in the
   test tree), and `tests/plugins/test_plugin_error_handling_phase3.cpp`.
2. Exercise registry concurrency with
   `tests/plugins/test_registry_concurrency_hardening.cpp`.
3. Exercise OCI, signed-catalog, hot-plug, and WASM paths with their focused
   tests under `tests/plugins/` or `tests/legacy/`.
4. For performance comparisons, run the mapped cases in
   [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) under the
   release profile and compare against the same recorded baseline. That
   document sets a regression budget and case-completeness requirement; this
   page does not claim that a particular run passed.

Record the build flags, platform, plugin artifact hash, manifest, test result,
and benchmark output with each comparison. Compare equivalent work and
configurations; do not infer runtime isolation from latency or digest checks.

## Trade-offs and limits

| Choice | Benefit | Cost or limitation |
|---|---|---|
| Native plugin | Uses the platform shared-library loader and `IThemisPlugin` lifecycle. | Runs with host-process privileges; a crash or memory-safety defect can affect the server. |
| WASM plugin | Uses a runtime-specific bridge and rejects absent, malformed, or mismatched module hashes. | Runtime support is compile-time optional; license gating applies, and hash matching alone does not establish publisher identity. |
| OCI retrieval | Resolves a registry reference, applies configured authentication, verifies the downloaded layer digest, and caches the binary. | Network, registry, cache, and credential availability add failure modes; digest verification establishes content equality, not trust in the publisher. |
| Signed catalog | `SignedPluginRepository` verifies Ed25519 signatures against active pinned keys. | Its in-process catalog is a separate API; do not assume `PluginManager`'s `.sig` check is the same mechanism. |

## Embedding-related boundary

The plugin type enum includes `EMBEDDING`, and the image-analysis interface
defines embedding result and call shapes. The core plugin runtime does not
provide an embedding model, vector-quality evaluation, or an embedding benchmark
in `src/plugins`. Model selection and semantic quality must be evaluated by the
specific backend and its consuming search pipeline; see
[`EMBEDDING.md`](./EMBEDDING.md).

## Evidence and open questions

Primary evidence is in
[`plugin_manager.cpp`](./plugin_manager.cpp),
[`wasm_plugin_loader.cpp`](./wasm_plugin_loader.cpp),
[`oci_registry_client.cpp`](./oci_registry_client.cpp),
[`plugin_interface.h`](../../include/plugins/plugin_interface.h), and
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md). Test names and
benchmark mappings are evidence sources, not proof of a current passing run.
Before comparing isolation guarantees, verify the enabled WASM runtime and its
host imports in the target build; before production native loading, verify the
artifact provenance and active signature policy independently.

## Related documents

[`MODELING.md`](./MODELING.md) describes the runtime state model;
[`TRUST.md`](./TRUST.md) describes trust boundaries; and
[`RUNBOOK.md`](./RUNBOOK.md) provides incident triage.
