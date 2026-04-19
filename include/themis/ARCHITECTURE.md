<!-- Status: current | validated: 2026-04-06 -->

# Architecture — include/themis/

> Public-header interface layer for the ThemisDB core framework.
> Implementation details live in [`../../src/themis/`](../../src/themis/).

---

## Overview

The `include/themis/` directory exposes the **foundational C++ API** for the
ThemisDB runtime.  These headers define:

- **Edition management** — community / enterprise / cloud tier selection
- **License enforcement** — runtime feature gating by license tier
- **Module integrity** — SHA-256 hash and cryptographic signature verification
- **Build metadata** — version, build flags, and feature-availability queries

The directory also contains three sub-namespaces surfaced as subdirectories:

| Subdirectory | Purpose |
|--------------|---------|
| `base/` | Foundational utilities (allocators, error codes, logging contracts) |
| `gpu/` | GPU compute tier headers (CUDA/ROCm availability guards) |
| `network/` | Distributed networking primitives (cluster membership, RPC contracts) |

---

## Design Principles

1. **Defence in depth** — `runtime_license_gate.h` enforces license tier at
   runtime; `module_signature_verifier.h` and `module_hash_verifier.h` enforce
   binary integrity at load time.  Neither can be bypassed without modifying the
   library binary.
2. **Single source of truth for version and edition** — `build_info.h` and
   `edition.h` are the only authoritative sources for version strings and
   edition constants; all other headers and subsystems derive their knowledge
   from these.
3. **Fail-closed security** — when license gate or module verification fails,
   the API returns an error code; it never silently degrades to a lower tier.
4. **Stable ABI for export symbols** — `export.h` centralises `THEMISDB_API`
   visibility macros, ensuring consistent symbol export across platforms.
5. **Composability with subsystems** — the headers in `include/themis/` are
   deliberately independent of subsystem headers (temporal, spatial, graph) so
   that the core can be linked and tested without pulling in heavy subsystem
   dependencies.

---

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|---------------------|---------|
| `build_info.h` | `BuildInfo`, `BuildInfo::version()`, `BuildInfo::features()` | Compile-time and runtime build metadata; feature-availability bitmask |
| `edition.h` | `Edition` enum, `edition_name()` | Canonical edition identifiers: COMMUNITY, ENTERPRISE, CLOUD, DEVELOPER |
| `edition_manager.h` | `EditionManager`, `EditionCapabilities` | Runtime edition resolution; capability query API |
| `export.h` | `THEMISDB_API`, `THEMISDB_NO_EXPORT`, `THEMISDB_DEPRECATED` | Cross-platform symbol visibility macros |
| `license_info.h` | `LicenseInfo`, `LicenseStatus`, `LicenseConstraint` | License metadata: expiry, tier, constraint enumeration |
| `module_hash_verifier.h` | `ModuleHashVerifier`, `HashVerificationResult` | SHA-256 hash verification of module binaries before load |
| `module_signature_verifier.h` | `ModuleSignatureVerifier`, `SignatureVerificationResult` | Ed25519 / RSA-PSS signature verification of loaded modules |
| `runtime_license_gate.h` | `RuntimeLicenseGate`, `FeatureFlag`, `GateResult` | Runtime feature gating by license tier; `check(feature)` returns `GateResult` |
| `base/` | See `base/README.md` | Allocators, error codes, intrusive logging contracts |
| `gpu/` | See `gpu/README.md` | GPU compute tier availability guards and device query helpers |
| `network/` | See `network/README.md` | Cluster membership, RPC transport contracts |

---

## Dependency Graph

```
build_info.h   edition.h
     │              │
     └──────┬───────┘
            ▼
     edition_manager.h
            │
            ▼
    license_info.h
            │
            ▼
  runtime_license_gate.h
            │
     ┌──────┴────────────────────┐
     ▼                           ▼
module_hash_verifier.h   module_signature_verifier.h
```

`export.h` is a leaf — it has no includes from `include/themis/`.

---

## Implementation Reference

> All `.cpp` translation units are located in **`../../src/themis/`**.
> Do not include headers from `src/` directly; the public API is fully
> described here.
