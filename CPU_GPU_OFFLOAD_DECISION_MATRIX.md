# CPU/GPU Offload Decision Matrix for Layered Retrieval

**Status:** Active  
**Date:** 2026-07-06  
**Closes:** Issue #5465  
**Scope:** Query-time retrieval and tensor artifact maintenance across all four retrieval layers  
**Related:** `TARGET_ARCHITECTURE.md`, `HARDWARE_REQUIREMENTS.md`, `TENSOR_INTEGRATION_GUIDE.md`

---

## 1. Purpose

This document defines the implementation-facing decision model for selecting the execution target — CPU, GPU, or mixed CPU/GPU — at each layer of the ThemisDB retrieval pipeline.

It is intended to guide:
- query planner design
- benchmark scenario selection
- module boundary decisions
- future execution-policy code
- tensor artifact maintenance scheduling

GPU use in ThemisDB is **selective, not universal**. This matrix makes explicit when GPU acceleration is worth its overhead and when it must be avoided.

---

## 2. Definitions

| Term | Meaning |
|---|---|
| **CPU-only** | Work executes entirely on host CPU. No device transfer occurs. |
| **GPU-preferred** | GPU is the default target when available. CPU fallback applies on failure or absence. |
| **Mixed** | Parts of the workload run on GPU (e.g., FAISS ANN kernel), others on CPU (e.g., graph validation, policy checks). |
| **Forced CPU fallback** | Policy, correctness, or overhead conditions require dropping GPU even when hardware is present. |
| **Break-even point** | Minimum workload size at which GPU acceleration yields net latency benefit after accounting for host↔device transfer cost. |
| **Transfer overhead** | PCIe/NVLink cost to move data between host RAM and device VRAM. |
| **Zero-copy / mmap path** | Tensor artifacts accessed directly from memory-mapped files without explicit CPU buffer copy. May be incompatible with direct GPU DMA on most deployments. |
| **Advisory-only artifact** | A tensor summary used only as a hint; Graph Truth Layer provides the authoritative answer regardless. |

---

## 3. Primary Decision Criteria

The following signals inform the execution target decision. They are evaluated in priority order during query planning.

### 3.1 Correctness and Policy Constraints (Highest Priority)

These conditions **always** force CPU-only execution regardless of workload size:

| Condition | Enforcement |
|---|---|
| ACL / authorization check required on intermediate candidates | CPU-only |
| Provenance validation required before result use | CPU-only |
| Data-residency policy prohibits device-side copies | CPU-only |
| Exactness required (non-approximate answer mandated) | CPU-only |
| Audit-trail requirement on result selection path | CPU-only |
| Cross-shard federated query where final merge requires policy gates | CPU-only for merge step |
| Tensor artifact marked advisory-only (low quality, stale) | CPU-only; use Graph Truth Layer directly |

> **Rule:** Policy and correctness conditions override all size-based thresholds.

### 3.2 Artifact Freshness and Quality (High Priority)

Stale or low-quality tensor artifacts must not be fed into GPU kernels as primary retrieval results.

| Condition | Effect |
|---|---|
| Artifact age > `max_age_ms` | Mark advisory-only; trigger exact graph fallback |
| Delta lag > `max_delta_lag` (pending commits not yet applied to tensor) | Downgrade to advisory-only; CPU path preferred |
| Residual / reconstruction error > `epsilon_threshold` | Downgrade to advisory-only; force Graph Truth validation |
| Rank cap exceeded (TT rank too high to be a compact summary) | Rebuild required; GPU use for inference suspended until rebuild complete |
| Artifact flagged as partial (refit not yet converged) | Advisory-only; mark in query plan |

Default thresholds (configurable per tenant):

```yaml
artifact_freshness:
  max_age_ms: 300_000          # 5 minutes; configurable per collection
  max_delta_lag: 1_000         # number of uncommitted deltas before advisory downgrade
  epsilon_threshold: 0.05      # reconstruction error fraction above which advisory applies
  rank_cap: 256                # maximum TT rank before forced rebuild
```

