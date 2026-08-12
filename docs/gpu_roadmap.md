## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


> **⚠️ STATUS: STALE – Archivierungskandidat**
> Dieser Inhalt enthält veraltete TODO/FIXME/PLACEHOLDER-Marker und wird im nächsten Archiv-Run nach `docs/ARCHIVED/` verschoben.
> Bitte nicht als aktuelle Referenz nutzen. Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

# GPU Module Production Readiness Assessment & Roadmap

## Current State Assessment

The GPU module (`src/gpu`) is **not yet 100% production ready**. While it provides
edition-aware VRAM limit enforcement, the implementation is a skeleton with no real
GPU runtime integration.

### Identified Gaps (original)

- **No CUDA/ROCm kernels**: No implemented GPU kernels; query acceleration, vector
  operations, and matrix multiplication are absent (listed as future in `src/gpu/FUTURE_ENHANCEMENTS.md`)
- **No real GPU allocations**: `gpu_memory_manager_edition.cpp` tracks a counter only;
  no actual `cudaMalloc`/`hipMalloc` calls or device handles
- **No device discovery**: No enumeration of available GPUs, device capability checks,
  or fallback when no GPU is present
- **No multi-GPU support**: Single device assumed; no partitioning, pooling, or
  load balancing across devices
- **No async/streaming**: No CUDA streams or ROCm queues; all operations would be
  synchronous stubs
- **No memory pooling or fragmentation handling**: Every allocation is tracked by a
  counter with no pool, slab, or defragmentation logic
- **No owner/tag tracking or leak detection**: Allocation records carry only a string
  reason; no caller identity, lifetime, or leak-detection pass
- **No observability**: No Prometheus/OpenTelemetry metrics (VRAM usage, alloc failures,
  fallback counts, peak/heatmaps); no structured telemetry on alloc-fail or CPU-fallback
  events
- **No security/isolation**: No tenant or domain separation for GPU allocations; any
  caller can exhaust VRAM for other tenants
- **No admin APIs**: No stats endpoint, dry-run simulation, or policy introspection
- **Missing tests**: No unit, integration, stress, fuzz, or chaos tests for alloc/free,
  device loss, multi-GPU, edition limits, or CPU-fallback paths
- **FUTURE_ENHANCEMENTS.md is plans only**: CUDA kernel support, GPU query acceleration,
  multi-GPU pooling, and async streams are all unimplemented

## Production Readiness Roadmap

### Stabilität & Sicherheit (Stability & Security)

- ✅ Implement device discovery: enumerate CUDA/ROCm devices at startup, log capability
  and VRAM, and skip GPU paths gracefully when no device is available
  (`include/themis/gpu/device_discovery.h`)
- ✅ Add fail-safe fallbacks: any GPU path must fall back to CPU with a structured warning
  when the device is unavailable, lost, or OOM (`include/themis/gpu/safe_fail.h`)
- ✅ Handle OOM and timeouts: distinguish soft OOM (limit hit) from hard OOM (driver
  error); enforce per-operation timeouts for kernel launches (`config.h`, `safe_fail.h`)
- ✅ Enforce tenant/domain isolation: tag allocations with tenant ID and enforce per-tenant
  VRAM quotas; reject allocations that would starve other tenants (`memory_manager.h`)
- ✅ Add policy-gated GPU usage: require explicit capability grant before a caller can
  use GPU resources; default-deny for new callers (`policy.h`)
- ✅ Validate kernel integrity: verify checksums/signatures of loaded GPU kernel blobs
  before execution; reject unrecognized kernels (`kernel_validator.h`)

### Korrektheit & Tests (Correctness & Tests)

- ✅ Unit tests for `GPUMemoryManager`: alloc success/failure, deallocation, double-free
  guard, overflow protection, edition-limit enforcement
- ✅ Integration tests: verify CPU fallback is triggered on alloc failure; test edition
  transitions; test concurrent alloc/dealloc under load
- ✅ Stress tests: hammer alloc/free at high concurrency to surface races or counter
  drift
- ✅ Fuzz tests: feed arbitrary sizes and reasons to `TryAllocateGPU`/`ValidateAllocation`
  to find assertion or exception paths
- ✅ Chaos tests: simulate device loss mid-operation; verify graceful degradation and
  error propagation
- ⬜ Golden tests for kernel launch paths once CUDA/ROCm kernels are added: capture
  expected outputs for regression detection *(blocked on real CUDA/ROCm integration)*
- ✅ Test multi-GPU scenarios: correct device selection, balanced load, failure of one
  device does not crash the process

### Observability & Operations

- ✅ Expose Prometheus/OpenTelemetry metrics: current VRAM allocated, peak VRAM,
  alloc success/failure counters, fallback-to-CPU counter, per-tenant usage
  (`metrics.h`)
- ✅ Add VRAM heatmaps and utilization histograms to Grafana dashboards
  (`grafana/dashboards/gpu_metrics.json`)
- ✅ Emit structured log events on alloc failure, edition-limit rejection, CPU fallback,
  and device loss with caller context and remediation hints (`audit_log.h`)
- ✅ Define and fire alerts: VRAM > 80% of limit, alloc failure rate spike, device
  unavailable, tenant quota exceeded (`alerts.h`)
- ✅ Provide admin/ops endpoints: `GET /admin/gpu/stats`, `GET /admin/gpu/tenants`,
  dry-run allocation simulation (`POST /admin/gpu/simulate`) (`admin_api.h`)

### Performance

