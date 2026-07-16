# gpu Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: gpu
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 173
- Actionable Findings (Critical + High): 133
- Affected Files: 25

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 16 |
| High | 117 |
| Medium | 33 |
| Low | 7 |

## Category Summary

| Category | Count |
|---|---:|
| db_connection_leak | 55 |
| unchecked_cuda_call | 18 |
| uncaught_exception | 11 |
| uninitialized_access | 11 |
| string_concat_loop | 9 |
| resource_leaked_in_exception | 8 |
| use_after_free_gpu | 8 |
| generic_catch | 7 |
| size_assumption | 6 |
| hardcoded_output | 5 |
| o_n_squared | 5 |
| gpu_memory_leak | 4 |
| data_race | 3 |
| hardcoded_path | 3 |
| missing_trace_point | 3 |
| primitive_no_volatile | 3 |
| fp_exact_comparison | 2 |
| manual_cleanup | 2 |
| module_doc_linkset_drift | 2 |
| allocation_loop | 1 |
| deadlock_risk | 1 |
| explicit_lock_unlock | 1 |
| iterator_invalidation | 1 |
| lock_contention | 1 |
| stale_doc_section_reference | 1 |
| unchecked_malloc | 1 |
| unordered_container_iter | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| gpu/query_accelerator.cpp | 46 | 8 | 21 | 12 | 5 |
| gpu/gpu_memory_manager_edition.cpp | 43 | 0 | 42 | 1 | 0 |
| gpu/admin_api.cpp | 11 | 0 | 5 | 6 | 0 |
| gpu/unified_memory.cpp | 11 | 4 | 5 | 2 | 0 |
| gpu/memory_pool.cpp | 9 | 1 | 8 | 0 | 0 |
| gpu/rocm_backend.cpp | 8 | 0 | 8 | 0 | 0 |
| gpu/time_slice_scheduler.cpp | 8 | 3 | 5 | 0 | 0 |
| gpu/gpu_module.cpp | 5 | 0 | 5 | 0 | 0 |
| gpu/stream_manager.cpp | 5 | 0 | 2 | 3 | 0 |
| gpu/metrics.cpp | 4 | 0 | 1 | 3 | 0 |
| gpu/profiler.cpp | 3 | 0 | 0 | 3 | 0 |
| gpu/alerts.cpp | 2 | 0 | 2 | 0 | 0 |
| gpu/feature_flags.cpp | 2 | 0 | 2 | 0 | 0 |
| gpu/launcher.cpp | 2 | 0 | 0 | 2 | 0 |
| gpu/p2p_transfer.cpp | 2 | 0 | 2 | 0 | 0 |
| gpu/tensor_buffer.cpp | 2 | 0 | 2 | 0 | 0 |
| gpu/training_loop.cpp | 2 | 0 | 2 | 0 | 0 |
| gpu/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| gpu/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| gpu/cluster_coordinator.cpp | 1 | 0 | 1 | 0 | 0 |
| gpu/cluster_topology.cpp | 1 | 0 | 1 | 0 | 0 |
| gpu/config.cpp | 1 | 0 | 1 | 0 | 0 |
| gpu/device_discovery.cpp | 1 | 0 | 1 | 0 | 0 |
| gpu/vulkan_backend.cpp | 1 | 0 | 1 | 0 | 0 |
| gpu/wasm_kernel_sandbox.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### gpu/query_accelerator.cpp
Total findings: 46

- Line 787: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_a
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: __half *d_a         = nullptr;
- Line 788: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_b
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: __half *d_b         = nullptr;
- Line 825: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_a
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: __nv_bfloat16 *d_a  = nullptr;
- Line 826: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_b
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: __nv_bfloat16 *d_b  = nullptr;
- Line 827: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_c
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: float *d_c          = nullptr; // accumulate in FP32
- Line 916: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_a
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: hipblasHalf *d_a    = nullptr;
- Line 917: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_b
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: hipblasHalf *d_b    = nullptr;
- Line 1100: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // cudaMalloc failure or cuVS error — fall through to CPU path.
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4137 feat(gpu): replace CPU fall... (2026-03-12) | #3561 docs(gpu): reality-
- Line 763: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(float)) == cudaSuccess
- Line 766: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: && cudaMemcpy(d_b, b.data(), n * sizeof(float), cudaMemcpyHostToDevice) == cudaSuccess) {
- Line 777: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_b);
- Line 791: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(__half)) == cudaSuccess
- Line 792: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: && cudaMalloc(&d_b, n * sizeof(__half)) == cudaSuccess
- Line 795: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: && cudaMemcpy(d_a, ha.data(), n * sizeof(__half), cudaMemcpyHostToDevice) == cudaSuccess
- Line 796: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: && cudaMemcpy(d_b, hb.data(), n * sizeof(__half), cudaMemcpyHostToDevice) == cudaSuccess) {
- Line 817: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_c);
- Line 829: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(__nv_bfloat16)) == cudaSuccess
- Line 829: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(__nv_bfloat16)) == cudaSuccess
- Line 830: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: && cudaMalloc(&d_b, n * sizeof(__nv_bfloat16)) == cudaSuccess
- Line 830: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: && cudaMalloc(&d_b, n * sizeof(__nv_bfloat16)) == cudaSuccess
- Line 833: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: && cudaMemcpy(d_a, ba.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice) == cudaSuccess
- Line 833: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: && cudaMemcpy(d_a, ba.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice) == cudaSuccess
- Line 834: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: && cudaMemcpy(d_b, bb.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice)
- Line 834: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: && cudaMemcpy(d_b, bb.data(), n * sizeof(__nv_bfloat16), cudaMemcpyHostToDevice)
- Line 854: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_c);
- Line 1100: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // cudaMalloc failure or cuVS error — fall through to CPU path.
- Line 1107: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint64_t bytes  = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
- Line 1177: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: uint64_t bytes = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
- Line 291: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Thrust system_error or std::runtime_error — fall through.

            result.rows.clear();

            gpu_done = false;

        } catch (...) {

            result.rows.clear();

            gpu_done = false;

        }