### 3.3 Batch Size and Candidate-Set Thresholds

GPU acceleration is only worth its overhead above minimum viable workload sizes.

| Layer | CPU-only threshold | GPU-preferred threshold | Notes |
|---|---|---|---|
| ANN Frontdoor (HNSW, ScaNN, DiskANN) | < 512 candidates | ≥ 512 candidates AND available FAISS GPU | HNSW CPU-first below 512 |
| Tensor Mid-Layer scoring | < 256 candidates | ≥ 256 candidates | Matrix scoring kernel |
| Tensor artifact update (partial refit) | < 64 K elements | ≥ 64 K elements | SVD / TT iteration |
| Tensor snapshot rebuild | < 512 K elements | ≥ 512 K elements | Full rebuild SVD/TT |
| Graph Truth Layer | always CPU | GPU not applicable | Graph validation is CPU-first by design |
| LLM / LoRA inference | < 1 token/req (trivial) | batch ≥ 1 request with tokens ≥ context threshold | Context threshold: typically 512 tokens |
| LLM / LoRA adapter scoring | < 32 adapters | ≥ 32 adapters (batch scoring) | Adapter selection pass |

### 3.4 Host↔Device Transfer Overhead

Transfer overhead is evaluated against kernel compute time.

Break-even formula (approximate):

```
break_even_elements = transfer_bandwidth_bytes_per_sec
                      * kernel_latency_sec
                      / bytes_per_element
```

Practical guidance:

| Transfer Path | Approximate Bandwidth | Implication |
|---|---|---|
| PCIe Gen 4 x16 | ~26 GB/s | Small tensors (< 4 MB) rarely break even |
| NVLink 3.0 (multi-GPU) | ~600 GB/s | Break-even is much lower; GPU preferred for medium workloads |
| NUMA-local host RAM | ~50–100 GB/s | CPU often competitive for moderate workloads |

**Rule:** If the tensor or candidate set transferred to device is smaller than 4 MB AND the operation has no opportunity for kernel fusion with adjacent GPU work, execute on CPU.

### 3.5 Memory Map / Zero-Copy Availability

| Condition | Decision |
|---|---|
| Tensor artifact is mmap-backed and accessed sequentially | Prefer CPU SIMD; avoid device copy to preserve zero-copy semantics |
| Tensor artifact is pinned host memory (explicit page-lock) | Can DMA directly; GPU viable if batch is large enough |
| Tensor artifact resides on NVMe via DiskANN cold path | CPU-only; direct NVMe reads do not benefit from GPU staging |
| Artifact already resident in VRAM from prior operation | Prefer GPU (reuse amortizes transfer cost) |

### 3.6 Tensor Artifact Size and Reuse

| Artifact Size | Reuse Pattern | Decision |
|---|---|---|
| Small (< 4 MB) | Query-time one-shot | CPU-only |
| Small (< 4 MB) | Hot (reused > 10 queries/s) | Consider VRAM-pinning; GPU viable if available |
| Medium (4–256 MB) | Query-time | Mixed: scoring on GPU, policy gating on CPU |
| Large (> 256 MB) | Rebuild / snapshot | GPU-preferred if ≥ 512 K elements and batch rebuild |
| Very large (> 1 GB) | Distributed shard summary | GPU per-shard if per-shard batch is viable; merge on CPU |

### 3.7 Graph Traversal Structure Regularity

| Structure | Decision |
|---|---|
| Irregular sparse traversal (few edges, variable degree) | CPU-only; GPU poor at irregular memory access |
| Regular dense subgraph (matrix-like adjacency) | GPU viable for batch scoring; CPU still handles final validation |
| Multi-hop cross-shard traversal | CPU-only for path resolution; GPU may assist per-hop batch if dense |
| Policy-gated traversal (ACL per edge) | CPU-only |

---

## 4. Execution Category Definitions

### Category A: CPU-Only

