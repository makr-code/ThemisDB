# Cache Module Benchmark Runbook

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: release_gate_manifest_cache.json · src/cache/PERFORMANCE_EXPECTATIONS.md · src/cache/ROADMAP.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot Coding Agent | Human_Review: false | Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone. -->

## Purpose

This runbook describes how to execute and collect the cache module benchmark
suite. Manifest presence can be checked here; full performance-gate sign-off remains blocked by the validator gap below.

## Benchmark Sources

| File | Gates covered |
|---|---|
| `benchmarks/query/bench_adaptive_query_cache.cpp` | Adaptive query cache cases, including GATE-CAC and SGATE-CAC mappings |
| `benchmarks/cache/bench_embedding_cache_performance.cpp` | Embedding cache cases, including GATE-CAC-06 and SGATE-CAC-04 |
| `benchmarks/cache/bench_cache_eviction_coordinator.cpp` | Eviction/coordinator cases, including GATE-CAP-01..06 |

## Prerequisites

- RocksDB development dependencies required by the selected CMake preset/build. The normal community preset is strict; do not assume missing RocksDB causes benchmark cases to be automatically skipped. `EmbeddingCache` can fall back to an in-memory scan if vector-index initialization fails at runtime.
- Google Benchmark (gbenchmark) linked to the build.
- Stable CPU environment: disable frequency scaling (`cpupower frequency-set -g performance`) on dedicated hardware.
- The embedding benchmark's dataset generator uses seed `42`; keep that source unchanged when comparing its runs.

## Build

```bash
cmake --preset community-release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build-community-release --target bench_adaptive_query_cache bench_embedding_cache_performance --parallel 8
cmake --build build-community-release --target bench_cache_eviction_coordinator --parallel 8
```

The preset disables benchmarks by default; the explicit option above enables them. CMake also requires Google Benchmark to be found. Benchmark executables use the configured runtime output directory `build-community-release/bin/`.

## Run

```bash
mkdir -p benchmarks/results/cache

# Adaptive query cache benchmarks
./build-community-release/bin/bench_adaptive_query_cache \
  --benchmark_repetitions=5 \
  --benchmark_min_warmup_time=0.2 \
  --benchmark_out=benchmarks/results/cache/adaptive_query_cache_results.json \
  --benchmark_out_format=json

# Embedding cache benchmarks
./build-community-release/bin/bench_embedding_cache_performance \
  --benchmark_repetitions=5 \
  --benchmark_min_warmup_time=0.2 \
  --benchmark_out=benchmarks/results/cache/embedding_cache_results.json \
  --benchmark_out_format=json

# Cache eviction/coordinator benchmarks
./build-community-release/bin/bench_cache_eviction_coordinator \
  --benchmark_repetitions=5 \
  --benchmark_min_warmup_time=0.2 \
  --benchmark_out=benchmarks/results/cache/cache_eviction_coordinator_results.json \
  --benchmark_out_format=json
```

## Manifest completeness and gate-validation gap

The manifest currently requires 24 benchmark IDs. The completeness check below checks all three raw result files, including argument-qualified cases. It is a presence check only; it is not proof that performance gates passed.

**TODO — cache gate validator:** `benchmarks/wave7/report_variance_w7.py` accepts one `--input` file (not `--results`) and its metric mapping does not cover all cache manifest metrics, including `throughput_ops_per_s` and `latency_median_us`. No combined, unit-verified end-to-end gate command is verified here. Do not treat that reporter or the completeness check alone as cache release sign-off; add/verify an aggregator and cache-aware metric handling before claiming a gate pass.

```python
import json
import sys

manifest = json.load(open("benchmarks/cache/release_gate_manifest_cache.json"))
result_files = [
    "benchmarks/results/cache/adaptive_query_cache_results.json",
    "benchmarks/results/cache/embedding_cache_results.json",
    "benchmarks/results/cache/cache_eviction_coordinator_results.json",
]
observed = set()
for path in result_files:
    with open(path) as f:
        for item in json.load(f).get("benchmarks", []):
            observed.add(item.get("name", "").split("/repeats:")[0])

required = set(manifest["manifest_completeness"]["required_benchmark_ids"])
missing = sorted(
    name for name in required
    if not any(actual == name or actual.startswith(name + "/") for actual in observed)
)
if missing:
    print("MISSING benchmark IDs:", missing)
    sys.exit(1)
print(f"All {len(required)} required benchmark IDs are present.")
```

### Gate follow-up protocol

1. Identify the failing gate ID (e.g. `GATE-CAC-01`) using a verified cache-aware report.
2. Check raw results and variance on the same benchmark case.
3. If `CV > 10%`: re-run on dedicated hardware with frequency pinning.
4. If gate still fails: file a regression ticket and block the merge.
5. Gate override requires two maintainer sign-offs and an entry in `CHANGELOG.md`.

## Soft Gate Warning Protocol

1. Soft gates log warnings but do not block merge.
2. Document each soft-gate warning in the release PR description.
3. If the same soft gate fires for two consecutive releases, escalate to a hard gate investigation.

## Evidence Package

For each release, preserve the following under `benchmarks/results/cache/`:

- `adaptive_query_cache_results.json` — raw Google Benchmark output
- `embedding_cache_results.json` — raw Google Benchmark output
- `cache_eviction_coordinator_results.json` — raw Google Benchmark output
- manifest-completeness output and the exact code/build/hardware context
- **TODO:** `gate_report.json` and `regression_tickets.json` require a cache-aware validator/automation path; the reporter invocation previously documented here is not supported by its CLI.

Reference `release_gate_manifest_cache.json` and this runbook in the release PR description. Do not record release-gate sign-off until the validator gap above is closed.

## Known Limitations

See `release_gate_manifest_cache.json → known_limitations` for environment-specific skip conditions and variance caveats. **TODO:** its statement that the embedding vector-index path is a stub has not been verified against the current `VectorIndexManager` implementation; confirm that backend before using the statement as current evidence.