- ✅ Implement a memory pool: pre-allocate VRAM slabs and serve requests from the pool
  to avoid per-call driver overhead (`memory_pool.h`)
- ✅ Add async streams: named GPU streams with CPU fallback budget enforcement
  (`launcher.h`, `stream_manager.h`)
- ✅ Support batching: group small allocations and kernel launches into single calls to
  reduce round-trip latency (`launcher.h` — `submitBatch()`)
- ✅ Handle fragmentation: track free blocks and compact or coalesce when fragmentation
  exceeds a threshold (`memory_pool.h`)
- ✅ Multi-GPU load balancing: distribute work across available devices based on current
  utilization; rebalance when a device becomes hot (`load_balancer.h`)
- ✅ Pre-allocation hints: allow callers to declare expected peak usage so the pool can
  reserve capacity upfront (`memory_manager.h` — `ReserveHint`/`ConsumeHint`)
- ✅ Define CPU fallback performance budgets: document and enforce maximum acceptable
  latency penalty when GPU is unavailable (`config.h` — `fallback_cpu_budget_ms`,
  `stream_manager.h` — `StreamConfig::cpu_budget_ms`)

### Security & Privacy

- ⬜ Sandbox kernel loading: load and JIT-compile GPU kernels in an isolated process or
  container; do not allow kernel blobs to execute arbitrary host code
  *(blocked on real CUDA/ROCm integration)*
- ✅ Validate and sign kernels: require all GPU kernel blobs to carry a trusted signature;
  reject unsigned or tampered kernels at load time (`kernel_validator.h`)
- ✅ Tenant-aware allocation domains: prevent one tenant from inspecting or overwriting
  another tenant's VRAM; zero memory on deallocation before returning to pool
  (`memory_pool.h` — `setZeroOnFree()`)
- ✅ Audit-log all GPU operations: record alloc, free, kernel launch, and fallback events
  with tenant/caller identity for compliance (`audit_log.h`)

### API/Config & DX

- ✅ High-level kernel/launch API: expose a typed, safe API for submitting GPU work
  rather than raw memory handles; hide driver details from callers (`launcher.h`,
  `gpu_module.h`)
- ✅ Dry-run and simulate: allow operators to test allocation plans and kernel configs
  without touching real GPU state (`config.h` — `simulateAllocation()`)
- ✅ Stats endpoints: surface current and historical VRAM usage, per-tenant breakdown,
  and edition-limit details via HTTP and CLI (`admin_api.h`)
- ✅ Config validation: fail fast at startup if GPU config is inconsistent (e.g.,
  VRAM limit exceeds physical device memory, unknown device specified)
- ✅ Expose edition-limit introspection: allow callers to query their current limit and
  remaining headroom without attempting an allocation (`memory_manager.h` —
  `GetTenantHeadroom()`, `GetEditionInfo()`)

### Delivery & Governance

- ✅ Add CI gates for GPU paths: compile and test GPU code paths (including CPU-only
  fallback) on every PR; block merge on alloc/deallocation test failures
  (`.github/workflows/gpu-ci.yml`)
- ✅ Simulate device loss in CI: use mock GPU driver or fault-injection harness to
  validate fallback behavior without real hardware
- ✅ Benchmark suite: track VRAM allocation latency, kernel launch throughput, and
  pool efficiency in the regression benchmark pipeline
  (`benchmarks/bench_gpu_module.cpp`)
- ✅ Runbooks: document on-call procedures for GPU OOM, device unavailability, tenant
  quota exhaustion, and kernel load failures (`docs/gpu_runbooks.md`)
- ✅ Governance gates: require GPU feature flags to be explicitly enabled per edition;
  deprecation notices before removing GPU API surface (`feature_flags.h`)

## Remaining Work (CUDA/ROCm Hardware Integration)

The following items require real GPU hardware or a CUDA/ROCm driver and are outside
the scope of the current bookkeeping-level implementation:

- Implement `cudaMalloc`/`hipMalloc` in `GPUMemoryManager` (replace counter logic)
- Wire CUDA streams into `GPUStreamManager` and `GPULauncher`
- Implement real CUDA/ROCm kernel loading and execution in `GPULauncher`
- Activate `cudaMemset` zero-on-free in `GPUMemoryPool::release()`
- Sandboxed kernel loading (process/container isolation for JIT compilation)
- Golden tests for kernel launch paths (need working kernels)
- GPU query acceleration kernels (vector ops, matrix multiply, parallel scan)
- GPU spatial acceleration kernels for `GpuBatchBackend::batchIntersects`
  (CUDA/OpenCL point-in-polygon and segment-intersection; see `TODO(gpu-spatial)`)

## Completed — Geo Spatial Backend

- ✅ `GpuBatchBackend` (`src/geo/gpu_backend_stub.cpp`) replaces the original
  all-zero stub with a real CPU-implemented geometry predicate and full
  circuit-breaker / fallback / observability integration
- ✅ `SpatialBatchInputs` extended with `geoms_a`/`geoms_b` geometry pair vectors
- ✅ Latency and throughput tracking (`batch_avg_latency_us`, `batch_max_latency_us`,
  `batch_pairs_processed`) exposed via `GpuBatchBackend::getStats()`
- ✅ `getGpuSpatialBackend()` factory function exposed in `spatial_backend.h`
- ✅ Comprehensive tests: geometry correctness, edge-case/degenerate inputs,
  count-vector mismatch, large-batch stress, concurrent access
  (`tests/test_geo_gpu_backend.cpp`)
- ✅ Geo backend operational runbook added (`docs/gpu_runbooks.md §6`)
