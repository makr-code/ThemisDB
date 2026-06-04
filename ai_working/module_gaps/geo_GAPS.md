# geo Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: geo
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 141
- Actionable Findings (Critical + High): 82
- Affected Files: 17

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 25 |
| High | 57 |
| Medium | 41 |
| Low | 18 |

## Category Summary

| Category | Count |
|---|---:|
| unchecked_cuda_call | 24 |
| use_after_free_gpu | 17 |
| copy_overhead | 15 |
| hardcoded_output | 15 |
| string_concat_loop | 10 |
| uninitialized_access | 10 |
| size_assumption | 7 |
| gpu_memory_leak | 5 |
| primitive_no_volatile | 4 |
| stale_doc_section_reference | 4 |
| null_dereference | 3 |
| resource_leaked_in_exception | 3 |
| unnecessary_copy | 3 |
| arithmetic_overflow | 2 |
| db_connection_leak | 2 |
| module_doc_linkset_drift | 2 |
| range_temporary | 2 |
| blocking_no_timeout | 1 |
| fp_exact_comparison | 1 |
| generic_catch | 1 |
| missing_latency_metric | 1 |
| missing_trace_point | 1 |
| missing_vector_reserve | 1 |
| no_timeout | 1 |
| o_n_squared | 1 |
| pointer_arithmetic_unbounded | 1 |
| thread_join_no_timeout | 1 |
| uncaught_exception | 1 |
| unordered_container_iter | 1 |
| unstructured_log | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| geo/gpu_backend_production.cpp | 59 | 16 | 30 | 6 | 7 |
| geo/cpu_backend.cpp | 14 | 0 | 2 | 10 | 2 |
| geo/geo_json_geometry.cpp | 13 | 0 | 2 | 11 | 0 |
| geo/gpu_backend_hip.cpp | 10 | 9 | 1 | 0 | 0 |
| geo/geo_rtree.cpp | 8 | 0 | 6 | 2 | 0 |
| geo/boost_cpu_exact_backend.cpp | 7 | 0 | 1 | 3 | 3 |
| geo/gpu_backend_stub.cpp | 7 | 0 | 1 | 3 | 3 |
| geo/spatial_join.cpp | 7 | 0 | 5 | 2 | 0 |
| geo/geo_clustering.cpp | 5 | 0 | 5 | 0 | 0 |
| geo/device_detector.cpp | 2 | 0 | 1 | 1 | 0 |
| geo/geo_faiss_knn.cpp | 2 | 0 | 1 | 1 | 0 |
| geo/temporal_spatial_query_builder.cpp | 2 | 0 | 1 | 1 | 0 |
| geo/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| geo/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| geo/gpu_kernel_dispatcher_cpu.cpp | 1 | 0 | 0 | 1 | 0 |
| geo/raster.cpp | 1 | 0 | 1 | 0 | 0 |
| geo/tile_server.cpp | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### geo/gpu_backend_production.cpp
Total findings: 59

- Line 83: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        for (auto &thread : threads) {

            thread.join();

        }



        return out;
