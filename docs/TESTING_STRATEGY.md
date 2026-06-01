# Testing Strategy for Epics 1, 2, and 3

<!-- Status: current | validated: 2026-06-01 -->

## Test layers

### Unit tests
- Validate public header contracts and error semantics.
- Keep tests mapped one-to-one to the planned sub-issue files.

### Integration tests
- Validate handoff between ANN, tensor, graph, planner, manifest, and recovery paths.
- Focus on evidence assembly, policy enforcement, and distributed shard behavior.

### Benchmarks
- Measure latency, throughput, memory, disk, network, and artifact rebuild cost.
- Compare alternative approximations under the governance rules from EPIC 2.

## Epic-specific test focus

### EPIC 1
- ANN recall and candidate budget propagation
- Tensor summary compression and routing correctness
- Graph evidence assembly, provenance, and model-switch safety

### EPIC 2
- Hardware profile selection and planner determinism
- Benchmark comparability across HNSW, DiskANN, tensor, graph, and distributed flows
- Lifecycle staleness windows, refresh triggers, and storage strategy trade-offs

### EPIC 3
- Manifest consistency, placement determinism, and integrity receipts
- Recovery planning under shard loss or partial corruption
- Cross-shard fragment assembly and planner integration

## Planned directory ownership

| Directory | Responsibility |
|---|---|
| `tests/epic1_retrieval/` | contract and integration coverage for the layered retrieval stack |
| `tests/epic2_evaluation/` | metrics, planner, profile, and lifecycle validation |
| `tests/epic3_distributed_tensor/` | manifest, placement, integrity, recovery, and distributed planner validation |
| `benchmarks/epic1_retrieval/` | retrieval-stage latency and accuracy trade-offs |
| `benchmarks/epic2_evaluation/` | planner decision cost and evaluation framework overhead |
| `benchmarks/epic3_distributed_tensor/` | placement, integrity, rebuild, and cross-shard retrieval cost |

## Acceptance signals
- Each sub-issue has at least one planned unit test file and one benchmark or explicit reason not to benchmark.
- Planner-facing behavior is validated with deterministic fixtures before any learned policy is introduced.
- Distributed artifact recovery paths are benchmarked under degraded conditions, not only on the happy path.
