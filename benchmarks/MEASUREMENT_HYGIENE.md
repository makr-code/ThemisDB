# Benchmark Measurement Hygiene
<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

Canonical baseline reference:
- [BENCHMARK_STANDARDS.md](BENCHMARK_STANDARDS.md)

This file is a detailed companion for measurement protocol. If a structural
rule conflicts, BENCHMARK_STANDARDS.md is authoritative.

Standardised warmup / run protocol for all ThemisDB benchmark suites.  
Applies from Wave 1 onward; Wave 5 enforces the canonical 3-phase warmup.

---

## 1. Canonical Seed

All benchmarks that use a random number generator **must** be seeded with the
canonical value:

```cpp
static constexpr uint64_t kW5CanonicalSeed = 42; // Wave 5
static constexpr uint64_t kCanonicalRngSeed = 42; // Wave 1 (bench_fixtures.h)
```

This guarantees that:
- Data sequences are identical across machines and CI runs.
- Baseline comparisons are not polluted by different data distributions.
- Reproducibility investigations start from a known state.

---

## 2. Temporary Directory Convention

All I/O benchmarks must create their databases in the OS temp directory to
avoid collision across parallel runs and to ensure cleanup on failure:

```cpp
static fs::path makeTempPath(std::string_view prefix) {
    auto suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return fs::temp_directory_path() / (std::string(prefix) + "_" + suffix);
}
```

**Never** use a hard-coded relative path such as `"bench_crud_db"` in new
benchmarks — it causes conflicts when multiple benchmarks run in parallel and
leaves artefacts in the working directory.

---

## 3. Canonical 3-Phase Warmup Protocol

Every benchmark that measures a data-path operation **must** warm up through
three phases before the measurement window begins.  The phases are defined as
constants in each wave's header/source and must not be skipped.

| Phase   | Constant             | Purpose                                             |
|---------|----------------------|-----------------------------------------------------|
| Cold    | `kW5CWarmupCold = 50`  | Fill write buffer, prime OS I/O path                |
| Warm    | `kW5CWarmupWarm = 100` | Sequential reads to warm OS page cache              |
| Hot     | `kW5CWarmupHot  = 200` | Random reads to stabilise CPU cache + branch predictor |

```cpp
// Phase 1: cold writes
for (int i = 0; i < kW5CWarmupCold; ++i) { idx.put(/* ... */); }

// Phase 2: sequential warm reads
for (int i = 0; i < kW5CWarmupWarm; ++i) { idx.get(keys[i % n]); }

// Phase 3: random hot reads
for (int i = 0; i < kW5CWarmupHot; ++i) { idx.get(keys[rng.integer(0, n-1)]); }

// --- measurement window starts here ---
for (auto _ : state) { /* benchmark body */ }
```

### Why 3 phases?

- **Cold writes** exercise the same code path as the benchmark body, ensuring
  write buffers and compaction have initialised.
- **Sequential warm reads** fill the OS page cache for the corpus blocks.
- **Random hot reads** stabilise the CPU branch predictor and instruction cache
  so the first measured iteration does not pay an outlier penalty.

---

## 4. Real-Time vs. CPU-Time

- I/O-bound benchmarks **must** call `UseRealTime()` to capture wall-clock
  latency including OS scheduling and I/O wait.
- Compute-bound micro-benchmarks may use the default CPU time.

```cpp
BENCHMARK_REGISTER_F(MyFixture, MyBench)
    ->UseRealTime()           // mandatory for I/O paths
    ->Unit(benchmark::kMicrosecond);
```

---

## 5. Iteration Counts

Minimum recommended iterations:

| Benchmark type          | Minimum iterations |
|-------------------------|--------------------|
| Point lookup (in-memory)| 50 000             |
| Point lookup (I/O path) | 5 000              |
| Vector search (ANN)     | 5 000              |
| Graph traversal         | 3 000              |
| Write (single)          | 30 000             |
| Batch commit (100 rec)  | 1 000              |
| Range scan              | 2 000              |

