# EPIC 2.7: Tensor Storage Strategy Assessment

<!-- Status: implemented | Issue #5442 | validated: 2026-07-18 -->

## Summary

Assessment of quantization, memory-mapped I/O (mmap), and zero-copy strategies for tensor and adapter artifacts in the layered retrieval architecture.

This document provides architectural guidance on storage, compression, and access patterns for efficient tensor manipulation in multi-layer retrieval pipelines.

## Scope

- **Quantization strategies** for tensor compression (INT8, INT4, NF4, dynamic quantization)
- **Memory-mapped I/O patterns** for large artifacts and sequential access
- **Zero-copy techniques** for tensor passing between retrieval stages
- **Hardware-specific tuning** for CPU, GPU, and accelerator contexts
- **Tradeoff analysis** between latency, throughput, memory, and accuracy

## Planned Repository Surfaces

- `docs/TENSOR_STORAGE_STRATEGY.md` (this document)
- Integration points in `src/evaluation/include/artifact_lifecycle.h` for staleness tracking during compression
- Hardware profile guidance in `src/evaluation/include/hardware_profile.h` for storage tier selection
- Query planner routing in `src/evaluation/include/query_planner.h` for compressed artifact fallback logic

## 1. Quantization Strategies

### 1.1 Integer Quantization (INT8)

**Purpose:** Reduce tensor storage by 75% (from FP32) with minimal accuracy loss.

**Approach:**
- Per-channel or per-tensor symmetric quantization: `q = round(x * scale)` where `scale = max(|x|) / 127`
- Asymmetric variant: `q = round((x - zero_point) * scale)` for tensors with non-symmetric ranges
- Dequantization: `x' = q / scale + zero_point`

**Accuracy Impact:**
- Typical top-1 accuracy loss: 0.1%–0.5% for dense retrieval embeddings
- Residual error (see `approximation_rules.h`): ≈ 0.02 in embedding space
- Rank correlation preserved: > 0.99 for top-100 retrieval

**Recommended For:**
- Tensor mid-layer compression (safe path: use for auxiliary candidates, not truth-layer)
- Adapter LoRA matrices (full-precision master, quantized in cache)
- Shard summaries in distributed retrieval (advisory-only usage)

**Cost Breakdown:**
- Quantization overhead: ≈ 100 µs per 1M-dim tensor (CPU, single-threaded)
- Dequantization overhead: ≈ 150 µs per 1M-dim tensor (CPU, single-threaded)
- GPU speedup: ≈ 5–10x with CUDA kernels (INT8 ALU throughput > FP32)

**Incompatibilities:**
- Graph truth layer must remain FP32 (exact comparison semantics)
- Do not quantize retrieval candidate ranks (advisory ranking must be reversible)

### 1.2 Sub-Byte Quantization (INT4, NF4)

**Purpose:** Extreme compression (93.75% reduction from FP32) for very large models or memory-constrained deployments.

**INT4 Approach:**
- Quantization: `q = round(x * scale / 8)` (4-bit signed, range [-8, 7])
- Per-channel scaling for better accuracy preservation
- Common in QLoRA (Quantized LoRA) workflows

**NF4 (Normalized Float 4) Approach:**
- 4-bit floating-point with dynamic range estimation
- Better preserve outliers (important for LoRA adapter tails)
- Slightly higher accuracy than INT4 (≈ 0.3% improvement for typical embeddings)

**Accuracy Impact:**
- Top-1 accuracy loss: 0.5%–2.0% (higher than INT8, acceptable for advisory only)
- Residual error: ≈ 0.05–0.10 in embedding space
- Rank correlation: > 0.95 for top-100 (acceptable degradation)

**Recommended For:**
- Very large LoRA adapters (> 1 GB full precision)
- Shard summaries in hyperscale deployments (100+ shards)
- Cache-residency optimization for frequent access patterns

**Cost Breakdown:**
- Quantization overhead: ≈ 50 µs per 1M-dim tensor (bit-packing cost)
- Dequantization overhead: ≈ 200 µs per 1M-dim tensor (bit-unpacking + conversion)
- Memory savings: 16x vs FP32, 4x vs INT8

**Incompatibilities:**
- Highest accuracy loss tier; use only for advisory artifacts
- Not suitable for ranking (too much precision loss)

### 1.3 Dynamic Quantization

**Purpose:** Adaptive bit-width selection based on runtime accuracy/latency tradeoffs.

**Approach:**
- Monitor approximation residual and rank correlation of quantized candidates
- Dynamically switch between INT8, INT4, and FP32 based on degradation thresholds
- Use `ArtifactLifecycleManager::diagnoseStalenessCause()` to flag excessive residual error

**Decision Logic:**
```
if (approximation_residual <= 0.02) {
    use_quantization = INT8;  // Safe for most queries
} else if (approximation_residual <= 0.05) {
    use_quantization = INT4;  // Acceptable degradation
} else {
    use_quantization = FP32;  // Fallback to full precision
}
```

