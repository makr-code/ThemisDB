# ThemisDB Benchmark Measurement Hygiene

## Scope

This document defines the mandatory hygiene rules for C++ Google Benchmark
targets in `benchmarks/`.  It covers Welle 1 / PR-A of the benchmark hardening
initiative and applies to all new and modified benchmark files.

## Affected Benchmarks (Welle 1 / PR-A)

| File | Issue fixed |
|---|---|
| `bench_crud.cpp` | Non-deterministic RNG seed; hardcoded relative DB path |
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
