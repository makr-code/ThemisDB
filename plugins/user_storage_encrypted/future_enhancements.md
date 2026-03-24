# User Storage Encrypted Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

## ✅ Implemented (graduated from this backlog)

| Item | Version | Notes |
|------|---------|-------|
| Stdin key delivery (no `/tmp` password file) | v0.1.0 | `executeCommandWithStdin` + `deliverKeyViaStdin`; `explicit_bzero` |
| Argon2id KDF per-container key derivation | v0.1.0 | `Argon2idKeyDerivationService`; m=65536/t=3/p=4; salt in `.themis_kdf_salt` |
| Key rotation persistence via `IRotationStore` | v0.1.0 | `last_check_ms` + `interval_days` persisted after every rotation callback |
| Startup stale mount reconciliation | v0.2.0 | `reconcileStaleMounts()` scans `/proc/mounts`; `fusermount -u` / `umount` fallback; non-fatal |

---

## Scope

- Enhancements to the encrypted user storage plugin: new encryption algorithms (ChaCha20-Poly1305, post-quantum KEM), expanded HSM/PKCS#11 support, ABAC policy engine, and compliance tooling (audit log export, GDPR erasure).
- 4-tier security model: gocryptfs AES-256-GCM, HashiCorp Vault, HSM/PKCS#11, and streng-geheim (M-of-N threshold).
- Out of scope: changes to ThemisDB query engine or vector storage; this plugin only manages encryption lifecycle and key management.
- Covers Cloud KMS integration (AWS KMS, Azure Key Vault) as alternatives to Vault.

## Design Constraints

- [ ] HSM PKCS#11 operations MUST never expose raw key material in process memory; all cryptographic operations happen inside the HSM.
- [ ] Vault tokens MUST have a short TTL (≤ 1 hour); automatic renewal MUST occur at 75 % of TTL elapsed.
- [ ] All tier transitions (e.g., confidential → streng-geheim) MUST produce an audit log entry with timestamp, user, and reason.
- [ ] AES-256-GCM encryption MUST use AES-NI hardware acceleration when available; software fallback MUST be explicitly configured.
- [ ] Key rotation MUST be online (no downtime); old key remains active for reads during rotation window (≤ 10 s per tenant).
- [ ] GDPR erasure requests MUST be fulfilled by deleting the tenant's data-encryption key; ciphertext becomes irrecoverable within ≤ 30 s.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IEncryptedStorageBackend` | `UserStorageEncryptedPlugin` | `encrypt`, `decrypt`, `rotate_key`, `erase_tenant` |
| `IKeyProvider` | `IEncryptedStorageBackend` impls | Vault, HSM/PKCS#11, AWS KMS, Azure Key Vault |
| `IAuditLogger` | `UserStorageEncryptedPlugin` | Logs tier transitions, key rotations, access events; tamper-evident |
| `IAccessPolicyEngine` | `UserStorageEncryptedPlugin` | OPA-based ABAC; evaluates `allow/deny` per (principal, tier, operation) |
| `IThresholdKeyReconstructor` | `streng-geheim` tier | M-of-N Shamir secret sharing; requires M key-holder confirmations |

## Idea Backlog

### Cryptographic Enhancements

- [ ] **Searchable symmetric encryption (SSE)** – allow AQL queries over encrypted fields without decryption.
- [ ] **Homomorphic encryption** – compute aggregate statistics on encrypted data.
- [ ] **Threshold cryptography** – require M-of-N key holders to unlock streng-geheim tier.
- [ ] **Key escrow** – secure key recovery mechanism for regulated industries.

### Access Control

- [ ] **Dynamic policy engine** – OPA (Open Policy Agent) integration for expressive ABAC policies.
- [ ] **Time-bounded access** – grant access to a security level for a fixed duration only.
- [ ] **Break-glass access** – emergency override with mandatory audit notification.
- [ ] **Separation of duties** – require dual authorisation for highest security tier.

### Compliance & Regulation

