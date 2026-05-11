> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# chaos architecture

## Core Types
- `FaultType`, `FaultSpec`, `ActiveFault`
- `FaultInjector` for lifecycle of active faults
- `ChaosScheduler` for time-driven injection orchestration
- `WakeStrategy`, `ChaosSchedulerConfig` for scheduler wake policy

## Data Model
- Active faults are keyed by `node_id::fault_type`.
- Each entry stores insertion and expiry timestamps.

## Concurrency Model
- `FaultInjector` protects fault map with mutex.
- `ChaosScheduler` has a worker thread controlled by atomic running flag.
- Pending schedule list is protected by mutex.

## Integration Boundaries
- Header API: `include/chaos/chaos_framework.h`
- Implementation: `src/chaos/chaos_framework.cpp`

## Error Handling Boundaries
- `FaultInjector::injectFault` rejects empty `target_node_id` and out-of-range `probability`.
- `ChaosScheduler` rejects null injector (`std::invalid_argument`).

## Non-Goals / Limits
- No distributed control plane for multi-node chaos synchronization (roadmap item).
- No direct OS-level fault injection (network/disk/process sabotage is simulated only).

## Related Docs
- Module overview: [`./README.md`](./README.md)
- Security: [`./SECURITY.md`](./SECURITY.md)
- Audit: [`./AUDIT.md`](./AUDIT.md)
- Roadmap: [`./ROADMAP.md`](./ROADMAP.md)
- Future enhancements: [`./FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