**When:** Any of the following apply:
- correctness or policy conditions force CPU (see §3.1)
- artifact is advisory-only due to staleness or error (see §3.2)
- batch / candidate-set below minimum threshold (see §3.3)
- transfer overhead exceeds kernel savings (see §3.4)
- zero-copy / mmap path would be broken by device copy (see §3.5)
- graph traversal is irregular or policy-gated (see §3.7)
- cross-shard merge with policy gating

**Behavior:** Execute entirely on host CPU. SIMD intrinsics (AVX2 / AVX-512 where available) may be used within this category.

### Category B: GPU-Preferred

**When:** All of the following apply:
- no correctness or policy override
- artifact is fresh and meets quality thresholds
- batch / candidate-set above minimum threshold
- transfer overhead is justified
- data is not mmap zero-copy only
- operation is structurally amenable to GPU parallelism

**Behavior:** Issue work to GPU backend. CPU fallback activates automatically on device error, OOM, or timeout (exponential backoff then CPU).

### Category C: Mixed CPU/GPU

**When:**
- parts of the pipeline meet GPU-preferred criteria (e.g., ANN kernel, tensor scoring)
- other parts require CPU (e.g., graph validation, policy checks, merge)

**Behavior:** GPU used for compute-intensive kernels; CPU handles policy, provenance, and result assembly. Result buffers copied back from device before policy gates.

### Category D: Forced CPU Fallback

**When:** GPU-preferred was selected but:
- GPU out-of-memory error
- kernel execution timeout exceeded
- device error or hardware fault detected
- GPU temporarily unavailable (driver reset in progress)

**Behavior:** Immediately re-execute on CPU without surfacing a query error. Log fallback event with reason code.

---

## 5. Decision Matrix by Layer

### 5.1 ANN Frontdoor

| Condition | CPU-Only (A) | GPU-Preferred (B) | Mixed (C) |
|---|---|---|---|
| Dataset ≤ 1 M vectors, hot tier | ✅ (HNSW CPU) | — | — |
| Dataset > 1 M vectors, FAISS GPU available, batch ≥ 512 candidates | — | ✅ | — |
| ScaNN mid tier (1 M – 50 M vectors) | — | ✅ if FAISS GPU available | ✅ CPU ScaNN + GPU reranking |
| DiskANN cold tier (NVMe-backed) | ✅ | — | — |
| Cross-shard distributed fan-out | ✅ (per-shard merge) | Per-shard B if viable | ✅ (per-shard GPU, merge CPU) |
| ACL pre-filter required before candidate list | ✅ | — | — |
| Candidate count < 512 | ✅ | — | — |

**Planner signals:**
- `ann_candidate_count` — compared against 512 threshold
- `dataset_tier` (`hot` / `cold`)
- `faiss_gpu_available` (from acceleration backend registry)
- `acl_prefilter_required`

---

### 5.2 Tensor Mid-Layer

#### 5.2.1 Query-Time Scoring

| Condition | CPU-Only (A) | GPU-Preferred (B) | Mixed (C) |
|---|---|---|---|
| Candidate set < 256 | ✅ | — | — |
| Candidate set ≥ 256, artifact fresh, GPU available | — | ✅ | — |
| Artifact advisory-only (stale / high error) | ✅ (advisory hint only) | — | — |
| mmap zero-copy artifact, read-once | ✅ | — | — |
| Artifact already in VRAM | — | ✅ | — |
| Policy gate on intermediate tensor results | ✅ for gate | — | ✅ (GPU scoring, CPU gate) |

**Planner signals:**
- `tensor_candidate_count`
- `artifact_age_ms` vs `max_age_ms`
- `artifact_delta_lag` vs `max_delta_lag`
- `reconstruction_error` vs `epsilon_threshold`
- `artifact_in_vram` (VRAM residency hint from device manager)
- `mmap_backed` (storage layout flag)

#### 5.2.2 Tensor Artifact Maintenance (Dynamic Update Path)

This section covers the tensor update and rebuild pipeline, which runs on the commit path and background maintenance tasks — not during query time.

##### Delta Logging on Commit Path

