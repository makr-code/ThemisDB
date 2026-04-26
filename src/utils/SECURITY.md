> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Utils Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| PII leakage via audit logs | `AuditLogger` redacts PII fields; streaming PII detector flags sensitive values |
| Key material in logs | HKDF keys and derived secrets never logged; only key IDs referenced |
| Audit log tampering | Tamper-evident hash-chain audit writer; each entry links to previous hash |
| Weak key derivation | HKDF (RFC 5869) with SHA-256; minimum salt length enforced |
| Bloom filter false-negative exploitation | Bloom filter used for performance only; never as sole security check |

## Security Controls
- `AuditLogger` is append-only and tamper-evident (hash-chain)
- PII detector applied before writing to logs or external systems
- HKDF key derivation follows RFC 5869
- Error messages sanitized to avoid leaking internal state

## Known Limitations
- Sampled logger may miss security-relevant events if sampling rate is too low — use 100% sampling for auth/audit paths
