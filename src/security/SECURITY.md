> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Security Module

> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Key material exposure | HSM-backed key storage; keys never in plaintext memory |
| Cryptographic algorithm weakness | Post-quantum algorithms available; NIST-approved algorithms only |
| Side-channel attacks on crypto operations | Constant-time implementations for sensitive operations |
| Secret leakage via logs | Secret values never logged; only key IDs referenced |
| Certificate spoofing | Full chain validation; OCSP/CRL revocation checking |

## Security Controls

- AES-256-GCM for symmetric encryption (authenticated encryption)
- RSA-4096 / ECDSA-P384 for asymmetric operations
- HSM integration via PKCS#11 for production key storage
- All cryptographic operations audited via `StorageAuditLogger`
- Zero-knowledge proof support for privacy-preserving verification

## Known Limitations

- Post-quantum algorithms are not yet used by default (opt-in)
- HSM support requires PKCS#11-compatible hardware