| Condition | CPU-Only (A) | GPU-Preferred (B) |
|---|---|---|
| Single-record commit, delta < 1 K elements | ✅ | — |
| Batch commit, delta ≥ 64 K elements | — | ✅ |
| Commit under write-ahead lock (latency-critical) | ✅ | — |
| Delta log append only (no immediate refit required) | ✅ (append to delta log, defer GPU) | — |

**Rule:** Delta logging itself is always CPU-only. GPU is considered only for the subsequent refit step, not for writing the delta log entry.

##### Partial Refit / Bounded Incremental Update

| Condition | CPU-Only (A) | GPU-Preferred (B) | Notes |
|---|---|---|---|
| Delta window < 64 K elements | ✅ (CPU SIMD) | — | SIMD sufficient |
| Delta window ≥ 64 K elements, GPU available | — | ✅ | TT-iteration or SVD update |
| Artifact freshness constraint requires immediate completion | ✅ | — | GPU launch latency unacceptable |
| Residual exceeds `epsilon_threshold` after partial refit | ✅ (force full rebuild) | — | Refit failed; schedule full rebuild |
| Rank cap exceeded during refit | ✅ (suspend GPU use until rebuild) | — | Mark artifact advisory-only |

##### Snapshot-Based Full Tensor Rebuild

| Condition | CPU-Only (A) | GPU-Preferred (B) |
|---|---|---|
| Tensor element count < 512 K | ✅ | — |
| Tensor element count ≥ 512 K, GPU available | — | ✅ |
| Rebuild during peak query load (resource contention) | ✅ (defer to off-peak) | — |
| Multi-tenant isolation required during rebuild | ✅ (per-tenant CPU rebuild) | — |

##### Shard Summary Refresh

| Condition | CPU-Only (A) | GPU-Preferred (B) |
|---|---|---|
| Single shard, small summary (< 4 MB) | ✅ | — |
| Single shard, large summary (≥ 4 MB), GPU available | — | ✅ |
| Cross-shard summary aggregation (merge step) | ✅ | — |
| Summary refresh under strict freshness window (< 100 ms) | ✅ | — |

**Planner signals for maintenance:**
- `delta_element_count`
- `rebuild_element_count`
- `gpu_available` and `vram_free_bytes` (from device manager)
- `peak_load_active` (from query load monitor)
- `artifact_residual_error`
- `artifact_rank`
- `tenant_isolation_required`

---

### 5.3 Graph Truth Layer

**Default:** CPU-only always.

The Graph Truth Layer is the exactness and evidence layer. Its outputs are authoritative and policy-gated. GPU acceleration is not applicable except in the narrow experimental path noted below.

| Condition | CPU-Only (A) | GPU (experimental only) |
|---|---|---|
| Multi-hop graph traversal | ✅ | — |
| Provenance validation | ✅ | — |
| ACL / policy check per edge or node | ✅ | — |
| Exactness required (governance, legal, audit) | ✅ | — |
| Dense subgraph scoring (experimental, not production) | ✅ (default) | Experimental flag only |
| Cross-shard path merge | ✅ | — |

**Rationale:** Graph traversal in ThemisDB involves irregular memory access patterns, per-edge ACL evaluation, and provenance tracking. These are structurally incompatible with GPU parallelism at production quality. Any GPU-assisted graph path is explicitly advisory and must not override the CPU-side exact result.

> **Invariant:** Graph Truth Layer output is always CPU-computed. Tensor or ANN results may only *suggest* candidates to the graph layer; they cannot substitute for it.

---

### 5.4 LLM / LoRA Final Layer

| Condition | CPU-Only (A) | GPU-Preferred (B) | Mixed (C) |
|---|---|---|---|
| Inference request batch < 1 sequence | ✅ (trivial path) | — | — |
| Inference, GPU available, VRAM sufficient | — | ✅ | — |
| VRAM insufficient for model + context | ✅ (CPU inference or quantized) | — | — |
| Adapter (LoRA) loading: single adapter, < 32 MB | ✅ | — | — |
| Adapter scoring: ≥ 32 adapters batch | — | ✅ | — |
| Adapter selection with ACL / policy gate | ✅ for gate | — | ✅ (GPU for scoring, CPU for gate) |
| Prompt assembly and safety sanitization | ✅ | — | — |
| Quantized inference (INT8 / GGUF) on CPU | ✅ | — | — |

