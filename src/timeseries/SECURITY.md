> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Timeseries Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Unauthorized metric ingestion | Write auth enforced at API layer; series-level ACLs |
| Data injection with backdated timestamps | Timestamp validation; future timestamps beyond threshold rejected |
| Storage exhaustion via unbounded writes | Retention policies enforce max data age; disk quota monitoring |
| Sensitive metric exfiltration | Read auth checks per series/collection |

## Security Controls
- All write and read operations require authentication
- Retention policies prevent indefinite data accumulation
- Series names validated against injection patterns

## Known Limitations
- Row-level encryption not yet implemented for time series values
