# gpu Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: gpu
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 218
- Actionable Findings (Critical + High): 132
- Affected Files: 29

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 17 |
| High | 115 |
| Medium | 86 |
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
| src/gpu/query_accelerator.cpp | 46 | 8 | 21 | 17 | 0 |
| src/gpu/gpu_memory_manager_edition.cpp | 44 | 0 | 40 | 4 | 0 |
| src/gpu/unified_memory.cpp | 12 | 5 | 5 | 2 | 0 |
| src/gpu/admin_api.cpp | 11 | 0 | 5 | 6 | 0 |
| src/gpu/memory_pool.cpp | 9 | 1 | 8 | 0 | 0 |
| src/gpu/rocm_backend.cpp | 9 | 0 | 8 | 1 | 0 |
| src/gpu/time_slice_scheduler.cpp | 8 | 3 | 4 | 1 | 0 |
| src/gpu/metrics.cpp | 6 | 0 | 1 | 5 | 0 |
| src/gpu/profiler.cpp | 6 | 0 | 0 | 6 | 0 |
| src/gpu/alerts.cpp | 5 | 0 | 2 | 3 | 0 |
| src/gpu/cluster_coordinator.cpp | 5 | 0 | 1 | 4 | 0 |
| src/gpu/cluster_topology.cpp | 5 | 0 | 1 | 4 | 0 |
| src/gpu/device_discovery.cpp | 5 | 0 | 1 | 4 | 0 |
| src/gpu/feature_flags.cpp | 5 | 0 | 2 | 3 | 0 |
| src/gpu/gpu_module.cpp | 5 | 0 | 5 | 0 | 0 |
| src/gpu/stream_manager.cpp | 5 | 0 | 2 | 3 | 0 |
| src/gpu/mig_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/gpu/p2p_transfer.cpp | 4 | 0 | 4 | 0 | 0 |
| src/gpu/launcher.cpp | 3 | 0 | 0 | 3 | 0 |
| src/gpu/load_balancer.cpp | 3 | 0 | 1 | 2 | 0 |
| src/gpu/safe_fail.cpp | 3 | 0 | 0 | 3 | 0 |
| src/gpu/training_loop.cpp | 3 | 0 | 2 | 1 | 0 |
| src/gpu/audit_log.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/policy.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/tensor_buffer.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/vulkan_backend.cpp | 2 | 0 | 1 | 1 | 0 |
| src/gpu/wasm_kernel_sandbox.cpp | 2 | 0 | 0 | 2 | 0 |
| src/gpu/config.cpp | 1 | 0 | 1 | 0 | 0 |
| src/gpu/kernel_validator.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/gpu/query_accelerator.cpp
Total findings: 46

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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4137 feat(gpu): replace CPU fall... (2026-03-12) | #3561 docs(gpu): reality-
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
- Line 829: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(__nv_bfloat16)) == cudaSuccess
- Line 830: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: && cudaMalloc(&d_b, n * sizeof(__nv_bfloat16)) == cudaSuccess
  Confidence: band=very_high; score=0.9
- Line 830: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: && cudaMalloc(&d_b, n * sizeof(__nv_bfloat16)) == cudaSuccess
- Line 833: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: && cudaMemcpy(d_a, ba.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice) == cudaSuccess
- Line 834: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: && cudaMemcpy(d_b, bb.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice)
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
- Line 1107: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint64_t bytes  = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
- Line 1177: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: uint64_t bytes = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(rows[static_cast<size_t>(i)]);
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rows.push_back(row);
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 505: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pairs.emplace_back((*build_side)[bi], (*probe_side)[pi]);
  Confidence: band=high; score=0.74
- Line 655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pairs.emplace_back((*build_side)[bi], (*probe_side)[pi]);
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 860: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 956: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1099: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: heap.emplace_back(dist, vi);
  Confidence: band=high; score=0.74
- Line 1155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: heap.emplace_back(dist, vi);
  Confidence: band=high; score=0.74

