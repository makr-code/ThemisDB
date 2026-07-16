# Benchmark Measurement Hygiene
<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

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
