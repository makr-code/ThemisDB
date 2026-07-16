# acceleration Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: acceleration
- Generated: 2026-06-04 08:50:21
- Status: Critical Findings Present
- Total Findings: 435
- Actionable Findings (Critical + High): 227
- Affected Files: 27

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 57 |
| High | 170 |
| Medium | 101 |
| Low | 107 |

## Category Summary

| Category | Count |
|---|---:|
| hardcoded_output | 99 |
| resource_leaked_in_exception | 47 |
| manual_cleanup | 43 |
| unchecked_cuda_call | 32 |
| explicit_delete | 19 |
| delete_no_nullptr | 17 |
| delete_without_nullptr | 17 |
| primitive_no_volatile | 15 |
| data_race | 13 |
| expensive_inner_op | 12 |
| smart_ptr_misuse | 12 |
| stale_doc_section_reference | 12 |
| uninitialized_access | 12 |
| array_bounds_violation | 11 |
| unwrapped_resource | 8 |
| duplicate_qualified_signature | 6 |
| size_assumption | 6 |
| thread_join_no_timeout | 5 |
| unstructured_log | 5 |
| fp_exact_comparison | 4 |
| range_temporary | 4 |
| legacy_or_compat_path | 3 |
| missing_move_constructor_defaulted | 3 |
| no_timeout | 3 |
| blocking_no_timeout | 2 |
| cast_to_smaller_type | 2 |
| copy_overhead | 2 |
| crypto_weakness | 2 |
| db_connection_leak | 2 |
| endl_in_loop | 2 |
| missing_health_check | 2 |
| shared_state_no_sync | 2 |
| hardcoded_path | 1 |
| manual_cleanup_in_destructor | 1 |
| missing_resource_limits | 1 |
| module_doc_linkset_drift | 1 |
| new_without_raii | 1 |
| path_traversal | 1 |
| repeated_search | 1 |
| uncaught_exception | 1 |
| uninitialized_array | 1 |
| uninitialized_member_field | 1 |
| unordered_container_iter | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| acceleration/faiss_gpu_backend.cpp | 100 | 22 | 52 | 11 | 15 |
| acceleration/cuda_backend.cpp | 92 | 3 | 74 | 0 | 15 |
| acceleration/plugin_security.cpp | 40 | 5 | 4 | 31 | 0 |
| acceleration/graphics_backends.cpp | 29 | 9 | 3 | 7 | 10 |
| acceleration/oneapi_backend.cpp | 29 | 16 | 3 | 5 | 5 |
| acceleration/hip_backend.cpp | 28 | 0 | 2 | 12 | 14 |
| acceleration/plugin_loader.cpp | 13 | 0 | 3 | 3 | 7 |
| acceleration/directx_backend_full.cpp | 11 | 0 | 4 | 5 | 2 |
| acceleration/opencl_backend.cpp | 10 | 0 | 2 | 1 | 7 |
| acceleration/vllm_resource_manager.cpp | 9 | 2 | 5 | 2 | 0 |
| acceleration/zluda_backend.cpp | 9 | 0 | 2 | 2 | 5 |
| acceleration/cpu_backend_mt.cpp | 6 | 0 | 0 | 0 | 6 |
| acceleration/cpu_backend_tbb.cpp | 6 | 0 | 0 | 0 | 6 |
| acceleration/nccl_vector_backend.cpp | 6 | 0 | 0 | 4 | 2 |
| acceleration/rccl_vector_backend.cpp | 6 | 0 | 0 | 4 | 2 |
| acceleration/vulkan_backend_full.cpp | 6 | 0 | 1 | 3 | 2 |
| acceleration/backend_registry.cpp | 5 | 0 | 3 | 2 | 0 |
| acceleration/cpu_backend.cpp | 5 | 0 | 2 | 2 | 1 |
| acceleration/device_manager.cpp | 5 | 0 | 1 | 1 | 3 |
| acceleration/shader_integrity.cpp | 5 | 0 | 3 | 2 | 0 |
| acceleration/geo_acceleration_bridge.cpp | 4 | 0 | 1 | 0 | 3 |
| acceleration/ai_hardware_dispatcher.cpp | 3 | 0 | 1 | 2 | 0 |
| acceleration/multi_gpu_backend.cpp | 3 | 0 | 0 | 2 | 1 |
| acceleration/tensor_core_matmul.cpp | 2 | 0 | 2 | 0 | 0 |
| acceleration/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| acceleration/compute_backend.cpp | 1 | 0 | 1 | 0 | 0 |
| acceleration/vec_knn.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### acceleration/faiss_gpu_backend.cpp
Total findings: 100

