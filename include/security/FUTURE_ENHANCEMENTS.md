> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/security/FUTURE_ENHANCEMENTS.md -->

# Security Module — Public Header Future Enhancements

**Module Path:** `include/security/`
**Canonical implementation enhancements:** [`../../src/security/FUTURE_ENHANCEMENTS.md`](../../src/security/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/security/`. Implementation-level enhancements are in:

→ [`../../src/security/FUTURE_ENHANCEMENTS.md`](../../src/security/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` `IKeyProvider`, `ISigningProvider`, `IHSMProvider` must remain pure-virtual interfaces — no implementation in headers.
- `[x]` `post_quantum_crypto.h` is explicitly experimental; callers must define `THEMIS_ENABLE_PQC`.
- `[x]` `input_validator.hpp` is a template header; all specializations must be explicitly instantiated.
- `[x]` `FIPSCryptoMode` must be enforced before any `EncryptionProvider` use at startup.
- `[x]` `output_encoding.h` must be the sole output sanitization path; bypass is prohibited.
- `[x]` Security headers must not include server or storage headers (no reverse dependency).

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `IKeyProvider::getKey()` | `key_provider.h` | `EncryptionProvider`, `HSMKeyProviderAdapter` | ✅ Stable |
| `IHSMProvider::generateKey()` | `hsm_provider.h` | `HSMKeyProviderAdapter` | ✅ Stable |
| `BehavioralAnomalyDetector::score()` | `behavioral_anomaly_detector.h` | Server middleware | ✅ Stable |
| `AQLInjectionDetector::check()` | `aql_injection_detector.h` | Query API handler | ✅ Stable |
| `ZeroTrustPolicyEnforcer::verify()` | `zero_trust_policy_enforcer.h` | Auth middleware | ✅ Stable |
| `RBACPolicy::allow()` | `rbac.h` | Access control manager | ✅ Stable |
| `RowLevelSecurityPolicy::predicate()` | `row_level_security.h` | Query execution | ✅ Stable |
| `PIIRedactionPolicy::apply()` | `pii_redaction_policy.h` | Response transformer | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- `secret_rotation_policy.h` — `ISecretRotationPolicy` interface for declarative rotation schedules; integrates with `SecretManager::scheduleRotation()`.
- `IKeyProvider::rotateKey(keyId)` — zero-downtime key rotation; returns `RotationResult` with old/new version IDs.
- Stabilize `post_quantum_crypto.h` key-size API surface (Kyber-1024, Dilithium-3) pending NIST FIPS publication.

### Medium-Term (Q4 2026)

- `hardware_attestation.h` — `IAttestationProvider` interface for SGX/TDX remote attestation; returns `AttestationReport` with quote and nonce.
- `audit_trail_sealer.h` — `AuditTrailSealer::seal(logSegment)` cryptographic sealing using HSM-backed HMAC chains; immutability guarantee for compliance.
- `differential_privacy.h` — `DifferentialPrivacyGuard` wrapper applying ε-DP noise before query result emission.

### Long-Term

- Unified crypto policy object: `CryptoPolicy` declarative struct replacing per-component algorithm flags.
- FIDO2/WebAuthn token authentication: `FIDO2Authenticator` interface for hardware security key admin auth.
- Post-quantum-ready TLS config via `TransportSecurityChecker`; depends on BoringSSL PQC stabilization.
