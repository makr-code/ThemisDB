# Tensor Storage Strategy Assessment
<!-- Status: validated | Issue: #5443 -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Scope

This document records the assessment of quantization, memory-mapped I/O
(`mmap`), and zero-copy load strategies for ThemisDB tensor and adapter
artifacts (Issue #5443, Phases 1–7).

Artifacts in scope:

- Advisory summary tensors managed by `ManifestStore` (Phase A)
- LoRA / adapter weight tensors (PRIMARY `ArtifactClass`)
- Shard-level derived tensors (DERIVED `ArtifactClass`)

---

## Phase 1 — Design / API Contract

### 1.1 Artifact Format and Layout Evaluation Matrix

| Format     | Precision  | Bytes/param | Typical L2 error | Kernel support           | Calibration required |
|------------|------------|-------------|------------------|--------------------------|----------------------|
| F32        | Full       | 4           | 0 (lossless)     | Native everywhere        | No                   |
| F16        | High       | 2           | ≤ 0.1 %          | All modern CPUs/GPUs     | No                   |
| BF16       | High       | 2           | ≤ 0.2 %          | x86 AVX-512-BF16, ARM    | No                   |
| INT8       | Medium     | 1           | ≤ 1 %            | All CPUs (VNNI optional) | Yes (affine quant)   |
| INT4       | Low        | 0.5 (nibble)| ≤ 5 %            | ARMv8.4-A, AVX-512VNNI  | Yes                  |
| BINARY     | Extreme    | 0.125 bit   | ≤ 20 %           | Popcount SIMD            | Yes                  |

**Recommendation matrix by artifact class:**

| Artifact class  | Recommended level    | Rationale                                          |
|-----------------|----------------------|----------------------------------------------------|
| Training ckpt   | F32                  | Lossless reference; never degrade training state   |
| Base model infer| F16 / INT8           | F16 default; INT8 with calibration for 4× savings  |
| LoRA adapter    | INT4 / INT8          | Small rank (8–64); INT4 acceptable at > 98 % quality|
| Advisory summary| F16 or lower         | Not authoritative; compression acceptable           |
| Shard summary   | BF16 / INT8          | Routing heuristic; accuracy < recall matters more  |

### 1.2 mmap / Zero-Copy API Design

The public API is defined in
`include/distributed_tensor/tensor_storage_strategy.h`:

```
QuantizationLevel / QuantizationConstraints / QuantizationAssessment
QuantizationAssessor::assess(constraints)   → QuantizationAssessment
QuantizationAssessor::isFeasible(level, c)  → bool

MmapRegion        (RAII handle: data, size, close, as_span<T>)
MmapLoader::open(path, lock_pages)          → (MmapRegion, MmapError)
MmapLoader::advise(region, AccessPattern)   → MmapError

ZeroCopyAccessor<T>(region)                 → typed span view

StorageStrategyAssessor::assess(Config)     → StorageStrategyRecommendation
```

Key design decisions:

1. **Move-only MmapRegion** — prevents aliased mapping handles. Ownership is
   explicit and deterministic.
2. **Typed `as_span<T>()`** — zero allocation, no copy; caller gets a
   `std::span<const T>` directly into the kernel page cache.
3. **Stateless assessors** — `QuantizationAssessor` and
   `StorageStrategyAssessor` have no mutable state; safe to call concurrently.
4. **Advisory-only invariant preserved** — the API does not write back
   to the mapped region (PROT_READ only).

---

## Phase 2 — Core Implementation

Implementation lives in `src/distributed_tensor/src/tensor_storage_strategy.cc`.

### mmap Loader

- POSIX path: `open(O_RDONLY|O_CLOEXEC)` → `fstat` → `mmap(PROT_READ, MAP_SHARED)`.
- Windows path: `CreateFileW` → `CreateFileMappingW(PAGE_READONLY)` → `MapViewOfFile(FILE_MAP_READ)`.
- Empty-file edge case: returns `MmapError::OK` with `size() == 0` (zero-length
  mappings are not valid on POSIX; handled by returning a null region).
- `lock_pages`: optional `mlock()` after mapping; failure is non-fatal (common
  without root/`RLIMIT_MEMLOCK`).

### Quantization Assessor Decision Tree

```
if max_l2_error_relative == 0 → F32 (lossless)
else
  for candidate in [BINARY, INT4, INT8, BF16, F16, F32]:
    if isFeasible(candidate, constraints):
      return candidate

isFeasible(level, c):
  - F32 always feasible
  - INT8/INT4 require has_calibration_data == true
  - error: typicalL2Error(level) <= max_l2_error_relative
  - memory: packedBytesForParams(num_params, level) <= memory_budget_bytes
```

### StorageStrategyAssessor

Combines quantization selection with load-mechanism selection:

| Condition                                  | Load mechanism   |
|--------------------------------------------|------------------|
| os_supports_mmap == false                  | BUFFERED_READ    |
| multi_consumer == true && nvme             | MMAP_PREFAULT    |
| multi_consumer == true && !nvme            | MMAP_ZERO_COPY   |
| multi_consumer == false && nvme            | MMAP_PREFAULT    |
| multi_consumer == false && !nvme           | MMAP_ZERO_COPY   |

---

## Phase 3 — Error Handling & Edge Cases

| Scenario                          | Handling                                           |
|-----------------------------------|----------------------------------------------------|
| File not found                    | `MmapError::FILE_NOT_FOUND`, region stays closed   |
| Permission denied                 | `MmapError::PERMISSION_DENIED`                     |
| Empty file (0 bytes)              | `MmapError::OK`, `region.size() == 0`; accessor empty |
| mmap not available (THEMIS_NO_MMAP) | `MmapError::UNSUPPORTED_PLATFORM`               |
| mlock fails (no privilege)        | Non-fatal; mapping continues without locking       |
| advise on closed region           | `MmapError::NOT_OPEN`                              |
| INT8 without calibration          | `isFeasible` returns false; falls back to F16/BF16 |
| No feasible level in budget       | F32 selected as safe fallback (always feasible)    |
| Zero num_params                   | Memory-budget check skipped; level selected by error budget only |

---

## Phase 4 — Tests

Test file: `tests/epic3_distributed_tensor/test_tensor_storage_strategy.cpp`

| Group | ID      | Description                                            |
|-------|---------|--------------------------------------------------------|
| QSE   | QSE-01  | Zero error budget → F32                                |
| QSE   | QSE-02  | Large error budget + calibration → INT4 (adapter)      |
| QSE   | QSE-03  | Tight budget, no calibration → F16 or BF16             |
| QSE   | QSE-04  | INT8 skipped without calibration                        |
| QSE   | QSE-05  | Memory budget forces INT8                              |
| QSE   | QSE-06  | F32 always feasible                                    |
| QSE   | QSE-07  | INT8/INT4 not feasible without calibration             |
| QSE   | QSE-08  | AVX-512-BF16 hardware triggers advisory warning        |
| QSE   | QSE-09  | bytesPerParam correct for all levels                   |
| QSE   | QSE-10  | packedBytesForParams handles sub-byte packing          |
| MML   | MML-01  | Open valid file; region open, size matches             |
| MML   | MML-02  | Mapped data matches file content                       |
| MML   | MML-03  | Open missing file → FILE_NOT_FOUND                     |
| MML   | MML-04  | close() releases mapping                               |
| MML   | MML-05  | Double close() is safe                                 |
| MML   | MML-06  | Move constructor transfers ownership                   |
| MML   | MML-07  | advise() on open region returns OK                     |
| MML   | MML-08  | advise() on closed region returns NOT_OPEN             |
| ZCA   | ZCA-01  | Accessor size = bytes / sizeof(T)                      |
| ZCA   | ZCA-02  | operator[] returns correct value                       |
| ZCA   | ZCA-03  | Range-based for loop iterates all elements             |
| ZCA   | ZCA-04  | span() points to same data                             |
| ZCA   | ZCA-05  | int8_t accessor reinterprets bytes correctly           |
| ZCA   | ZCA-06  | Empty region → accessor empty                          |
| SSA   | SSA-01  | No mmap support → BUFFERED_READ                        |
| SSA   | SSA-02  | NVMe + single consumer → MMAP_PREFAULT                 |
| SSA   | SSA-03  | HDD + single consumer → MMAP_ZERO_COPY                 |
| SSA   | SSA-04  | Multi-consumer → MMAP_PREFAULT or MMAP_ZERO_COPY       |
| SSA   | SSA-05  | Quantization and size estimate propagate correctly     |
| SSA   | SSA-06  | No-mmap caveat appears in caveats list                 |

---

## Phase 5 — Performance / Hardening

### mmap vs. `read()` trade-off summary

| Aspect               | `mmap` + zero-copy               | Buffered `read()`                |
|----------------------|----------------------------------|----------------------------------|
| Kernel→user copies   | 0 (page-fault only)              | 1 per `read()` call              |
| RSS at steady state  | Shared page cache (all consumers)| Private pages per process        |
| Random access        | O(1) via page fault              | Seek + read overhead             |
| Sequential large file| MADV_SEQUENTIAL + WILLNEED       | `read()` + kernel read-ahead     |
| NVMe cold load       | Page faults (latency spike)      | Predictable buffered latency     |
| NVMe warm load       | ≈ 0 (pages cached)               | Re-read + copy                   |
| mlock pages          | Optional; pins against eviction  | Not applicable                   |

**Recommendation:** For tensor artifacts > 64 MiB with ≥ 2 concurrent readers,
`MMAP_PREFAULT` (MADV_WILLNEED) reduces first-query latency by front-loading
page faults during startup rather than during serving.  For single-reader or
streaming workloads `MMAP_ZERO_COPY` saves RSS.

### Quantization compression savings (7B model, float32 baseline = 28 GiB)

| Level  | Size    | Savings | Notes                          |
|--------|---------|---------|--------------------------------|
| F32    | 28.0 GiB| —       | Reference baseline             |
| F16    | 14.0 GiB| 50 %    | Safe default for inference     |
| BF16   | 14.0 GiB| 50 %    | Preferable on AVX-512-BF16     |
| INT8   |  7.0 GiB| 75 %    | Requires calibration dataset   |
| INT4   |  3.5 GiB| 87.5 %  | Requires calibration; LoRA OK  |

### Known bottlenecks

- **mlock with large tensors**: `RLIMIT_MEMLOCK` defaults (64 KiB on many
  Linux distros) prevent locking > 64 KiB without privilege escalation or
  sysctl adjustment.
- **INT4 nibble packing/unpacking**: de-quantization is a hot path; AVX-512
  VNNI provides 4× throughput vs. scalar unpacking.
- **Cold-start on NVMe**: MADV_WILLNEED does not guarantee synchronous fault-in;
  `io_uring` or `preadv` with direct I/O may be needed for hard SLA.

---

## Phase 6 — Documentation & Production Recommendations

### Recommended production strategy

1. **Default to F16** for inference artifacts unless a memory budget forces
   lower precision.  F16 provides a 2× reduction with negligible quality loss.
2. **Use INT8** (with calibration) when serving > 4 concurrent model instances
   on the same host; 4× reduction pays for calibration cost after 1-2 days.
3. **INT4** is appropriate only for adapter/LoRA tensors where rank ≤ 64 and
   end-to-end quality is validated against F32 baseline.  Do not apply INT4
   to base model weights without evidence.
4. **Use `MMAP_PREFAULT`** (MADV_WILLNEED) on NVMe hosts; use `MMAP_ZERO_COPY`
   on HDD/SAN storage where random seeks are expensive.
5. **Avoid `BUFFERED_READ`** in multi-consumer deployments; each process gets
   its own copy, multiplying RAM usage.
6. **Do not mlock large tensors** unless `RLIMIT_MEMLOCK` is configured; prefer
   MADV_WILLNEED for hot paths.

### Integration path (Phase 7)

The recommended approach to integrate into the deployment pipeline:

1. Add `tensor_storage_strategy.cc` to the `themis_distributed_tensor` library
   target in `src/distributed_tensor/CMakeLists.txt`.
2. Call `StorageStrategyAssessor::assess()` during artifact registration in
   `ManifestStore::store()` to attach the recommended strategy to each entry.
3. Wire `MmapLoader` into the tensor fetch path so consumers receive a
   `ZeroCopyAccessor<float>` or `ZeroCopyAccessor<int8_t>` depending on the
   manifest's quantization level.
4. Expose `QuantizationLevel` as a field in `ArtifactManifest` once Phase B
   delta-log work begins, to track format across versions.

---

## Phase 7 — Integration

### Build integration

Add the following to `src/distributed_tensor/CMakeLists.txt`:

```cmake
list(APPEND DISTRIBUTED_TENSOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tensor_storage_strategy.cc
)
```

And add the public header:

```cmake
list(APPEND DISTRIBUTED_TENSOR_PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/distributed_tensor/tensor_storage_strategy.h
)
```

### CTest registration

The test target `test_tensor_storage_strategy` is automatically picked up by
the glob in `tests/epic3_distributed_tensor/CMakeLists.txt` via the
`test_*.cpp` pattern.

### Advisory-only invariant

All code paths using `MmapLoader` and `ZeroCopyAccessor` handle
**advisory-only** tensor data.  They must never be used to override
graph-verified query results (see `ArtifactManifest` advisory-only invariant).

---

## Known Issues & Limitations

- Zero-copy is fully supported on Linux (mmap/madvise) and Windows
  (MapViewOfFile). macOS is supported for mmap but `madvise` hints for
  `MADV_WILLNEED` may have reduced effectiveness on Apple Silicon.
- INT4 de-quantization path is not implemented in this assessment phase;
  it is a Phase B/C deliverable.
- Direct I/O (`O_DIRECT`) is defined in the `LoadMechanism` enum but not
  implemented; it requires caller-aligned buffers and is a follow-on item.
- `mlock()` failures are currently silent (non-fatal). A future hardening
  pass should surface these as observable metrics.

## Breaking Changes

None. All new APIs are additions under the `themis::distributed_tensor`
namespace. No existing headers or classes are modified.