- Line 291: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 395: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch ([[maybe_unused]] const std::exception &ex) {

            // Thrust system_error or std::runtime_error — fall through to CPU.

            gpu_done = false;

        } catch (...) {

            gpu_done = false;

        }

        if (gpu_done) {
- Line 395: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 505: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch ([[maybe_unused]] const std::exception &ex) {

            // Thrust system_error or std::runtime_error — fall through to CPU.

            gpu_done = false;

        } catch (...) {

            gpu_done = false;

        }

        if (gpu_done) {
- Line 505: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 667: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Thrust system_error or std::runtime_error — fall through to CPU.

            result.pairs.clear();

            gpu_done = false;

        } catch (...) {

            result.pairs.clear();

            gpu_done = false;

        }
- Line 667: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 860: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch ([[maybe_unused]] const std::exception &ex) {

            // cuBLAS, Thrust, or std::runtime_error — fall through to CPU.

            gpu_done = false;

        } catch (...) {

            gpu_done = false;

        }

        if (blas_handle)
- Line 860: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 956: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch ([[maybe_unused]] const std::exception &ex) {

            // hipBLAS, Thrust, or std::runtime_error — fall through to CPU.

            gpu_done = false;

        } catch (...) {

            gpu_done = false;

        }

        if (blas_handle)
- Line 956: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 742: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // FP16: cublasGemmEx / hipblasGemmEx with CUDA_R_16F / HIPBLAS_R_16F inputs
- Line 745: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // BF16: cublasGemmEx with CUDA_R_16BF inputs + CUDA_R_32F output (CUDA only;
- Line 781: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Use CUDA_R_16F inputs with CUDA_R_32F output + FP32 compute
- Line 910: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Use HIPBLAS_R_16F inputs + HIPBLAS_R_32F output to avoid
- Line 1020: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Validate inputs --------------------------------------------------------

### gpu/gpu_memory_manager_edition.cpp
Total findings: 43

- Line 30: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const uint64_t new_total = gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes;
- Line 33: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 41: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (it->second.allocated_bytes + size_bytes > it->second.quota_bytes) {
- Line 48: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_memory_allocated_ = new_total;
- Line 49: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (gpu_memory_allocated_ > peak_bytes_) {
- Line 50: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: peak_bytes_ = gpu_memory_allocated_;
- Line 58: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ts.allocated_bytes += size_bytes;
- Line 59: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (ts.allocated_bytes > ts.peak_bytes) {
- Line 60: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ts.peak_bytes = ts.allocated_bytes;
- Line 98: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (gpu_memory_allocated_ + hint_reserved_bytes_ + size_bytes > max_vram) {
- Line 113: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (it->second.allocated_bytes + tenant_hint_bytes + size_bytes > it->second.quota_bytes) {
- Line 161: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_memory_allocated_ += bytes;
- Line 162: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (gpu_memory_allocated_ > peak_bytes_) {
- Line 163: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: peak_bytes_ = gpu_memory_allocated_;
- Line 169: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ts.allocated_bytes += bytes;
- Line 170: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (ts.allocated_bytes > ts.peak_bytes) {
- Line 171: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ts.peak_bytes = ts.allocated_bytes;
- Line 199: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void GPUMemoryManager::DeallocateGPU(uint64_t size_bytes) {
- Line 201: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (gpu_memory_allocated_ >= size_bytes) {
- Line 202: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_memory_allocated_ -= size_bytes;
- Line 204: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_memory_allocated_ = 0; // guard against mis-matched sizes
- Line 217: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (tit->second.allocated_bytes >= size_bytes) {
- Line 218: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: tit->second.allocated_bytes -= size_bytes;
- Line 220: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: tit->second.allocated_bytes = 0;
- Line 229: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void GPUMemoryManager::DeallocateGPU(uint64_t size_bytes, const std::string &tenant_id) {
- Line 231: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (gpu_memory_allocated_ >= size_bytes) {
- Line 232: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_memory_allocated_ -= size_bytes;
- Line 234: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_memory_allocated_ = 0;
- Line 250: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (tit->second.allocated_bytes >= size_bytes) {
- Line 251: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: tit->second.allocated_bytes -= size_bytes;
- Line 253: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: tit->second.allocated_bytes = 0;
- Line 273: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: error += std::to_string(GetMaxGPUVRAMGB());

        error += "GB). Edition: ";

        error += std::string(edition::EDITION_STRING);

        throw std::runtime_error(error);

    }



    std::lock_guard<std::mutex> lock(mutex_);
- Line 277: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const uint64_t new_total = gpu_memory_allocated_ + size_bytes;
- Line 279: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 281: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: error += std::to_string(gpu_memory_allocated_ / (1024ULL * 1024ULL * 1024ULL));
- Line 287: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: error += "GB, limit: ";

        error += std::to_string(GetMaxGPUVRAMGB());

        error += "GB";

        throw std::runtime_error(error);

    }

}
- Line 297: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return gpu_memory_allocated_;
- Line 303: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return max_vram == 0 ? 0.0f : (static_cast<float>(gpu_memory_allocated_) / static_cast<float>(max_vr
- Line 313: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.allocated_bytes    = gpu_memory_allocated_;
- Line 332: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ts.allocated_bytes = it->second.allocated_bytes;
- Line 346: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ts.allocated_bytes = kv.second.allocated_bytes;
- Line 356: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: const uint64_t global_used = gpu_memory_allocated_;
- Line 268: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: error += "GB) exceeds edition limit (";

### gpu/admin_api.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3334 [gpu] Integrate MIGManager ... (2026-03-12) | #2913 docs(cache): comple
- Line 71: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << "\"allocated_bytes\":" << s.allocated_bytes << ","
- Line 102: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << "\"allocated_bytes\":" << t.allocated_bytes << ","
- Line 157: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto [accepted, reason] = effective.simulateAllocation(bytes, stats.allocated_bytes);
- Line 164: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: << "\"current_allocated_bytes\":" << stats.allocated_bytes << "}";
- Line 32: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "\\\"";
- Line 33: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\"";
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\\";
- Line 39: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\n";
- Line 42: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\r";
- Line 45: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\t";

### gpu/unified_memory.cpp
Total findings: 11

- Line 13: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: * Real cudaMallocManaged / hipMallocManaged calls are gated behind
- Line 45: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: static const bool result = []() noexcept -> bool {
- Line 80: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: if (cudaMallocManaged(&ptr, bytes, cudaMemAttachGlobal) != cudaSuccess) {
- Line 161: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: ptr
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: std::free(ptr);
- Line 13: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: * Real cudaMallocManaged / hipMallocManaged calls are gated behind
- Line 72: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void *GPUUnifiedMemoryAllocator::allocate(size_t bytes, const std::string &tag, const std::string &t
- Line 72: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void *GPUUnifiedMemoryAllocator::allocate(size_t bytes, const std::string &tag, const std::string &tenant_id) {
- Line 88: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ptr = std::malloc(bytes);
- Line 268: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.allocated_bytes   = allocated_bytes_;
- Line 162: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr);
- Line 313: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(rec.ptr);

### gpu/memory_pool.cpp
Total findings: 9

- Line 208: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // caller must have performed a cudaMalloc / hipMalloc of at
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4137 feat(gpu): replace CPU fall... (2026-03-12) | #4138 feat(index): Implem
- Line 194: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 195: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 202: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Record the old→new mapping so callers can update raw device
- Line 203: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 208: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // caller must have performed a cudaMalloc / hipMalloc of at
- Line 235: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 251: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: = (allocated_bytes_ > 0) ? (static_cast<float>(wasted_bytes_) / static_cast<float>(total_bytes_)) :

### gpu/rocm_backend.cpp
Total findings: 8

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3425 [gpu] Mark multi-node GPU c... (2026-03-12) | #3336 feat(gpu): complete
- Line 58: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [device_index](const GPULauncher::WorkItem& item) -> bool {
- Line 78: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [](const GPULauncher::WorkItem&) -> bool { return true; };
- Line 191: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ROCmBackend::AllocationRecord ROCmBackend::allocate(size_t size_bytes,
- Line 218: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ROCmBackend::Result ROCmBackend::deallocate(AllocationRecord& rec) {
- Line 241: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (stats_.bytes_allocated >= rec.size_bytes) {
- Line 242: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats_.bytes_allocated -= rec.size_bytes;
- Line 244: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats_.bytes_allocated = 0;

### gpu/time_slice_scheduler.cpp
Total findings: 8

- Line 85: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = tenants_.find(tenant_id);
- Line 182: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: it->second.stats.total_elapsed_ms += static_cast<uint64_t>(item_elapsed.count());
- Line 188: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: it->second.stats.queue_depth = it->second.queue.size();
- Line 121: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = tenants_.find(tenant_id);
- Line 137: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: it = tenants_.find(tenant_id);
- Line 171: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 201: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 248: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = tenants_.find(tenant_id);

### gpu/gpu_module.cpp
Total findings: 5

- Line 130: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool GPUModule::allocate(const std::string &caller_id, const std::string &tenant_id, uint64_t bytes,
- Line 175: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void GPUModule::deallocate(const std::string &tenant_id, uint64_t bytes) {
- Line 175: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void GPUModule::deallocate(const std::string &tenant_id, uint64_t bytes) {
- Line 182: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: mgr.DeallocateGPU(bytes);
- Line 184: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: mgr.DeallocateGPU(bytes, tenant_id);

### gpu/stream_manager.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3425 [gpu] Mark multi-node GPU c... (2026-03-12) | #3077 fix(gpu): Resolve c
- Line 194: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 173: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'CUDA Stream Manager Activation' that was not found in 'src/gpu/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/gpu/FUTURE_ENHANCEMENTS.md §"CUDA Stream Manager Activation"
- Line 185: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) {

            try {

                backend_fn = fn(device_index);

            } catch (...) {

                backend_fn = ROCmBackend::GetInstance().createBackendFn(device_index);

            }

        } else {
- Line 185: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### gpu/metrics.cpp
Total findings: 4

- Line 115: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: setGauge("themis_gpu_vram_allocated_bytes", labels, static_cast<double>(bytes));
- Line 26: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::string GPUMetrics::buildKey(const std::string &name, const std::unordered_map<std::string, std::string> &labels) {
- Line 33: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key += ',';
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: key += ',';

### gpu/profiler.cpp
Total findings: 3

- Line 133: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "\"ts\": " << (r.start_ns / 1000) << ", "
- Line 137: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "\"ts\": " << (r.start_ns / 1000) << ", "
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "\"dur\": " << ((r.end_ns - r.start_ns) / 1000) << ", "

### gpu/alerts.cpp
Total findings: 2

- Line 80: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 82: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### gpu/feature_flags.cpp
Total findings: 2

- Line 189: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto ov = overrides_.find(k);
- Line 194: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto def = defaults_.find(k);

### gpu/launcher.cpp
Total findings: 2

- Line 42: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool ok = false;
- Line 43: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool timed_out = false;

### gpu/p2p_transfer.cpp
Total findings: 2

- Line 314: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMemcpyPeer(req.dst_ptr, dst_idx, req.src_ptr, src_idx, req.size_bytes);
- Line 318: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: result.error_message = "cudaMemcpyPeer failed";

### gpu/tensor_buffer.cpp
Total findings: 2

- Line 288: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto need = [&](size_t n) {

        if (p + n > end) {

            throw std::runtime_error("GPUTensorBuffer::deserialize: truncated data");

        }

    };
- Line 296: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: uint32_t magic = read32(p);

    p += 4;

    if (magic != 0x54454E53u) {

        throw std::runtime_error("GPUTensorBuffer::deserialize: bad magic");

    }



    need(4);

### gpu/training_loop.cpp
Total findings: 2

- Line 112: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: es.min_loss  = (min_loss == std::numeric_limits<double>::max()) ? 0.0 : min_loss;
- Line 113: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: es.max_loss  = (max_loss == std::numeric_limits<double>::lowest()) ? 0.0 : max_loss;

### gpu/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### gpu/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### gpu/cluster_coordinator.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3425 [gpu] Mark multi-node GPU c... (2026-03-12) | #3019 [gpu] Fix multi-nod

### gpu/cluster_topology.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4485 feat(gpu): fix NVLink topol... (2026-04-09) | #3425 [gpu] Mark multi-no

### gpu/config.cpp
Total findings: 1

- Line 90: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: uint64_t current_allocated_bytes) const {

### gpu/device_discovery.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3561 docs(gpu): reality-check sr... (2026-03-12) | #3171 [gpu] Implement MIG

### gpu/vulkan_backend.cpp
Total findings: 1

- Line 158: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [this](const GPULauncher::WorkItem & /*item*/) -> bool {

### gpu/wasm_kernel_sandbox.cpp
Total findings: 1

- Line 197: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool ok = false;

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
