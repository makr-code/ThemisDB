# GPU Module - Future Enhancements

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->

## IVRAMPolicy Hierarchy — Extension Points

The `IVRAMPolicy` abstract interface (`include/themis/gpu/ivram_policy.h`) is the
consolidation point for all GPU memory policy concerns.  Future enhancements can extend
the hierarchy by implementing the interface or composing it:

### Scope
- Per-backend policy specializations (CUDA-specific OOM retry, Vulkan memory type hints)
- Remote/distributed VRAM pool coordination (cluster-wide tenant quota enforcement)
- Priority-based policy: preemptable vs. pinned allocations
- Adaptive OOM policy: spill-to-CPU, block compaction, graceful degradation

### Design Constraints
- New policy implementations MUST honor the `canAllocate()` / `onAllocate()` / `onDeallocate()` contract.
- Policy objects MUST be thread-safe (internal synchronization expected by callers).
- Subsystem managers (`themis::llm::GPUMemoryManager`, `themis::llm::lora::VRAMAllocator`) MUST NOT bypass the canonical policy gate.

### Required Interfaces
| Extension Point | Mechanism |
|---|---|
| Per-backend specialization | Derive from `IVRAMPolicy`, register with subsystem factory |
| Cluster-wide quotas | Composable policy wrapper implementing `IVRAMPolicy` that calls remote quota service |
| Priority / preemption | Add `Priority` parameter to `onAllocate()`; canonical manager enforces eviction order |

### Implementation Notes
- `isGPUEnabled()` returning `false` fully disables the canonical gate (CPU-only builds unaffected).
- Tenant quota map on `GPUMemoryManager` is the authoritative quota store for all subsystems.
- New backends must register via the `BackendType` enum in `lora_framework/vram_allocator.h` and update `is_gpu_backend()` in `vram_allocator.cpp`.

### Test Strategy
- Unit tests for each `IVRAMPolicy` implementation in `tests/gpu/`.
- Integration tests validating delegation chain: LLM manager → canonical → policy.
- OOM simulation tests must verify rollback atomicity across all hierarchy levels.

### Performance Targets
- Policy gate overhead: < 1 µs per allocation (lock-free read path for the fast case).
- Tenant quota lookup: O(1) amortized (hash map).

### Security / Reliability
- Tenant quota enforcement must be fail-closed: a missing quota entry MUST NOT allow
  unbounded allocation (treat missing = unlimited OR denied, based on edition policy).
- Double-accounting bugs (allocate without matching deallocate) should be detectable via
  `usedBytes()` drift monitoring.

---

## Scope

- hardening and refinement of GPU resource governance and execution runtime behavior
- expansion of deterministic reliability under mixed backend and mixed capability workloads
- stronger benchmark-backed guardrails for GPU hot paths

## Design Constraints

- GPU contracts remain backward compatible within major release line.
- quota and policy checks remain explicit and enforced before execution.
- backend degradation behavior remains bounded and deterministic.
- advanced hardware features remain feature-gated and observable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| resource interfaces | deterministic quota, allocation, and pool semantics |
| backend interfaces | explicit capability-aware backend selection and execution |
| acceleration interfaces | bounded query/training acceleration with fallback behavior |
| operations interfaces | stable telemetry/profiling/admin/coordination behavior |

## Implementation Notes (v1.4.0)

### GPU Query Accelerator (In Progress)
- CUDA kernel launchers for common query types (filter, join, aggregation)
- HIP backend support for AMD GPUs
- Deterministic query result parity with CPU execution
- GPU memory management and stream orchestration
- Circuit-breaker fallback for unsupported operations

### GPU Vector Index Backend (In Progress)
- CUDA kernels for vector similarity: L2 (Euclidean), cosine distance, inner product
- HIP kernels as AMD GPU alternative
- Device memory optimization for large indices
- Batch search operations with RAFT/FAISS integration
- GPU/CPU result parity within tolerance (< 1e-3 relative error)

### Geospatial GPU Backend (In Progress)
- CUDA Haversine distance kernel (spherical earth, < 0.5% error)
- CUDA point-in-polygon kernel (ray-casting, batch support)
- HIP equivalents for AMD ROCm support
- OpenCL path for broader GPU compatibility
- Geometry validation with deterministic fallback

---

### Broader Hardening (v1.4.0+)
- tighten parity and edge handling across CUDA/ROCm/Vulkan and fallback modes.
- standardize diagnostics for quota denials, capability mismatch, and runtime degradation.
- expand resilience tests for prolonged acceleration load and mixed tenant pressure.
- broaden benchmark depth for topology, partitioning, transfer, and high-concurrency paths.

## Test Strategy

- unit and integration suites for allocation, backend, streams, launcher, and fallback surfaces.
- regressions for feature-gated advanced paths (P2P/topology/partition) and degraded-capability scenarios.
- deterministic stress runs for high-volume multi-tenant acceleration workloads.
- release-profile benchmark runs for mapped GPU targets.

## Performance Targets

- allocation, policy checks, and control-plane operations remain inside regression budgets.
- backend and acceleration paths remain stable at p95/p99 envelopes.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- maintain strict policy and quota gating before acceleration execution.
- preserve explicit fallback signaling for backend or capability failures.
- enforce bounded behavior for advanced hardware features under degraded conditions.
- keep diagnostics actionable for production GPU incidents.