**Planner signals:**
- `vram_free_bytes` vs `model_vram_requirement_bytes`
- `adapter_count` vs 32 threshold
- `batch_sequence_count`
- `acl_required` on adapter selection

---

## 6. Transfer Overhead Break-Even Reference

Use this table to estimate whether GPU dispatch is likely beneficial. All values assume PCIe Gen 4 x16 (26 GB/s peak, ~16 GB/s effective for small transfers due to DMA setup overhead).

| Data Size | Transfer Time (estimate) | Minimum Kernel Savings to Break Even |
|---|---|---|
| 1 MB | ~0.06 ms | Kernel must save > 0.06 ms vs CPU |
| 4 MB | ~0.25 ms | Kernel must save > 0.25 ms vs CPU |
| 16 MB | ~1.0 ms | Kernel must save > 1.0 ms vs CPU |
| 64 MB | ~4.0 ms | Kernel must save > 4.0 ms vs CPU |
| 256 MB | ~16 ms | GPU only justified for very compute-heavy ops |
| > 256 MB | > 16 ms | Full rebuild or snapshot; GPU justified only for TT/SVD workloads |

**Practical rule:** For query-time operations, transfers above 64 MB per query are a strong signal that candidate sets are too large and should be pruned before device dispatch. For maintenance operations (rebuild, refit), transfers above 256 MB are acceptable but must run on background threads.

---

## 7. Policy Dimension Reference

The following dimensions must be tracked per query plan and per artifact by the planner and runtime:

| Dimension | Type | Default | Enforcement Point |
|---|---|---|---|
| `max_age_ms` | ms integer | 300 000 | Tensor mid-layer routing, before GPU dispatch |
| `max_delta_lag` | integer (commit count) | 1 000 | Tensor mid-layer routing, before GPU dispatch |
| `epsilon_threshold` | float [0, 1] | 0.05 | Tensor quality gate; triggers advisory-only flag |
| `rank_cap` | integer | 256 | Refit loop termination; triggers rebuild schedule |
| `advisory_only` | boolean | false | Set when freshness or quality thresholds exceeded |
| `exact_fallback_required` | boolean | false | Set by ACL/policy constraints; forces Graph Truth |
| `vram_free_bytes` | bytes | from device mgr | GPU dispatch gate per layer |
| `acl_prefilter_required` | boolean | false | Set by query security context |
| `mmap_backed` | boolean | from storage layout | Prevents device DMA of mmap-only artifacts |
| `tenant_isolation_required` | boolean | per tenant config | Forces per-tenant CPU rebuild paths |

---

## 8. Decision Flow Summary

```
Query arrives at planner
  │
  ├─ Check policy/ACL/exactness constraints (§3.1)
  │     → YES any constraint? → CPU-only (Category A)
  │
  ├─ Check artifact freshness/quality (§3.2)
  │     → Stale or advisory? → CPU-only or advisory hint; Graph Truth fallback
  │
  ├─ Determine layer-specific batch/candidate thresholds (§3.3)
  │     → Below threshold? → CPU-only
  │
  ├─ Estimate transfer overhead vs kernel savings (§3.4)
  │     → Transfer cost > kernel savings? → CPU-only
  │
  ├─ Check memory map / zero-copy constraints (§3.5)
  │     → mmap-only artifact, read-once? → CPU SIMD
  │
  ├─ Check GPU availability and VRAM headroom (§5.x planner signals)
  │     → GPU not available or VRAM insufficient? → CPU-only
  │
  └─ All checks pass? → GPU-Preferred (Category B)
                            │
                            └─ GPU error/timeout? → Forced CPU Fallback (Category D)
```

---

## 9. Benchmark Scenarios

The following scenarios must be covered by benchmark suites to validate these thresholds empirically.

