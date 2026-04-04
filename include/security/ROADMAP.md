<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

# Roadmap — Security Module (Public Headers)

## Current Status

Public headers at v1.5.0. All core security contracts are stable and production-ready.  
Implementation: `../../src/security/` at v1.5.0.

---

## Completed

- [x] Core crypto interfaces: `encryption.h`, `signing.h`, `key_provider.h`
- [x] RBAC and RLS policy interfaces: `rbac.h`, `row_level_security.h`
- [x] HSM provider chain: `hsm_provider.h`, `hsm_key_provider_adapter.h`, `pkcs11_wrapper.h`
- [x] Vault integration: `vault_key_provider.h`, `vault_signing_provider.h`
- [x] Post-quantum interfaces: `post_quantum_crypto.h`
- [x] Confidential computing: `confidential_computing.h`
- [x] Zero-trust enforcement: `zero_trust_policy_enforcer.h`
- [x] FIPS mode: `fips_crypto_mode.h`
- [x] TSA/RFC 3161: `timestamp_authority.h`, `tsa_api.h`

---

## Planned Features

- [ ] `threshold_signing.h` — multi-party threshold signature interface (Target: Q3 2026)
  - Inputs: signing shares from N-of-M participants
  - Outputs: combined signature verifiable by standard `IVerifier`
  - Constraints: must be compatible with existing `signing.h` `Signature` type
  - Tests: unit + integration with HSM-backed shares
- [ ] `audit_trail.h` — append-only tamper-evident audit log interface (Target: Q3 2026)
  - Outputs: structured `AuditEntry` with RFC 3161 timestamp
  - Constraints: no deletion API; append-only by contract
  - Tests: property-based tamper detection
- [ ] `secure_enclave_kv.h` — hardware-backed KV store interface for key caching (Target: Q4 2026)
  - Inputs: key handle + plaintext value (max 4 KiB)
  - Outputs: opaque encrypted blob backed by TEE/SGX
  - Tests: SGX simulation + real hardware CI path

---

## Implementation Phases

### Phase 1: Design / API Contract ✅
- All core security interfaces defined with stable ABIs

### Phase 2: Core Implementation ✅
- 41 public headers covering crypto, identity, policy, and compliance

### Phase 3: Error Handling & Edge Cases ✅
- All interfaces return `std::expected` or throw typed exceptions

### Phase 4: Tests ✅
- Full test coverage via `../../src/security/tests/`

### Phase 5: Performance / Hardening ✅
- HSM metrics, FIPS mode, and transport security validation

### Phase 6: Documentation & Sign-off ✅
- This document

---

## Production Readiness Checklist

- [x] All headers compile with `-Wall -Wextra -Wpedantic`
- [x] No implementation code in public headers
- [x] All interfaces documented with Doxygen comments
- [x] `mock_key_provider.h` isolated to test builds only
- [x] No circular include dependencies
- [x] PQC interface added before v2.0 breaking change window

---

## Known Issues & Limitations

- PQC algorithm names in `post_quantum_crypto.h` may require revision after NIST final
  publication of FIPS 204/205/206.
- `confidential_computing.h` attestation flow is Intel TDX/SGX-centric; AMD SEV support
  is planned for Q4 2026.
- `vram_secure_clear.h` GPU zeroing is best-effort when the driver does not expose secure
  memory APIs; CPU fallback is always applied.
