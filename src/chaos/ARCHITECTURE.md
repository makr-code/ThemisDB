> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# chaos architecture

## Core Types
- `FaultType`, `FaultSpec`, `ActiveFault`
- `FaultInjector` for lifecycle of active faults
- `ChaosScheduler` for time-driven injection orchestration

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