### src/gpu/gpu_memory_manager_edition.cpp
Total findings: 44

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 30: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const uint64_t new_total = gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes;
- Line 41: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (it->second.allocated_bytes + size_bytes > it->second.quota_bytes) {
- Line 48: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_memory_allocated_ = new_total;
- Line 49: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (gpu_memory_allocated_ > peak_bytes_) {
- Line 50: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: peak_bytes_ = gpu_memory_allocated_;
- Line 58: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ts.allocated_bytes += size_bytes;
- Line 59: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ts.allocated_bytes > ts.peak_bytes) {
- Line 60: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ts.peak_bytes = ts.allocated_bytes;
- Line 98: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes > max_vram) {
- Line 113: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (it->second.allocated_bytes + tenant_hint_bytes + size_bytes > it->second.quota_bytes) {
- Line 161: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_memory_allocated_ += bytes;
- Line 162: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (gpu_memory_allocated_ > peak_bytes_) {
- Line 163: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: peak_bytes_ = gpu_memory_allocated_;
- Line 169: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ts.allocated_bytes += bytes;
- Line 170: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (ts.allocated_bytes > ts.peak_bytes) {
- Line 171: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ts.peak_bytes = ts.allocated_bytes;
- Line 199: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void GPUMemoryManager::DeallocateGPU(uint64_t size_bytes) {
- Line 201: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (gpu_memory_allocated_ >= size_bytes) {
- Line 202: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_memory_allocated_ -= size_bytes;
- Line 204: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_memory_allocated_ = 0; // guard against mis-matched sizes
- Line 217: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (tit->second.allocated_bytes >= size_bytes) {
- Line 218: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: tit->second.allocated_bytes -= size_bytes;
- Line 220: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: tit->second.allocated_bytes = 0;
- Line 229: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void GPUMemoryManager::DeallocateGPU(uint64_t size_bytes, const std::string &tenant_id) {
- Line 231: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (gpu_memory_allocated_ >= size_bytes) {
- Line 232: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_memory_allocated_ -= size_bytes;
- Line 234: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_memory_allocated_ = 0;
- Line 250: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (tit->second.allocated_bytes >= size_bytes) {
- Line 251: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: tit->second.allocated_bytes -= size_bytes;
- Line 253: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: tit->second.allocated_bytes = 0;
- Line 277: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const uint64_t new_total = gpu_memory_allocated_ + size_bytes;
- Line 281: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: error += std::to_string(gpu_memory_allocated_ / (1024ULL * 1024ULL * 1024ULL));
- Line 297: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return gpu_memory_allocated_;
- Line 303: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return max_vram == 0 ? 0.0f : (static_cast<float>(gpu_memory_allocated_) / static_cast<float>(max_vr
- Line 313: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: s.allocated_bytes    = gpu_memory_allocated_;
- Line 332: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ts.allocated_bytes = it->second.allocated_bytes;
- Line 346: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ts.allocated_bytes = kv.second.allocated_bytes;
- Line 356: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: const uint64_t global_used = gpu_memory_allocated_;
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_hints_.push_back({id, size_bytes, tag, tenant_id});
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: active_allocations_.push_back({bytes, tag, tenant});
- Line 268: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: error += "GB) exceeds edition limit (";
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ts));
  Confidence: band=high; score=0.74

### src/gpu/unified_memory.cpp
Total findings: 12

- Line 13: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: * Real cudaMallocManaged / hipMallocManaged calls are gated behind
  Confidence: band=very_high; score=0.99
- Line 45: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: static const bool result = []() noexcept -> bool {
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
- Line 72: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void *GPUUnifiedMemoryAllocator::allocate(size_t bytes, const std::string &tag, const std::string &t
- Line 72: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void *GPUUnifiedMemoryAllocator::allocate(size_t bytes, const std::string &tag, const std::string &tenant_id) {
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Check: if (ptr != nullptr) before dereferencing
  Context: ptr = std::malloc(bytes);
- Line 268: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: s.allocated_bytes   = allocated_bytes_;
- Line 162: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: std::free(ptr);
- Line 313: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: std::free(rec.ptr);

### src/gpu/admin_api.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3334 [gpu] Integrate MIGManager ... (2026-03-12) | #2913 docs(cache): comple
- Line 71: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: << "\"allocated_bytes\":" << s.allocated_bytes << ","
- Line 102: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: << "\"allocated_bytes\":" << t.allocated_bytes << ","
- Line 157: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: auto [accepted, reason] = effective.simulateAllocation(bytes, stats.allocated_bytes);
- Line 164: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: << "\"current_allocated_bytes\":" << stats.allocated_bytes << "}";
- Line 32: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 33: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 39: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\n";
- Line 42: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\r";
- Line 45: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\t";

### src/gpu/memory_pool.cpp
Total findings: 9

- Line 208: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // caller must have performed a cudaMalloc / hipMalloc of at
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4137 feat(gpu): replace CPU fall... (2026-03-12) | #4138 feat(index): Implem
- Line 202: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Record the old→new mapping so callers can update raw device
- Line 208: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // caller must have performed a cudaMalloc / hipMalloc of at
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: = (allocated_bytes_ > 0) ? (static_cast<float>(wasted_bytes_) / static_cast<float>(total_bytes_)) :

### src/gpu/rocm_backend.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3425 [gpu] Mark multi-node GPU c... (2026-03-12) | #3336 feat(gpu): complete
- Line 58: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [device_index](const GPULauncher::WorkItem& item) -> bool {
- Line 78: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [](const GPULauncher::WorkItem&) -> bool { return true; };
- Line 191: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ROCmBackend::AllocationRecord ROCmBackend::allocate(size_t size_bytes,
- Line 218: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: ROCmBackend::Result ROCmBackend::deallocate(AllocationRecord& rec) {
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (stats_.bytes_allocated >= rec.size_bytes) {
- Line 242: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated -= rec.size_bytes;
- Line 244: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: stats_.bytes_allocated = 0;
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/gpu/time_slice_scheduler.cpp
Total findings: 8

- Line 85: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tenants_.find(tenant_id);
- Line 182: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second.stats.total_elapsed_ms += static_cast<uint64_t>(item_elapsed.count());
- Line 188: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second.stats.queue_depth = it->second.queue.size();
- Line 121: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = tenants_.find(tenant_id);
- Line 137: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: it = tenants_.find(tenant_id);
- Line 201: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 248: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = tenants_.find(tenant_id);
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s);
  Confidence: band=high; score=0.74

### src/gpu/metrics.cpp
Total findings: 6

- Line 115: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: setGauge("themis_gpu_vram_allocated_bytes", labels, static_cast<double>(bytes));
- Line 26: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::string GPUMetrics::buildKey(const std::string &name, const std::unordered_map<std::string, std::string> &labels) {
  Confidence: band=medium; score=0.66
- Line 33: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ',';
  Confidence: band=high; score=0.74
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ',';
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/gpu/profiler.cpp
Total findings: 6

- Line 133: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"ts\": " << (r.start_ns / 1000) << ", "
- Line 133: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"ts\": " << (r.start_ns / 1000) << ", "
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"ts\": " << (r.start_ns / 1000) << ", "
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"ts\": " << (r.start_ns / 1000) << ", "
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"dur\": " << ((r.end_ns - r.start_ns) / 1000) << ", "
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\"dur\": " << ((r.end_ns - r.start_ns) / 1000) << ", "

### src/gpu/alerts.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 115: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "No healthy GPU device available");
- Line 132: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<AlertStatus> result;
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/gpu/cluster_coordinator.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3425 [gpu] Mark multi-node GPU c... (2026-03-12) | #3019 [gpu] Fix multi-nod
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: local.device_indices.push_back(d.index);
  Confidence: band=high; score=0.74
- Line 53: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: local.device_indices.push_back(d.index);
- Line 299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(std::move(n));
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(n);
  Confidence: band=high; score=0.74

### src/gpu/cluster_topology.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4485 feat(gpu): fix NVLink topol... (2026-04-09) | #3425 [gpu] Mark multi-no
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
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3561 docs(gpu): reality-check sr... (2026-03-12) | #3171 [gpu] Implement MIG
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

### src/gpu/feature_flags.cpp
Total findings: 5

- Line 189: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto ov = overrides_.find(k);
- Line 194: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto def = defaults_.find(k);
- Line 183: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<FeatureStatus> result;
- Line 186: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: FeatureStatus s;
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s);
  Confidence: band=high; score=0.74

### src/gpu/gpu_module.cpp
Total findings: 5

- Line 130: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool GPUModule::allocate(const std::string &caller_id, const std::string &tenant_id, uint64_t bytes,
- Line 175: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: void GPUModule::deallocate(const std::string &tenant_id, uint64_t bytes) {
- Line 175: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void GPUModule::deallocate(const std::string &tenant_id, uint64_t bytes) {
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: mgr.DeallocateGPU(bytes);
- Line 184: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: mgr.DeallocateGPU(bytes, tenant_id);

### src/gpu/stream_manager.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3425 [gpu] Mark multi-node GPU c... (2026-03-12) | #3077 fix(gpu): Resolve c
- Line 194: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 185: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second.stats);
  Confidence: band=high; score=0.74

### src/gpu/mig_manager.cpp
Total findings: 4

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
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.second);

### src/gpu/p2p_transfer.cpp
Total findings: 4

- Line 97: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: InterconnectType detectInterconnect(int src, int dst, const std::vector<DeviceInfo> &devs) {
- Line 102: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return topo.preferredInterconnect(src, dst);
- Line 318: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: result.error_message = "cudaMemcpyPeer failed";
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: const InterconnectType itype = detectInterconnect(req.src_device, req.dst_device, devs);

### src/gpu/launcher.cpp
Total findings: 3

- Line 60: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 70: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(executeOne(std::move(item)));
  Confidence: band=high; score=0.74

### src/gpu/load_balancer.cpp
Total findings: 3

- Line 197: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return entry ? &entry->info : nullptr;
- Line 41: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devices_.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(dl));
  Confidence: band=high; score=0.74

### src/gpu/safe_fail.cpp
Total findings: 3

- Line 62: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 88: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 179: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: HealthStatus s;

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

### src/gpu/tensor_buffer.cpp
Total findings: 2

- Line 309: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shape.dims.push_back(read32(p));
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shape.dims.push_back(read32(p));

### src/gpu/vulkan_backend.cpp
Total findings: 2

- Line 158: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [this](const GPULauncher::WorkItem & /*item*/) -> bool {
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/gpu/wasm_kernel_sandbox.cpp
Total findings: 2

- Line 200: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 237: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/gpu/config.cpp
Total findings: 1

- Line 90: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: uint64_t current_allocated_bytes) const {

### src/gpu/kernel_validator.cpp
Total findings: 1

- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