- Line 83: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: thread.join();
- Line 83: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 321: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // Device buffers are cached and grown on demand to amortise cudaMalloc cost.
- Line 462: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_lon_in
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(d_lon_in, &h_lon, sizeof(double), cudaMemcpyHostToDevice);
- Line 463: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_lat_in
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(d_lat_in, &h_lat, sizeof(double), cudaMemcpyHostToDevice);
- Line 465: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_x
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cuda_batch_point_buffer_kernel<<<1, n_verts>>>(d_lon_in, d_lat_in, d_ring_x, d_ring_y, arc_points, d_lat, d_lon,
- Line 482: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_x
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(h_ring_x.data(), d_ring_x, buf_sz, cudaMemcpyDeviceToHost);
- Line 483: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_y
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(h_ring_y.data(), d_ring_y, buf_sz, cudaMemcpyDeviceToHost);
- Line 529: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_mbrs_a_
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_cached_mbrs_a_ = nullptr;
- Line 533: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_mbrs_b_
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_cached_mbrs_b_ = nullptr;
- Line 537: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_results_
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_cached_results_ = nullptr;
- Line 550: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: if ((e = cudaMalloc(&d_cached_mbrs_a_, mbr_sz)) != cudaSuccess
- Line 551: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: || (e = cudaMalloc(&d_cached_mbrs_b_, mbr_sz)) != cudaSuccess
- Line 552: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: || (e = cudaMalloc(&d_cached_results_, res_sz)) != cudaSuccess) {
- Line 554: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: THEMIS_WARN("CUDA cudaMalloc failed ({})", static_cast<int>(e));
- Line 63: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        // Parallel processing using multiple threads; each thread owns a disjoint', '        // index range of `out.mask` so no synchronisation is required on writes.', '        const size_t batch_size = (in.count + thread_count_ - 1) / thread_count_;', '        std::vector<std::thread> threads;', '        threads.reserve(thread_count_);']
- Line 321: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // Device buffers are cached and grown on demand to amortise cudaMalloc cost.
- Line 354: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t mbr_sz = static_cast<size_t>(n) * 4 * sizeof(double);
- Line 355: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t res_sz = static_cast<size_t>(n) * sizeof(uint8_t);
- Line 375: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        // Phase 1: dispatch pairwise MBR intersection kernel.', '        const int blockSize = 256;', '        const int gridSize  = (n + blockSize - 1) / blockSize;', '        cuda_pairwise_intersects_kernel<<<gridSize, blockSize>>>(d_cached_mbrs_a_, d_cached_mbrs_b_, d_cached_results_,', '                                                                 n);']
- Line 381: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpy(out.mask.data(), d_cached_results_, res_sz, cudaMemcpyDeviceToHost);
- Line 446: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lon_in);
- Line 451: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lon_in);
- Line 452: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lat_in);
- Line 457: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lon_in);
- Line 458: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lat_in);
- Line 459: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_ring_x);
- Line 463: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(d_lon_in, &h_lon, sizeof(double), cudaMemcpyHostToDevice);
- Line 464: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(d_lat_in, &h_lat, sizeof(double), cudaMemcpyHostToDevice);
- Line 472: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lon_in);
- Line 473: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lat_in);
- Line 474: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_ring_x);
- Line 475: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_ring_y);
- Line 483: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(h_ring_x.data(), d_ring_x, buf_sz, cudaMemcpyDeviceToHost);
- Line 484: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpy(h_ring_y.data(), d_ring_y, buf_sz, cudaMemcpyDeviceToHost);
- Line 485: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lon_in);
- Line 486: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_lat_in);
- Line 487: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_ring_x);
- Line 488: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_ring_y);
- Line 529: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_cached_mbrs_a_);
- Line 533: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_cached_mbrs_b_);
- Line 537: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_cached_results_);
- Line 554: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: THEMIS_WARN("CUDA cudaMalloc failed ({})", static_cast<int>(e));
- Line 695: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t mbr_sz = static_cast<size_t>(n) * 4 * sizeof(double);
- Line 696: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t res_sz = static_cast<size_t>(n) * sizeof(uint8_t);
- Line 145: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool inside = false;
- Line 284: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: const double angle = 2.0 * 3.14159265358979323846 * vtx / arc_points;
- Line 395: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < n; ++i) {
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidate_indices.push_back(static_cast<size_t>(i));
- Line 802: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < n; ++i) {
- Line 804: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidate_indices.push_back(static_cast<size_t>(i));
- Line 48: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
- Line 322: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
- Line 393: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchInputs candidates;
- Line 664: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
- Line 800: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchInputs candidates;
- Line 913: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
- Line 975: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {

### geo/cpu_backend.cpp
Total findings: 14

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4139 feat(geo): Implement CUDA a... (2026-03-12) | #3466 docs(acceleration):
- Line 465: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: lambda

                = L

                  + (1.0 - C) * f * sinAlpha

                        * (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));



            if (std::abs(lambda - lambda_prev) <= kTol) {

                converged = true;
- Line 45: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Boost.Geometry Integration' that was not found in 'src/geo/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/geo/FUTURE_ENHANCEMENTS.md §"Boost.Geometry Integration"
- Line 685: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ips.push_back({static_cast<double>(i) + t, static_cast<double>(j) + s,
- Line 791: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back({v.x, v.y});
- Line 804: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back({v.x, v.y});
- Line 814: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back({v.x, v.y});
- Line 824: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back(ring[0]);
- Line 855: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back({v.x, v.y});
- Line 870: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back({v.x, v.y});
- Line 880: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back({v.x, v.y});
- Line 889: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back(ring[0]);
- Line 268: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
- Line 1093: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {

### geo/geo_json_geometry.cpp
Total findings: 13

- Line 78: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vr.addError({"LONGITUDE_OUT_OF_RANGE", ctx + ": longitude " + std::to_string(c.x) + " outside [-180,
- Line 81: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vr.addError({"LATITUDE_OUT_OF_RANGE", ctx + ": latitude " + std::to_string(c.y) + " outside [-90,90]
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += ",";
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ",";
- Line 198: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += ",";
- Line 199: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ",";
- Line 241: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += ",";
- Line 242: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ",";
- Line 245: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto polyJson = polygons_[i].toGeoJSON();
- Line 259: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto sub = polygons_[i].validate();
- Line 287: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += ",";
- Line 288: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ",";
- Line 299: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto sub = members_[i]->validate();

### geo/gpu_backend_hip.cpp
Total findings: 10

- Line 91: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: if ((e = hipMemcpy(d_lats, point_lats,     pts_sz,  hipMemcpyHostToDevice)) != hipSuccess ||
- Line 93: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_poly
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: (e = hipMemcpy(d_poly, polygon_coords, poly_sz, hipMemcpyHostToDevice)) != hipSuccess ||
- Line 103: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_lats, d_lons, numPoints,
- Line 104: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_poly
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_poly, numPolygonVertices,
- Line 171: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: if ((e = hipMemcpy(d_lats1, lats1, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
- Line 173: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: (e = hipMemcpy(d_lats2, lats2, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
- Line 175: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_out
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: (e = hipMemset(d_out, 0, out_sz))                                 != hipSuccess) {
- Line 184: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_lats1, d_lons1, d_lats2, d_lons2,
- Line 185: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_out
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: d_out, count, formula, nullptr);
- Line 72: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t out_sz  = static_cast<size_t>(numPoints) * sizeof(uint8_t);

### geo/geo_rtree.cpp
Total findings: 8

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4145 feat(geo): Add SpatialIndex... (2026-03-13) | #3622 feat(geo): Build sy
- Line 90: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = tree.qbegin(bgi::intersects(qbox)); it != tree.qend(); ++it) {
- Line 102: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = tree.qbegin(bgi::intersects(qbox)); it != tree.qend(); ++it) {
- Line 115: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr std::size_t kNodeOverhead = 4 * sizeof(double) + 32;  // box + key avg
- Line 199: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("GeoRTree::bulkLoad completed: entries={}, geo_index_bytes_allocated={}",
- Line 205: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("GeoRTree::insert: key={}, geo_index_bytes_allocated={}",
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(e.key);
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(e.key);

### geo/boost_cpu_exact_backend.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4145 feat(geo): Add SpatialIndex... (2026-03-13) | #4139 feat(geo): Implemen
- Line 232: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ring.push_back({bg::get<0>(p), bg::get<1>(p)});
- Line 370: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& ex) {

        // Log to stderr - avoid logger during static init

        std::cerr << "WARNING: Boost geometry backend registration failed: " << ex.what() << std::endl;

    } catch (...) {

        std::cerr << "WARNING: Boost geometry backend registration failed with unknown exception" << std::endl;

    }

}
- Line 370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 104: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) override {
- Line 268: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // For Polygon inputs uses boost::geometry::union_ which handles all
- Line 305: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // For Polygon inputs uses boost::geometry::difference.

### geo/gpu_backend_stub.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3111 [geo] Implement runtime GPU... (2026-03-12) | #3091 [geo] Fix circuit-b
- Line 584: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'CUDA Geospatial Kernels' that was not found in 'src/geo/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/geo/FUTURE_ENHANCEMENTS.md §"CUDA Geospatial Kernels"
- Line 677: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\"";
- Line 679: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\\";
- Line 219: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
- Line 591: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: static bool isAllPointsVsPolygon(const SpatialBatchInputs &in, std::size_t n) noexcept {
- Line 610: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: GpuKernelDispatcher::ContainmentResult tryGpuContainmentDispatch(const SpatialBatchInputs &in, std::size_t n) {

### geo/spatial_join.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4176 feat(geo): Spatial JOIN Sup... (2026-03-13) | #2978 [geo] Implement spa
- Line 83: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = inner_key_idx.find(key_b);
- Line 155: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (outer_idx >= outer_ptr->size()) {
- Line 170: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: while (outer_idx < outer_ptr->size()) {
- Line 199: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (outer_idx < outer_ptr->size()) {
- Line 62: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::size_t> inner_key_idx;
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back({key_a, key_b, dist});

### geo/geo_clustering.cpp
Total findings: 5

- Line 71: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const std::size_t adj_sz   = n * n * sizeof(uint8_t);
- Line 170: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::vector<uint8_t> gpu_adj; // flat [n×n], empty on CPU path
- Line 231: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 345: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 522: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### geo/device_detector.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3111 [geo] Implement runtime GPU... (2026-03-12) | #3091 [geo] Fix circuit-b
- Line 47: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'CUDA Geospatial Kernels' that was not found in 'src/geo/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/geo/FUTURE_ENHANCEMENTS.md §"CUDA Geospatial Kernels"

### geo/geo_faiss_knn.cpp
Total findings: 2

- Line 80: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::vector<float>       ecef_data; // flat [indexed_count × kDim]
- Line 102: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: ecef_data.push_back(x);

### geo/temporal_spatial_query_builder.cpp
Total findings: 2

- Line 28: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {
- Line 28: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {

### geo/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### geo/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### geo/gpu_kernel_dispatcher_cpu.cpp
Total findings: 1

- Line 25: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'CUDA Geospatial Kernels' that was not found in 'src/geo/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/geo/FUTURE_ENHANCEMENTS.md §"CUDA Geospatial Kernels"

### geo/raster.cpp
Total findings: 1

- Line 63: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: return v == no_data_value;

### geo/tile_server.cpp
Total findings: 1

- Line 61: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: const double y_f   = (1.0 - std::log(std::tan(lat_r) + 1.0 / std::cos(lat_r)) / kTilePi) / 2.0 * n;

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
