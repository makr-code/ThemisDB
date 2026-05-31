# geo Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: geo
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 270
- Actionable Findings (Critical + High): 114
- Affected Files: 17

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 29 |
| High | 85 |
| Medium | 156 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 84 |
| performance_patterns | 71 |
| gpu_memory_safety | 50 |
| audit_logging | 15 |
| reliability | 13 |
| platform | 7 |
| memory | 6 |
| performance | 6 |
| security | 6 |
| exception_safety | 3 |
| observability | 3 |
| determinism | 2 |
| raii | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/geo/gpu_backend_production.cpp | 66 | 17 | 38 | 11 | 0 |
| src/geo/cpu_backend.cpp | 40 | 0 | 3 | 37 | 0 |
| src/geo/boost_cpu_exact_backend.cpp | 30 | 0 | 3 | 27 | 0 |
| src/geo/geo_clustering.cpp | 20 | 0 | 9 | 11 | 0 |
| src/geo/geo_rtree.cpp | 19 | 1 | 5 | 13 | 0 |
| src/geo/geo_json_geometry.cpp | 18 | 0 | 2 | 16 | 0 |
| src/geo/geo_faiss_knn.cpp | 14 | 0 | 4 | 10 | 0 |
| src/geo/gpu_backend_hip.cpp | 11 | 10 | 1 | 0 | 0 |
| src/geo/spatial_join.cpp | 11 | 1 | 7 | 3 | 0 |
| src/geo/temporal_spatial_query_builder.cpp | 9 | 0 | 8 | 1 | 0 |
| src/geo/gpu_backend_stub.cpp | 8 | 0 | 3 | 5 | 0 |
| src/geo/tile_server.cpp | 8 | 0 | 0 | 7 | 1 |
| src/geo/temporal_spatial_query.cpp | 6 | 0 | 0 | 6 | 0 |
| src/geo/device_detector.cpp | 4 | 0 | 1 | 3 | 0 |
| src/geo/rtree_cursor.cpp | 3 | 0 | 0 | 3 | 0 |
| src/geo/gpu_kernel_dispatcher_cpu.cpp | 2 | 0 | 0 | 2 | 0 |
| src/geo/raster.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/geo/gpu_backend_production.cpp
Total findings: 66

- Line 82: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: thread.join();
- Line 320: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // Device buffers are cached and grown on demand to amortise cudaMalloc cost.
  Confidence: band=very_high; score=0.99
