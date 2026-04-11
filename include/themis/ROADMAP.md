<!-- Status: current | validated: 2026-04-06 -->

# Roadmap — include/themis/

> This roadmap covers planned evolution of the **public themis core header API**.  
> Implementation work is tracked in [`../../src/themis/`](../../src/themis/).

---

## Current Status

| Field | Value |
|-------|-------|
| **Version** | v1.7.0 |
| **Release Date** | 2026-03-12 |
| **API Stability** | Stable (soname `.7`) |
| **Open Findings** | 1 minor (see AUDIT.md) |

---

## Completed

- [x] `edition.h` — scoped `Edition` enum; `edition_name()` helper (`v1.0.0`)
- [x] `edition_manager.h` — lazy-init singleton; `EditionCapabilities` bitmask (`v1.7.0`)
- [x] `export.h` — `THEMISDB_API` / `THEMISDB_DEPRECATED` macros; MSVC+GCC+Clang (`v1.3.0`)
- [x] `build_info.h` — version string + `uint64_t` feature bitmask (`v1.7.0`)
- [x] `license_info.h` — `LicenseStatus`, `LicenseConstraint`, `GRACE_PERIOD` (`v1.6.0`)
- [x] `module_hash_verifier.h` — SHA-256 binary hash verification (`v1.5.0`)
- [x] `module_signature_verifier.h` — Ed25519 / RSA-PSS signature verification (`v1.7.0`)
- [x] `runtime_license_gate.h` — `[[nodiscard]]` feature gating by license tier (`v1.6.0`)

---

## Planned Features

- [ ] `runtime_license_gate.h` — `LicenseDenialReason` enum + `GateResult::message()` (Target: v1.7.1)
  - Values: `TIER_TOO_LOW`, `LICENSE_EXPIRED`, `SIGNATURE_MISMATCH`, `NODE_LIMIT_EXCEEDED`, `STORAGE_LIMIT_EXCEEDED`
  - `message()` returns a locale-independent English string for logging
  - Backward-compatible: `GateResult` layout extended, not replaced
- [ ] `license_info.h` — `LicenseInfo::remaining_grace_days()` helper (Target: v1.7.1)
- [ ] `module_signature_verifier.h` — certificate-chain verification support for enterprise CA-signed modules (Target: v1.8.0)
  - Inputs: DER-encoded certificate chain + module binary
  - Validation: chain to pinned ThemisDB root CA; revocation check via OCSP
  - Errors: `CHAIN_TOO_LONG`, `ROOT_NOT_TRUSTED`, `REVOKED`, `EXPIRED_CERT`
  - Tests: unit tests with self-signed chain; integration test with production CA
- [ ] `edition_manager.h` — `EditionManager::upgrade(token)` hot-upgrade path without restart (Target: v1.8.0)
  - Constraints: upgrade must be atomic; downgrade not supported at runtime
  - Thread-safe: ongoing feature checks must not observe partial upgrade state
- [ ] `build_info.h` — `BuildInfo::plugin_api_version()` for plugin ABI compatibility checks (Target: v1.8.0)
- [ ] `runtime_license_gate.h` — async license refresh API for long-running daemons (Target: v1.9.0)
  - `RuntimeLicenseGate::scheduleRefresh(interval, callback)` — background license re-check
  - Errors: network unreachable → use cached license with grace window
- [ ] `gpu/` subdirectory — public headers for GPU tier availability and device query (Target: v1.9.0)
- [ ] `network/` subdirectory — public RPC transport contract headers (Target: v2.0.0)

---

## Implementation Phases

### Phase 1 — Design / API Contract (current sprint)
- [ ] Finalise `LicenseDenialReason` enum values with legal/licensing team
- [ ] Specify `GateResult` memory layout for ABI stability
- [ ] Draft certificate-chain verification API signature

### Phase 2 — Core Implementation
- [ ] Implement `LicenseDenialReason` + `GateResult::message()` in `src/themis/license_info.cpp`
- [ ] Implement `LicenseInfo::remaining_grace_days()` in `src/themis/license_info.cpp`
- [ ] Implement cert-chain validation in `ModuleSignatureVerifier`

### Phase 3 — Error Handling & Edge Cases
- [ ] `EditionManager::upgrade()` — rollback on partial failure
- [ ] Async license refresh — handle clock skew; validate server certificate
- [ ] Cert-chain — OCSP stapling fallback when network is unavailable

### Phase 4 — Tests
- [ ] Unit: `GateResult::message()` for each `LicenseDenialReason` value
- [ ] Unit: `remaining_grace_days()` boundary (0, 1, max) days
- [ ] Integration: cert-chain verification with production and revoked certificates
- [ ] Thread-safety: `EditionManager::upgrade()` under concurrent `check()` calls

### Phase 5 — Performance / Hardening
- [ ] `RuntimeLicenseGate::check()` must complete in < 1 µs (hot path; no syscall)
- [ ] Async refresh must not block any caller thread
- [ ] Cert-chain verification cached after first successful load

### Phase 6 — Documentation & Acceptance
- [ ] Update doxygen for all modified/new symbols
- [ ] Update `ARCHITECTURE.md` interface inventory
- [ ] Update `CHANGELOG.md` under `[Unreleased]`
- [ ] Security review of cert-chain and async refresh implementations

---

## Production Readiness Checklist

- [x] All verification functions marked `[[nodiscard]]`
- [x] No private key material in public headers
- [x] Fail-closed: verification failure returns error, never silently passes
- [x] `export.h` covers MSVC / GCC / Clang
- [x] `EditionManager` singleton is thread-safe
- [x] ABI soname policy documented and enforced
- [ ] `GateResult` denial reason (Finding 1 — Target v1.7.1)
- [ ] Certificate-chain verification (Target v1.8.0)
- [ ] Async license refresh for long-running daemons (Target v1.9.0)
