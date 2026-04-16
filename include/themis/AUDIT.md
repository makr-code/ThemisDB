<!-- Status: current | validated: 2026-04-06 -->

# Audit Report — include/themis/

| Field | Value |
|-------|-------|
| **Last Audit Date** | 2026-03-22 |
| **Auditor** | ThemisDB Security Team |
| **Audit Status** | ✅ Pass |
| **Component Version** | v1.7.0 |
| **Headers Audited** | 8 (top-level; subdirectories audited separately) |
| **Critical Findings** | 0 |
| **Minor Findings** | 0 |

---

## Summary

All 8 top-level public headers in `include/themis/` were reviewed for:
- Correct symbol-visibility macro usage (`THEMISDB_API` / `export.h`)
- Fail-closed behaviour in `RuntimeLicenseGate` and verification APIs
- No sensitive data (private keys, license secrets) exposed in header types
- `[[nodiscard]]` annotations on security-critical return values
- Thread-safety documentation
- No implementation leakage into public headers

---

## Header Files Audited

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `build_info.h` | `BuildInfo` | ✅ Version strings are `const`; no mutable globals |
| `edition.h` | `Edition`, `edition_name()` | ✅ Enum is scoped (`enum class`); no integer conversion hazard |
| `edition_manager.h` | `EditionManager`, `EditionCapabilities` | ✅ Singleton access is thread-safe (double-checked locking with `std::atomic`) |
| `export.h` | `THEMISDB_API`, `THEMISDB_NO_EXPORT`, `THEMISDB_DEPRECATED` | ✅ Platform guards present for MSVC / GCC / Clang |
| `license_info.h` | `LicenseInfo`, `LicenseStatus`, `LicenseConstraint` | ✅ License payload is opaque; private key material not present in public type |
| `module_hash_verifier.h` | `ModuleHashVerifier`, `HashVerificationResult` | ✅ SHA-256; `[[nodiscard]]` on `verify()`; returns typed result, not raw bool |
| `module_signature_verifier.h` | `ModuleSignatureVerifier`, `SignatureVerificationResult` | ✅ Ed25519 / RSA-PSS; `[[nodiscard]]` on `verify()` |
| `runtime_license_gate.h` | `RuntimeLicenseGate`, `FeatureFlag`, `GateResult` | ✅ `GateResult` includes `LicenseDenialReason` and `message()` |

---

## Findings

### Minor Finding 1 — `GateResult` Denial Reason *(Resolved in v1.7.1)*

| Field | Detail |
|-------|--------|
| **File** | `runtime_license_gate.h` |
| **Severity** | Minor |
| **Status** | Closed |
| **Description** | Historical finding: `GateResult` previously exposed only `allowed` without a structured reason. |
| **Resolution** | `runtime_license_gate.h` now provides `LicenseDenialReason`, `GateResult::denial_reason`, and `GateResult::message()`. |

---

## Audit Checklist

- [x] `THEMISDB_API` applied to all exported classes and free functions
- [x] Scoped enums (`enum class`) used throughout; no implicit integer conversion
- [x] `[[nodiscard]]` on `ModuleHashVerifier::verify()` and `ModuleSignatureVerifier::verify()`
- [x] `[[nodiscard]]` on `RuntimeLicenseGate::check()`
- [x] No private key / license secret material in public type definitions
- [x] Thread-safety of `EditionManager` singleton documented
- [x] `export.h` is self-contained (no transitive includes)
- [x] `GateResult` denial reason *(Finding 1 resolved in v1.7.1)*