- Line 163: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* idx = new faiss::gpu::GpuIndexFlatL2(
- Line 163: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* idx = new faiss::gpu::GpuIndexFlatL2(
- Line 172: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* idx = new faiss::gpu::GpuIndexFlatIP(
- Line 172: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* idx = new faiss::gpu::GpuIndexFlatIP(
- Line 182: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 182: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 191: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* idx = new faiss::gpu::GpuIndexIVFFlat(
- Line 191: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* idx = new faiss::gpu::GpuIndexIVFFlat(
- Line 199: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: idx->nprobe = config_.nprobe;
- Line 205: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 205: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 214: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* idx = new faiss::gpu::GpuIndexIVFPQ(
- Line 214: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* idx = new faiss::gpu::GpuIndexIVFPQ(
- Line 223: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: idx->nprobe = config_.nprobe;
- Line 232: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* idx = new faiss::gpu::GpuIndexIVFScalarQuantizer(
- Line 232: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* idx = new faiss::gpu::GpuIndexIVFScalarQuantizer(
- Line 241: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: idx->nprobe = config_.nprobe;
- Line 248: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto* idx = new faiss::IndexHNSWFlat(dimension, config_.hnswM);
- Line 248: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* idx = new faiss::IndexHNSWFlat(dimension, config_.hnswM);
- Line 750: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config_.dimension = gpuIndex->d;
- Line 800: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats.numVectors = static_cast<size_t>(idx->ntotal);
- Line 801: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats.dimension  = static_cast<size_t>(idx->d);
- Line 248: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 266: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(index_);
- Line 266: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: switch (currentIndexType_) {

        case IndexType::FLAT_L2:

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(index_);

            break;

        case IndexType::FLAT_IP:

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);
- Line 266: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(index_);
- Line 269: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);
- Line 269: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(index_);

            break;

        case IndexType::FLAT_IP:

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);

            break;

        case IndexType::IVF_FLAT:

            delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 269: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);
- Line 272: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 272: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);

            break;

        case IndexType::IVF_FLAT:

            delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);

            break;

        case IndexType::IVF_PQ:

            delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 272: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 275: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 275: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);

            break;

        case IndexType::IVF_PQ:

            delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);

            break;

        case IndexType::IVF_SQ8:

            delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 275: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 278: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 278: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);

            break;

        case IndexType::IVF_SQ8:

            delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);

            break;

        case IndexType::HNSW_FLAT:

            delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 278: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 281: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 281: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);

            break;

        case IndexType::HNSW_FLAT:

            delete static_cast<faiss::IndexHNSWFlat*>(index_);

            break;

        default:

            // Unknown type — release via base faiss::Index destructor to avoid leak
- Line 281: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 285: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::Index*>(index_);
- Line 285: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: break;

        default:

            // Unknown type — release via base faiss::Index destructor to avoid leak

            delete static_cast<faiss::Index*>(index_);

            break;

    }
- Line 285: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::Index*>(index_);
- Line 536: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 536: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Cleanup

        if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }
- Line 536: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 538: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 538: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }



        return distances;
- Line 538: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 547: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 547: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: setError(AccelerationErrorCode::KernelExecutionFailed,

                 std::string("computeDistances failed: ") + e.what());

        if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }
- Line 547: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 549: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 549: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }

        

        return {};
- Line 549: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 619: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 619: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Cleanup

        if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }
- Line 619: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 621: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 621: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }



        return results;
- Line 621: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 632: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 632: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Cleanup on error

        if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }
- Line 632: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 634: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 634: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (useL2) {

            delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);

        } else {

            delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

        }



        return {};
- Line 634: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 697: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete cpuIndex;
- Line 697: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (cpuIndex) {

            faiss::write_index(cpuIndex, filepath.c_str());

            delete cpuIndex;

            std::cout << "Index saved to: " << filepath << std::endl;

            return true;

        }
