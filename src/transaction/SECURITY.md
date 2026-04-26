> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Transaction Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Phantom reads enabling authorization bypass | Predicate locking (SSI) prevents phantom reads |
| SAGA compensation manipulation | Compensating actions are deterministic and idempotent; audit logged |
| 2PC coordinator crash exploit | Raft-logged prepare/commit records ensure crash recovery |
| Deadlock-based DoS | Deadlock detection with configurable timeout and auto-abort |
| Transaction log replay attack | WAL entries include sequence numbers and checksums |

## Security Controls
- All transactions require authenticated session
- SAGA compensations logged with full operation context
- 2PC state persisted to Raft log before sending to participants
- Audit logging for all commit and rollback events

## Known Limitations
- 2PC has a brief blocking window during coordinator failure before Raft log recovery
