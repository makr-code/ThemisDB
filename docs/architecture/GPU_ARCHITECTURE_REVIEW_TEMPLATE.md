# GPU Architecture Review Template

**Purpose:** Structured template for human architecture reviews of GPU/VRAM changes in ThemisDB.  
**Issue:** [#5383 — GPU/VRAM Architekturdiagramm und Refactoring-Konzept dokumentieren](https://github.com/makr-code/ThemisDB/issues/5383)  
**Last Updated:** 2026-06-30

> Copy this template into a new file named `GPU_ARCHITECTURE_REVIEW_<YYYY-MM-DD>_<topic>.md` in `docs/architecture/` or as a GitHub PR review comment.

---

## Review Metadata

| Field | Value |
|-------|-------|
| **Review Date** | YYYY-MM-DD |
| **Reviewer(s)** | @username |
| **PR / Branch** | PR #XXXX / `branch-name` |
| **Related Issue** | #XXXX |
| **Scope** | (e.g., Memory Manager consolidation / Kernel dispatcher / Multi-GPU) |
| **Design Doc** | [GPU_VRAM_REFACTORING_DESIGN.md](../en/gpu/GPU_VRAM_REFACTORING_DESIGN.md) |
| **Architecture Diagram** | [GPU_VRAM_ORCHESTRATION_ARCHITECTURE.md](../en/gpu/GPU_VRAM_ORCHESTRATION_ARCHITECTURE.md) |

---

## 1. Scope Confirmation

- [ ] The change is aligned with the target architecture in [GPU_VRAM_ORCHESTRATION_ARCHITECTURE.md](../en/gpu/GPU_VRAM_ORCHESTRATION_ARCHITECTURE.md).
- [ ] The change scope is minimal and targeted (no unrelated refactoring bundled).
- [ ] The phase gate from [GPU_VRAM_REFACTORING_DESIGN.md](../en/gpu/GPU_VRAM_REFACTORING_DESIGN.md) applicable to this change has been completed.

**Notes:**

---

## 2. Memory Management

- [ ] All VRAM allocations flow through `themis::gpu::GPUMemoryManager::TryAllocateGPU()`.
- [ ] No subsystem maintains its own VRAM allocation ledger outside the canonical manager.
- [ ] Edition VRAM limits (`GetMaxGPUVRAMBytes()`) are enforced; no bypass paths exist.
- [ ] Per-tenant quotas are registered via `SetTenantQuota()` before the first allocation.
- [ ] `DeallocateGPU()` is always called on the matching handle (no leaks, RAII pattern used where possible).
- [ ] `VRAMSecureClear` is invoked on deallocation for security-sensitive buffers.

**Outstanding issues:**

---

## 3. Kernel / Shader Dispatch

- [ ] New kernel types are registered through `IKernelDispatcher` / `KernelDescriptor`, not via ad-hoc backend-specific calls.
- [ ] `KernelDispatchRouter` selects the backend; callers do not hard-code backend types.
- [ ] CUDA kernel launches use the CUDA Graph cache for repeated query shapes (no redundant graph captures).
- [ ] HLSL and Vulkan shaders are dispatched through their respective `IKernelDispatcher` implementations.
- [ ] `KernelValidator` is invoked before kernel execution; failure results in fail-closed behavior.
- [ ] CPU fallback path via `KernelFallbackDispatcher` is exercised by tests.

**Outstanding issues:**

---

## 4. Separation of Concerns (SoC)

- [ ] `gpu_module.cpp` (coordinator) does not contain allocation or kernel-launch logic directly.
- [ ] Stream/command-queue lifecycle is owned exclusively by `StreamManager`.
- [ ] Observability (profiling, memory pressure, audit logging) is isolated in `GPUObservability` / dedicated classes.
- [ ] OOM recovery logic is owned exclusively by `GPUSafeFail`.
- [ ] No class has more than two primary concerns from the [responsibility matrix](../en/gpu/GPU_VRAM_REFACTORING_DESIGN.md#6-separation-of-concerns--responsibility-matrix).

**Outstanding issues:**

---

## 5. Multi-GPU

- [ ] Multi-GPU paths are routed via `MultiGPUBackend` and `GPULoadBalancer`; no hard-coded device indices in callers.
- [ ] P2P transfers use `P2PTransfer` abstraction; no raw `cudaMemcpyPeer` in business logic.
- [ ] MIG slice assignments are managed by `MIGManager`; callers request a slice by quota, not by device ID.
- [ ] `TimeSliceScheduler` priority queues are respected; no scheduler bypass.

**Outstanding issues:**

---

## 6. Observability

- [ ] Kernel execution timings are emitted via `GPUProfiler` (not ad-hoc `cudaEvent*` calls in business logic).
- [ ] Memory pressure threshold breaches generate alerts to `GPUSafeFail`.
- [ ] VRAM tenant usage is reflected in `VRAMAuditLog` within 1 allocation cycle.
- [ ] All GPU error paths use `THEMIS_ERROR` / `THEMIS_WARN` macros (no bare `std::cerr` / `printf`).

**Outstanding issues:**

---

## 7. Security

- [ ] `VRAMSecureClear` is verified to zero-fill device memory before deallocation for sensitive data.
- [ ] No tenant can exceed its registered quota (verified by test).
- [ ] No tenant can read another tenant's VRAM (MIG / namespace isolation verified where applicable).
- [ ] Kernel integrity checks (`KernelValidator`) are not bypassable by callers.

**Outstanding issues:**

---

## 8. Performance

- [ ] Kernel dispatch overhead measured vs pre-change baseline; regression ≤ 2% on RTX-class GPU.
- [ ] No unnecessary host↔device copies introduced.
- [ ] CUDA Graph cache hit rate unaffected (same or better) for repeated query shapes.
- [ ] Memory pool slab fragmentation within acceptable bounds after change.

**Benchmark results:**

| Metric | Baseline | After Change | Delta |
|--------|---------|--------------|-------|
| Dispatch overhead (ns) | — | — | — |
| VRAM utilization (%) | — | — | — |
| Cache hit rate (%) | — | — | — |
| Throughput (queries/s) | — | — | — |

---

## 9. Documentation

- [ ] Doxygen comments updated for all modified public API methods.
- [ ] [GPU_VRAM_ORCHESTRATION_ARCHITECTURE.md](../en/gpu/GPU_VRAM_ORCHESTRATION_ARCHITECTURE.md) updated if architecture changed.
- [ ] [GPU_VRAM_REFACTORING_DESIGN.md](../en/gpu/GPU_VRAM_REFACTORING_DESIGN.md) phase checklist items marked `[x]` for completed work.
- [ ] No new stubs, simulations, or legacy fallback paths without explicit human approval and marking.

**Outstanding issues:**

---

## 10. Test Coverage

- [ ] Unit tests cover the new/modified GPU component in isolation.
- [ ] Integration tests verify end-to-end behavior under realistic VRAM pressure.
- [ ] OOM injection test confirms CPU fallback without data corruption.
- [ ] Edition VRAM limit enforcement tested for all affected subsystems (vector, LLM, LoRA).
- [ ] No existing GPU test removed or disabled.

**Outstanding issues:**

---

## 11. Legacy / Stub / Simulation Governance

- [ ] No new legacy compatibility paths introduced without explicit human approval.
- [ ] No stub / mock / simulation code in production paths without explicit human approval and `LEGACY PATH` / `STUB/SIMULATION NOTE` comment markers.
- [ ] All approved legacy/stub paths cite approver, reason, activation condition, and removal target.

**Outstanding issues:**

---

## 12. Overall Assessment

| Category | Status | Notes |
|----------|--------|-------|
| Memory Management | ✅ / ⚠️ / ❌ | |
| Kernel Dispatch | ✅ / ⚠️ / ❌ | |
| SoC | ✅ / ⚠️ / ❌ | |
| Multi-GPU | ✅ / ⚠️ / ❌ | |
| Observability | ✅ / ⚠️ / ❌ | |
| Security | ✅ / ⚠️ / ❌ | |
| Performance | ✅ / ⚠️ / ❌ | |
| Documentation | ✅ / ⚠️ / ❌ | |
| Test Coverage | ✅ / ⚠️ / ❌ | |
| Legacy Governance | ✅ / ⚠️ / ❌ | |

**Verdict:**

- [ ] **APPROVED** — all required checks pass; ready to merge.
- [ ] **APPROVED WITH CONDITIONS** — merge after addressing the outstanding issues listed above.
- [ ] **CHANGES REQUESTED** — blocking issues must be resolved and re-reviewed.

**Reviewer signature:** @username — YYYY-MM-DD

---

## Appendix: Quick Reference Links

- [GPU/VRAM Orchestration Architecture](../en/gpu/GPU_VRAM_ORCHESTRATION_ARCHITECTURE.md)
- [GPU/VRAM Refactoring Design](../en/gpu/GPU_VRAM_REFACTORING_DESIGN.md)
- [GPU Master Tracking](../en/gpu/GPU_MASTER_TRACKING.md)
- [GPU Vector Indexing Architecture](../en/gpu/GPU_VECTOR_INDEXING_ARCHITECTURE.md)
- [Multi-GPU Implementation Summary V2](../en/gpu/MULTI_GPU_IMPLEMENTATION_SUMMARY_V2.md)
- **Main Issue:** [#5383](https://github.com/makr-code/ThemisDB/issues/5383)