- Line 697: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete cpuIndex;
- Line 741: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete cpuIndex;
- Line 741: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: &options

        );



        delete cpuIndex;



        if (!gpuIndex) {

            setError(AccelerationErrorCode::MemoryCopyFailed,
- Line 741: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete cpuIndex;
- Line 272: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 275: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 278: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 281: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 285: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::Index*>(index_);
- Line 510: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    Config tempConfig;', '    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;', '    tempConfig.dimension = static_cast<int>(dim);', '    tempConfig.deviceId = config_.deviceId;', '']
- Line 536: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 538: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 574: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    Config tempConfig;', '    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;', '    tempConfig.dimension = static_cast<int>(dim);', '    tempConfig.deviceId = config_.deviceId;', '']
- Line 619: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 621: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 100: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Faiss GPU Backend initialized successfully" << std::endl;
- Line 101: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Device ID: " << config_.deviceId << std::endl;
- Line 102: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Memory Limit: " << config_.maxMemoryMB << " MB" << std::endl;
- Line 145: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Faiss index created — type: "
- Line 333: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Training Faiss index with " << numVectors << " vectors..." << std::endl;
- Line 335: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Training complete" << std::endl;
- Line 395: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Added " << numVectors << " vectors to index (total: "
- Line 503: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: "computeDistances: null pointers or zero-size inputs");
- Line 567: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: "batchKnnSearch: null pointers or zero-size inputs");
- Line 686: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Index saved to: " << filepath << std::endl;
- Line 698: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Index saved to: " << filepath << std::endl;
- Line 752: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Index loaded from: " << filepath << std::endl;
- Line 753: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Vectors: " << gpuIndex->ntotal << std::endl;
- Line 754: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Dimension: " << gpuIndex->d << std::endl;
- Line 851: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Index reset" << std::endl;

### acceleration/cuda_backend.cpp
Total findings: 92

- Line 348: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k), ef);
- Line 457: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k));
- Line 637: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



