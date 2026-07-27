# Cache Module Benchmark Runbook

<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: release_gate_manifest_cache.json · src/cache/PERFORMANCE_EXPECTATIONS.md · src/cache/ROADMAP.md -->

## Purpose

This runbook describes how to execute, validate, and interpret the cache module
benchmark suite for Phase 5 release-gate sign-off.

## Benchmark Sources

| File | Gates covered |
|---|---|
| `benchmarks/bench_adaptive_query_cache.cpp` | GATE-CAC-01..GATE-CAC-05, SGATE-CAC-01..SGATE-CAC-03 |
| `benchmarks/bench_embedding_cache_performance.cpp` | GATE-CAC-06, SGATE-CAC-04 |

## Prerequisites

- `librocksdb-dev` installed (for L3 benchmark cases; without it, L3 cases are auto-skipped and must be marked `skipped` in evidence).
- Google Benchmark (gbenchmark) linked to the build.
- Stable CPU environment: disable frequency scaling (`cpupower frequency-set -g performance`) on dedicated hardware.
- Seed: `42` (hardcoded in benchmark suite; do not override for release evidence).

## Build

```bash
cmake --preset community-release
cmake --build build-community-release --target bench_adaptive_query_cache bench_embedding_cache_performance --parallel 8
```

## Run

```bash
# Adaptive query cache benchmarks
./build-community-release/benchmarks/bench_adaptive_query_cache \
  --benchmark_repetitions=5 \
  --benchmark_min_warmup_time=0.2 \
  --benchmark_out=benchmarks/results/cache/adaptive_query_cache_results.json \
  --benchmark_out_format=json

# Embedding cache benchmarks
./build-community-release/benchmarks/bench_embedding_cache_performance \
  --benchmark_repetitions=5 \
  --benchmark_min_warmup_time=0.2 \
  --benchmark_out=benchmarks/results/cache/embedding_cache_results.json \
  --benchmark_out_format=json
```

## Gate Validation

Compare results against `release_gate_manifest_cache.json`:

```bash
# Manual gate check (print p99 for each mapped case):
python3 benchmarks/wave7/report_variance_w7.py \
  --manifest benchmarks/cache/release_gate_manifest_cache.json \
  --results benchmarks/results/cache/adaptive_query_cache_results.json \
           benchmarks/results/cache/embedding_cache_results.json
```

## Hard Gate Failure Protocol

1. Identify the failing gate ID (e.g. `GATE-CAC-01`).
2. Check raw results for variance (`CV` column in report).
3. If `CV > 10%`: re-run on dedicated hardware with frequency pinning.
4. If gate still fails: file a regression ticket and block the merge.
5. Gate override requires two maintainer sign-offs and an entry in `CHANGELOG.md`.

## Soft Gate Warning Protocol

1. Soft gates log warnings but do not block merge.
2. Document each soft-gate warning in the release PR description.
3. If the same soft gate fires for two consecutive releases, escalate to a hard gate investigation.

## Manifest Completeness Check

Every benchmark ID listed in `release_gate_manifest_cache.json → manifest_completeness.required_benchmark_ids`
must appear in the run output. A missing ID is equivalent to a hard gate failure.

```bash
# Check completeness:
python3 - <<'EOF'
import json, sys
with open('benchmarks/cache/release_gate_manifest_cache.json') as f:
    manifest = json.load(f)
with open('benchmarks/results/cache/adaptive_query_cache_results.json') as f:
    results_aqc = json.load(f)
with open('benchmarks/results/cache/embedding_cache_results.json') as f:
    results_emb = json.load(f)

all_names = {b['name'].split('/')[0] for b in results_aqc.get('benchmarks', [])}
all_names |= {b['name'].split('/')[0] for b in results_emb.get('benchmarks', [])}
required = set(manifest['manifest_completeness']['required_benchmark_ids'])
missing = required - all_names
if missing:
    print('MISSING benchmark IDs:', missing)
    sys.exit(1)
print('All required benchmark IDs present.')
EOF
```

## Evidence Package

For each release, commit the following to `benchmarks/results/cache/`:

- `adaptive_query_cache_results.json` — raw Google Benchmark output
- `embedding_cache_results.json` — raw Google Benchmark output
- `gate_report.json` — generated gate comparison report
- `regression_tickets.json` — auto-generated ticket list (empty if no regressions)

Reference `release_gate_manifest_cache.json` and this runbook in the release PR description.

## Known Limitations

See `release_gate_manifest_cache.json → known_limitations` for the current list of
environment-specific skip conditions and variance caveats.
