> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Updates Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Malicious update package injection | Digital signature verification (HSM-backed) before applying any update |
| MITM on update download | HTTPS with certificate pinning; SHA-256 manifest hash verification |
| Rollback attack to vulnerable version | Version monotonicity enforced; downgrade below min-version blocked |
| Update-triggered data corruption | Automatic backup before update; rollback restores previous state |
| Unauthorized update trigger | Update execution requires admin role; MFA recommended |
| Canary traffic manipulation | Canary percentage changes require elevated privileges and are audit-logged |

## Security Controls
- HSM-backed `SigningService` for update package signing and verification
- All update packages verified against SHA-256 manifest before installation
- Automatic pre-update backup ensures rollback capability
- All update lifecycle events audit-logged
- Pre-flight health checks abort update if system is in degraded state

## Known Limitations
- Hot-reload of core cryptographic libraries requires full restart