CUDAGraphEntry &CUDAGraphCache::put(const QueryShape &shape, CUDAGraphEntry entry) {

    // Only evict if this is truly a new key (not a replacement)

    if (entries_.size() >= kMaxEntries && entries_.count(shape) == 0) {

        evictLRU();

    }
- Line 730: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 731: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 732: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 733: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 737: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 737: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_queries.get(), 0, querySize);
- Line 738: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 738: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_vectors.get(), 0, vectorSize);
- Line 739: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 739: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_distances.get(), 0, distanceSize);
- Line 740: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 740: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_topkIndices.get(), 0, topkIdxSize);
- Line 741: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_topkDistances.get(), 0, topkDistSize);
- Line 742: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // cudaMemset is synchronous: it blocks the host until the fill is
- Line 795: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 799: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 836: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(entry->d_queries.get(), queries, querySize, cudaMemcpyHostToDevice, mainStream);
- Line 837: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(entry->d_vectors.get(), vectors, vectorSize, cudaMemcpyHostToDevice, mainStream);
- Line 853: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(topkIndices.data(), entry->d_topkIndices.get(), topkIdxSize, cudaMemcpyDeviceToHost,
- Line 855: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(topkDistances.data(), entry->d_topkDistances.get(), topkDistSize, cudaMemcpyDeviceToHost,
- Line 1244: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1245: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1246: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1247: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1248: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1249: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1250: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1254: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1254: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
- Line 1255: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1255: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
- Line 1256: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1256: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_frontier_a.get(), 0, frontierSz);
- Line 1257: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1257: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_frontier_b.get(), 0, frontierSz);
- Line 1258: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1258: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_visited.get(), 0, frontierSz);
- Line 1259: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1259: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_depths.get(), 0, frontierSz);
- Line 1260: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1260: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_result_vertices.get(), 0, resultsSz);
- Line 1261: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_result_sizes.get(), 0, sizesSz);
- Line 1289: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1295: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1297: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1302: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1309: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1313: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1345: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream);
- Line 1346: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream);
- Line 1360: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(h_result_vertices.data(), entry->d_result_vertices.get(), resultsSz, cudaMemcpyDeviceToHost,
- Line 1362: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(h_result_sizes.data(), entry->d_result_sizes.get(), sizesSz, cudaMemcpyDeviceToHost,
- Line 1452: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1453: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1454: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1455: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1458: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1458: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
- Line 1459: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1459: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_weights.get(), 0, wgtSize);
- Line 1460: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1460: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
- Line 1461: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1461: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_distances.get(), 0, distSize);
- Line 1462: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemset(newEntry.d_predecessors.get(), 0, predSize);
- Line 1487: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1495: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1499: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1530: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream);
- Line 1531: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(entry->d_weights.get(), weights, wgtSize, cudaMemcpyHostToDevice, mainStream);
- Line 1532: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream);
- Line 1546: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(h_distances.data(), entry->d_distances.get(), distSize, cudaMemcpyDeviceToHost, mainStream);
- Line 1547: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyAsync(h_predecessors.data(), entry->d_predecessors.get(), predSize, cudaMemcpyDeviceToHost,
- Line 1816: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t resultSize = numPoints * sizeof(uint8_t);
- Line 178: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "CUDA: Created low-priority stream for vLLM co-location (priority=" << leastPriority << ")"
- Line 206: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "CUDA Backend initialized successfully:" << std::endl;
- Line 207: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Device: " << prop.name << std::endl;
- Line 208: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Compute Capability: " << prop.major << "." << prop.minor << std::endl;
- Line 209: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Global Memory: " << (prop.totalGlobalMem / (1024 * 1024 * 1024)) << " GB" << std::endl;
- Line 210: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Multiprocessors: " << prop.multiProcessorCount << std::endl;
- Line 211: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  CUDA Runtime: " << (runtimeVersion / 1000) << "." << ((runtimeVersion % 100) / 10) << std::endl;
- Line 213: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  vLLM Co-Location: ENABLED (low-priority stream, max " << THEMIS_MAX_GPU_VRAM_MB << " MB VRAM)"
- Line 220: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Occupancy-tuned vector block dim: " << vecBlockDim << "x" << vecBlockDim << std::endl;
- Line 285: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "CUDAVectorBackend: clamping maxBatchSize from " << maxBatchSize_ << " to "
- Line 1158: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "CUDA Graph Backend: occupancy-tuned BFS block dim = " << bfsBlockDim << std::endl;
- Line 1343: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Replay: copy inputs → device, launch graph, copy results ← device
- Line 1686: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "CUDA Geo Backend initialized successfully:" << std::endl;
- Line 1687: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Device: " << prop.name << std::endl;
- Line 1691: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Occupancy-tuned geo block size: " << geoBlockSize << std::endl;

### acceleration/plugin_security.cpp
Total findings: 40

- Line 927: severity=CRITICAL; category=crypto_weakness
  Description: weak_hash_sha1_usage: SHA-1 hash — use SHA-256 or stronger
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: OCSP_CERTID *certid = OCSP_cert_to_id(EVP_sha1(), cert, issuer_cert);
- Line 987: severity=CRITICAL; category=crypto_weakness
  Description: weak_hash_sha1_usage: SHA-1 hash — use SHA-256 or stronger
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: OCSP_CERTID *lookup_id = OCSP_cert_to_id(EVP_sha1(), cert, issuer_cert);
- Line 1287: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: X509 *cert             = d2i_X509(nullptr, &p, static_cast<long>(cert_data->size()));
- Line 1391: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::string &signing_cert = metadata->signature.signingCertificate;
- Line 2046: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4292 fix(acceleration): PE certi... (2026-03-16) | #4283 feat(acceleration):
- Line 885: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1446: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2043: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 71: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 287: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 295: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 298: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 578: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 579: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pubKey);
- Line 580: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(cert);
- Line 765: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_STORE_CTX_free(chain_ctx);
- Line 772: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(crl_issuer_key);
- Line 773: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_STORE_free(trust_store);
- Line 774: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_CRL_free(crl);
- Line 777: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(crl_issuer_key);
- Line 783: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_STORE_free(trust_store);
- Line 793: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_CRL_free(crl);
- Line 828: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: sk_DIST_POINT_pop_free(crldp, DIST_POINT_free);
- Line 902: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_STORE_CTX_free(chain_ctx);
- Line 943: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OCSP_REQUEST_free(req);
- Line 950: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(req_der);
- Line 982: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OCSP_BASICRESP_free(basic);
- Line 999: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OCSP_CERTID_free(lookup_id);
- Line 1005: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OCSP_BASICRESP_free(basic);
- Line 1037: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(issuer_cert);
- Line 1040: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_STORE_free(trust_store);
- Line 1042: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_email_free(ocsp_list);
- Line 1838: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (pe_signature == 0x00004550) { // "PE\0\0"
- Line 2034: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(pipefd[1]);
- Line 2050: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(pipefd[0]);
- Line 2114: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 2122: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 2125: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 2150: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);

### acceleration/graphics_backends.cpp
Total findings: 29

- Line 2618: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 5 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: fLon2[i] = static_cast<float>(lon2[i]);

        }

        const GL_GLsizeiptr cBytes = static_cast<GL_GLsizeiptr>(count * sizeof(float));

        GL_GLuint bufs[5] = {};

        pfnGlGenBuffers(5, bufs);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[0]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat2.data(), k_STREAM_COPY);
- Line 2620: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 5 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const GL_GLsizeiptr cBytes = static_cast<GL_GLsizeiptr>(count * sizeof(float));

        GL_GLuint bufs[5] = {};

        pfnGlGenBuffers(5, bufs);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[0]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat2.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[3]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon2.data(), k_STREAM_COPY);
- Line 2622: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 5 > array size 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: pfnGlGenBuffers(5, bufs);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[0]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat2.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[3]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon2.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[4]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, nullptr, k_DYNAMIC_COPY);
- Line 2624: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 5 > array size 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat2.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[3]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon2.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[4]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, nullptr, k_DYNAMIC_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, 0);

        for (GL_GLuint b = 0; b < 5; ++b)