- [ ] **BSI IT-Grundschutz compliance mapping** – document alignment with German BSI standards.
- [ ] **ISO 27001 / SOC 2 artefacts** – generate compliance evidence automatically.
- [ ] **GDPR right-to-erasure** – cryptographic deletion (key destruction) for data removal.
- [ ] **NIS2 incident reporting hooks** – trigger alerts on anomalous access patterns.

### Platform

- [ ] **macOS stable support** – macFUSE tested in CI.
- [ ] **EncFS / CryFS alternative backends** – pluggable filesystem encryption backend.
- [ ] **Cloud KMS** – AWS KMS / Azure Key Vault as key provider alternatives to HashiCorp Vault.

---

## Test Strategy

- AES-NI throughput tests: encrypt 1 GB with AES-256-GCM; assert ≥ 1 GB/s on hardware with AES-NI; assert software fallback also completes correctly.
- Key rotation tests: rotate a tenant key while 100 concurrent reads are in flight; assert zero read failures and rotation completes within 10 s.
- PKCS#11 tests (using SoftHSM2 in CI): assert that raw key material is never accessible via process memory dumps after HSM operations.
- GDPR erasure tests: delete tenant key; assert all ciphertext in that tenant's collection becomes unreadable within 30 s.
- Vault token renewal tests: expire token at 75 % TTL; assert automatic renewal succeeds and no operation returns `VAULT_TOKEN_EXPIRED`.
- Audit log tamper tests: modify an audit log entry; assert integrity check detects the modification.

## Performance Targets

- AES-256-GCM encrypt/decrypt throughput ≥ 1 GB/s with AES-NI hardware acceleration (single core).
- Key rotation completion ≤ 10 s per tenant (online rotation, no downtime).
- HSM PKCS#11 sign/verify operation ≤ 5 ms per call (SoftHSM2 baseline; hardware HSM ≤ 1 ms).
- Vault secret fetch latency ≤ 10 ms p99 (local Vault agent cache; ≤ 50 ms without cache).
- ABAC policy evaluation ≤ 1 ms per access decision (OPA in-process evaluation).

## Security / Reliability

- HSM PKCS#11 operations MUST never expose raw key material in process memory; use `CKA_SENSITIVE` + `CKA_EXTRACTABLE=FALSE` attributes.
- Vault tokens MUST have TTL ≤ 1 hour; renewal at 75 % TTL; failure to renew MUST halt new encrypt/decrypt operations and alert.
- All tier transitions, key rotations, and GDPR erasure events MUST be written to the tamper-evident audit log before the operation is considered complete.
- GDPR erasure MUST be fulfilled by deleting the tenant's DEK in the key provider; no plaintext data destruction is required (ciphertext is irrecoverable).
- Break-glass access MUST send an immediate alert to all configured security contacts and require post-access justification within 24 hours.

## Research / References

- D. X. Song, D. Wagner, and A. Perrig, "Practical techniques for searches on encrypted data," in *Proc. 2000 IEEE Symp. Security and Privacy*, 2000, pp. 44–55. DOI: [10.1109/SECPRI.2000.848445](https://doi.org/10.1109/SECPRI.2000.848445)
- C. Gentry, "Fully homomorphic encryption using ideal lattices," in *Proc. 41st Annual ACM Symp. Theory of Computing (STOC)*, 2009, pp. 169–178. DOI: [10.1145/1536414.1536440](https://doi.org/10.1145/1536414.1536440)
- D. J. Bernstein and T. Lange, "Post-quantum cryptography," *Nature*, vol. 549, pp. 188–194, 2017. DOI: [10.1038/nature23461](https://doi.org/10.1038/nature23461)
- NIST, "FIPS 203: Module-lattice-based key-encapsulation mechanism standard," National Institute of Standards and Technology, 2024. DOI: [10.6028/NIST.FIPS.203](https://doi.org/10.6028/NIST.FIPS.203)
- M. S. Islam, M. Kuzu, and M. Kantarcioglu, "Access pattern disclosure on searchable encryption: Ramification, attack and mitigation," in *Proc. 19th Annual Network and Distributed System Security Symp. (NDSS)*, 2012.
