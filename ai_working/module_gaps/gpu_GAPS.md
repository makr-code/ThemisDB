# gpu Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: gpu
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 83
- Actionable Findings (Critical + High): 35
- Affected Files: 29

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 13 |
| High | 22 |
| Medium | 48 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| raii | 57 |
| performance_patterns | 47 |
| gpu_memory_safety | 31 |
| container | 22 |
| reliability | 22 |
| platform | 12 |
| exception_safety | 8 |
| performance | 8 |
| audit_logging | 5 |
| concurrency | 4 |
| determinism | 3 |
| observability | 3 |
| memory | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/gpu/query_accelerator.cpp | 32 | 8 | 14 | 10 | 0 |
| src/gpu/unified_memory.cpp | 6 | 4 | 2 | 0 | 0 |
| src/gpu/cluster_topology.cpp | 4 | 0 | 0 | 4 | 0 |
| src/gpu/device_discovery.cpp | 4 | 0 | 0 | 4 | 0 |
| src/gpu/metrics.cpp | 4 | 0 | 0 | 4 | 0 |
| src/gpu/cluster_coordinator.cpp | 3 | 0 | 0 | 3 | 0 |
| src/gpu/mig_manager.cpp | 3 | 0 | 0 | 3 | 0 |
| src/gpu/training_loop.cpp | 3 | 0 | 2 | 1 | 0 |
| src/gpu/audit_log.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/gpu_memory_manager_edition.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/load_balancer.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/memory_pool.cpp | 2 | 1 | 1 | 0 | 0 |
| src/gpu/policy.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/rocm_backend.cpp | 2 | 0 | 1 | 1 | 0 |
| src/gpu/stream_manager.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/admin_api.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/alerts.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/feature_flags.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/gpu_module.cpp | 1 | 0 | 1 | 0 | 0 |
| src/gpu/kernel_validator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/launcher.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/p2p_transfer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/gpu/tensor_buffer.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/time_slice_scheduler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/vulkan_backend.cpp | 1 | 0 | 0 | 1 | 0 |
| src/gpu/config.cpp | 0 | 0 | 0 | 0 | 0 |
| src/gpu/profiler.cpp | 0 | 0 | 0 | 0 | 0 |
| src/gpu/safe_fail.cpp | 0 | 0 | 0 | 0 | 0 |
| src/gpu/wasm_kernel_sandbox.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/gpu/query_accelerator.cpp
Total findings: 32

- Line 787: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_a
  Context: __half *d_a         = nullptr;
  Confidence: band=very_high; score=0.99
- Line 788: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_b
  Context: __half *d_b         = nullptr;
  Confidence: band=very_high; score=0.99
- Line 825: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_a
  Context: __nv_bfloat16 *d_a  = nullptr;
  Confidence: band=very_high; score=0.99
- Line 826: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_b
  Context: __nv_bfloat16 *d_b  = nullptr;
  Confidence: band=very_high; score=0.99
- Line 827: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_c
  Context: float *d_c          = nullptr; // accumulate in FP32
  Confidence: band=very_high; score=0.99
- Line 916: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_a
  Context: hipblasHalf *d_a    = nullptr;
  Confidence: band=very_high; score=0.99
- Line 917: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_b
  Context: hipblasHalf *d_b    = nullptr;
  Confidence: band=very_high; score=0.99
- Line 1100: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // cudaMalloc failure or cuVS error — fall through to CPU path.
  Confidence: band=very_high; score=0.99
- Line 742: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // FP16: cublasGemmEx / hipblasGemmEx with CUDA_R_16F / HIPBLAS_R_16F inputs
  Confidence: band=very_high; score=0.9
- Line 745: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // BF16: cublasGemmEx with CUDA_R_16BF inputs + CUDA_R_32F output (CUDA only;
  Confidence: band=very_high; score=0.9
- Line 763: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(float)) == cudaSuccess
  Confidence: band=very_high; score=0.9
- Line 777: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_b);
  Confidence: band=very_high; score=0.9
- Line 781: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Use CUDA_R_16F inputs with CUDA_R_32F output + FP32 compute
  Confidence: band=very_high; score=0.9
- Line 791: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(__half)) == cudaSuccess
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: && cudaMalloc(&d_b, n * sizeof(__half)) == cudaSuccess
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_c);
  Confidence: band=very_high; score=0.9
- Line 829: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(__nv_bfloat16)) == cudaSuccess
  Confidence: band=very_high; score=0.9
- Line 830: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: && cudaMalloc(&d_b, n * sizeof(__nv_bfloat16)) == cudaSuccess
  Confidence: band=very_high; score=0.9
- Line 854: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_c);
  Confidence: band=very_high; score=0.9
- Line 910: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Use HIPBLAS_R_16F inputs + HIPBLAS_R_32F output to avoid
  Confidence: band=very_high; score=0.9
- Line 1020: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs --------------------------------------------------------
  Confidence: band=very_high; score=0.9
- Line 1100: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // cudaMalloc failure or cuVS error — fall through to CPU path.
  Confidence: band=very_high; score=0.9
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(rows[static_cast<size_t>(i)]);
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pairs.emplace_back((*build_side)[bi], (*probe_side)[pi]);
  Confidence: band=high; score=0.74
