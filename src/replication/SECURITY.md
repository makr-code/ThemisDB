> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Replication Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Unauthorized replication node joins | Mutual TLS authentication between nodes |
| Raft log tampering | Cryptographic log integrity verification |
| Split-brain under network partition | Quorum-based commit (majority vote required) |
| Replay attacks on replication stream | Sequence numbers and term validation |

## Security Controls

- All inter-node communication uses mTLS
- Leader-only writes prevent unauthorized state changes
- Audit logging for all replication events

## Known Limitations

None critical.
