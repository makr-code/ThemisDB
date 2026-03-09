# Plugins Module – Missing Implementations Report

**Generated:** 2026-03-09
**Validated against:** commit `09f7c55` (HEAD, branch `copilot/sync-documentation-with-sourcecode`)
**Primary source:** `src/plugins/`, `include/plugins/`

---

## Executive Summary

The plugins module is **Beta-level** as of v1.5.0.  Core plugin loading, Ed25519 signature
verification, manifest validation, capability negotiation, dependency resolution, hot-plug
monitoring, health monitoring, and OCI registry integration are all implemented and
production-quality.

The reality-check found **four documentation-accuracy issues** in `src/plugins/README.md`
and one stale secondary-docs issue — all corrected in this review cycle.

No ROADMAP `[x]` items lack code evidence.  WASM sandbox isolation is correctly tracked as
planned in `FUTURE_ENHANCEMENTS.md` and is **not** falsely marked as complete.

---

## Findings

### FINDING-P-001: Ghost File References in "Relevant Interfaces"

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | ✅ Fixed (commit `09f7c55`) |
| **Claim source** | `src/plugins/README.md`, "Relevant Interfaces" section |
| **Expected** | Files `plugin_loader.cpp`, `plugin_api.cpp`, `manifest_validator.cpp`, `plugin_signer.cpp` exist in `src/plugins/` |
| **Observed** | None of these four files exist anywhere in `src/plugins/`.  Dynamic loading is implemented inside `plugin_manager.cpp` (via `dlopen`/`LoadLibrary`); the API is header-only in `include/plugins/plugin_api.h`; manifest validation is via `ManifestSchemaValidator` in `plugin_interface.h`; signing is in `signed_plugin_repository.cpp` and `tools/plugin_signer/sign_plugin.py`. |
| **Evidence** | `ls src/plugins/*.cpp` — no such files |
| **Fix applied** | "Relevant Interfaces" section replaced with accurate "Source Files" tables listing all 10 real `.cpp` files and 15 headers |

---

### FINDING-P-002: Wrong Status Note — "Ed25519 Signing In Progress"

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | ✅ Fixed (commit `09f7c55`) |
| **Claim source** | `src/plugins/README.md`, "Current Delivery Status" |
| **Claim** | "WASM sandbox and Ed25519 signing in progress" |
| **Observed** | Ed25519 signature verification is fully implemented: `include/plugins/signed_plugin_repository.h` declares `SignedPluginRepository` with key-pinning and `verifyEd25519Signature()`; `src/plugins/signed_plugin_repository.cpp` implements it; `PluginManager::verifyPlugin()` (line 135 of `plugin_manager.cpp`) calls it (NDEBUG-guarded).  Only the WASM sandbox is still planned. |
| **Evidence** | `include/plugins/signed_plugin_repository.h` (full Ed25519 API), `src/plugins/signed_plugin_repository.cpp`, `grep -n "verifyPlugin\|Ed25519" src/plugins/plugin_manager.cpp` |
| **Fix applied** | Status updated to "Ed25519 signature verification implemented; WASM sandbox isolation is planned" |

---

### FINDING-P-003: Three Broken Documentation Links

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | ✅ Fixed (commit `09f7c55`) |
| **Claim source** | `src/plugins/README.md`, "Documentation" section |
| **Claim** | Links to `../../docs/plugins/PLUGIN_SECURITY.md`, `../../docs/plugins/PLUGIN_MIGRATION.md`, `../../docs/plugins/MANIFEST_SIGNATURES.md` |
| **Observed** | The directory `docs/plugins/` does **not** contain these three files; `ls docs/plugins/` shows only `README.md` and `sign_plugin.py`.  The correct paths are `docs/de/plugins/PLUGIN_MIGRATION.md` and `docs/de/plugins/MANIFEST_SIGNATURES.md`.  `PLUGIN_SECURITY.md` does not exist anywhere. |
| **Evidence** | `ls docs/plugins/PLUGIN_SECURITY.md docs/plugins/PLUGIN_MIGRATION.md docs/plugins/MANIFEST_SIGNATURES.md` → all "No such file" |
| **Fix applied** | Links corrected to `docs/de/plugins/PLUGIN_MIGRATION.md`, `docs/de/plugins/MANIFEST_SIGNATURES.md`, `docs/de/plugins/RPC_PLUGIN_ARCHITECTURE.md`; broken `PLUGIN_SECURITY.md` link removed |