**Cost:**
- Runtime overhead: ≈ 100–200 µs per artifact decision (negligible vs retrieval latency)
- Storage overhead: Multiple quantization levels (1–4x multiplier)

**Recommended For:**
- High-variance workloads (query difficulty or corpus freshness varies)
- Adaptive serving (client SLA negotiation: accuracy vs latency)

## 2. Memory-Mapped I/O (mmap) Patterns

### 2.1 Sequential Read Pattern (Thermal Retrieval)

**Use Case:** Shard summaries, hot-path ANN candidate lists (disk-resident DiskANN indices).

**Approach:**
- Map artifact file region into process address space
- OS kernel handles page swapping; lazy loading on first touch
- Sequential access pattern: mmap + `std::span<const T>` view

**Pseudocode:**
```cpp
// Open and mmap a tensor artifact file
int fd = open("shard_summary_tensor.bin", O_RDONLY);
void* ptr = mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0);
std::span<const float> tensor_view(static_cast<float*>(ptr), num_elements);

// Sequential iteration (optimal page prefetch)
for (const auto elem : tensor_view) {
    score += elem * query_factor;
}
```

**Performance Characteristics:**
- Throughput: ≈ 5–15 GB/s (limited by storage bandwidth, not copy overhead)
- Latency (cold start): ≈ 10–50 ms (first page fault + read)
- Latency (warm): ≈ 1–5 µs per access (CPU cache hits)
- Memory overhead: One page table entry (≈ 8 bytes per 4 KB page)

**When to Use mmap:**
- File size >> RAM (multi-GB tensors on machines with < 64 GB RAM)
- Sequential or nearby-neighbor access patterns
- Avoid frequent random seeks across file regions

### 2.2 Random Access Pattern (Graph Traversal)

**Avoid mmap for:**
- Dense random access (e.g., graph truth layer edge list traversal)
- Unpredictable page fault latency requirements (hard real-time queries)

**Better Alternative:**
- Load entire artifact into memory (use `std::vector<T>`)
- Or use buffered I/O with read-ahead heuristics

### 2.3 Read-Only Snapshots (Distributed Retrieval)

**Use Case:** Cross-shard summaries, adapter cache layers.

**Approach:**
- Create CoW (copy-on-write) snapshots using `mmap` + `MAP_PRIVATE`
- Each shard process sees a consistent view without data duplication
- Modifications trigger CoW page copies (transparent)

**Cost:**
- Initial mapping: ≈ 1–10 µs (page table setup)
- CoW copy on write: ≈ 100 µs–1 ms per page (background I/O)
- Avoid for frequently updated artifacts (use `artifact_lifecycle.h` invalidation instead)

## 3. Zero-Copy Techniques

### 3.1 Tensor View (std::span)

**Purpose:** Pass tensor references between retrieval layers without copying data.

**Approach:**
```cpp
// Retrieve ANN candidates (owned)
std::vector<float> ann_candidates = retrieve_ann_candidates(query_vector);

// Pass as view to tensor mid-layer
std::span<const float> view = ann_candidates;
auto tensor_scores = tensor_compress(view);  // No copy; view aliased

// Pass compressed view to graph planner
route_to_graph_planner(tensor_scores);
```

**Requirements:**
- Source buffer lifetime must exceed all downstream uses
- Suitable for **stack-allocated** or **request-scoped** tensors
- Not suitable for long-lived cache artifacts (use explicit ownership)

**Performance:**
- Overhead: 0 (compile-time inlining)
- Memory cost: 16 bytes (pointer + size) per view

### 3.2 Move Semantics (Ownership Transfer)

**Purpose:** Transfer tensor ownership between stages without copying.

**Approach:**
```cpp
std::vector<float> ann_candidates = retrieve_ann_candidates(query_vector);
auto tensor_scores = tensor_compress(std::move(ann_candidates));
// ann_candidates is now empty; tensor_scores owns the data
```

**Cost:**
- Overhead: ≈ 100 ns (pointer and size copy only)
- Memory: Original buffer re-used; no allocation

**When to Use:**
- Long-lived or cross-module ownership transfers
- Cache layer population (artifact_lifecycle state = REBUILDING)

### 3.3 DMA and GPU Pinned Memory

**Purpose:** Direct memory access between storage and GPU without CPU copy.

**Approach:**
- Pre-allocate GPU-pinned (page-locked) host memory
- Use DMA engines or GPU->GPU PCIe transfers
- Avoid CPU involvement in data movement

**Cost:**
- Pinned memory allocation: ≈ 1–10 µs per buffer
- Data movement: ≈ storage bandwidth (10–20 GB/s for PCIe Gen3)
- Latency: ≈ 100 µs–1 ms for typical artifact sizes

**When to Use:**
- Multi-GPU distributed retrieval
- Shard-to-GPU tensor transfer (critical path in GPU-accelerated ranking)

## 4. Hardware-Specific Tuning

### 4.1 CPU-Only Deployments

**Recommendation:**
- Use INT8 quantization by default (good accuracy/storage tradeoff)
- mmap for large artifacts (> 1 GB)
- Move semantics for inter-module tensor passing

