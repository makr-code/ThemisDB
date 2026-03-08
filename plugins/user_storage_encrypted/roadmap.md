# User Storage Encrypted Plugin – Roadmap

## Current Status

**Status:** 🔧 Implemented (WIP / validation pending)

Entry-point: `plugins/user_storage_encrypted/` · `src/` (TODO: confirm exact source file location)

| Feature | Status |
|---------|--------|
| 4-tier security classification | ✅ Implemented |
| gocryptfs AES-256-GCM encryption | ✅ Implemented |
| HashiCorp Vault integration | ✅ Implemented |
| HSM / PKCS#11 (streng-geheim) | ✅ Implemented |
| Automatic key rotation | ✅ Implemented |
| Apache Ranger RBAC | 🔧 Integration-ready |

---

## In Progress

- [~] Integration tests for all four security levels using a test Vault instance
- [~] Zero-downtime key rotation smoke test in CI
- [~] Confirmation and documentation of exact source file(s) in `src/`

## Planned Features

- [ ] **Windows stable support** – VeraCrypt or native NTFS encryption (Target: Q3 2026)
- [ ] **Audit log** – record every access/modification with timestamp, user, security level (Target: Q3 2026)
- [ ] **Vault AppRole / Kubernetes auth** – replace token-based Vault auth (Target: Q3 2026)
- [ ] Performance benchmark: encryption/decryption throughput per security level (Target: Q3 2026)
- [ ] **Post-quantum cryptography** – ML-KEM / SLH-DSA for streng-geheim tier (Target: Q4 2026)
- [ ] **FIDO2 / WebAuthn** – hardware security key authentication (Target: 2027)
- [ ] **Attribute-Based Encryption (ABE)** – fine-grained access control (Target: 2027)
- [ ] Formal security audit / penetration test and remediation (Target: 2027)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Confirm and document the exact source file(s) in `src/` that implement this plugin.
- [ ] Add integration tests for all four security levels using a test Vault instance.
- [ ] Validate key rotation without service downtime (zero-downtime smoke test in CI).
- [ ] Document minimum gocryptfs and FUSE version requirements.

## Mid-term Goals (1–3 months)

- [ ] **Windows support** – promote from "experimental" to stable on Windows using VeraCrypt or native NTFS encryption.
- [ ] **Audit log** – record every access/modification with timestamp, user, security level to ThemisDB timeline storage.
- [ ] **Vault AppRole / Kubernetes auth** – replace token-based Vault auth with more secure methods.
- [ ] Performance benchmark: measure encryption/decryption throughput per security level.

## Long-term Goals (3–12 months)

- [ ] **Post-quantum cryptography** – integrate NIST PQC algorithms (ML-KEM, SLH-DSA) for streng-geheim tier.
- [ ] **FIDO2 / WebAuthn** – hardware security key integration for authentication.
- [ ] **Attribute-Based Encryption (ABE)** – fine-grained access control using ABE schemes.
- [ ] Formal security audit / penetration test and remediation.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| Windows stable support | TODO | 🔲 Planned |
| Full audit log | TODO | 🔲 Planned |
| Post-quantum crypto tier | TODO | 🔲 Planned |
| Security audit | TODO | 🔲 Planned |

## Implementation Phases

### Phase 1 – Integration Tests & Key Rotation
- [ ] Docker Compose fixture: Vault dev server + ThemisDB for all four security levels
- [ ] Integration tests: encrypt, store, retrieve, and decrypt for each tier
- [ ] Zero-downtime key rotation: rotate key while active readers/writers are running; assert no data loss
- [ ] Confirm and document exact source file location in `src/`

### Phase 2 – Windows Support & Audit Log
- [ ] Windows implementation: evaluate VeraCrypt CLI wrapper vs. native NTFS EFS
- [ ] Audit log: append-only ThemisDB timeline entries (user, operation, security level, timestamp)
- [ ] Vault AppRole and Kubernetes auth methods replacing static token authentication
- [ ] Encryption/decryption throughput benchmark per security tier (MB/s)

### Phase 3 – Vault AppRole & Post-Quantum Crypto Tier
- [ ] Vault AppRole auth: role_id + secret_id rotation integrated with CI
- [ ] Post-quantum tier: ML-KEM-1024 key encapsulation (liboqs / NIST PQC reference)
- [ ] Hybrid classical + PQ encryption for streng-geheim tier
- [ ] Define and document Vault unavailability policy (read-only vs. full lockout)

### Phase 4 – FIDO2, ABE & Security Audit
- [ ] FIDO2 / WebAuthn: hardware security key as second factor for streng-geheim tier
- [ ] Attribute-Based Encryption (ABE): policy-based key derivation using charm-crypto or libbswabe
- [ ] Formal penetration test and security audit; remediate all findings
- [ ] gocryptfs mount management: define ownership (plugin vs. external service)

---

## Dependencies

- gocryptfs ≥ 2.0 + FUSE
- HashiCorp Vault (optional, required for vs-nfd+)
- HSM with PKCS#11 (required for streng-geheim)
- Apache Ranger (optional, for RBAC)
- ThemisDB `BaseEntity` storage layer

## Open Questions

- [ ] Should the plugin manage gocryptfs mounts directly or delegate to a separate service?
- [ ] What happens when Vault is temporarily unavailable – read-only mode or full lockout?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| AES-256-GCM encryption (gocryptfs) | ✅ Implemented |
| HashiCorp Vault integration | ✅ Implemented |
| HSM / PKCS#11 (streng-geheim tier) | ✅ Implemented |
| 4-tier security classification | ✅ Implemented |
| Integration tests for all four security levels | ❌ Pending |
| Zero-downtime key rotation tested | ❌ Pending |
| Windows stable support | ❌ Not implemented |
| Audit log | ❌ Not implemented |
| Post-quantum cryptography tier | ❌ Not implemented |
| Formal security audit completed | ❌ Pending |

## Known Issues & Limitations

- Exact source file location in `src/` has not been confirmed; documentation is incomplete
- gocryptfs mount management strategy is unclear: direct plugin control vs. external service
- Vault unavailability behavior (read-only mode vs. full lockout) is not yet decided or implemented
- Formal security audit / penetration test has not been performed; production use of streng-geheim tier is at operator risk until audit is complete

---

*See also: [`future_enhancements.md`](future_enhancements.md)*
