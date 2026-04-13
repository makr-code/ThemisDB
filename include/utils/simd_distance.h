/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            simd_distance.h                                    ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:28:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     68                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9e61c0defa  2026-02-27  audit: fix Stubs:1 annotations, update line counts, add m... ║
    • 54593e02cb  2026-02-27  feat(performance): AVX-512 SIMD path for vector distance ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstddef>

namespace themis {
namespace simd {

// Compute L2 distance (Euclidean) between two float vectors of length dim.
// Returns sqrt(sum((a[i]-b[i])^2)). Uses SIMD when available:
// - x86_64: AVX512 or AVX2
// - ARM64/AArch64: NEON
// Falls back to portable scalar implementation if no SIMD available.
float l2_distance(const float* a, const float* b, std::size_t dim);

// Compute squared L2 distance (without sqrt). Useful for ranking when only
// relative ordering is required. Also SIMD-optimized when available.
float l2_distance_sq(const float* a, const float* b, std::size_t dim);

// Batch compute squared L2 distances (without sqrt) from a single query vector
// to multiple database vectors. More efficient than calling l2_distance_sq in a
// loop due to better cache utilization. Callers who need true Euclidean
// distances should apply sqrt to each output value.
// query: single query vector of length dim
// database: n database vectors stored contiguously (n * dim floats)
// n: number of database vectors
// dim: vector dimension
// distances: output array of n squared distances (must be pre-allocated)
void batch_l2_distance_sq(const float* query, const float* database, 
                          std::size_t n, std::size_t dim, float* distances);

// Compute inner product (dot product) between two float vectors of length dim.
// Returns sum(a[i] * b[i]). Uses AVX-512/AVX2/NEON when available, with
// scalar fallback. Useful for pre-normalized vectors where cosine similarity
// equals the inner product.
float inner_product(const float* a, const float* b, std::size_t dim);

// Compute cosine distance (1 - cosine_similarity) between two float vectors.
// Returns a value in [0, 2]: 0 means identical direction, 1 means orthogonal,
// 2 means opposite direction. Uses AVX-512/AVX2/NEON when available.
// Zero-norm vectors are treated as maximally distant (returns 1.0).
float cosine_distance(const float* a, const float* b, std::size_t dim);

} // namespace simd
} // namespace themis