---

### FINDING-P-004: Secondary Docs Severely Stale (December 2025)

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | ✅ Fixed (commit `09f7c55`) |
| **Claim source** | `docs/de/plugins/README.md`, "Stand: 22. Dezember 2025" |
| **Observed** | Component table listed only 4 features (Manifest Signatures, Plugin Migration, RPC Framework, Image Analysis).  Actual module has 15 headers, 10 source files, and 13 implemented features.  Missing: `plugin_manager`, `plugin_registry`, `plugin_metrics`, `plugin_health_monitor`, `plugin_hot_plug_monitor`, `plugin_dependency_resolver`, `signed_plugin_repository`, `oci_registry_client`, `plugin_api`, `plugin_system_edition`, `self_healing_plugin`.  Additionally referenced `include/enterprise/analytics_plugins.h` which does not exist at that path (ghost source reference). |
| **Evidence** | `ls include/plugins/*.h \| wc -l` = 15; `ls src/plugins/*.cpp \| wc -l` = 10; `ls include/enterprise/analytics_plugins.h` → "No such file" |
| **Fix applied** | Full component table rewritten with all 17 components; ghost source reference removed; "Stand" updated to 2026-03-09; primary source doc links added; WASM sandbox row added as 🔲 Geplant |

---

## Open / Remaining Items

These are **correctly tracked** as planned in ROADMAP/FUTURE_ENHANCEMENTS and are **not** false completions:

| Item | ROADMAP Status | Evidence |
|---|---|---|
| WASM plugin runtime (Wasmtime) | `[?]` Phase 3 | `FUTURE_ENHANCEMENTS.md` §WASM; no code yet |
| Per-plugin resource quotas (CPU/memory) | `[?]` Short-term | `ROADMAP.md` |
| Plugin configuration JSON Schema validation | `[?]` Short-term | `ROADMAP.md` |
| Plugin SDK (C++, Python, Go bindings) | `[?]` Phase 3 | `ROADMAP.md` |
| Community plugin repository | `[?]` Phase 3 | `ROADMAP.md` |
| Plugin hot-reload (full, Issue #2223) | `[I]` In Progress | `ROADMAP.md`; `plugin_hot_plug_monitor.cpp` implements file watching; full hot-reload via `reloadPlugin()` is implemented |
| `manifest_signer.cpp` offline signing tool | `[?]` Phase 2 | `FUTURE_ENHANCEMENTS.md`; `tools/plugin_signer/sign_plugin.py` is a Python implementation |

> **Note on hot-reload:** `PluginManager::reloadPlugin()` is fully implemented in `plugin_manager.cpp` (Phase-2 TOCTOU-safe atomic swap, line 953+).  The ROADMAP `[I]` for Issue #2223 likely tracks production hardening / server-integration work beyond the core reload mechanism.

---

## Suggested Issue Titles (for tracking)

> These are suggestions only; no auto-issues were created per DoD §4 rule.

| # | Suggested Title | Labels |
|---|---|---|
| — | `[plugins] WASM sandbox runtime via Wasmtime` | `enhancement`, `plugins`, `security` |
| — | `[plugins] Per-plugin CPU/memory resource quotas` | `enhancement`, `plugins`, `resource-mgmt` |
| — | `[plugins] Plugin configuration JSON Schema validation` | `enhancement`, `plugins` |
| — | `[plugins] Plugin SDK: C++, Python, Go bindings` | `enhancement`, `plugins`, `sdk` |

---

*Reviewed by: Copilot agent (2026-03-09)*
*Next review: v1.6.0 milestone (Target: Q3 2026)*