Use `->Iterations(N)` to override the Google Benchmark auto-calibration and
ensure reproducible run durations.

---

## 6. Output Format

All benchmark runs intended for regression tracking **must** emit JSON:

```bash
./bench_w5d_governance \
    --benchmark_out=bench_w5d.json \
    --benchmark_out_format=json
```

The JSON file is consumed by:
- `benchmarks/wave5/report_variance_w5.py` (regression comparison)
- `benchmarks/baselines/wave5/` (baseline update)

---

## 7. Coefficient of Variation (CV) Gate

**Acceptance criterion**: CV ≤ 5% across 5 repeated runs on the same machine.

If CV > 5%:
1. Verify CPU frequency governor is set to `performance`.
2. Disable transparent huge pages (THP) before running.
3. Pin the process to a single CPU socket (`numactl --cpunodebind=0`).
4. Increase warmup iteration counts.
5. Re-run 5 times and check `report_variance_w5.py --multi-run`.

---

## 8. Prohibited Patterns

The following patterns are **banned** in Wave 5 benchmarks:

| Pattern                          | Reason                                            |
|----------------------------------|---------------------------------------------------|
| `std::random_device{}()` as seed | Non-deterministic; breaks reproducibility         |
| Hard-coded local path (e.g. `"bench_db"`) | Causes parallel-run conflicts            |
| Missing `UseRealTime()` on I/O path | CPU time misses I/O wait, understates latency  |
| Warmup inside measurement loop   | Contaminates the first measured iterations        |
| `state.SetBytesProcessed` without `SetItemsProcessed` | Loses ops/s counter    |

---

## 9. Exemptions

The following legacy benchmarks pre-date this standard and are grandfathered
until they are next modified:

- All `benchmarks/bench_*.cpp` files at the root level (Wave 0 / legacy)
- `benchmarks/aql/bench_aql_functions.cpp` (Wave 0 AQL benchmarks)

Any modification to a grandfathered file **must** bring that file into
compliance with this standard before the PR is merged.
# ThemisDB Benchmark Measurement Hygiene

## Scope

This document defines the mandatory hygiene rules for C++ Google Benchmark
targets in `benchmarks/`.  It covers Welle 1 / PR-A of the benchmark hardening
initiative and applies to all new and modified benchmark files.

## Affected Benchmarks (Welle 1 / PR-A)

| File | Issue fixed |
|---|---|
| `storage/bench_crud.cpp` | Non-deterministic RNG seed; hardcoded relative DB path |
| `bench_ycsb.cpp` | Non-deterministic namespace-scope RNG; relative `tmp/` path |
| `bench_batch_insert.cpp` | Shared static DB path across fixture instances |
| `bench_graph_traversal.cpp` | Hardcoded relative `./data/` path |
| `bench_vector_search.cpp` | Hardcoded `data/` paths; missing `UseRealTime()` on I/O benchmarks |

---

## Rules

### 1. RNG Seeds Must Be Fixed

All `std::mt19937` (and other PRNG) instances inside benchmark bodies and
fixtures **must** use a fixed, documented seed.

**Canonical seed:** `themis::bench::kCanonicalRngSeed` (= `42`, declared in
`bench_fixtures.h`).

```cpp
// ✅ correct
std::mt19937 rng{themis::bench::kCanonicalRngSeed};

// ❌ forbidden — produces different data on every run
std::mt19937 rng{std::random_device{}()};
```

Using a different seed is allowed when there is an explicit documented reason
(e.g., multiple independent distributions in the same benchmark).  Document
the reason with a comment.

### 2. Temporary Artifact Paths

All DB/artifact paths used by benchmark fixtures must:

* Be placed under the **OS temporary directory** (`std::filesystem::temp_directory_path()`).
* Include a **unique suffix** (e.g., steady-clock tick count) to prevent
  collisions when benchmarks run concurrently or repeatedly.
* Be **cleaned up** in `TearDown()` (and defensively at the start of `SetUp()`).

