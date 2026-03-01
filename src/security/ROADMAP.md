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

## In Progress 🚧
- [~] FIPS 140-2 / 140-3 validated cryptography mode (Target: Q3 2026) (Issue: #2297)
  - Requires FIPS-validated OpenSSL build; cipher suites restricted to approved list
- [x] Confidential computing support (Intel TDX / AMD SEV encrypted enclaves) (Issue: #2462)
  - Subsystems: `security/confidential_computing.h`, `security/confidential_computing.cpp`
  - Intel TDX: CPUID leaf 0x21 detection + `/dev/tdx_guest` kernel driver + `TDX_CMD_GET_REPORT0` ioctl
  - AMD SEV/SEV-SNP: CPUID leaf 0x8000_001F + MSR 0xC001_0131 probe + `/dev/sev-guest` + `SNP_GET_REPORT` ioctl
  - AES-256-GCM seal/unseal bound to TEE measurement (MRTD for TDX, MEASUREMENT field for SEV-SNP)
  - Software fallback for non-TEE environments (CI, developer machines)
  - Tests: `tests/test_confidential_computing.cpp` — detection, attestation, seal/unseal round-trip, tamper detection, independent-instance key isolation

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)
- [I] SOC 2 Type II compliance evidence collection (Issue: #2293)
  - Scope: audit-log export, metrics snapshot, key-rotation records, access-control reports
  - Storage: append-only JSON/CBOR log with tamper-evident chain; 12-month retention
  - Tests: evidence completeness check, retention enforcement
  - Target: Q4 2026
- [P] Post-quantum cryptography migration path (CRYSTALS-Kyber, Dilithium) (Issue: #2294)
  - Scope: replace RSA-OAEP DEK wrapping (HSM) with Kyber-1024; replace ECDSA with Dilithium-5 for CMS signing
  - Backward compat: hybrid mode (classical + PQ) during migration; PQ-only in final phase
  - Tests: classical/PQ parity tests; Kyber decapsulation round-trip; performance baseline ≥ 2000 ops/s
  - Implementation: `include/security/post_quantum_crypto.h`, `src/security/post_quantum_crypto.cpp`
  - Backend: OpenSSL simulation (X25519→Kyber, Ed25519→Dilithium); liboqs swap-in ready
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
- [ ] SOC 2 Type II compliance evidence collection
- [P] Post-quantum cryptography migration path (CRYSTALS-Kyber, Dilithium)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (QueryMaskingPolicy, RLSManager, ZeroTrustPolicyEnforcer, AuthRateLimiter, HsmProvider)
- [x] Integration tests (TLS handshake, key rotation, RBAC enforcement, RLS filtering, JWT revocation)
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
