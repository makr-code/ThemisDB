# geo Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: geo
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 133
- Actionable Findings (Critical + High): 60
- Affected Files: 17

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 21 |
| High | 39 |
| Medium | 72 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 70 |
| gpu_memory_safety | 50 |
| container | 37 |
| audit_logging | 15 |
| platform | 7 |
| performance | 6 |
| reliability | 6 |
| memory | 4 |
| exception_safety | 3 |
| observability | 3 |
| security | 3 |
| determinism | 2 |
| raii | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/geo/gpu_backend_production.cpp | 44 | 11 | 29 | 4 | 0 |
| src/geo/cpu_backend.cpp | 16 | 0 | 2 | 14 | 0 |
| src/geo/boost_cpu_exact_backend.cpp | 14 | 0 | 3 | 11 | 0 |
| src/geo/geo_json_geometry.cpp | 12 | 0 | 0 | 12 | 0 |
| src/geo/gpu_backend_hip.cpp | 10 | 10 | 0 | 0 | 0 |
| src/geo/geo_rtree.cpp | 7 | 0 | 0 | 7 | 0 |
| src/geo/geo_clustering.cpp | 6 | 0 | 0 | 6 | 0 |
| src/geo/tile_server.cpp | 6 | 0 | 0 | 5 | 1 |
| src/geo/gpu_backend_stub.cpp | 4 | 0 | 3 | 1 | 0 |
| src/geo/temporal_spatial_query.cpp | 4 | 0 | 0 | 4 | 0 |
| src/geo/geo_faiss_knn.cpp | 3 | 0 | 0 | 3 | 0 |
| src/geo/spatial_join.cpp | 2 | 0 | 0 | 2 | 0 |
| src/geo/temporal_spatial_query_builder.cpp | 2 | 0 | 1 | 1 | 0 |
| src/geo/device_detector.cpp | 1 | 0 | 0 | 1 | 0 |
| src/geo/raster.cpp | 1 | 0 | 1 | 0 | 0 |
| src/geo/rtree_cursor.cpp | 1 | 0 | 0 | 1 | 0 |
| src/geo/gpu_kernel_dispatcher_cpu.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/geo/gpu_backend_production.cpp
Total findings: 44

- Line 321: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // Device buffers are cached and grown on demand to amortise cudaMalloc cost.
  Confidence: band=very_high; score=0.99
- Line 465: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_x
  Context: cuda_batch_point_buffer_kernel<<<1, n_verts>>>(d_lon_in, d_lat_in, d_ring_x, d_ring_y, arc_points, d_lat, d_lon,
  Confidence: band=very_high; score=0.99
- Line 482: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_x
  Context: cudaMemcpy(h_ring_x.data(), d_ring_x, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.99
- Line 483: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_ring_y
  Context: cudaMemcpy(h_ring_y.data(), d_ring_y, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.99
- Line 529: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_mbrs_a_
  Context: d_cached_mbrs_a_ = nullptr;
  Confidence: band=very_high; score=0.99
- Line 533: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_mbrs_b_
  Context: d_cached_mbrs_b_ = nullptr;
  Confidence: band=very_high; score=0.99
- Line 537: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_cached_results_
  Context: d_cached_results_ = nullptr;
  Confidence: band=very_high; score=0.99
- Line 550: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: if ((e = cudaMalloc(&d_cached_mbrs_a_, mbr_sz)) != cudaSuccess
  Confidence: band=very_high; score=0.99
- Line 551: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: || (e = cudaMalloc(&d_cached_mbrs_b_, mbr_sz)) != cudaSuccess
  Confidence: band=very_high; score=0.99
- Line 552: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: || (e = cudaMalloc(&d_cached_results_, res_sz)) != cudaSuccess) {
  Confidence: band=very_high; score=0.99
- Line 554: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: THEMIS_WARN("CUDA cudaMalloc failed ({})", static_cast<int>(e));
  Confidence: band=very_high; score=0.99
- Line 48: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 321: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // Device buffers are cached and grown on demand to amortise cudaMalloc cost.
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 381: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(out.mask.data(), d_cached_results_, res_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 393: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchInputs candidates;
  Confidence: band=very_high; score=0.9
- Line 446: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 452: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 457: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 458: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 459: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_x);
  Confidence: band=very_high; score=0.9
- Line 472: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 473: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 474: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_x);
  Confidence: band=very_high; score=0.9
- Line 475: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_y);
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ring_x.data(), d_ring_x, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 484: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ring_y.data(), d_ring_y, buf_sz, cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 485: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lon_in);
  Confidence: band=very_high; score=0.9
- Line 486: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_lat_in);
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_x);
  Confidence: band=very_high; score=0.9
