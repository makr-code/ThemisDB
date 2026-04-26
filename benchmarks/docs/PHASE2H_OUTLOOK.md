> ⚠️ **Historischer Ausblick** – Beschreibt den Stand zum Zeitpunkt der Phase-2H-Abnahme.

# Phase 2H Outlook: Bottlenecks and Next Steps

## Current Findings
- Lock contention dominates when keys overlap; disjoint keys scale linearly.
- WAL sync is the primary throughput limiter; no-sync shows 2–3 orders of magnitude headroom.
- Compaction/background tuning (Phase 2H) helps only once lock/WAL bottlenecks are relieved and at high thread counts.
- Hybrid presets do not materially change WAL-bound or lock-bound microbenches.

## Improvement Trajectory
1) **Lock Striping**
   - Increase lock stripe count or shard hot key ranges; retest overlapping lock benchmark.
   - Evaluate optimistic transactions for low-conflict workloads.
2) **WAL Path**
   - Measure group commit/batching; tune `wal_bytes_per_sync` and `bytes_per_sync`.
   - Benchmark WAL on faster media (NVMe/pmem) to quantify upper bounds.
   - Consider pipelined/two-queue writes where correctness allows.
3) **Compaction/Background Threads**
   - Revisit Phase 2H presets after lock/WAL mitigation; expect gains at >=16 threads.
   - Tune L0 triggers and background compactions only after confirming compaction stall indicators.
4) **Workload Shaping**
   - Reduce overlapping hot keys; distribute keys across stripes/shards.
   - Batch commits where latency SLAs allow.

## Suggested Experiments
- Lock contention A/B: current vs higher stripe count; optimistic vs pessimistic txns.
- WAL batching matrix: sync/no-sync, batch sizes, `wal_bytes_per_sync`, `bytes_per_sync`.
- Storage path: WAL on fast disk/pmem vs baseline; measure fsync latency and stall counters.
- Re-run Phase2G vs Phase2G+2H after lock/WAL tweaks to isolate compaction benefit.

## Success Criteria
- Overlapping-lock throughput improves to within 50% of disjoint case at target thread counts.
- WAL sync throughput improves by >5–10x versus current sync baseline without correctness loss.
- High-thread Phase2H A/B shows additional uplift once WAL/lock are addressed.

## Artifacts
- Latest microbench JSON: build-msvc/Release/bench_lock_contention.json, build-msvc/Release/bench_wal_stress.json, build-msvc/Release/bench_hotspots_micro.json.
- Summary tables: benchmarks/PHASE2H_FINAL_RESULTS.md (2025-12-20 update).
