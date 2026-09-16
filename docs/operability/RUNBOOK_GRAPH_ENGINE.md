# RUNBOOK: Graph Engine — Operator Incident Triage

<!-- Status: current | validated: 2026-09-16 -->
<!-- Module: graph | Wave: D -->

## Overview

This runbook covers the five most common graph engine incident classes
encountered in production ThemisDB clusters.  Each scenario includes
detection signals, log patterns, triage steps, mitigation, and escalation
criteria.

---

## Scenario 1 — BFS Frontier Overflow

### Description
A BFS traversal query generates a frontier that exceeds the configured maximum
frontier size (`graph.bfs.max_frontier_size`), causing the query to be
terminated with a partial result or an error.

### Detection
- Log pattern: `[GRAPH:BFSFrontierOverflow]`
- Prometheus: `graph_bfs_frontier_overflow_total` counter increments
- Alert: `GraphBFSFrontierOverflowAlert`

### Triage Steps
1. Identify the query ID and starting vertex from `[GRAPH:BFSFrontierOverflow]`.
2. Check the graph shape around the starting vertex:
   ```
   themis-admin graph vertex-degree --vertex <id> --depth 2
   ```
3. Determine if the traversal depth limit is set too high for the graph's
   average fan-out.
4. Verify whether the overflow is a one-time event or recurring for a
   specific vertex class.

### Mitigation
- Reduce the traversal depth limit for affected query patterns:
  ```
  themis-admin graph set-traversal-depth --query-class <class> --max-depth 5
  ```
- Increase `graph.bfs.max_frontier_size` if the graph topology legitimately
  requires larger frontiers (requires config reload):
  ```
  themis-admin config set graph.bfs.max_frontier_size 1048576
  ```
- Apply a BFS cutoff predicate to bound frontier growth for high-fanout vertices.

### Escalation
Escalate if frontier overflows affect > 1 % of BFS queries — the graph may
have structural anomalies (e.g., a super-hub) that require schema review.

---

## Scenario 2 — Dijkstra Overflow

### Description
A Dijkstra shortest-path query exhausts its priority queue capacity or
produces a cost value that overflows the internal `double` cost accumulator.

### Detection
- Log pattern: `[GRAPH:DijkstraOverflow]`
- Prometheus: `graph_dijkstra_overflow_total` counter increments
- Alert: `GraphDijkstraOverflowAlert`

### Triage Steps
1. Identify the query, start/end vertices, and cost accumulator value from logs.
2. Check edge weight values around the overflow path:
   ```
   themis-admin graph edge-weights --path-query-id <id>
   ```
3. Verify whether the overflow is caused by negative-weight cycles (should be
   disallowed by schema constraints).
4. Check if cost field is using the correct numeric type in the schema.

### Mitigation
- Add a maximum-cost guard to affected query patterns.
- If negative-weight cycles are present: run the cycle detector:
  ```
  themis-admin graph detect-cycles --weighted
  ```
  and remove or correct the offending edges.
- Apply edge-weight normalization if values span many orders of magnitude.

### Escalation
Escalate if the overflow is caused by a schema bug that has already persisted
incorrect edge weights — a data audit is required before any writes resume.

---

## Scenario 3 — GPU→CPU Kernel Fallback Failure

### Description
The graph acceleration layer attempted to fall back from a GPU kernel to a
CPU kernel but the CPU kernel also failed, leaving the query without a
valid executor.

### Detection
- Log pattern: `[GRAPH:KernelFallbackFailed]`
- Prometheus: `graph_kernel_fallback_failures_total` counter increments
- Alert: `GraphKernelFallbackFailedAlert`

### Triage Steps
1. Determine which kernel type failed (BFS, Dijkstra, GNN inference) from logs.
2. Check GPU driver and CUDA/ROCm status (if GPU was the primary path):
   ```
   themis-admin gpu status
   ```
3. Verify that the CPU fallback binary is correctly installed and linked:
   ```
   themis-admin graph kernel-probe --type cpu --algorithm <bfs|dijkstra>
   ```
4. Check resource limits — CPU fallback may have been OOM-killed.

