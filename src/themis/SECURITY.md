> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Themis Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Malicious module injection | Authenticode (Windows) + GPG (Linux) signature verification before load |
| Module tampering after load | SHA-256 hash verification via `ModuleHashVerifier` |
| License bypass | License validation cryptographically signed; server-side verification available |
| Registry MITM | `pinned_public_key` (sha256// format) enforces certificate pinning on registry connections |
| Unsigned module execution | `ModuleSecurityPolicy::allowUnsigned` disabled by default in production |
| Zone.Identifier bypass (Windows) | `Zone.Identifier` ADS check blocks internet-downloaded modules |

## Security Controls
- Authenticode signature verification on Windows; GPG via `posix_spawn` on Linux
- SHA-256 manifest-based integrity checking for all loaded modules
- Certificate pinning on remote registry client
- Module trust levels: `TRUSTED`, `VERIFIED`, `UNTRUSTED`
- OCSP/CRL revocation checking for certificates
- Development mode clearly separated from production mode

## Known Limitations
- GPG verification requires `gpg` binary on PATH on Linux
