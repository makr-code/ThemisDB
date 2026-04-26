> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# failover architecture

## Core Types
- `AutoFailoverManager` state machine and task queue.
- `DisasterRecoveryManager` step pipeline and result model.
- Shared dependencies: replication manager and epoch fencing manager.

## Concurrency Model
- Auto-failover runs monitoring and failover worker threads.
- Queue access is synchronized with mutex + condition variable.
- Runtime state uses atomics with guarded statistics updates.

## Recovery Pipeline
- Prechecks
- Snapshot validation
- Epoch fencing
- Restore
- Replica catchup
- Traffic shift
- Verification

## Integration Boundaries
- API headers in `include/failover/`
- Source implementation in `src/failover/`