```cpp
// ✅ correct
const auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
db_path_ = (std::filesystem::temp_directory_path() /
            ("themis_bench_myfix_" + std::to_string(ts))).string();

// ❌ forbidden — relative path, requires specific working directory
db_path_ = "bench_myfix_db";

// ❌ forbidden — shared static path; two instances collide
static const std::string DB_PATH = "bench_myfix_db";
```

The `themis::bench::TempDir` RAII helper in `bench_fixtures.h` automates this
pattern for fixtures that only need a directory handle.

### 3. Setup vs. Measurement Separation

Setup work (index construction, data loading, DB open) **must** be placed in:

* `benchmark::Fixture::SetUp()` — for fixture-based benchmarks, or
* outside the `for (auto _ : state)` loop — for free-function benchmarks.

The inner loop must contain **only** the code being measured.  Use
`state.PauseTiming()` / `state.ResumeTiming()` when per-iteration reset is
unavoidable.

```cpp
// ✅ correct — setup in SetUp(), only measured work in the loop
void SetUp(const ::benchmark::State&) override {
    db_ = openDb();
    loadData();
}
BENCHMARK_F(MyFixture, BM_Read)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(db_->read(key_));
    }
}

// ❌ wrong — DB construction pollutes every iteration
static void BM_Read(benchmark::State& state) {
    for (auto _ : state) {
        RocksDBWrapper db(cfg);   // ← setup inside the loop
        db.open();
        benchmark::DoNotOptimize(db.read("k"));
    }
}
```

### 4. Real-Time Mode for I/O-Bound Benchmarks

All benchmarks that perform **disk I/O, network I/O, or external calls** must
register with `.UseRealTime()`.  CPU-only micro-benchmarks may omit it.

```cpp
// ✅ correct for storage benchmarks
BENCHMARK_REGISTER_F(CRUDFixture, InsertWithAllIndexes)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
```

### 5. Core Metric Naming

All benchmark registrations must report at least:

| Counter name | Type | Meaning |
|---|---|---|
| `items_processed` | via `state.SetItemsProcessed()` | throughput (ops/s) |
| `qps` | `Counter::kIsRate` | query throughput (when applicable) |
| `vectors` / `nodes` / `records` | scalar | dataset size |

Use consistent counter names across related benchmarks so automated
regression analysis can compare them reliably.

---

## Running Standardised Benchmarks

```bash
# Build (linux-release preset)
cmake --preset linux-release
cmake --build --preset linux-release --target bench_crud bench_ycsb bench_batch_insert bench_graph_traversal bench_vector_search

# Run with JSON output for regression tracking
./build/linux-release/benchmarks/bench_crud \
    --benchmark_out=results/bench_crud.json \
    --benchmark_out_format=json

./build/linux-release/benchmarks/bench_ycsb \
    --benchmark_out=results/bench_ycsb.json \
    --benchmark_out_format=json
```

## Interpreting Results

* **`real_time`** (µs/ms) — wall-clock latency per iteration; use for I/O-bound benchmarks.
* **`cpu_time`** — CPU time per iteration; use for CPU-only micro-benchmarks.
* **`items_per_second`** (ops/s) — throughput derived from `SetItemsProcessed`.
* **`qps`** (`Counter::kIsRate`) — explicit query throughput counter.

Compare results across runs using the same binary, preset, and hardware.
Do not compare `cpu_time` across machines; use `real_time` for cross-machine
comparisons when hardware is documented.

## Remaining Limitations (Welle 1 Scope)

* The singleton `SearchEnv` in `bench_vector_search.cpp` is not yet converted
  to a fixture; it is cleaned up on process exit but not between benchmark
  families within the same run.  This is tracked for Welle 2.
* Python/script benchmarks (`*.py`, `*.sh`) are not yet covered by these rules.
* Statistical analysis (Welch's t-test, effect size) is provided by
  `benchmarks/chimera.py` but is not yet integrated into the C++ benchmark
  runner output.  Integration is tracked in `ROADMAP.md`.