- Line 655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pairs.emplace_back((*build_side)[bi], (*probe_side)[pi]);
  Confidence: band=high; score=0.74
- Line 698: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pairs.emplace_back(*it->second, row);
  Confidence: band=high; score=0.74
- Line 698: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pairs.emplace_back(*it->second, row);
  Confidence: band=high; score=0.74
- Line 698: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pairs.emplace_back(*it->second, row);
  Confidence: band=high; score=0.74
- Line 1155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: heap.emplace_back(dist, vi);
  Confidence: band=high; score=0.74
- Line 1155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: heap.emplace_back(dist, vi);
  Confidence: band=high; score=0.74

### src/gpu/unified_memory.cpp
Total findings: 6

- Line 13: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: * Real cudaMallocManaged / hipMallocManaged calls are gated behind
  Confidence: band=very_high; score=0.99
- Line 80: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: if (cudaMallocManaged(&ptr, bytes, cudaMemAttachGlobal) != cudaSuccess) {
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: ptr
  Context: std::free(ptr);
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: ptr
  Context: std::free(ptr);
  Confidence: band=very_high; score=0.99
- Line 13: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: * Real cudaMallocManaged / hipMallocManaged calls are gated behind
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void *GPUUnifiedMemoryAllocator::allocate(size_t bytes, const std::string &tag, const std::string &tenant_id) {
  Confidence: band=very_high; score=0.9

### src/gpu/cluster_topology.cpp
Total findings: 4

- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.links.push_back(lnk);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: topo.links.push_back(lnk);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(node);
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(n.node_id);
  Confidence: band=high; score=0.74

### src/gpu/device_discovery.cpp
Total findings: 4

- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74

### src/gpu/metrics.cpp
Total findings: 4

- Line 26: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::string GPUMetrics::buildKey(const std::string &name, const std::unordered_map<std::string, std::string> &labels) {
  Confidence: band=medium; score=0.66
- Line 33: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ',';
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/gpu/cluster_coordinator.cpp
Total findings: 3

- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: local.device_indices.push_back(d.index);
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(n);
  Confidence: band=high; score=0.74

### src/gpu/mig_manager.cpp
Total findings: 3

- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/gpu/training_loop.cpp
Total findings: 3

- Line 112: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: es.min_loss  = (min_loss == std::numeric_limits<double>::max()) ? 0.0 : min_loss;
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: es.max_loss  = (max_loss == std::numeric_limits<double>::lowest()) ? 0.0 : max_loss;
  Confidence: band=very_high; score=0.9
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history_.push_back(rec);
  Confidence: band=high; score=0.74

### src/gpu/audit_log.cpp
Total findings: 2

- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ring_[i]);
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ring_[(head_ + i) % capacity_]);
  Confidence: band=high; score=0.74

### src/gpu/gpu_memory_manager_edition.cpp
Total findings: 2

- Line 268: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: error += "GB) exceeds edition limit (";
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ts));
  Confidence: band=high; score=0.74

### src/gpu/load_balancer.cpp
Total findings: 2

- Line 41: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devices_.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(dl));
  Confidence: band=high; score=0.74

### src/gpu/memory_pool.cpp
Total findings: 2

- Line 208: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // caller must have performed a cudaMalloc / hipMalloc of at
  Confidence: band=very_high; score=0.99
- Line 208: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // caller must have performed a cudaMalloc / hipMalloc of at
  Confidence: band=very_high; score=0.9

### src/gpu/policy.cpp
Total findings: 2

- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<Capability>(v));
  Confidence: band=high; score=0.74

### src/gpu/rocm_backend.cpp
Total findings: 2

- Line 218: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: ROCmBackend::Result ROCmBackend::deallocate(AllocationRecord& rec) {
  Confidence: band=very_high; score=0.9
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/gpu/stream_manager.cpp
Total findings: 2

- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second.stats);
  Confidence: band=high; score=0.74

### src/gpu/admin_api.cpp
Total findings: 1

- Line 32: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74

### src/gpu/alerts.cpp
Total findings: 1

- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/gpu/feature_flags.cpp
Total findings: 1

- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s);
  Confidence: band=high; score=0.74

### src/gpu/gpu_module.cpp
Total findings: 1

- Line 175: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void GPUModule::deallocate(const std::string &tenant_id, uint64_t bytes) {
  Confidence: band=very_high; score=0.9

### src/gpu/kernel_validator.cpp
Total findings: 1

- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/gpu/launcher.cpp
Total findings: 1

- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(executeOne(std::move(item)));
  Confidence: band=high; score=0.74

### src/gpu/p2p_transfer.cpp
Total findings: 1

- Line 318: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: result.error_message = "cudaMemcpyPeer failed";
  Confidence: band=very_high; score=0.9

### src/gpu/tensor_buffer.cpp
Total findings: 1

- Line 309: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shape.dims.push_back(read32(p));
  Confidence: band=high; score=0.74

### src/gpu/time_slice_scheduler.cpp
Total findings: 1

- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s);
  Confidence: band=high; score=0.74

### src/gpu/vulkan_backend.cpp
Total findings: 1

- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/gpu/config.cpp
Total findings: 0


### src/gpu/profiler.cpp
Total findings: 0


### src/gpu/safe_fail.cpp
Total findings: 0


### src/gpu/wasm_kernel_sandbox.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
