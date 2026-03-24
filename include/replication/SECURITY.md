<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Replication Module

## Scope

Covers all public headers in `include/replication/`. Implementation hardening in `../../src/replication/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Raft log injection / rogue leader | Critical — cluster takeover | `RaftConsensus` uses HMAC-SHA256 on all log entries; leader election requires quorum |
| Unauthorized replication slot consumer | High — data exfiltration | `ReplicationSlot` and `LogicalReplicationManager` enforce consumer authentication via mTLS |
| DDL streaming schema hijack | High — schema corruption | `SchemaCdc` DDL events are signed by the originating node; consumers verify signature |
| Multi-master write amplification (DoS) | Medium — storage exhaustion | `ReplicationPolicy` enforces per-node write rate limits |
| CRDT merge manipulation | Medium — data integrity | CRDT merge operations are deterministic and validated against type constraints |
| Replication lag exploitation | Medium — stale read attacks | `ReplicationObservability` exposes lag; `ReplicationPolicy` can enforce synchronous reads above threshold |
| Snapshot data interception | High — bulk data exfiltration | Snapshot replication uses TLS 1.3 + per-snapshot AES-256-GCM encryption |
| Replay attack on event stream | Medium — duplicate processing | `EventStream` includes monotonic sequence numbers and HMAC per event |

## Security Controls

1. **HMAC-SHA256 on Raft log entries** — every entry signed by leader before broadcast.
2. **mTLS for replication connections** — all node-to-node replication over mutual TLS.
3. **DDL event signatures** — `SchemaCdc` events include originating-node signature; forgery detected at consumer.
4. **Authenticated slot consumers** — `ReplicationSlot` requires client certificate and role check before slot attachment.
5. **Snapshot encryption** — per-snapshot AES-256-GCM key derived from cluster master key.
6. **Monotonic event sequence** — `EventStream` sequence numbers prevent replay.

## Known Limitations

- Global transaction ordering (TrueTime-style, planned Q4 2026) will require a new clock trust model — security review required before enabling.
- Kafka export (planned Q3 2026) will introduce a new authentication boundary (Kafka SASL/SCRAM) requiring separate review.