- Line 488: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_ring_y);
  Confidence: band=very_high; score=0.9
- Line 529: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_cached_mbrs_a_);
  Confidence: band=very_high; score=0.9
- Line 533: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_cached_mbrs_b_);
  Confidence: band=very_high; score=0.9
- Line 537: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_cached_results_);
  Confidence: band=very_high; score=0.9
- Line 554: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: THEMIS_WARN("CUDA cudaMalloc failed ({})", static_cast<int>(e));
  Confidence: band=very_high; score=0.9
- Line 664: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 800: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchInputs candidates;
  Confidence: band=very_high; score=0.9
- Line 913: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 975: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([this, &in, &out, start_idx, end_idx]() {
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidate_indices.push_back(static_cast<size_t>(i));
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.emplace_back(h_ring_x[static_cast<size_t>(i)], h_ring_y[static_cast<size_t>(i)]);
  Confidence: band=high; score=0.74
- Line 803: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidate_indices.push_back(static_cast<size_t>(i));
  Confidence: band=high; score=0.74

### src/geo/cpu_backend.cpp
Total findings: 16

- Line 268: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 1093: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({cx + d_lon * std::cos(angle), cy + d_lat * std::sin(angle)});
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e2.p1);
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ips.push_back({static_cast<double>(i) + t, static_cast<double>(j) + s,
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ips.push_back({static_cast<double>(i) + t, static_cast<double>(j) + s,
  Confidence: band=high; score=0.74
- Line 698: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: A.push_back(v);
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: B.push_back(v);
  Confidence: band=high; score=0.74
- Line 790: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 790: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 854: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 854: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({v.x, v.y});
  Confidence: band=high; score=0.74
- Line 944: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.geometries.push_back(geom1);
  Confidence: band=high; score=0.74
- Line 958: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rings.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 1017: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rings.push_back(ring1);
  Confidence: band=high; score=0.74
- Line 1033: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.rings.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/geo/boost_cpu_exact_backend.cpp
Total findings: 14

- Line 104: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) override {
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // For Polygon inputs uses boost::geometry::union_ which handles all
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // For Polygon inputs uses boost::geometry::difference.
  Confidence: band=very_high; score=0.9
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly.inners().push_back(hole);
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ring.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outer_ring.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.geometries.push_back(boostPolyToGeomInfo(p));
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col.geometries.push_back(boostPolyToGeomInfo(p));
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hole.push_back({bg::get<0>(p), bg::get<1>(p)});
  Confidence: band=high; score=0.74

### src/geo/geo_json_geometry.cpp
Total findings: 12

- Line 63: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto polyJson = polygons_[i].toGeoJSON();
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto sub = polygons_[i].validate();
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ",";
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto sub = members_[i]->validate();
  Confidence: band=high; score=0.74

### src/geo/gpu_backend_hip.cpp
Total findings: 10

- Line 91: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats
  Context: if ((e = hipMemcpy(d_lats, point_lats,     pts_sz,  hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 93: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_poly
  Context: (e = hipMemcpy(d_poly, polygon_coords, poly_sz, hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 103: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats
  Context: d_lats, d_lons, numPoints,
  Confidence: band=very_high; score=0.99
- Line 104: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_poly
  Context: d_poly, numPolygonVertices,
  Confidence: band=very_high; score=0.99
- Line 171: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats1
  Context: if ((e = hipMemcpy(d_lats1, lats1, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats2
  Context: (e = hipMemcpy(d_lats2, lats2, coord_sz, hipMemcpyHostToDevice)) != hipSuccess ||
  Confidence: band=very_high; score=0.99
- Line 175: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_out
  Context: (e = hipMemset(d_out, 0, out_sz))                                 != hipSuccess) {
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats1
  Context: d_lats1, d_lons1, d_lats2, d_lons2,
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_lats2
  Context: d_lats1, d_lons1, d_lats2, d_lons2,
  Confidence: band=very_high; score=0.99
- Line 185: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_out
  Context: d_out, count, formula, nullptr);
  Confidence: band=very_high; score=0.99

### src/geo/geo_rtree.cpp
Total findings: 7

- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(geometryBox(geom), key);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(it->second);
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back({geom.computeMBR(), key});
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.key);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.key);
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.key);
  Confidence: band=high; score=0.74

### src/geo/geo_clustering.cpp
Total findings: 6

- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbours.push_back(j);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queue.push_back(nb);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queue.push_back(nb);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: valid_idx.push_back(i);
  Confidence: band=high; score=0.74

### src/geo/tile_server.cpp
Total findings: 6

- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(TileCoord{x, y, zoom});
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(TileCoord{x, y, zoom});
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
  Confidence: band=high; score=0.74
- Line 61: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const double y_f   = (1.0 - std::log(std::tan(lat_r) + 1.0 / std::cos(lat_r)) / kTilePi) / 2.0 * n;
  Confidence: band=medium; score=0.6

### src/geo/gpu_backend_stub.cpp
Total findings: 4

- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: SpatialBatchResults batchIntersects(const SpatialBatchInputs &in) override {
  Confidence: band=very_high; score=0.9
- Line 591: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: static bool isAllPointsVsPolygon(const SpatialBatchInputs &in, std::size_t n) noexcept {
  Confidence: band=very_high; score=0.9
- Line 610: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: GpuKernelDispatcher::ContainmentResult tryGpuContainmentDispatch(const SpatialBatchInputs &in, std::size_t n) {
  Confidence: band=very_high; score=0.9
- Line 633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: poly_coords.push_back(v.x);
  Confidence: band=high; score=0.74

### src/geo/temporal_spatial_query.cpp
Total findings: 4

- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(row.key, std::move(*geom));
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(row));
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(std::move(row), dist);
  Confidence: band=high; score=0.74

### src/geo/geo_faiss_knn.cpp
Total findings: 3

- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ecef_data.push_back(x);
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(r);
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(c);
  Confidence: band=high; score=0.74

### src/geo/spatial_join.cpp
Total findings: 2

- Line 62: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> inner_key_idx;
  Confidence: band=medium; score=0.66
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({key_a, key_b, dist});
  Confidence: band=high; score=0.74

### src/geo/temporal_spatial_query_builder.cpp
Total findings: 2

- Line 28: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {
  Confidence: band=very_high; score=0.9
- Line 28: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: BuiltTemporalSpatialQuery::execute(const themisdb::temporal::SystemVersionedTable &table) const {
  Confidence: band=high; score=0.74

### src/geo/device_detector.cpp
Total findings: 1

- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(Assess(d));
  Confidence: band=high; score=0.74

### src/geo/raster.cpp
Total findings: 1

- Line 63: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return v == no_data_value;
  Confidence: band=very_high; score=0.9

### src/geo/rtree_cursor.cpp
Total findings: 1

- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({key, geom, dist});
  Confidence: band=high; score=0.74

### src/geo/gpu_kernel_dispatcher_cpu.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
