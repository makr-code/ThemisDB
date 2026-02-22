# User Storage Encrypted Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

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

## Research / References

- [ ] TODO: Add reference – *Searchable Symmetric Encryption: Survey* (DOI / arXiv placeholder)
- [ ] TODO: Add reference – *Homomorphic Encryption Standardisation* (URL: homomorphicencryption.org)
- [ ] TODO: Add reference – *NIST Post-Quantum Cryptography Standards* – FIPS 203/204/205 (URL placeholder)
- [ ] TODO: Add reference – *Open Policy Agent: Policy-Based Control Plane* (URL: openpolicyagent.org)
- [ ] TODO: Add reference – *BSI IT-Grundschutz Compendium* (URL placeholder)