- Line 461: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lon_in
  Context: cudaMemcpy(d_lon_in, &h_lon, sizeof(double), cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.99
- Line 461: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lon_in
  Context: cudaMemcpy(d_lon_in, &h_lon, sizeof(double), cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.99
- Line 461: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lon_in
  Context: cudaMemcpy(d_lon_in, &h_lon, sizeof(double), cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.99
- Line 462: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lat_in
  Context: cudaMemcpy(d_lat_in, &h_lat, sizeof(double), cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.99
- Line 462: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lat_in
  Context: cudaMemcpy(d_lat_in, &h_lat, sizeof(double), cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.99
- Line 464: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_x
  Context: cuda_batch_point_buffer_kernel<<<1, n_verts>>>(d_lon_in, d_lat_in, d_ring_x, d_ring_y, arc_points, d_lat, d_lon,
  Confidence: band=very_high; score=0.99
- Line 481: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_x
  Context: cudaMemcpy(h_ring_x.data(), d_ring_x, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.99
- Line 482: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_y
  Context: cudaMemcpy(h_ring_y.data(), d_ring_y, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.99
- Line 528: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_mbrs_a_
  Context: d_cached_mbrs_a_ = nullptr;
  Confidence: band=very_high; score=0.99
- Line 532: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_mbrs_b_
  Context: d_cached_mbrs_b_ = nullptr;
  Confidence: band=very_high; score=0.99
- Line 536: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_results_
  Context: d_cached_results_ = nullptr;
  Confidence: band=very_high; score=0.99
- Line 549: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: if ((e = cudaMalloc(&d_cached_mbrs_a_, mbr_sz)) != cudaSuccess
  Confidence: band=very_high; score=0.99
- Line 550: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: || (e = cudaMalloc(&d_cached_mbrs_b_, mbr_sz)) != cudaSuccess
  Confidence: band=very_high; score=0.99
- Line 551: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: || (e = cudaMalloc(&d_cached_results_, res_sz)) != cudaSuccess) {
  Confidence: band=very_high; score=0.99
- Line 553: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: THEMIS_WARN("CUDA cudaMalloc failed ({})", static_cast<int>(e));
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        // Parallel processing using multiple threads; each thread owns a disjoint', '        // index range of `out.mask` so no synchronisation is required on writes.', '        const size_t batch_size = (in.count + thread_count_ - 1) / thread_count_;', '        std::vector<std::thread> threads;', '        threads.reserve(thread_count_);']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        // Phase 1: dispatch pairwise MBR intersection kernel.', '        const int blockSize = 256;', '        const int gridSize  = (n + blockSize - 1) / blockSize;', '        cuda_pairwise_intersects_kernel<<<gridSize, blockSize>>>(d_cached_mbrs_a_, d_cached_mbrs_b_, d_cached_results_,', '                                                                 n);']
  Confidence: band=high; score=0.78
- Line 47: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: __global__ void cuda_batch_point_buffer_kernel(const double *lons, ///< [n] centre longitudes
- Line 320: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // Device buffers are cached and grown on demand to amortise cudaMalloc cost.
  Confidence: band=very_high; score=0.9
- Line 321: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 353: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t mbr_sz = static_cast<size_t>(n) * 4 * sizeof(double);
- Line 354: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t res_sz = static_cast<size_t>(n) * sizeof(uint8_t);
- Line 380: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(out.mask.data(), d_cached_results_, res_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 392: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchInputs candidates;
  Confidence: band=very_high; score=0.9
- Line 445: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 450: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 457: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 458: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_x);
  Confidence: band=very_high; score=0.9
- Line 462: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(d_lon_in, &h_lon, sizeof(double), cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 463: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(d_lat_in, &h_lat, sizeof(double), cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 471: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 472: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 473: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_x);
  Confidence: band=very_high; score=0.9
- Line 474: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_y);
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ring_x.data(), d_ring_x, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ring_y.data(), d_ring_y, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 484: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 485: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 486: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_x);
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_y);
  Confidence: band=very_high; score=0.9
- Line 528: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_cached_mbrs_a_);
  Confidence: band=very_high; score=0.9
- Line 532: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_cached_mbrs_b_);
  Confidence: band=very_high; score=0.9
- Line 536: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_cached_results_);
  Confidence: band=very_high; score=0.9
- Line 553: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: THEMIS_WARN("CUDA cudaMalloc failed ({})", static_cast<int>(e));
  Confidence: band=very_high; score=0.9
- Line 663: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 694: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t mbr_sz = static_cast<size_t>(n) * 4 * sizeof(double);
- Line 695: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t res_sz = static_cast<size_t>(n) * sizeof(uint8_t);
- Line 799: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchInputs candidates;
  Confidence: band=very_high; score=0.9
- Line 912: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 974: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &in, &out, start_idx, end_idx]() {
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidate_indices.push_back(static_cast<size_t>(i));
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidate_indices.push_back(static_cast<size_t>(i));
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.geoms_a.push_back(in.geoms_a[i]);
- Line 398: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.geoms_b.push_back(in.geoms_b[i]);
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.emplace_back(h_ring_x[static_cast<size_t>(i)], h_ring_y[static_cast<size_t>(i)]);
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(ring));
- Line 802: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidate_indices.push_back(static_cast<size_t>(i));
  Confidence: band=high; score=0.74
- Line 803: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidate_indices.push_back(static_cast<size_t>(i));
- Line 804: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.geoms_a.push_back(in.geoms_a[i]);
- Line 805: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.geoms_b.push_back(in.geoms_b[i]);

### src/geo/cpu_backend.cpp
Total findings: 40

- Line 267: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1092: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({cx + d_lon * std::cos(angle), cy + d_lat * std::sin(angle)});
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({cx + d_lon * std::cos(angle), cy + d_lat * std::sin(angle)});
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back(ring[0]); // close the ring
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e2.p1);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e2.p1);
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back({e1.p1.x + t * d1x, e1.p1.y + t * d1y});
- Line 509: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(cpuCircleRing(c.x, c.y, d_lat, d_lon, arc_points));
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ips.push_back({static_cast<double>(i) + t, static_cast<double>(j) + s,
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ips.push_back({static_cast<double>(i) + t, static_cast<double>(j) + s,
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ips.push_back({static_cast<double>(i) + t, static_cast<double>(j) + s,
- Line 697: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: A.push_back(v);
  Confidence: band=high; score=0.74
- Line 698: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: A.push_back(v);
- Line 706: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: B.push_back(v);
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: B.push_back(v);
- Line 789: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 789: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 790: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({v.x, v.y});
- Line 803: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({v.x, v.y});
- Line 853: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 853: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 854: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({v.x, v.y});
- Line 869: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({v.x, v.y});
- Line 879: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({v.x, v.y});
- Line 943: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.geometries.push_back(geom1);
  Confidence: band=high; score=0.74
- Line 944: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(geom1);
- Line 945: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(geom2);
- Line 957: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rings.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 958: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(r));
- Line 963: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(geom1);
- Line 964: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(geom2);
- Line 1016: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rings.push_back(ring1);
  Confidence: band=high; score=0.74
- Line 1017: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(ring1);
- Line 1018: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(ring2);
- Line 1032: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rings.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1033: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(r));
- Line 1055: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends_.push_back(std::move(b));
- Line 1123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/geo/boost_cpu_exact_backend.cpp
Total findings: 30

- Line 103: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) override {
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // For Polygon inputs uses boost::geometry::union_ which handles all
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // For Polygon inputs uses boost::geometry::difference.
  Confidence: band=very_high; score=0.9
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: poly.inners().push_back(hole);
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({bg::get<0>(p), bg::get<1>(p)});
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(ring));
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outer_ring.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: outer_ring.push_back({bg::get<0>(p), bg::get<1>(p)});
- Line 248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(outer_ring));
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(hole));
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(geom1);
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(geom2);
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.geometries.push_back(boostPolyToGeomInfo(p));
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(boostPolyToGeomInfo(p));
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.geometries.push_back(boostPolyToGeomInfo(p));
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col.geometries.push_back(boostPolyToGeomInfo(p));
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: outer.push_back({bg::get<0>(p), bg::get<1>(p)});
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(outer));
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.rings.push_back(std::move(hole));
- Line 369: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/geo/geo_clustering.cpp
Total findings: 20

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 70: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const std::size_t adj_sz   = n * n * sizeof(uint8_t);
- Line 169: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::vector<uint8_t> gpu_adj; // flat [n×n], empty on CPU path
- Line 304: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("kmeansCluster: k must be >= 1");
- Line 307: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("kmeansCluster: k (" + std::to_string(config.k)
- Line 412: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] float *d_dists  = nullptr;
- Line 413: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] uint32_t *d_idx = nullptr;
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbours.push_back(j);
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbours.push_back(j);
- Line 237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queue.push_back(nb);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queue.push_back(nb);
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queue.push_back(nb);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queue.push_back(nb);
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: valid_idx.push_back(i);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: valid_idx.push_back(i);

### src/geo/geo_rtree.cpp
Total findings: 19

- Line 143: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = entries.begin(); it != entries.end(); ++it) {
- Line 89: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = tree.qbegin(bgi::intersects(qbox)); it != tree.qend(); ++it) {
- Line 101: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = tree.qbegin(bgi::intersects(qbox)); it != tree.qend(); ++it) {
- Line 114: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: constexpr std::size_t kNodeOverhead = 4 * sizeof(double) + 32;  // box + key avg
- Line 198: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_INFO("GeoRTree::bulkLoad completed: entries={}, geo_index_bytes_allocated={}",
- Line 204: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: THEMIS_INFO("GeoRTree::insert: key={}, geo_index_bytes_allocated={}",
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(geometryBox(geom), key);
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(it->second);
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(it->second);
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back({geom.computeMBR(), key});
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back({geom.computeMBR(), key});
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back({geom.computeMBR(), key});
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.key);
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.key);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.key);
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.key);
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.key);

