> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/themis/ARCHITECTURE.md -->

# Themis Core Identity Module — Public Header Architecture

**Module Path:** `include/themis/`  
**Implementation:** `../../src/themis/`  
**Canonical architecture doc:** [`../../src/themis/ARCHITECTURE.md`](../../src/themis/ARCHITECTURE.md)

---

## 1. Overview

`include/themis/` defines the **public build metadata, edition management, license gating, module signature verification, and export macros API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/themis/ARCHITECTURE.md`](../../src/themis/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Build and Edition

| Header | Public Type | Purpose |
|--------|------------|---------|
| `build_info.h` | `BuildInfo` | Build metadata (version, commit, timestamp) |
| `edition.h` | `Edition` | Edition enum (Community/Enterprise/Cloud) |
| `edition_manager.h` | `EditionManager` | Runtime edition detection and feature gates |
| `license_info.h` | `LicenseInfo` | License metadata and expiry |
| `runtime_license_gate.h` | `RuntimeLicenseGate` | Feature-gate enforcement at runtime |
### 2.2 Security and Export

| Header | Public Type | Purpose |
|--------|------------|---------|
| `module_hash_verifier.h` | `ModuleHashVerifier` | Module binary hash verification |
| `module_signature_verifier.h` | `ModuleSignatureVerifier` | Module code-signing verification |
| `export.h` | `THEMIS_EXPORT` | DLL export/import macro definitions |

---

## 3. Namespace Layout

All public types reside in the `themis` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/themis/` expose the **stable public API**; internal types live in `src/themis/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
