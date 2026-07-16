### Context

This issue implements the roadmap item 'SIMD Vectorization — AVX-512 and ARM NEON' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 14 · SIMD Vectorization — AVX-512 and ARM NEON

### Goal

Deliver the scoped changes for SIMD Vectorization — AVX-512 and ARM NEON in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 14 · SIMD Vectorization — AVX-512 and ARM NEON
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/olap.cpp`, `src/analytics/columnar_execution.cpp`, `src/analytics/forecasting.cpp`

The existing SIMD acceleration covers AVX2 for aggregation kernels in `olap.cpp` and the
Yule–Walker autocovariance loop in `forecasting.cpp`.  AVX-512 (2× AVX2 width for double)
and ARM NEON (Cortex-A78 and Apple Silicon) paths are absent.

**Implementation Notes:**
- `[ ]` Add `#ifdef __AVX512F__` path in `olap.cpp` `vectorizedSum/Avg/Min/Max` — process 8 `double` per cycle vs AVX2's 4; use `_mm512_reduce_add_pd` for horizontal reduction
- `[ ]` Add `#ifdef __ARM_NEON` path with `float64x2_t` NEON intrinsics for `ColumnAggregator` in `columnar_execution.cpp` — ARM builds currently fall back to scalar
- `[ ]` Gate all SIMD paths behind runtime CPUID checks (`__builtin_cpu_supports("avx512f")`) when the binary must run on heterogeneous hardware
- `[ ]` Extend `forecasting.cpp` Yule–Walker AVX2 inner loop to AVX-512 (8 doubles/cycle) for the `acov0_avx2` function already scaffolded in the existing doc
- `[ ]` ARM NEON and AVX2 results must produce bit-identical output (within 1 ULP) to the scalar baseline — add a parity assertion in the CI test suite

**Performance Targets:**
- AVX-512 SUM over 10 M doubles: ≥ 2× throughput vs AVX2 baseline
- ARM NEON aggregation throughput: ≥ 4 GB/s on Cortex-A78

---

### Acceptance Criteria

- [ ] Add `#ifdef __AVX512F__` path in `olap.cpp` `vectorizedSum/Avg/Min/Max` — process 8 `double` per cycle vs AVX2's 4; use `_mm512_reduce_add_pd` for horizontal reduction
- [ ] Add `#ifdef __ARM_NEON` path with `float64x2_t` NEON intrinsics for `ColumnAggregator` in `columnar_execution.cpp` — ARM builds currently fall back to scalar
- [ ] Gate all SIMD paths behind runtime CPUID checks (`__builtin_cpu_supports("avx512f")`) when the binary must run on heterogeneous hardware
- [ ] Extend `forecasting.cpp` Yule–Walker AVX2 inner loop to AVX-512 (8 doubles/cycle) for the `acov0_avx2` function already scaffolded in the existing doc
- [ ] ARM NEON and AVX2 results must produce bit-identical output (within 1 ULP) to the scalar baseline — add a parity assertion in the CI test suite
- [ ] AVX-512 SUM over 10 M doubles: ≥ 2× throughput vs AVX2 baseline
- [ ] ARM NEON aggregation throughput: ≥ 4 GB/s on Cortex-A78

### Relationships

- Roadmap row: #46 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#14--simd-vectorization--avx-512-and-arm-neon
- Source key: roadmap:46:analytics:v1.8.0:14-simd-vectorization-avx-512-and-arm-neon

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:46:analytics:v1.8.0:14-simd-vectorization-avx-512-and-arm-neon -->
<!-- roadmap-ref: row=46;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#14--simd-vectorization--avx-512-and-arm-neon -->
