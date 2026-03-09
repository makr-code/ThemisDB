<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Security Module Roadmap

## Current Status
v1.x – Enterprise-grade, defense-in-depth security infrastructure. Six distinct security layers (transport, authentication, authorization, data protection, audit/compliance, and threat detection) are production-ready.

## Completed ✅
- [x] Transport security: TLS 1.3 and mutual TLS (mTLS)
- [x] Authentication: USB admin key, PKI certificates, multi-factor auth
- [x] RBAC with role hierarchy and permission inheritance
- [x] Field-level AES-256-GCM encryption (document, array, and VRAM fields)
- [x] Key management hierarchy (Master Key → KEK → DEK) with HSM support
- [x] Key rotation (active, deprecated, rotating DEK states)
- [x] HashiCorp Vault integration for key storage
- [x] CMS/PKCS#7 signing and eIDAS-compliant timestamping
- [x] Malware scanner for plugin manifests
- [x] AQL injection detection
- [x] SecurityManager orchestrator
- [x] Audit log with tamper-evident chaining
- [x] Compliance features (eIDAS, GDPR-related controls)
- [x] Attribute-Based Access Control (ABAC) alongside RBAC
- [x] Zero-trust network policy enforcement (per-request identity verification)
- [x] Dynamic data masking for PII fields in query results (`QueryMaskingPolicy`, PR: #3050, v1.5.0)
- [x] Row-level security policies in AQL execution (`RLSManager`, `RLSPolicy`, `RLSPredicate`, `AccessControlManager` integration)
- [x] JWT / OIDC federated authentication (OAuth 2.0 provider integration)
- [x] Session token revocation list with real-time invalidation (`TokenBlacklist`)
- [x] Anomaly detection on authentication patterns: brute-force and credential stuffing (`AuthRateLimiter`)
- [x] Post-quantum cryptography migration path (CRYSTALS-Kyber / Dilithium) (`include/security/post_quantum_crypto.h`, `src/security/post_quantum_crypto.cpp`)
- [x] Systematic attack vector test suite (`tests/security/attack-vectors/crypto/`, `injection/`, `authentication/`)

## In Progress 🚧
- [~] FIPS 140-2 / 140-3 validated cryptography mode (Target: Q3 2026) (Issue: #2297)
  - Requires FIPS-validated OpenSSL build; cipher suites restricted to approved list

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)
- [I] SOC 2 Type II compliance evidence collection (Issue: #2293)
  - Scope: audit-log export, metrics snapshot, key-rotation records, access-control reports
  - Storage: append-only JSON/CBOR log with tamper-evident chain; 12-month retention
  - Tests: evidence completeness check, retention enforcement
  - Target: Q4 2026

## Implementation Phases

### Phase 1: Transport, Authentication & Data Protection (Status: Completed ✅)
- [x] Transport security: TLS 1.3 and mutual TLS (mTLS)
- [x] Authentication: USB admin key, PKI certificates, multi-factor auth
- [x] RBAC with role hierarchy and permission inheritance
- [x] Field-level AES-256-GCM encryption (document, array, and VRAM fields)
- [x] Key management hierarchy (Master Key → KEK → DEK) with HSM support
- [x] Key rotation (active, deprecated, rotating DEK states)
- [x] HashiCorp Vault integration for key storage
- [x] CMS/PKCS#7 signing and eIDAS-compliant timestamping
- [x] Malware scanner for plugin manifests
- [x] AQL injection detection
- [x] `SecurityManager` orchestrator
- [x] Audit log with tamper-evident chaining
- [x] Compliance features (eIDAS, GDPR-related controls)

### Phase 2: ABAC & HSM Direct Integration (Status: Completed ✅)
- [x] Attribute-Based Access Control (ABAC) alongside RBAC
- [x] Hardware Security Module (HSM) direct PKCS#11 integration
- [x] PKCS#11 C++ wrapper interface (`include/security/pkcs11_wrapper.h`, Issue: #3252)
  - RAII `Pkcs11Library` (load/unload dynamic library)
  - RAII `Pkcs11Session` (open/close/login/logout)
  - `Pkcs11Category` + `makePkcs11Error()` for `std::error_code` integration
  - Free helpers: `listSlots`, `findObjects`, `findObjectsByLabel`, `signData`,
    `verifyData`, `encryptData`, `decryptData`, `generateRsaKeyPair`,
    `getAttribute`, `getAttributeBytes`
  - Tests: `tests/test_pkcs11_wrapper.cpp` (pure-unit + optional SoftHSM2 integration)
- [~] FIPS 140-2 / 140-3 validated cryptography mode

### Phase 3: Federated Auth & Anomaly Detection (Status: Completed ✅)
- [x] JWT / OIDC federated authentication (OAuth 2.0 provider integration)
- [x] Session token revocation list with real-time invalidation (`TokenBlacklist`)
- [x] Anomaly detection on authentication patterns (brute-force, credential stuffing) (`AuthRateLimiter`)
- [x] Row-level security policies in AQL execution (`RLSManager`, `AccessControlManager` integration)

### Phase 4: Zero-Trust & Post-Quantum Cryptography (Status: In Progress 🚧)
- [x] Zero-trust network policy enforcement (per-request identity verification)
- [x] Confidential computing support (Intel TDX / AMD SEV encrypted enclaves)
- [x] Dynamic data masking for PII fields in query results (`QueryMaskingPolicy`, PR: #3050, v1.5.0)
- [x] Secret scanning pre-commit hook for CI pipelines (`scripts/secret_scan.py`, `.pre-commit-config.yaml`, `.github/workflows/secret-scanning-ci.yml`)
- [x] Post-quantum cryptography migration path (CRYSTALS-Kyber, Dilithium) — `include/security/post_quantum_crypto.h`, `src/security/post_quantum_crypto.cpp`
  - KyberKEM: key generation, encapsulate/decapsulate round-trip, all three security levels (512/768/1024)
  - DilithiumSigner: sign/verify round-trip, all three security levels (2/3/5)
  - PostQuantumKeyProvider: Kyber-wrapped DEK wrapKeyWithKyber / unwrapKeyWithKyber
  - HybridEncryption: HYBRID / CLASSICAL_ONLY / POST_QUANTUM_ONLY modes; AES-256-GCM + Kyber-1024
  - Tests (production-ready): `tests/test_post_quantum_crypto.cpp` — 27 test cases; throughput ≥ 2 000 ops/s
- [x] Systematic attack vector test suite (`tests/security/attack-vectors/`)
  - `crypto/test_crypto_attack_vectors.cpp` — IV/nonce reuse, tag tampering, bit-flip, key confusion, PQ key confusion, signature forgery
  - `injection/test_injection_attack_vectors.cpp` — AQL injection: comment markers, dangerous ops, boolean-blind, union, stacked queries, case bypass, oversized params
  - `authentication/test_authentication_attack_vectors.cpp` — RBAC: privilege escalation, permission boundary, lateral movement, deleted/unknown roles, role injection, multi-role combinations
- [ ] SOC 2 Type II compliance evidence collection (Target: Q4 2026, Issue: #2293)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (QueryMaskingPolicy, RLSManager, ZeroTrustPolicyEnforcer, AuthRateLimiter, HsmProvider, KyberKEM, DilithiumSigner, HybridEncryption)
- [x] Integration tests (TLS handshake, key rotation, RBAC enforcement, RLS filtering, JWT revocation)
- [x] Attack vector tests (crypto IV/tag/key confusion, injection, authentication privilege escalation)
- [~] Performance benchmarks (encryption overhead, auth latency) (Target: Q2 2026)
- [~] Security audit (penetration testing, CVE dependency scan) (Target: Q2 2026)
- [x] Documentation complete (ROADMAP.md, FUTURE_ENHANCEMENTS.md, inline docblocks)
- [x] API stability guaranteed (SecurityManager v1.x, RLSManager v1.5.0, QueryMaskingPolicy v1.5.0)

## Known Issues & Limitations
- HSM integration uses RSA-OAEP (SHA-256 / MGF1-SHA-256) for DEK wrapping via PKCS#11 C_Encrypt/C_Decrypt.
- FIPS 140-2 mode requires a FIPS-validated OpenSSL build; not bundled by default.
- AQL injection detection uses pattern matching; semantic analysis deferred to v1.6.0+.
- Zero-trust `ZeroTrustPolicyEnforcer` supports IPv4 CIDR policies only; IPv6 support planned for a follow-up.

## Breaking Changes
- SecurityManager API is stable from v1.x.
- Key management API is stable in v1.5.0; additional rotation hooks planned for v1.6.0+.
- DEK versioning scheme is fixed; no breaking changes planned.
