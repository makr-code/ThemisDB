---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot, MAI-Code-1.1-Flash"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module Implementation Guide

## Objective

This note explains how the base module implements the runtime contract for module loading, verification, dependency safety, sandboxing, and hot reload. It is tied to the implementation in `src/base/*.cpp` and the API declarations in `include/themis/base/*.h`.

## Lifecycle implementation

The loader implementation is staged around `LoadStage`. The sequence intentionally distinguishes the phases so the system can stop early on failures instead of allowing partial or unsafe activation.

The major stages are:

- `UNLOADED` → no artifact resident
- `VERIFYING` → trust and signature checks begin
- `VERIFIED` → file and identity checks succeeded
- `VALIDATING` → compatibility and metadata validation
- `VALIDATED` → ABI and required symbols passed
- `STAGING` / `STAGED` → runtime prep and health gating
- `ACTIVATING` → final activation attempt
- `ACTIVE` → ready for runtime use
- `FAILED` / `UNLOADING` → explicit terminal or teardown state

The code in `module_loader.cpp` keeps verification and activation values explicit rather than silently treating the module as usable as soon as a handle is opened.

## Trust and compatibility checks

The loader and ABI checker are the primary trust boundary.

`ModuleSecurityVerifier` delegates to the acceleration plugin security layer and performs checks against the configured `PluginSecurityPolicy`. That means trust is not a simple file existence check; it is a policy-backed verification step with support for required signatures, revocation, and allowed/blocked hashes.

`AbiChecker` then validates:

- ABI major-version equality,
- minor-version compatibility,
- required exported symbols,
- deprecated symbol detection,
- optional struct-size sentinel checks.

This makes compatibility enforcement explicit and prevents a new module from silently violating host assumptions.

## Dependency and ordering logic

`PluginDependencyGraph` models a module graph as `nodes_` and `edges_` and provides:

- `addModule()` / `addDependency()` to build the graph,
- `detectCycles()` to detect loops,
- `topologicalOrder()` to produce a valid initial order,
- `exportTo()` to serialize DOT / JSON / ASCII output.

This is the concrete mechanism by which dependency order is reasoned about before activation or migration.

## Execution isolation

`ModuleSandbox` implements runtime isolation using platform-appropriate mechanisms. The implementation uses Linux cgroup detection and optionally creates a sandbox entry when the OS supports it. Windows support falls back to native job-object semantics. The design is intentionally degrade-safe: if a mechanism is unavailable, the sandbox still records the issue rather than pretending it is fully enforced.

`WasmPluginSandbox` adds a second boundary: it validates the `.wasm` magic header, enforces a capability allowlist, and optionally runs inside a runtime-backed sandbox with linear memory bounds. This makes untrusted extension execution explicitly constrained instead of “just trust the plugin.”

## Hot reload semantics

The hot reload implementation is critical to the production safety story. `HotReloadManager` does not do a blind replace.

The process is:

1. validate the module registration under lock,
2. save state if configured,
3. call `BEFORE_UNLOAD` callbacks,
4. ensure the module is still the same registration before proceeding,
5. load the candidate path under a temporary name,
6. unload the old module,
7. re-load the final version under the canonical name,
8. preserve rollback metadata and notify completion.

The implementation also tolerates callback-triggered re-registration or removal by re-checking ownership and registration ID before mutating the active slot. That is a concrete guard against stale reload operations.

## Remote registry intake

`RemoteRegistryClient` owns authenticated intake from a plugin registry. It supports:

- authentication via token or API key,
- GET listing of plugin metadata,
- binary download to a configured destination,
- SHA-256 verification,
- handoff to the loader.

The failure taxonomy in `base_error_taxonomy.h` covers the result paths, including path errors, signature failures, load failures, auth issues, and checksum mismatches.

## Operational invariants

The module intentionally prefers explicit failures over silent degradation. This is visible in:

- `ModuleErrorCode` enumerations for verification, ABI, dependency, policy, and system-load failures,
- `ErrorCategory` for transient vs. permanent vs. recoverable failure semantics,
- `BaseErrorTaxonomy` remediation hints for operators,
- structured observability counters in `RequestStats` and reload stats in `HotReloadManager`.

## Edge cases and failure handling

The implementation explicitly handles:

- missing module binaries,
- invalid signatures,
- ABI and dependency mismatches,
- circular dependency graphs,
- missing required symbols,
- failed state save or restore during reload,
- unavailable sandbox capabilities on the host OS,
- invalid remote registry responses or checksum mismatches,
- retries that exceed the configured budget.

These are the real edge cases to preserve when changing this module.

## Conclusion

The implementation is deliberately layered and conservative: trust checks first, then dependency legality, sandboxing, and reload logic. The code treats each stage as a safety boundary and keeps the failure modes observable and actionable rather than silent.