### Mitigation
- Force CPU-only mode for the affected algorithm to restore service:
  ```
  themis-admin graph force-cpu --algorithm <bfs|dijkstra>
  ```
- Restart the graph engine worker if the kernel loader state is corrupt:
  ```
  themis-admin graph restart-worker --worker-id <id>
  ```
- If GPU driver failure: follow `RUNBOOK_GPU_FALLBACK_PERFORMANCE.md`.

### Escalation
Escalate if CPU fallback fails on a Category C (policy/provenance/transaction)
kernel — these must remain CPU-first permanently; a failure here is a safety
violation requiring immediate escalation.

---

## Scenario 4 — High Fan-Out OOM

### Description
A traversal or aggregation query over a high-fan-out vertex consumes more
memory than the configured `graph.worker.max_memory_mb` limit, causing the
worker process to be OOM-killed or the query to be forcibly terminated.

### Detection
- Log pattern: `[GRAPH:FanOutOOM]`
- Prometheus: `graph_oom_terminations_total` increments
- Alert: `GraphFanOutOOMAlert`

### Triage Steps
1. Identify the OOM'd worker and the query that triggered it from logs.
2. Check the vertex fan-out distribution:
   ```
   themis-admin graph fan-out-histogram --top 20
   ```
3. Determine if the OOM was caused by a single super-hub or by concurrent
   traversals sharing the same worker.
4. Check current `graph.worker.max_memory_mb` setting vs. available host RAM.

### Mitigation
- Apply a per-query memory budget:
  ```
  themis-admin graph set-query-memory-limit --query-class <class> --mb 512
  ```
- Shard high-fan-out vertex traversals across multiple workers.
- If a single vertex has > 1M out-edges, add an indexed degree constraint to
  prevent unbounded traversals from that vertex.

### Escalation
Escalate if OOM events recur after memory limits are applied — the graph
schema may need to be restructured to bound vertex degree.

---

## Scenario 5 — Distributed Partition Stall

### Description
A distributed graph query across multiple shards stalls because one or more
partition coordinators fail to return their partial results within the
configured timeout.

### Detection
- Log pattern: `[GRAPH:PartitionStall]`
- Prometheus: `graph_distributed_partition_stalls_total` increments
- Alert: `GraphPartitionStallAlert`

### Triage Steps
1. Identify the stalled partition coordinator(s) and the query from logs.
2. Check shard health for the stalled partitions:
   ```
   themis-admin sharding shard-health --shard <id>
   ```
3. Verify network connectivity between the query coordinator and the shard
   partition workers.
4. Check if the stall is caused by a long-running local computation on the
   shard (high fan-out, memory pressure).

### Mitigation
- Cancel the stalled query and retry with a shorter per-partition timeout:
  ```
  themis-admin graph cancel-query --query-id <id>
  themis-admin graph set-partition-timeout --ms 5000
  ```
- If a specific shard is always stalling: check that shard for resource
  saturation and temporarily exclude it from distributed graph routing.
- Apply partial-result mode: return available partition results even if
  one partition times out.

### Escalation
Escalate if partition stalls affect > 5 % of distributed queries — the
distributed graph orchestrator may have a coordination bug requiring
patch-level fix.

---

## Quick Reference — Log Patterns

| Pattern                        | Scenario                      |
|--------------------------------|-------------------------------|
| `[GRAPH:BFSFrontierOverflow]`  | BFS frontier overflow         |
| `[GRAPH:DijkstraOverflow]`     | Dijkstra cost overflow        |
| `[GRAPH:KernelFallbackFailed]` | GPU→CPU fallback failure      |
| `[GRAPH:FanOutOOM]`            | High fan-out OOM              |
| `[GRAPH:PartitionStall]`       | Distributed partition stall   |

## Related Documents
- `src/graph/ROADMAP.md` — module roadmap
- `docs/operability/RUNBOOK_GPU_FALLBACK_PERFORMANCE.md` — GPU fallback runbook
- `docs/operability/WAVE_D_ROADMAP.md` — Wave D operability plan
- `include/graph/graph_error_taxonomy.h` — error taxonomy (44 codes)
