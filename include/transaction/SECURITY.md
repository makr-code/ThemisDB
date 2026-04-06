<!-- Status: current | validated: 2026-04-06 -->

# Transaction Module — Security Reference

## Scope

This document covers security properties exposed through the public headers in
`include/transaction/`.  Implementation-layer controls are documented in
`../../src/transaction/SECURITY.md`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Dirty reads across transactions | Data integrity violation | MVCC snapshot isolation enforced by `SnapshotManager` |
| Phantom reads under concurrent inserts | Query result inconsistency | Predicate locking via `LockManager` range locks |
| Deadlock-based DoS | Thread starvation, latency spike | `DeadlockPredictor` breaks cycles before they form |
| Distributed saga partial failure | Inconsistent distributed state | Compensating transactions journalled by `SagaOrchestrator` |
| WAL replay of malformed entries | Arbitrary state corruption | `CrashRecoveryManager` validates checksums before redo |
| Read-only bypass via write-path API | Unauthorised mutation | Guards on all 11 write paths; `setReadOnly()` irrevocable if writes exist |
| Audit log tampering | Forensic evidence loss | `TransactionAuditor` emits append-only structured records |
| Transaction ID exhaustion | New transactions rejected | `GlobalTransactionManager` monitors ID space and alerts |

## Security Controls

- **Isolation enforcement** — `IsolationLevel::SERIALIZABLE` is the default for
  write transactions; downgrade requires explicit opt-in.
- **Read-only immutability** — once `setReadOnly(true)` is called without prior writes,
  all 11 write-path guards throw `ReadOnlyViolation`; the flag cannot be cleared.
- **Compensation durability** — saga compensation steps are persisted before the
  forward step is confirmed, ensuring rollback is always possible.
- **Audit non-repudiation** — `TransactionAuditor` records caller identity, timestamp,
  and write-set hash; records are forwarded to the cluster audit log.

## Known Limitations

- Cross-shard snapshot consistency depends on clock synchronisation (< 50 ms skew
  assumed); larger skew may cause stale snapshot reads in distributed queries.
- `TransactionBatcher` group-commit window introduces a bounded durability lag
  (configurable, default 5 ms); applications requiring synchronous durability must
  call `flush()` explicitly.
- Saga compensation is best-effort in split-brain scenarios; manual reconciliation
  may be required after a network partition exceeding the configured timeout.
