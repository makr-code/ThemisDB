<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Graph Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Graph module public headers expose graph traversal, distributed execution, GPU compute, and scheduled edge refresh. Security concerns focus on traversal depth limits, shard isolation, GPU memory safety, and edge refresh integrity.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Infinite traversal / cycle exploitation | `ParallelTraversal` and `PathConstraints` enforce configurable max-depth limits |
| Cross-shard data leakage in distributed queries | `ShardGraphExecutor` isolates execution per shard; cross-shard result merging requires explicit join |
| GPU memory exhaustion in large traversals | `GPUGraphTraversal` checks available VRAM before dispatch; falls back to CPU |
| Malicious edge injection via refresh | `ScheduledGraphEdgeRefreshEngine` validates ANN scores against configurable similarity thresholds before committing edges |
| Path constraint bypass | `PathConstraints` validated at query plan time by `GraphQueryOptimizer` before execution |
| Distributed query result tampering | Shard results are checksummed before merge in `DistributedGraphManager` |

## Security Controls

- Max traversal depth enforced in all traversal engines.
- Shard-isolated execution prevents cross-shard data access.
- ANN similarity threshold gate on scheduled edge refresh.

## Known Limitations

- GPU traversal fallback to CPU is silent; monitor via metrics if VRAM exhaustion is a concern.
- CEP event callbacks in `ScheduledGraphEdgeRefreshEngine` are invoked synchronously; slow callbacks affect refresh latency.
- Implementation-level security details: `../../src/graph/SECURITY.md`.
