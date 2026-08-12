/**
 * @file simd_distance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

// Compute cosine similarity between two float vectors of length dim.
// Returns a value in [-1, 1]: 1 means identical direction, 0 means orthogonal,
// -1 means opposite direction. This is the canonical entry point; callers that
// only need a distance metric should prefer cosine_distance() which avoids the
// subtraction and is marginally faster.
// Zero-norm vectors are treated as maximally dissimilar (returns 0.0).
inline float cosine_similarity(const float* a, const float* b, std::size_t dim) {
    return 1.0f - cosine_distance(a, b, dim);
}

// Batch cosine similarity: compute cosine_similarity from a single query vector
// to n database vectors stored contiguously (n * dim floats).
// Results are written to the pre-allocated output array of n floats.
// More cache-efficient than calling cosine_similarity() in a loop.
void batch_cosine_similarity(const float* query, const float* database,
                              std::size_t n, std::size_t dim, float* results);

} // namespace simd
} // namespace themis
