/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            simd_distance.h                                    ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

} // namespace simd
} // namespace themis
