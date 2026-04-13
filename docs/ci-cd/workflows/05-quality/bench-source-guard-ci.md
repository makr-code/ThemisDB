# Bench-Source CI Guard

**Workflow file:** `.github/workflows/05-quality_build_bench-source-guard-ci.yml`  
**Guard script:** `tools/check_bench_targets.py`  
**Allowlist file:** `tools/bench_source_allowlist.toml`  
**Reference:** `PERFORMANCE_EXPECTATIONS.md` §1.4, Maßnahme #8

---

## Purpose

Prevents `bench_*.cpp` source files from silently existing without a
corresponding CMake build target or built binary.  Every benchmark source
that cannot be compiled produces a false sense of coverage and wastes
developer time when the file is later discovered to be broken or incomplete.

The guard runs on every PR that touches `benchmarks/` or the guard tooling
itself, on every push to `main` / `develop`, and nightly via cron.

---

## How the Guard Works

The guard performs two complementary checks:

### Check 8a – CMake-target coverage

`tools/check_bench_targets.py` scans `benchmarks/bench_*.cpp` and verifies
that each source is covered by at least one of:

1. An explicit `add_executable(<name> ...)` entry in
   `benchmarks/CMakeLists.txt` whose `<name>` matches the file stem.
2. The `THEMIS_AUTO_REGISTER_ELIGIBLE_BENCHMARKS` auto-registration block in
   `benchmarks/CMakeLists.txt`, which dynamically creates
   `EXCLUDE_FROM_ALL` targets for all remaining sources at CMake configure
   time.

If neither condition is true and the source is **not** in the allowlist,
the guard exits with code `1` and CI fails.

### Check 8b – Guard script presence

`tools/perf_expectations_audit.py` (Check 8b) verifies that
`tools/check_bench_targets.py` itself exists in the repository so that
developers can run the check locally and as a pre-commit hook.

### Optional: Binary-level check (`--build-dir`)

When a build directory is passed via `--build-dir DIR`, the guard also
verifies that each covered source has a corresponding executable
(`bench_*` or `bench_*.exe`) present under `DIR`.  This mode is used by
release-pipeline workflows that perform an actual build before running the
guard.

```bash
# Example: verify binaries after building
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target all_benchmarks
python3 tools/check_bench_targets.py --build-dir build-release --no-color
```

---

## Output Format

### Text (default)

```
ThemisDB Bench-Source CI Guard – benchmarks/CMakeLists.txt coverage check
ℹ️  Allowlist  13 entries loaded from bench_source_allowlist.toml

✅ PASS  All bench_*.cpp sources are covered.  (76 via auto-registration, 0 orphaned)

ℹ️  ALLOWLIST  13 source(s) are explicitly allowed to be absent:
    ○  benchmarks/bench_cuda_vs_cpu.cpp  [gpu]  # GPU_ONLY: requires CUDA device
    ○  benchmarks/bench_arm_simd.cpp  [platform]  # PLATFORM: ARM SIMD intrinsics
    ...
```

When failures occur, each orphaned source is printed with its
**Grundkategorie** (base category) in brackets:

```
❌ FAIL  2 orphaned bench_*.cpp source(s) detected.
    ✗  benchmarks/bench_my_new_feature.cpp  [query]
    ✗  benchmarks/bench_experimental.cpp  [misc]
```

### JSON (`--format json`)

```json
{
  "pass": false,
  "auto_registration_present": true,
  "orphaned_sources": [
    { "stem": "bench_my_new_feature", "category": "query" }
  ],
  "binary_missing_sources": [],
  "allowlisted_sources": [
    { "stem": "bench_cuda_vs_cpu", "reason": "GPU_ONLY: ...", "category": "gpu" }
  ],
  "counts": {
    "orphaned": 1,
    "auto_reg_covered": 75,
    "binary_missing": 0,
    "allowlisted": 13
  }
}
```

---

## Grundkategorie (Base Category)

The category is derived from the first `_`-separated segment of the stem
after `bench_`.  Known segments are mapped to the following canonical
categories:

| Segment prefix | Category | Examples |
|---|---|---|
| `query`, `adaptive`, `diff`, `fused`, `hybrid`, `olap` | `query` | bench_query, bench_adaptive_query_cache |
| `security`, `auth`, `compliance`, `encryption`, `governance`, `pii`, `policy`, `legal`, `hsm` | `security` | bench_security, bench_auth_token_validation |
| `storage`, `blob`, `snapshot`, `wal`, `insert`, `batch`, `crud`, `index`, `postgres` | `storage` | bench_storage_performance, bench_batch_insert |
| `vector`, `hnsw` | `vector` | bench_vector_search, bench_hnsw |
| `gpu`, `cuda`, `vulkan` | `gpu` | bench_cuda_vs_cpu, bench_gpu_vector_index |
| `ml`, `rag`, `llm`, `llama`, `embedding`, `qlora`, `lora`, `ethics`, `prompt`, `whisper` | `ml` | bench_rag_ethics, bench_llm_inference_performance |
| `geo`, `spatial`, `approximate`, `mmdb` | `geo` | bench_geo_radius_search |
| `graph`, `pagerank` | `graph` | bench_graph_traversal |
| `distributed`, `replication`, `shard`, `sharding`, `saga`, `gossip` | `distributed` | bench_distributed_coordinator |
| `concurrency`, `lock`, `mvcc`, `thread`, `multithreading`, `transaction` | `concurrency` | bench_mvcc_conflict |
| `timeseries`, `temporal`, `gorilla` | `timeseries` | bench_timeseries_insert |
| `platform`, `arm`, `simd`, `docker` | `platform` | bench_arm_simd, bench_docker_raid |
| `observability`, `metrics`, `exporters` | `observability` | bench_metrics_collector |
| `benchmark`, `tpcc`, `tpch`, `ycsb` | `benchmark` | bench_tpcc, bench_ycsb |
| `streaming`, `stream` | `streaming` | bench_stream_ingestion |
| `aql` | `aql` | bench_aql_functions |
| `cdc`, `changefeed` | `cdc` | bench_cdc_pipeline |
| `api` | `api` | bench_api_endpoints |
| `content`, `text` | `content` | bench_text_extraction |
| `acceleration`, `backend` | `acceleration` | bench_backend_comparison |
| `io`, `async` | `io` | bench_async_io_multiscan |
| other | raw segment | bench_my_custom_feature → `my` |

