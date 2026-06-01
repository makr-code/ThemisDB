> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/security/ROADMAP.md -->

# Security Module — Public Header Roadmap

**Module Path:** `include/security/`
**Canonical implementation roadmap:** [`../../src/security/ROADMAP.md`](../../src/security/ROADMAP.md)

---

## Overview

Tracks public security API contract stability, planned header additions, and breaking changes. Feature items affecting both implementation and headers are tracked in:

→ [`../../src/security/ROADMAP.md`](../../src/security/ROADMAP.md)

---

## Current Status

All 51 production security headers are present. `IKeyProvider`, `ISigningProvider`, and `IHSMProvider` interfaces are stable. PQC primitives (`post_quantum_crypto.h`) are experimental. `BehavioralAnomalyDetector` and `AQLInjectionDetector` are production-ready. `FIPSCryptoMode` and `ConfidentialComputingConfig` are present and enforced at startup.

---

## Completed ✅

- [x] `access_control.h` / `access_control_manager.h` / `rbac.h` / `row_level_security.h` — access control stack
- [x] `encryption.h` / `key_provider.h` / `secret_manager.h` — encryption and key management
- [x] `vault_key_provider.h` / `pki_key_provider.h` — external key providers
- [x] `hsm_key_provider_adapter.h` / `hsm_provider.h` / `pkcs11_wrapper.h` — HSM and PKCS#11
- [x] `signing.h` / `signing_provider.h` / `vault_signing_provider.h` / `cms_signing.h` — signing stack
- [x] `tsa_api.h` / `timestamp_authority.h` — RFC 3161 timestamping
- [x] `behavioral_anomaly_detector.h` / `intent_classifier.h` — ML-based anomaly detection
- [x] `aql_injection_detector.h` / `prompt_injection_pattern_registry.h` — injection detection
- [x] `pii_redaction_policy.h` / `query_masking_policy.h` / `output_encoding.h` — PII and output safety
- [x] `fips_crypto_mode.h` — FIPS 140-2/3 enforcement
- [x] `post_quantum_crypto.h` — Kyber/Dilithium primitives (experimental API)
- [x] `zero_trust_policy_enforcer.h` / `confidential_computing.h` — zero-trust and TEE
- [x] `security_evidence_collector.h` — compliance evidence aggregation

---

## In Progress

- [ ] Stabilize `post_quantum_crypto.h` API (Kyber-1024, Dilithium-3 key sizes) — Target: 2026-Q3
- [ ] Add `IKeyProvider::rotateKey()` to support zero-downtime key rotation — Target: 2026-Q3

---

## Planned

- [ ] `secret_rotation_policy.h` — declarative secret rotation schedule (Target: 2026-Q3)
- [ ] `hardware_attestation.h` — SGX/TDX remote attestation interface (Target: 2026-Q4)
- [ ] `audit_trail_sealer.h` — cryptographic audit log sealing (Target: 2026-Q4)
- [ ] Promote `post_quantum_crypto.h` from experimental to stable after NIST FIPS finalization (Target: 2027-Q1)

---

## Breaking Change History

None in v1.x. `IKeyProvider` interface is stable. `post_quantum_crypto.h` is explicitly experimental and may change. Any breaking change requires a MAJOR version bump.
