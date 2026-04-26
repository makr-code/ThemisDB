> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Temporal Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Temporal backdating attacks | HLC timestamps validated against cluster time; large skews rejected |
| Unauthorized time-travel queries | Auth module enforces read permissions on historical data |
| Data retention bypass | Retention policy changes require elevated privileges; audit logged |
| History poisoning via NON-SEQUENCED updates | NON-SEQUENCED writes require explicit flag; audited separately |

## Security Controls
- All temporal queries subject to same RBAC as current-data queries
- Retention policy mutations require admin role and are audit-logged
- HLC skew threshold prevents large timestamp manipulation

## Known Limitations
- SQL `PERIOD FOR` DDL syntax not yet supported
