# ThemisDB Benchmark Standards

Status: Canonical
Scope: All C++ benchmark files under benchmarks/ and related CMake/CTest wiring.

## 1. Purpose

This document defines binding standards for:
- Google Benchmark source structure and execution behavior
- CTest integration for benchmark smoke coverage where applicable
- Reproducibility and measurement hygiene baseline requirements

If another benchmark document conflicts with this file, this file wins for
benchmark structure and registration rules.

## 2. Google Benchmark Standard (Binding)

### 2.1 File Structure

- Benchmark files follow a consistent naming and fixture structure.
- Setup and teardown logic must be separated from measured loops.
- Shared helpers must be locally scoped to avoid global collisions.

### 2.2 Naming

- Benchmark names must express workload and scenario clearly.
- Keep naming consistent across modules for comparable workloads.
- Avoid aliases that obscure implementation path or dataset class.

### 2.3 Measurement Contract

- I/O-bound benchmarks must use wall-clock semantics (UseRealTime).
- benchmark_min_time must include an explicit unit (for example, 0.05s).
- Counters and processed-item metrics must be consistent and comparable.

### 2.4 Multi-Thread Rules

- No thread-unsafe finalization inside measured multi-thread loops.
- Shared mutable state must be synchronized or partitioned.
- Final aggregation belongs in fixture teardown or dedicated post-phase logic.

## 3. Reproducibility Standard (Binding)

- Use deterministic random seeds unless explicitly documented otherwise.
- Use unique temp paths for per-run artifacts.
- Keep warmup policy explicit and outside measured loops.
- JSON output mode should be available for baseline comparisons.

## 4. CMake and CTest Integration

- Benchmark targets must be built deterministically by preset/target mapping.
- CTest registration for benchmark smoke tests must use the canonical
  `themis_register_benchmark_ctest()` helper from
  `benchmarks/cmake/BenchmarkPolicy.cmake`.
- Direct `add_test()` + `set_tests_properties()` calls are **forbidden** in
  benchmark CMakeLists; they bypass `$<TARGET_FILE:...>` resolution and omit
  the mandatory label schema.
- Script-only benchmarks may remain outside CTest but must document runner flow.
- No orphaned CTest entries pointing to missing benchmark binaries; the helper
  already guards against this automatically.

### 4.1 Canonical Registration Pattern

```cmake
# Example: module-level smoke entry
themis_register_benchmark_ctest(
    NAME         bench_<module>_<workload>
    TARGET       bench_<module>_<workload>
    MODULE       "<module>"
    TIMEOUT      120           # seconds; adjust for long-running suites
    EXTRA_LABELS release_critical  # only when gate-critical
)
```

The helper enforces:
- `$<TARGET_FILE:TARGET>` resolution — CTest finds the binary regardless of
  output directory structure.
- Mandatory label set: `module:<module>;tier:benchmark;kind:smoke`.
- JSON output written to `${CMAKE_BINARY_DIR}/bench_results/<target>.json`.
- Standard `--benchmark_min_time=0.1s` unless overridden via `MIN_TIME`.
- Guard: the entry is silently skipped when the target is not available (avoids
  orphaned CTest entries when a benchmark is disabled by a feature flag).

## 5. Migration Rules for Existing Benchmarks

- Any touched benchmark file must be aligned to this standard in the same PR.
- Existing measurement docs should be retained as domain-specific addenda.
- Domain-specific docs may extend rules, but must not weaken this baseline.

## 6. Relationship to Measurement Hygiene

MEASUREMENT_HYGIENE.md remains a mandatory detailed companion for protocol,
warmup, and variance control. This file defines baseline standards; measurement
hygiene provides deeper execution detail.

## 7. Verification Checklist

- [ ] Naming and fixture structure are consistent
- [ ] Setup and measurement loops are cleanly separated
- [ ] benchmark_min_time uses explicit units
- [ ] I/O paths use real-time measurements where required
- [ ] CTest smoke registration is valid (if applicable)
- [ ] Reproducibility controls are documented and active