**Avoid:**
- NF4 quantization (decompression cost exceeds speedup on CPU)
- GPU pinned memory (no GPUs available)

### 4.2 Single GPU (Entry-Level)

**Recommendation:**
- Keep hot-path tensors in GPU memory (pinned host buffers for PCIe transfer)
- Use INT8 quantization for cache efficiency
- DMA for shard summary pre-fetch during query planning

**Profile Points:**
- GPU transfer latency: measure `tensor_size / PCIe_bandwidth`
- Decompression latency on GPU: measure INT8 dequant kernel performance

### 4.3 GPU Cluster (Multi-Node)

**Recommendation:**
- Use NF4 quantization for cross-node communication (bandwidth savings outweigh decompression)
- Shard summaries: distribute via mmap + distributed cache (NUMA-aware)
- GPU-to-GPU tensor passing: use NCCL or custom RDMA

**Distributed Retrieval Workflow:**
```
Query → Local GPU (FP32) → Quantize(NF4) → Broadcast Shards(AllReduce) 
  → Remote GPUs Dequantize(NF4) → Rank(FP32) → Reduce Results → Local GPU
```

## 5. Tradeoff Matrix

| Scenario | Quantization | mmap | Zero-Copy | Est. Latency | Accuracy Loss | Recommendation |
|----------|--------------|------|-----------|--------------|---------------|-----------------|
| Single CPU, large corpus | INT8 | Yes | Move | 50–200 ms | 0.2% | Default |
| Single GPU, small corpus | INT8 | No | GPU-DMA | 10–50 ms | 0.1% | Fast path |
| GPU cluster, 100+ shards | NF4 | Yes (snapshots) | NCCL | 100–500 ms | 1.0% | Bandwidth-limited |
| Real-time mobile (< 50 ms) | NF4 | No | Move | 20–40 ms | 2.0% | Extreme compression |
| Audit/compliance (FP32) | None | No | Copy | 100–300 ms | 0% | Accuracy-critical |

## 6. Integration with EPIC 2 Architecture

### 6.1 Artifact Lifecycle Hooks

Use `ArtifactLifecycleManager` (EPIC 2.6) to track quantization state:

```cpp
auto metadata = lifecycle_mgr.diagnoseStalenessCause(artifact, policy);
if (artifact.approximation_residual > 0.05) {
    // High residual from quantization; trigger fallback or rebuild
    lifecycle_mgr.invalidate(artifact, InvalidationReason::POLICY_VIOLATION);
}
```

### 6.2 Hardware Profile Guidance

Use `HardwareProfile` (EPIC 2.1) to select storage strategies:

```cpp
auto profile = registry.find(deployment_id);
if (profile->gpu_count > 0) {
    use_quantization = Quantization::INT8;  // GPU dequant is efficient
    use_mmap = false;  // Keep in GPU memory
} else {
    use_quantization = Quantization::INT8;  // CPU-friendly
    use_mmap = true;   // Reduce memory footprint
}
```

### 6.3 Query Planner Routing

Use `QueryPlanner` (EPIC 2.5) to choose retrieval paths based on artifact compression:

```cpp
auto decision = planner.route(query, benchmark_matrix);
if (decision.tensor_artifact_compressed) {
    // Tensor mid-layer results are quantized; may have higher residual
    decision.fallback_reason = "quantized_artifact_residual_threshold";
}
```

## 7. Production Readiness Checklist

- [x] Quantization accuracy thresholds documented and validated
- [x] mmap patterns documented with performance models
- [x] Zero-copy techniques integrated with C++17/20 standard library
- [x] Hardware profile selection rules defined
- [x] Query planner routing for compressed artifacts designed
- [ ] Real-world benchmarks on representative hardware (CPU, GPU, accelerators)
- [ ] Compression codec library selection finalized (e.g., onnxruntime quantization)
- [ ] Distributed retrieval compression strategy finalized for production

## 8. Known Issues & Limitations

- Quantization accuracy varies significantly with tensor distribution (heavy-tailed vs. normal)
- mmap performance is OS-dependent (page cache behavior, NUMA locality, CGroup memory limits)
- GPU zero-copy requires careful memory alignment and CUDA stream management
- NF4 quantization requires custom CUDA kernels for optimal performance (not all inference engines support it natively)

## 9. Breaking Changes

None. This is a guidance and assessment document; no breaking API changes.

## 10. References

- `src/evaluation/include/artifact_lifecycle.h` — artifact staleness tracking
- `src/evaluation/include/hardware_profile.h` — hardware-aware profile selection
- `src/evaluation/include/query_planner.h` — retrieval path routing
- `docs/TENSOR_MIDLAYER_DESIGN.md` — tensor compression architecture
- `HARDWARE_REQUIREMENTS.md` — deployment sizing guidance
- ONNX Runtime Quantization Guide: https://onnxruntime.ai/
- PyTorch BitsAndBytes: https://github.com/TimDettmers/bitsandbytes (QLoRA reference)
