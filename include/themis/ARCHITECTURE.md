---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-11-01"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
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
| `build_info.h` | `BuildConfiguration`, `ModuleInfo` | Build metadata and module availability |
| `edition.h` | `EditionType`, `EditionInfo` | Compile-time edition identity, resource ceilings, and feature flags |
| `themis_api_contract.h` | `themis::engine::ThemisError`, `themis::engine::Edition` | Error-category and edition identifier enums; no runtime build-info or capability API |
| `edition_manager.h` | `EditionManager` | Runtime edition detection and feature gates |
| `license_info.h` | `LicenseData`, `LicenseClient`, `LicenseInfo` | License metadata, client validation, and grace-period view |
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

- Headers in `include/themis/` expose public contracts; this does not by itself promise source or ABI stability.
- Implementation details in `src/` are not public API; callers should use the declared headers and the repository's versioning policy.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- No layer classification is established by this header architecture document.
