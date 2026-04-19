<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md -->

# Security Notes — Security Module (Public Headers)

## Scope

This document covers security considerations for the public headers in `include/security/`.
For implementation-level security analysis see `../../src/security/SECURITY.md`.

---

## Threat Model

| Threat | Impact | Mitigation |
|--------|--------|-----------|
| Key material exposure via header | Critical | `KeyMaterial` uses opaque handle; raw bytes never in public API |
| ABI breakage leaking internals | High | All headers use PIMPL or pure virtual; no private fields exposed |
| Mock provider in production | High | `mock_key_provider.h` guarded by `THEMIS_TESTING` macro; build system excludes it |
| Type confusion in crypto dispatch | High | Strongly-typed algorithm enums; no raw integer casts |
| FIPS bypass via default algorithm | High | `FipsCryptoMode::enforce()` must be called at startup; checked in `encryption.h` factory |
| Injection via AQL queries | High | `AqlInjectionDetector` interface mandatory for all query paths |
| PQC algorithm negotiation downgrade | Medium | `post_quantum_crypto.h` requires explicit algorithm selection; no fallback to classical |

---

## Security Controls

- **No raw key material** in any public header type; all keys are opaque handles or interfaces.
- **Algorithm agility** through interface abstractions prevents hard-coded cipher lock-in.
- **Zero-trust boundary** enforced at every `IAccessControl::evaluate()` call site.
- **GDPR compliance** facilitated by `IPiiRedactionPolicy` and `IQueryMaskingPolicy`.
- **Auditability** guaranteed by `SecurityEvidenceCollector` at every privileged operation.

---

## Known Limitations

- `confidential_computing.h` TEE attestation is platform-specific; implementations differ per
  vendor.
- `vram_secure_clear.h` depends on GPU driver support; fallback is a CPU memset.
- PQC algorithms in `post_quantum_crypto.h` are pre-standardization interfaces; may change with
  NIST finalization.
