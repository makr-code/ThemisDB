> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Sharding Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Unauthorized shard access | All inter-shard RPCs require mTLS authentication |
| Shard routing poisoning | Shard map updates require Raft quorum consensus |
| Split-brain data divergence | Raft leader election prevents concurrent writes to same shard |
| Tenant data leakage across shards | Tenant ID embedded in shard key; enforced at routing layer |
| DoS via shard rebalancing | Rebalancing rate-limited; background priority only |

## Security Controls
- mTLS between all shard nodes
- Quorum-based shard map updates via Raft consensus
- Tenant isolation enforced at key prefix level
- Audit logging for all shard routing decisions and topology changes

## Known Limitations
- Cross-shard transactions use 2PC which has a blocking window during coordinator failure
