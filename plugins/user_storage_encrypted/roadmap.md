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

*See also: [`future_enhancements.md`](future_enhancements.md)*
