> ⚠️ **Historische Ergebnisse** – Phase-2H-Messdaten beschreiben einen abgeschlossenen Entwicklungsstand.

# Phase 2H: Background Thread Optimization - Final Results

## Summary

Phase 2H investigated whether explicit background thread tuning could improve upon Phase 2G's performance. Initial benchmarks showed significantly lower performance due to flawed implementation (thread creation per iteration). After adding an A/B fixture with identical workload (ParallelExecutor), Phase 2H tuning shows clear benefits at higher thread counts.

## Problem Identified

The initial Phase 2H benchmarks (`BM_Phase2H_BgThreads`, `BM_Phase2H_FullOptimized`) showed only 2.5K ops/s at 16 threads compared to Phase 2G's 2.5M ops/s - a 1000x difference!

###Root Cause
The Phase 2H benchmarks had a flawed implementation:
1. **Thread creation per iteration**: Created and destroyed `std::thread` objects on every benchmark iteration
2. **Minimal work per iteration**: Only 10 operations per thread per iteration
3. **No thread reuse**: Unlike Phase 2G which uses `ParallelExecutor` with thread pools

This made the benchmark measure thread creation overhead, not database performance.

## Correct Approach

We added a second fixture `ParallelityBenchPhase2G_2H` with Phase 2H configuration and kept the workload identical to Phase 2G (ParallelExecutor, same records per thread). This enables a fair A/B comparison.

## A/B Results (Phase 2G vs Phase 2G+2H)

Measured with `bench_advanced_patterns.exe` (Release), identical workload:

- 1T: Phase2G 3.48M ops/s vs Phase2G+2H 3.02M ops/s (−13%)
- 4T: Phase2G 806k vs Phase2G+2H 722k (−10%)
- 8T: Phase2G 460k vs Phase2G+2H 422k (−8%)
- 16T: Phase2G 217k vs Phase2G+2H 281k (+29%)
- 32T: Phase2G 98k vs Phase2G+2H 149k (+52%)

Interpretation:
- Background/Level0 tuning regresses slightly at low thread counts.
- It improves throughput noticeably at 16+ threads where compaction and L0 pressure grow.

## Decision Point

Before investing more time in Phase 2H, we need to answer:

**Q: Is background compaction actually a bottleneck?**

Evidence suggests NO:
1. Phase 2G uses `max_background_jobs` which auto-configures background threads
2. The real bottleneck appears to be lock contention (16 lock stripes) and transaction overhead
3. Document research showed compaction is only a bottleneck when:
   - Write rate exceeds compaction throughput → causes write stalls
   - Level 0 files accumulate → blocks new writes
   
Phase 2G likely doesn't hit these conditions with its current write rates.

## Recommendations

### For Production
- For ≤8 threads: keep Phase 2G defaults (slightly better single/low‑thread throughput).
- For ≥16 threads: enable Phase 2H tuning (compaction/flush threads, L0 triggers, cache sharding) for +30–50% gains observed in A/B.

Suggested tuning for ≥16 threads:
```cpp
config.max_background_compactions = 8;
config.max_background_flushes = 2;
config.background_threads_low = 8;
config.background_threads_high = 2;
config.max_subcompactions = 2;
config.level0_file_num_compaction_trigger = 2;
config.level0_slowdown_writes_trigger = 8;
config.level0_stop_writes_trigger = 16;
config.block_cache_shard_bits = 6;
config.db_write_buffer_size_mb = 512;
```

## Time Investment Analysis

**Spent**: ~2 hours on Phase 2H
- Documentation research: 30 min
- Configuration implementation: 30 min  
- Benchmark creation: 30 min
- Debug and analysis: 30 min

**Potential Return**: 0-10% improvement (unvalidated)

**Verdict**: Proceed with hybrid deployment.
- Use Phase 2G defaults for low concurrency; enable Phase 2H tuning for high concurrency.
- Architectural limits (lock stripes, txn coordination, WAL) remain, but Phase 2H improves headroom at ≥16T.

## Implementation Note (Hybrid Flag)
- Added `enable_high_parallel_tuning` and `high_parallel_thread_threshold` to `RocksDBWrapper::Config`.
- `main_server` auto-enables the tuning if `worker_threads >= threshold` (default threshold = 16). Can be overridden in config.json/yaml via `storage.enable_high_parallel_tuning` and `storage.high_parallel_thread_threshold`.

## Next Steps (If Needed)

If you absolutely must squeeze more performance:

1. **Profile with perf/VTune** to confirm bottlenecks
2. **Increase lock stripe count** (requires RocksDB patch)
3. **Try optimistic transactions** (different concurrency control)
4. **Batch commits** to reduce commit overhead
5. **Consider alternative engines** (LMDB, TiKV, FoundationDB)

But for 99% of use cases, **Phase 2G is the answer**.

---

## Conclusion

Phase 2H was a valuable learning experience:
- ✅ Learned about RocksDB background thread tuning
- ✅ Identified benchmark implementation pitfalls
- ✅ Confirmed Phase 2G is near-optimal
- ❌ No performance improvement found (benchmarks were flawed)

**Final recommendation: Deploy Phase 2G (NoPipe Txn10) configuration.**

## Fresh Bottleneck Probes (2025-12-20)

Quick MSVC/Release microbench runs (JSON outputs: [build-msvc/Release/bench_lock_contention.json](build-msvc/Release/bench_lock_contention.json), [build-msvc/Release/bench_wal_stress.json](build-msvc/Release/bench_wal_stress.json), [build-msvc/Release/bench_hotspots_micro.json](build-msvc/Release/bench_hotspots_micro.json)). Throughput = items/s.

Lock contention (TransactionDB pessimistic, WritePrepared):

| Threads | Disjoint keys | Overlapping keys |
|---------|---------------|------------------|
| 1       | 20,018        | 20,746           |
| 4       | 41,407        | 962              |
| 8       | 76,180        | 169              |
| 16      | 149,037       | 777              |
| 32      | 214,651       | 405              |

- Disjoint scales linearly; overlapping collapses (lock stripe contention dominates). Action: increase lock striping or reduce overlapping hot keys before enabling Phase 2H.

WAL stress (64-byte payloads):

| Threads | WAL sync | WAL no-sync |
|---------|----------|-------------|
| 1       | 349      | 149,082     |
| 4       | 770      | 335,610     |
| 8       | 1,451    | 442,731     |
| 16      | 2,716    | 455,106     |

- Sync fsync dominates; no-sync unlocks two to three orders of magnitude. Action: keep sync for correctness; for perf tests use no-sync as an upper bound and quantify WAL stall impact.

Hotspot raw writes (same workload, WAL on/off, hybrid tuned matches Phase 2H presets):

| Threads | WAL on | WAL off |
|---------|--------|---------|
| 1       | 312    | 135,133 |
| 4       | 735    | 333,583 |
| 8       | 1,439  | 434,808 |
| 16      | 2,789  | 428,167 |

- WAL overhead dwarfs compaction effects; hybrid vs baseline difference is in the noise at this scale. Focus: WAL path and lock striping before further compaction tweaks.

