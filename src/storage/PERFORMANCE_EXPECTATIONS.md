---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "../../benchmarks/storage/CMakeLists.txt"
  - "../../benchmarks/storage/bench_storage_performance.cpp"
  - "../../benchmarks/storage/bench_storage_release_gates.cpp"
  - "../../benchmarks/storage/bench_storage_dedicated_gates.cpp"
  - "ROADMAP.md"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage benchmark inventory and limits

This file maps benchmark source code; it does not report measured performance
or assert that a threshold is a production release gate. The release-gate and
dedicated-gate sources explicitly use in-process mocks/stubs and do not perform
real RocksDB or disk I/O.

| Source | Registered cases relevant to this module | What is measured |
|---|---|---|
| `benchmarks/storage/bench_storage_performance.cpp` | `BM_Allocator_*`, `BM_Memory_*`, `BM_RCU_*`, storage sustained-write and WAL cases | Allocator, memory-page, RCU, in-memory storage/WAL behavior |
| `benchmarks/storage/bench_storage_release_gates.cpp` | `BM_SGRG01_WalAppendThroughput`, `BM_SGRG02_MvccReadWarm`, `BM_SGRG03_MvccWrite`, `BM_SGRG04_CheckpointOverhead`, `BM_SGRG05_TieringDecision`, `BM_SGRG06_CompactionTriggerCheck` | In-memory/mock gate harnesses; not production I/O |
| `benchmarks/storage/bench_storage_dedicated_gates.cpp` | `BM_ST_BM_01_KVWrite`, `BM_ST_BM_02_KVRead`, `BM_ST_BM_03_WALReplayThroughput`, `BM_ST_BM_04_CompactionTriggerCheck` | In-process stub gates; not real RocksDB or disk I/O |

`benchmarks/storage/CMakeLists.txt` only configures these targets when
`THEMIS_BUILD_BENCHMARKS` is enabled and Google Benchmark is available. The
source registers the SGRG smoke test with CTest; registration is not evidence
that a benchmark ran or passed a latency/throughput threshold.

## Follow-up

- TODO: attach results, machine/build configuration, and baseline-comparison
  evidence before publishing numerical performance claims.
- TODO: identify and register any intended user-storage mount benchmark; the
  previously cited `bench_user_storage_mount_latency.cpp` is not present at
  either `benchmarks/` or `benchmarks/storage/`.