---

## Allowlist: `tools/bench_source_allowlist.toml`

### Purpose

The allowlist explicitly permits certain `bench_*.cpp` files to be absent
from the build without triggering CI failure.  This is necessary for
benchmarks that require hardware or third-party dependencies not available
in the standard CI environment.

### File Format

```toml
# Comment
bench_cuda_vs_cpu = "GPU_ONLY: requires CUDA device"
bench_arm_simd = "PLATFORM: ARM SIMD intrinsics"
```

Plain stem-only lines are also accepted:

```
bench_experimental_feature  # EXPERIMENTAL: not yet wired
```

### Allowlist Reason Codes

| Code | When to use |
|---|---|
| `GPU_ONLY` | Benchmark requires physical GPU (CUDA, HIP, Vulkan compute).  CMake gate: `THEMIS_ENABLE_CUDA` / `THEMIS_ENABLE_HIP` / `THEMIS_ENABLE_VULKAN`. |
| `PLATFORM` | Benchmark only compiles on a specific OS or CPU architecture (ARM, Linux-only, Windows-only). |
| `EXPERIMENTAL` | Benchmark source exists but is not yet wired into CMake.  **Must be removed from the allowlist** when `add_executable()` is added. |
| `THIRD_PARTY` | Benchmark requires model weights, hardware devices, or libraries that are intentionally absent from CI runners. |
| `DEPRECATED` | Source is kept for historical reference only.  **Must be deleted from the repository** within one minor release cycle. |

### Adding an Entry

1. Open `tools/bench_source_allowlist.toml`.
2. Add a line with the stem and a reason code:
   ```toml
   bench_my_gpu_bench = "GPU_ONLY: requires CUDA 12+"
   ```
3. Run the guard locally to confirm the entry is recognised:
   ```bash
   python3 tools/check_bench_targets.py --no-color
   ```
4. Commit the allowlist change together with the new `bench_*.cpp` file.

### Removing an Entry

Once the blocking issue is resolved (hardware added to CI, library
installed, feature wired into CMake):

1. Delete the allowlist line.
2. Verify the guard still passes:
   ```bash
   python3 tools/check_bench_targets.py --no-color
   ```

### Audit

Run with `--allowlist /dev/null` to see the full gap list ignoring all
allowlist entries:

```bash
python3 tools/check_bench_targets.py --allowlist /dev/null --no-color
```

---

## Running Locally

```bash
# Default check (CMake-target coverage + auto-loaded allowlist)
python3 tools/check_bench_targets.py

# Strict mode: every bench_*.cpp needs an explicit add_executable()
python3 tools/check_bench_targets.py --strict

# With a specific allowlist
python3 tools/check_bench_targets.py --allowlist tools/bench_source_allowlist.toml

# Binary check against a build directory
python3 tools/check_bench_targets.py --build-dir build-release

# JSON output for tooling
python3 tools/check_bench_targets.py --format json

# Quiet (summary only)
python3 tools/check_bench_targets.py -q
```

---

## CI Workflow Details

The workflow `.github/workflows/05-quality_build_bench-source-guard-ci.yml`
runs two steps:

| Step | Tool | Checks |
|---|---|---|
| `guard` | `tools/check_bench_targets.py` | 8a: orphaned bench sources |
| `audit` | `tools/perf_expectations_audit.py` | 8a + 8b: coverage + guard script exists |

Both steps must pass for the PR to be mergeable.  The full audit JSON is
uploaded as a CI artefact (`perf-audit-results-<sha>`) and appended to the
job summary.

---

## Adding a New `bench_*.cpp`

1. Create `benchmarks/bench_<name>.cpp`.
2. Add `add_executable(bench_<name> bench_<name>.cpp)` to
   `benchmarks/CMakeLists.txt` (preferred), **or** rely on the
   auto-registration block if the target has no special link dependencies.
3. If the binary cannot be built in standard CI (GPU-only, platform-specific,
   etc.), add an entry to `tools/bench_source_allowlist.toml` with an
   appropriate reason code.
4. Run `python3 tools/check_bench_targets.py --no-color` locally and confirm
   the guard passes before opening the PR.
