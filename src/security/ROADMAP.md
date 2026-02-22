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

## In Progress 🚧
- [~] Attribute-Based Access Control (ABAC) alongside RBAC (Target: Q2 2026) (Issue: #2464)
- [P] Hardware Security Module (HSM) direct PKCS#11 integration (Target: Q2 2026) (Issue: #2465)
- [I] FIPS 140-2 / 140-3 validated cryptography mode (Target: Q3 2026) (Issue: #2297)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [!] JWT / OIDC federated authentication (OAuth 2.0 provider integration) (Issue: #2458)
- [I] Session token revocation list with real-time invalidation (Issue: #2286)
- [!] Anomaly detection on authentication patterns (brute-force, credential stuffing) (Issue: #2459)
- [!] Row-level security policies in AQL execution (Issue: #2460)
- [I] Secret scanning pre-commit hook for CI pipelines (Issue: #2289)

### Long-term (6-12 months)
- [I] Zero-trust network policy enforcement (per-request identity verification) (Issue: #2461)
- [!] Confidential computing support (Intel TDX / AMD SEV encrypted enclaves) (Issue: #2462)
- [I] Dynamic data masking for PII fields in query results (Issue: #2463)
- [I] SOC 2 Type II compliance evidence collection (Issue: #2293)
- [I] Post-quantum cryptography migration path (CRYSTALS-Kyber, Dilithium) (Issue: #2294)

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

### Phase 2: ABAC & HSM Direct Integration (Status: In Progress 🚧)
- [~] Attribute-Based Access Control (ABAC) alongside RBAC
- [P] Hardware Security Module (HSM) direct PKCS#11 integration
- [~] FIPS 140-2 / 140-3 validated cryptography mode

### Phase 3: Federated Auth & Anomaly Detection (Status: Planned 📋)
- [ ] JWT / OIDC federated authentication (OAuth 2.0 provider integration)
- [ ] Session token revocation list with real-time invalidation
- [ ] Anomaly detection on authentication patterns (brute-force, credential stuffing)
- [ ] Row-level security policies in AQL execution
- [ ] Secret scanning pre-commit hook for CI pipelines

### Phase 4: Zero-Trust & Post-Quantum Cryptography (Status: Planned 📋)
- [ ] Zero-trust network policy enforcement (per-request identity verification)
- [ ] Confidential computing support (Intel TDX / AMD SEV encrypted enclaves)
- [ ] Dynamic data masking for PII fields in query results
- [ ] SOC 2 Type II compliance evidence collection
- [ ] Post-quantum cryptography migration path (CRYSTALS-Kyber, Dilithium)

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (TLS handshake, key rotation, RBAC enforcement)
- [?] Performance benchmarks (encryption overhead, auth latency)
- [?] Security audit (penetration testing, CVE dependency scan)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- HSM integration uses RSA-PKCS#1 v1.5 for DEK wrapping; RSA-OAEP upgrade is planned for v1.5.0.
- FIPS 140-2 mode requires a FIPS-validated OpenSSL build; not bundled by default.
- AQL injection detection uses pattern matching; semantic analysis planned for v1.5.0.

## Breaking Changes
- SecurityManager API is stable from v1.x.
- Key management API may gain new rotation hooks in v1.5.0; backward-compatible.
- DEK versioning scheme is fixed; no breaking changes planned.
