### Context

This issue implements the roadmap item '`TSStore`: Single-Point Insert Buffering for Gorilla Compression' for the timeseries domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `TSStore`: Single-Point Insert Buffering for Gorilla Compression

### Goal

Deliver the scoped changes for `TSStore`: Single-Point Insert Buffering for Gorilla Compression in src/timeseries/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `TSStore`: Single-Point Insert Buffering for Gorilla Compression
**Priority:** High
**Target Version:** v1.8.0

`tsstore.cpp` line 213 (1 confirmed TODO): "TODO: Implement buffering strategy for single-point inserts with Gorilla". When individual data points are inserted one at a time (the common IoT write pattern), Gorilla compression cannot be applied because it requires a buffer of consecutive timestamps. Each point is stored uncompressed, negating the 4–8× compression ratio Gorilla provides for batched writes.

**Implementation Notes:**
- `[ ]` The `TSAutoBuffer` (`ts_auto_buffer.cpp`) already exists as the adaptive flush layer; wire `TSStore::insert(single_point)` to route through `TSAutoBuffer` rather than writing directly to RocksDB when batch size = 1.
- `[ ]` `TSAutoBuffer` should accumulate up to `config_.gorilla_batch_size` (default 128) points before encoding with Gorilla and writing as a single chunk.
- `[ ]` Add backpressure signal to `TSAutoBuffer::push()`: return `BUFFER_FULL` when the in-memory buffer exceeds `config_.max_buffer_bytes`.
- `[ ]` Add unit test: 1000 single-point inserts, verify compressed on-disk size is ≤ 15% of raw (Gorilla target), p99 insert latency ≤ 50 µs.

---


**Priority:** High
**Target Version:** v0.9.0

Rewrite the `gorilla.cpp` decode path to use SIMD intrinsics (AVX2 on x86-64, NEON on ARM) for delta-of-delta reconstruction, dramatically increasing scan throughput for range queries over long time windows.

**Implementation Notes:**
- Add `gorilla_simd.cpp` alongside `gorilla.cpp` with AVX2 and NEON implementations selected via CMake feature detection; `gorilla.cpp` dispatches at runtime via CPUID check.
- `utils/simd_distance.cpp` already contains AVX2 helper patterns; reuse the lane-shuffle utilities for bit-unpacking the Gorilla XOR stream.
- `query_optimizer.cpp` should hint the expected decode width (float32 vs. float64) to the decoder to allow width-specific vectorisation paths.
- Benchmark with the existing `benchmarks/` harness; compare against the scalar baseline from `gorilla.cpp`.

**Performance Targets:**
- Gorilla decode throughput: >2 GB/s of decoded data per core (up from ~400 MB/s scalar).
- Range scan over 1M points (float64): <50 ms P99 including chunk fetch from `tsstore.cpp`.

---

### Acceptance Criteria

- [ ] The `TSAutoBuffer` (`ts_auto_buffer.cpp`) already exists as the adaptive flush layer; wire `TSStore::insert(single_point)` to route through `TSAutoBuffer` rather than writing directly to RocksDB when batch size = 1.
- [ ] `TSAutoBuffer` should accumulate up to `config_.gorilla_batch_size` (default 128) points before encoding with Gorilla and writing as a single chunk.
- [ ] Add backpressure signal to `TSAutoBuffer::push()`: return `BUFFER_FULL` when the in-memory buffer exceeds `config_.max_buffer_bytes`.
- [ ] Add unit test: 1000 single-point inserts, verify compressed on-disk size is ≤ 15% of raw (Gorilla target), p99 insert latency ≤ 50 µs.
- [ ] Add `gorilla_simd.cpp` alongside `gorilla.cpp` with AVX2 and NEON implementations selected via CMake feature detection; `gorilla.cpp` dispatches at runtime via CPUID check.
- [ ] `utils/simd_distance.cpp` already contains AVX2 helper patterns; reuse the lane-shuffle utilities for bit-unpacking the Gorilla XOR stream.
- [ ] `query_optimizer.cpp` should hint the expected decode width (float32 vs. float64) to the decoder to allow width-specific vectorisation paths.
- [ ] Benchmark with the existing `benchmarks/` harness; compare against the scalar baseline from `gorilla.cpp`.
- [ ] Gorilla decode throughput: >2 GB/s of decoded data per core (up from ~400 MB/s scalar).
- [ ] Range scan over 1M points (float64): <50 ms P99 including chunk fetch from `tsstore.cpp`.

### Relationships

- Roadmap row: #116 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/timeseries/FUTURE_ENHANCEMENTS.md#tsstore-single-point-insert-buffering-for-gorilla-compression
- Source key: roadmap:116:timeseries:v1.8.0:tsstore-single-point-insert-buffering-for-gorilla-compression

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:116:timeseries:v1.8.0:tsstore-single-point-insert-buffering-for-gorilla-compression -->
<!-- roadmap-ref: row=116;module=timeseries;target=v1.8.0 -->
<!-- roadmap-detail: src/timeseries/FUTURE_ENHANCEMENTS.md#tsstore-single-point-insert-buffering-for-gorilla-compression -->
