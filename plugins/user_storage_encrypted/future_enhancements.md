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

- D. X. Song, D. Wagner, and A. Perrig, "Practical techniques for searches on encrypted data," in *Proc. 2000 IEEE Symp. Security and Privacy*, 2000, pp. 44–55. DOI: [10.1109/SECPRI.2000.848445](https://doi.org/10.1109/SECPRI.2000.848445)
- C. Gentry, "Fully homomorphic encryption using ideal lattices," in *Proc. 41st Annual ACM Symp. Theory of Computing (STOC)*, 2009, pp. 169–178. DOI: [10.1145/1536414.1536440](https://doi.org/10.1145/1536414.1536440)
- D. J. Bernstein and T. Lange, "Post-quantum cryptography," *Nature*, vol. 549, pp. 188–194, 2017. DOI: [10.1038/nature23461](https://doi.org/10.1038/nature23461)
- NIST, "FIPS 203: Module-lattice-based key-encapsulation mechanism standard," National Institute of Standards and Technology, 2024. DOI: [10.6028/NIST.FIPS.203](https://doi.org/10.6028/NIST.FIPS.203)
- M. S. Islam, M. Kuzu, and M. Kantarcioglu, "Access pattern disclosure on searchable encryption: Ramification, attack and mitigation," in *Proc. 19th Annual Network and Distributed System Security Symp. (NDSS)*, 2012.
