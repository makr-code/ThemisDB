### Context

This issue implements the roadmap item 'LoRA Adapter Hot-Loading at Inference Time' for the llm domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: LoRA Adapter Hot-Loading at Inference Time

### Goal

Deliver the scoped changes for LoRA Adapter Hot-Loading at Inference Time in src/llm/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### LoRA Adapter Hot-Loading at Inference Time
**Priority:** Medium
**Target Version:** v1.8.0

Extend `adapter_registry.cpp` and `AdapterLoadBalancer` (`adapter_load_balancer.cpp`) to support loading new LoRA adapters into a running `InferenceEngineEnhanced` without engine restart. Currently adapter sets are fixed at startup; adding a new fine-tuned adapter requires a rolling restart.

**Implementation Notes:**
- Add `AdapterRegistry::hotLoad(adapter_id, weights_path, metadata)` which loads adapter weights into a pre-allocated VRAM slot managed by `adaptive_vram_allocator.cpp`.
- Use a read-write lock on the adapter registry: hot-load acquires write lock briefly to register the new adapter; inference requests hold read locks and proceed without interruption.
- `AdapterLoadBalancer` must handle the case where `hot_load` is in progress and temporarily routes requests for the loading adapter to a fallback (base model or another adapter variant).
- Add admin API endpoint `POST /llm/adapters/{id}/load` that triggers hot-load; returns a `202 Accepted` with a job ID; status queryable via `GET /llm/adapters/{id}/load-status`.

**Performance Targets:**
- Hot-load of a 7B-parameter LoRA adapter (16-bit weights, rank 64) ≤ 5 s wall-clock from API call to adapter available for inference.
- Zero inference requests dropped during hot-load (all requests served via fallback or existing adapters).

---

### Acceptance Criteria

- [ ] Add `AdapterRegistry::hotLoad(adapter_id, weights_path, metadata)` which loads adapter weights into a pre-allocated VRAM slot managed by `adaptive_vram_allocator.cpp`.
- [ ] Use a read-write lock on the adapter registry: hot-load acquires write lock briefly to register the new adapter; inference requests hold read locks and proceed without interruption.
- [ ] `AdapterLoadBalancer` must handle the case where `hot_load` is in progress and temporarily routes requests for the loading adapter to a fallback (base model or another adapter variant).
- [ ] Add admin API endpoint `POST /llm/adapters/{id}/load` that triggers hot-load; returns a `202 Accepted` with a job ID; status queryable via `GET /llm/adapters/{id}/load-status`.
- [ ] Hot-load of a 7B-parameter LoRA adapter (16-bit weights, rank 64) ≤ 5 s wall-clock from API call to adapter available for inference.
- [ ] Zero inference requests dropped during hot-load (all requests served via fallback or existing adapters).

### Relationships

- Roadmap row: #182 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/llm/FUTURE_ENHANCEMENTS.md#lora-adapter-hot-loading-at-inference-time
- Source key: roadmap:182:llm:v1.8.0:lora-adapter-hot-loading-at-inference-time

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:182:llm:v1.8.0:lora-adapter-hot-loading-at-inference-time -->
<!-- roadmap-ref: row=182;module=llm;target=v1.8.0 -->
<!-- roadmap-detail: src/llm/FUTURE_ENHANCEMENTS.md#lora-adapter-hot-loading-at-inference-time -->