- Line 2626: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 5 > array size 4
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon1.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLat2.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[3]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, fLon2.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[4]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, cBytes, nullptr, k_DYNAMIC_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, 0);

        for (GL_GLuint b = 0; b < 5; ++b)

            pfnGlBindBufferBase(k_SHADER_STORAGE_BUFFER, b, bufs[b]);

        pfnGlUseProgram(haversineProgram_);
- Line 2665: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 4 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const GL_GLsizeiptr polyBytes = static_cast<GL_GLsizeiptr>(numVerts * 2 * sizeof(float));

        const GL_GLsizeiptr resBytes  = static_cast<GL_GLsizeiptr>(numPoints * sizeof(GL_GLuint));



        GL_GLuint bufs[4] = {};

        pfnGlGenBuffers(4, bufs);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[0]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLat.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLon.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, polyBytes, fPoly.data(), k_STREAM_COPY);
- Line 2667: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 4 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: GL_GLuint bufs[4] = {};

        pfnGlGenBuffers(4, bufs);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[0]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLat.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLon.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, polyBytes, fPoly.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[3]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, resBytes, nullptr, k_DYNAMIC_COPY);
- Line 2669: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 4 > array size 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: pfnGlGenBuffers(4, bufs);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[0]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLat.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLon.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, polyBytes, fPoly.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[3]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, resBytes, nullptr, k_DYNAMIC_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, 0);

        for (GL_GLuint b = 0; b < 4; ++b)
- Line 2671: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 4 > array size 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLat.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[1]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, ptBytes, fPLon.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[2]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, polyBytes, fPoly.data(), k_STREAM_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, bufs[3]);

        pfnGlBufferData(k_SHADER_STORAGE_BUFFER, resBytes, nullptr, k_DYNAMIC_COPY);

        pfnGlBindBuffer(k_SHADER_STORAGE_BUFFER, 0);

        for (GL_GLuint b = 0; b < 4; ++b)

            pfnGlBindBufferBase(k_SHADER_STORAGE_BUFFER, b, bufs[b]);

        pfnGlUseProgram(pipProgram_);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #4206 feat(acceleration):
- Line 237: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: (memoryProps.memoryTypes[i].propertyFlags & flags) == flags)

                return i;

        }

        throw std::runtime_error("findMemoryType: no suitable type");

    }



    // ---- Shader module ------------------------------------------------