### src/geo/geo_json_geometry.cpp
Total findings: 18

- Line 77: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: vr.addError({"LONGITUDE_OUT_OF_RANGE", ctx + ": longitude " + std::to_string(c.x) + " outside [-180,
- Line 80: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: vr.addError({"LATITUDE_OUT_OF_RANGE", ctx + ": latitude " + std::to_string(c.y) + " outside [-90,90]
- Line 62: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ",";
- Line 197: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ",";
- Line 240: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ",";
- Line 244: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto polyJson = polygons_[i].toGeoJSON();
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto sub = polygons_[i].validate();
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ",";
- Line 298: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto sub = members_[i]->validate();
  Confidence: band=high; score=0.74

### src/geo/geo_faiss_knn.cpp
Total findings: 14

- Line 79: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::vector<float>       ecef_data; // flat [indexed_count × kDim]
- Line 79: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::vector<float>       ecef_data; // flat [indexed_count × kDim]
- Line 218: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: r.index  = dataset_map[candidates[i].second];
- Line 259: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->build(dataset);
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ecef_data.push_back(x);
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ecef_data.push_back(x);
- Line 103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ecef_data.push_back(y);
- Line 104: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ecef_data.push_back(z);
- Line 105: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dataset_map.push_back(i);
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(r);
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(r);
- Line 239: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(c);
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(c);

### src/geo/gpu_backend_hip.cpp
Total findings: 11

- Line 90: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats
  Context: if ((e = hipMemcpy(d_lats, point_lats,     pts_sz,  hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 92: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_poly
  Context: (e = hipMemcpy(d_poly, polygon_coords, poly_sz, hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 102: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats
  Context: d_lats, d_lons, numPoints,
  Confidence: band=very_high; score=0.99
- Line 103: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_poly
  Context: d_poly, numPolygonVertices,
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats1
  Context: if ((e = hipMemcpy(d_lats1, lats1, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats2
  Context: (e = hipMemcpy(d_lats2, lats2, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 174: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_out
  Context: (e = hipMemset(d_out, 0, out_sz))                                 != hipSuccess) {
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats1
  Context: d_lats1, d_lons1, d_lats2, d_lons2,
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats2
  Context: d_lats1, d_lons1, d_lats2, d_lons2,
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_out
  Context: d_out, count, formula, nullptr);
  Confidence: band=very_high; score=0.99
- Line 71: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t out_sz  = static_cast<size_t>(numPoints) * sizeof(uint8_t);

### src/geo/spatial_join.cpp
Total findings: 11

- Line 83: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = inner_key_idx.find(key_b);
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = inner_key_idx.find(key_b);
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = inner_key_idx.find(key_b);
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = inner_key_idx.find(key_b);
- Line 154: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (outer_idx >= outer_ptr->size()) {
- Line 169: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while (outer_idx < outer_ptr->size()) {
- Line 181: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const Coordinate centroid_b = geometryCentroid((*inner_ptr)[it->second].second);
- Line 198: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (outer_idx < outer_ptr->size()) {
- Line 61: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> inner_key_idx;
  Confidence: band=medium; score=0.66
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({key_a, key_b, dist});
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({key_a, key_b, dist});

### src/geo/temporal_spatial_query_builder.cpp
Total findings: 9

- Line 27: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {
- Line 27: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {
  Confidence: band=very_high; score=0.9
- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: spatial_match = spatial_.predicate->matches(ref, point);
- Line 103: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("duringInterval: end must be >= start");
- Line 123: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("slidingWindow: width_ms must be > 0");
- Line 134: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("withGeoField: field name must not be empty");
- Line 142: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("TemporalSpatialQueryBuilder::build(): temporal constraint not set; "
- Line 146: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("TemporalSpatialQueryBuilder::build(): spatial constraint not set; "
- Line 27: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {
  Confidence: band=high; score=0.74

### src/geo/gpu_backend_stub.cpp
Total findings: 8

- Line 218: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 590: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: static bool isAllPointsVsPolygon(const SpatialBatchInputs &in, std::size_t n) noexcept {
  Confidence: band=very_high; score=0.9
- Line 609: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: GpuKernelDispatcher::ContainmentResult tryGpuContainmentDispatch(const SpatialBatchInputs &in, std::size_t n) {
  Confidence: band=very_high; score=0.9
- Line 632: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly_coords.push_back(v.x);
  Confidence: band=high; score=0.74
- Line 633: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: poly_coords.push_back(v.x);
- Line 634: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: poly_coords.push_back(v.y);
- Line 676: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 678: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";

### src/geo/tile_server.cpp
Total findings: 8

- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(TileCoord{x, y, zoom});
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(TileCoord{x, y, zoom});
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(TileCoord{x, y, zoom});
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.features.push_back(std::move(feat));
- Line 60: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double y_f   = (1.0 - std::log(std::tan(lat_r) + 1.0 / std::cos(lat_r)) / kTilePi) / 2.0 * n;
  Confidence: band=medium; score=0.6

### src/geo/temporal_spatial_query.cpp
Total findings: 6

- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(row.key, std::move(*geom));
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(row));
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(row));
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(std::move(row), dist);
  Confidence: band=high; score=0.74

### src/geo/device_detector.cpp
Total findings: 4

- Line 143: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (best == nullptr || cap.device.free_vram_bytes > best->device.free_vram_bytes) {
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(Assess(d));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(Assess(d));
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(MakeCpuFallbackCapability());

### src/geo/rtree_cursor.cpp
Total findings: 3

- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hits.push_back({key, geom, 0.0});
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({key, geom, dist});
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back({key, geom, dist});

### src/geo/gpu_kernel_dispatcher_cpu.cpp
Total findings: 2

- Line 83: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 108: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/geo/raster.cpp
Total findings: 1

- Line 62: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return v == no_data_value;
  Confidence: band=very_high; score=0.9

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