| Scenario | Expected Category | Key Metric |
|---|---|---|
| HNSW query, 256 candidates, hot tier, no GPU | A | p95 query latency |
| FAISS GPU query, 512 candidates, GPU available | B | p95 query latency vs CPU baseline |
| Tensor scoring, 512 candidates, fresh artifact, GPU | B | scoring latency, GPU utilization |
| Tensor scoring, 64 candidates, advisory artifact | A | advisory downgrade rate |
| Graph Truth traversal, 3-hop, ACL per edge | A | traversal correctness, CPU time |
| LLM inference, batch 8, GPU, VRAM sufficient | B | tokens/sec |
| LLM inference, VRAM insufficient, CPU fallback | D | fallback trigger rate, latency |
| Tensor partial refit, delta 32 K elements | A | refit time |
| Tensor partial refit, delta 128 K elements, GPU | B | refit time, GPU vs CPU |
| Tensor full rebuild, 1 M elements, GPU | B | rebuild time |
| Tensor full rebuild, 200 K elements, CPU | A | rebuild time |
| Transfer overhead test, 4 MB tensor, GPU dispatch | B/A | break-even validation |
| Transfer overhead test, 64 MB tensor, GPU dispatch | B | transfer + kernel latency |
| mmap artifact, zero-copy, GPU dispatch attempt | A | correctness, no DMA of mmap |
| Cross-shard merge, 4 shards, policy gate | C | merge latency, correctness |

---

## 10. Implementation Notes for Planner and Runtime

### Planner Responsibilities
- Evaluate policy and freshness gates before emitting any GPU dispatch instruction.
- Annotate each pipeline stage with its resolved execution category (A / B / C / D).
- Propagate `advisory_only` and `exact_fallback_required` flags downstream.

### Runtime Responsibilities
- Maintain GPU availability and VRAM state via `DeviceManager` (60-second TTL cache).
- On GPU error or OOM, immediately re-route to CPU fallback (Category D) without query failure.
- Log execution category, fallback events, and policy gate triggers at debug level for observability.
- Do not issue device DMA for artifacts backed by memory-mapped files unless the artifact is page-locked pinned memory.

### Artifact Manifest Integration
- Tensor artifacts must carry freshness metadata (`created_at`, `delta_count`, `reconstruction_error`, `tt_rank`) in their `ArtifactManifest`.
- The planner reads manifest metadata before GPU dispatch to evaluate §3.2 conditions.
- Artifacts failing quality thresholds must be marked `advisory_only=true` in the manifest before being returned to the planner.

---

## 11. Known Limitations and Open Questions

| Item | Status |
|---|---|
| Exact break-even thresholds (PCIe Gen 5, NVLink) are not yet benchmarked | Open — pending hardware profiling |
| SIMD break-even for CPU path (AVX-512 vs baseline) is not yet formalized | Open |
| Per-collection `max_age_ms` override API is not yet implemented | Planned |
| GPU-assisted experimental graph scoring path is not production-ready | Explicitly advisory/experimental only |
| Multi-GPU load balancing for tensor rebuild is not yet specified | Deferred to hyperscaler edition work |
| Cross-shard GPU dispatch coordination is not yet designed | Deferred |

---

## 12. References

- `TARGET_ARCHITECTURE.md` — Four-layer retrieval model
- `HARDWARE_REQUIREMENTS.md` — Hardware profiles and GPU offload boundary notes
- `TENSOR_INTEGRATION_GUIDE.md` — Tensor artifact lifecycle and manifest
- `docs/TENSOR_MIDLAYER_DESIGN.md` — Tensor mid-layer abstractions
- `docs/de/acceleration/README.md` — Acceleration module, backend registry, fallback chain
- `docs/de/gpu/GPU_VRAM_QUICK_REFERENCE.md` — VRAM limits per edition
- `include/storage/tensor_router.h` — `TensorRoutingPolicy` and `TensorRouteDecision`
- `src/storage/tensor_router.cpp` — Routing decision implementation (§3 thresholds)
- `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md` — ANN/tensor boundary analysis
- `docs/adr/adr-e2-003-query-planner-routing-model.md` — ADR for query planner routing