- Line 1894: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: dist[p * uNumVerts + v] = (v == startVerts[p]) ? 0.0 : 1e30;
- Line 891: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Vulkan Vector Backend' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md § "Vulkan Vector Backend"
- Line 910: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'DirectX Vector Backend' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md § "DirectX Vector Backend"
- Line 943: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::getCapabilities()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: BackendCapabilities DirectXVectorBackend::getCapabilities() const {
- Line 947: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::initialize()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool DirectXVectorBackend::initialize() {
- Line 964: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::shutdown()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void DirectXVectorBackend::shutdown() {}
- Line 2389: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'OpenGL Backend.' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §OpenGL Backend.
- Line 3898: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: path.push_back(static_cast<uint32_t>(cur));
- Line 273: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "[ShaderIntegrity] " << result.message << std::endl;
- Line 1226: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "[Vulkan] Initialized: " << impl_->deviceProps.deviceName << std::endl;
- Line 1227: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "[Vulkan] VK_KHR_buffer_device_address: "
- Line 2210: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<char> log(static_cast<size_t>(logLen));
- Line 2231: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<char> log(static_cast<size_t>(logLen));
- Line 2579: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<char> log(static_cast<size_t>(logLen));
- Line 2602: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Dispatch haversine shader; lat/lon inputs converted from double to float.
- Line 2645: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Dispatch PIP shader; inputs converted from double to float.
- Line 2889: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<char> log(static_cast<size_t>(logLen));
- Line 3217: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "[OpenGL] Initialized: " << impl_->rendererName_

### acceleration/oneapi_backend.cpp
Total findings: 29

- Line 72: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: queue_ = new sycl::queue(sycl::gpu_selector_v);
- Line 76: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 79: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 83: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 86: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto device = queue_->get_device();
- Line 131: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: queue_->memcpy(d_queries, queries, numQueries * dimension * sizeof(float)).wait();
- Line 132: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: queue_->memcpy(d_vectors, vectors, numVectors * dimension * sizeof(float)).wait();
- Line 139: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Launch kernel

            if (useL2) {

                // L2 Distance Kernel

                queue_->parallel_for(sycl::range<2>(numQueries, numVectors),

                    [=](sycl::id<2> idx) {

                        size_t q = idx[0];

                        size_t v = idx[1];

                        

                        float sum = 0.0f;

                        for (size_t d = 0; d < dimension; d++) {

                            float diff = d_queries[q * dimension + d] - d_vectors[v * dimension + d];
- Line 140: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 2 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (useL2) {

                // L2 Distance Kernel

                queue_->parallel_for(sycl::range<2>(numQueries, numVectors),

                    [=](sycl::id<2> idx) {

                        size_t q = idx[0];

                        size_t v = idx[1];

                        

                        float sum = 0.0f;

                        for (size_t d = 0; d < dimension; d++) {

                            float diff = d_queries[q * dimension + d] - d_vectors[v * dimension + d];

                            sum += diff * diff;
- Line 149: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: }).wait();
- Line 171: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: float cosineSim = dotProduct / (sycl::sqrt(normQ) * sycl::sqrt(normV) + 1e-8f);

                        d_distances[q * numVectors + v] = 1.0f - cosineSim;

                    }).wait();

            }

            

            // Copy results back
- Line 171: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: }).wait();
- Line 171: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: }).wait();
- Line 175: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            

            // Copy results back

            queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();

            

            // Cleanup

            sycl::free(d_queries, *queue_);
- Line 175: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();
- Line 175: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();
- Line 41: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: ~OneAPIVectorBackend() {
- Line 43: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete queue_;
- Line 105: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete queue_;
- Line 68: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool initialize() override {
- Line 178: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: sycl::free(d_queries, *queue_);
- Line 179: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: sycl::free(d_vectors, *queue_);
- Line 233: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'OneAPI Backend Activation' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §"OneAPI Backend Activation"
- Line 259: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool initialize() override { return false; }
- Line 89: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "OneAPI backend initialized successfully\n";
- Line 90: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Platform: " << platform.get_info<sycl::info::platform::name>() << "\n";
- Line 91: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Device: " << device.get_info<sycl::info::device::name>() << "\n";
- Line 92: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Max Compute Units: " << device.get_info<sycl::info::device::max_compute_units>() << "\n";
- Line 93: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Max Work Group Size: " << device.get_info<sycl::info::device::max_work_group_size>() << "\n";

### acceleration/hip_backend.cpp
Total findings: 28

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4618 feat(acceleration): Kernel ... (2026-04-13) | #4470 feat(acceleration):
- Line 1053: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t resultBytes = numPoints          * sizeof(uint8_t);
- Line 87: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 90: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < dim; i++) {
- Line 116: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float dotProduct = 0.0f;
- Line 117: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float normQuery = 0.0f;
- Line 118: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float normVector = 0.0f;
- Line 157: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float dotProduct = 0.0f;
- Line 160: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < dim; i++) {
- Line 324: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool initialized = false;
- Line 325: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int deviceId = 0;
- Line 333: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int occupancyTunedBlockSize = 256;
- Line 435: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "Device " << i << ": " << prop.name
- Line 508: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int minGridSize   = 0;
- Line 407: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "HIP Backend: Initializing..." << std::endl;
- Line 435: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Device " << i << ": " << prop.name
- Line 477: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "HIP Backend: Selected device " << impl_->deviceId
- Line 479: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Compute Units: " << impl_->deviceProps.multiProcessorCount << std::endl;
- Line 480: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Global Memory: " << (impl_->deviceProps.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;
- Line 481: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Warp Size: " << impl_->deviceProps.warpSize << std::endl;
- Line 482: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  GCN Arch: " << impl_->deviceProps.gcnArchName << std::endl;
- Line 483: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  ROCm Runtime: " << (runtimeVersion / 10000000) << "."
- Line 489: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Auto-detected Wave Size: " << impl_->config.waveSize << std::endl;
- Line 521: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Occupancy-tuned block size: " << impl_->occupancyTunedBlockSize << std::endl;
- Line 940: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "HIP Geo Backend initialized successfully:" << std::endl;
- Line 941: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Device: " << prop.name << std::endl;
- Line 942: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  GCN Arch: " << prop.gcnArchName << std::endl;
- Line 943: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Global Memory: " << (prop.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;

### acceleration/plugin_loader.cpp
Total findings: 13

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #3581 docs(plugins, promp
- Line 81: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
- Line 247: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &entry : fs::directory_iterator(directoryPath)) {
- Line 261: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "SECURITY: Skipping symlink that escapes plugin directory: " << entry.path() << std::en
- Line 293: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "Unloading plugin: " << pluginName << std::endl;
- Line 303: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "Unloading plugin: " << plugin.name << std::endl;
- Line 175: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "SECURITY: Plugin verification passed: " << libraryPath << std::endl;
- Line 213: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Loaded plugin: " << plugin->pluginName() << " v" << plugin->pluginVersion()
- Line 285: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Loaded " << loadedCount << " acceleration plugins from " << directoryPath << std::endl;
- Line 293: severity=LOW; category=endl_in_loop
  Description: std::endl in loop (causes unnecessary flush, use '\n')
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::cout << "Unloading plugin: " << pluginName << std::endl;
- Line 293: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Unloading plugin: " << pluginName << std::endl;
- Line 303: severity=LOW; category=endl_in_loop
  Description: std::endl in loop (causes unnecessary flush, use '\n')
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::cout << "Unloading plugin: " << plugin.name << std::endl;
- Line 303: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Unloading plugin: " << plugin.name << std::endl;

### acceleration/directx_backend_full.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3665 feat(acceleration): Impleme... (2026-03-12) | #417 [DOCS] CRITICAL: Cor
- Line 45: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: HRESULT _hr = (call); \
- Line 55: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: HRESULT _hr = (call); \
- Line 58: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: snprintf(_buf, sizeof(_buf), "DirectX error: HRESULT 0x%08X", \
- Line 89: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 122: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float dot = 0.0f, normQ = 0.0f, normV = 0.0f;
- Line 618: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::getCapabilities()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: BackendCapabilities DirectXVectorBackend::getCapabilities() const {
- Line 630: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::initialize()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool DirectXVectorBackend::initialize() {
- Line 642: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::shutdown()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void DirectXVectorBackend::shutdown() {
- Line 58: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(_buf, sizeof(_buf), "DirectX error: HRESULT 0x%08X", \
- Line 638: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "[DirectX] Initialized: " << impl_->deviceName() << std::endl;

### acceleration/opencl_backend.cpp
Total findings: 10

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2708 feat(acceleration): OpenCL ... (2026-03-12) | #417 [DOCS] CRITICAL: Cor
- Line 290: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: clSetKernelArg(kernel, 4, sizeof(unsigned int), &uNumVectors);
- Line 359: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'OpenCL Backend Activation' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §"OpenCL Backend Activation"
- Line 158: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "OpenCL backend initialized successfully" << std::endl;
- Line 159: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Platform: " << platformName << " (" << platformVersion << ")" << std::endl;
- Line 160: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Device: " << deviceName << std::endl;
- Line 161: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Device Version: " << deviceVersion << std::endl;
- Line 162: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Compute Units: " << computeUnits << std::endl;
- Line 163: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Global Memory: " << (globalMemSize / (1024*1024*1024)) << " GB" << std::endl;
- Line 205: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<char> log(logSize);

### acceleration/vllm_resource_manager.cpp
Total findings: 9

- Line 153: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (shared_future->wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
- Line 154: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: gpu_util = shared_future->get();
- Line 135: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 153: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (shared_future->wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
- Line 237: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 319: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 467: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 136: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: double max_util = 0.0;
- Line 137: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool got_any    = false;

### acceleration/zluda_backend.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3609 feat(acceleration): wire mi... (2026-03-12) | #3551 docs(chimera + acce
- Line 11: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ZLUDA: CUDA compatibility layer for AMD GPUs
- Line 190: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (zludaLib_) dlclose(zludaLib_);
- Line 243: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'ZLUDA Activation.' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §ZLUDA Activation.
- Line 133: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "ZLUDA Backend: Initializing..." << std::endl;
- Line 134: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "ZLUDA: CUDA compatibility layer for AMD GPUs" << std::endl;
- Line 165: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "ZLUDA: Found " << deviceCount << " AMD GPU(s)" << std::endl;
- Line 181: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "ZLUDA Backend: Successfully initialized" << std::endl;
- Line 182: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Note: ZLUDA allows running CUDA kernels on AMD GPUs" << std::endl;

### acceleration/cpu_backend_mt.cpp
Total findings: 6

- Line 68: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Multi-threaded CPU backend initialized\n";
- Line 69: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Threads: " << numThreads_ << "\n";
- Line 70: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  OpenMP: " << (THEMIS_HAS_OPENMP ? "Yes" : "No") << "\n";
- Line 72: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
- Line 74: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  SIMD: NEON\n";
- Line 76: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  SIMD: No\n";

### acceleration/cpu_backend_tbb.cpp
Total findings: 6

- Line 57: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Intel TBB CPU backend initialized\n";
- Line 58: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Threads: " << numThreads_ << "\n";
- Line 59: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  TBB Version: " << TBB_VERSION_MAJOR << "." << TBB_VERSION_MINOR << "\n";
- Line 61: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
- Line 63: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  SIMD: NEON\n";
- Line 65: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  SIMD: Scalar\n";

### acceleration/nccl_vector_backend.cpp
Total findings: 6

- Line 146: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 157: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 577: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'NCCL/RCCL Activation' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §"NCCL/RCCL Activation"
- Line 607: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 85: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "NCCL: Initializing with rank " << config.rank
- Line 114: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "NCCL: Initialization successful" << std::endl;

### acceleration/rccl_vector_backend.cpp
Total findings: 6

- Line 173: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 184: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 587: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'NCCL/RCCL Activation' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §"NCCL/RCCL Activation"
- Line 600: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 112: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "RCCL: Initializing with rank " << config.rank
- Line 141: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "RCCL: Initialization successful" << std::endl;

### acceleration/vulkan_backend_full.cpp
Total findings: 6

- Line 533: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: throw std::runtime_error("Failed to allocate buffer memory");
- Line 64: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Vulkan GLSL Compiler.' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §Vulkan GLSL Compiler.
- Line 185: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Vulkan GLSL Compiler.' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §Vulkan GLSL Compiler.
- Line 206: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Vulkan GLSL Compiler.' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §Vulkan GLSL Compiler.
- Line 260: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Validation layers requested but not available" << std::endl;
- Line 309: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Selected Vulkan device: " << ctx.deviceProps.deviceName << std::endl;

### acceleration/backend_registry.cpp
Total findings: 5

- Line 114: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Register OpenCL backend for broad hardware compatibility.
- Line 201: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 543: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return runtimeInitialized_.load(std::memory_order_acquire);
- Line 246: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 274: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: static T *selectTyped(const std::unordered_map<BackendType, RegisteredBackend> &index,

### acceleration/cpu_backend.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3466 docs(acceleration): Add IEE... (2026-03-12) | #3111 [geo] Implement run
- Line 101: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (a.second != b.second) {
- Line 229: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "[CPUGraph] batchShortestPath: negative weight " << raw_w << " on edge " << u << "→"
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: path.push_back(static_cast<uint32_t>(v));
- Line 97: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // happens when the same computational path is applied to equal inputs.

### acceleration/device_manager.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #4207 feat(acceleration):
- Line 223: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "  [" << (d.is_healthy ? "OK" : "!!") << "] " << d.name
- Line 220: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "[acceleration] Device capability probe — " << devices.size() << " device(s) found:" << std::endl;
- Line 223: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  [" << (d.is_healthy ? "OK" : "!!") << "] " << d.name
- Line 231: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "[acceleration] Best device: " << best.name << " (backend=" << static_cast<int>(best.backend_type)

### acceleration/shader_integrity.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #3609 feat(acceleration):
- Line 125: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return verify(name, reinterpret_cast<const uint8_t *>(spvWords.data()), spvWords.size() * sizeof(uin
- Line 202: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return sha256Hex(reinterpret_cast<const uint8_t *>(spvWords.data()), spvWords.size() * sizeof(uint32
- Line 189: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 192: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);

### acceleration/geo_acceleration_bridge.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #3609 feat(acceleration):
- Line 159: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: themis::geo::SpatialBatchInputs batch;
- Line 283: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: "invalid inputs (null pointer or < 3 polygon vertices)");
- Line 299: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: themis::geo::SpatialBatchInputs batch;

### acceleration/ai_hardware_dispatcher.cpp
Total findings: 3

- Line 255: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check precision compatibility
- Line 624: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Apple ANE Core ML Activation' that was not found in 'src/acceleration/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/acceleration/FUTURE_ENHANCEMENTS.md §"Apple ANE Core ML Activation"
- Line 664: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: infer_req.infer();

### acceleration/multi_gpu_backend.cpp
Total findings: 3

- Line 133: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "MultiGPUVectorBackend: sub-backend init failed for device " << shardDescs[i].deviceId
- Line 137: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "MultiGPUVectorBackend: warning — sub-backend init failed "
- Line 147: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "MultiGPUVectorBackend: initialised with " << shardDescs.size()

### acceleration/tensor_core_matmul.cpp
Total findings: 2

- Line 42: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (beta == 0.0f) {
- Line 44: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: } else if (beta != 1.0f) {

### acceleration/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### acceleration/compute_backend.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10)

### acceleration/vec_knn.cpp
Total findings: 1

- Line 342: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto oi = std::find(order_.begin(), order_.end(), k);

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
