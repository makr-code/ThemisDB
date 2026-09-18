/**
 * @file fp16_vector_kernels.h
 * @brief Declarations for FP16 and BF16 GPU vector distance kernel launchers.
 *
 * These functions are the C++ host-side launchers for the mixed-precision CUDA
 * kernels implemented in @c src/acceleration/cuda/fp16_vector_kernels.cu.
 * They accept FP32 host-layout pointers and return FP32 results; the precision
 * reduction happens on-device inside the kernel.
 *
 * @note Only available when @c THEMIS_ENABLE_CUDA is defined and the build
 *       includes the CUDA toolkit (nvcc ≥ 11.8).
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <cstddef>

namespace themis {
namespace acceleration {

/// @brief Launch FP16 L2 distance kernel.
///
/// Allocates temporary `__half` device buffers, converts FP32 inputs to FP16
/// on-device, computes pairwise L2-squared distances in FP16, and writes FP32
/// results back to @p d_distances.  Requires sm_70 or later; the function
/// returns @c cudaErrorNotSupported on devices that do not meet this
/// requirement.
///
/// Output layout: @c d_distances[q * numVectors + v] = L2²(query_q, vector_v).
///
/// @param d_queries   FP32 device pointer: query matrix [numQueries × dim]
/// @param d_vectors   FP32 device pointer: vector matrix [numVectors × dim]
/// @param d_distances FP32 device pointer: output matrix [numQueries × numVectors]
/// @param numQueries  Number of query vectors
/// @param numVectors  Number of database vectors
/// @param dim         Vector dimensionality
/// @param stream      CUDA stream (may be @c nullptr to use the default stream)
/// @return @c cudaSuccess on success; a non-zero @c cudaError_t otherwise.
cudaError_t launchFP16L2DistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float*        d_distances,
    int           numQueries,
    int           numVectors,
    int           dim,
    cudaStream_t  stream);

/// @brief Launch BF16 L2 distance kernel.
///
/// Identical in interface to @c launchFP16L2DistanceKernel but uses
/// @c __nv_bfloat16 arithmetic internally (sm_80+).  On devices with
/// @c __CUDA_ARCH__ < 800, the kernel falls back to FP32 arithmetic
/// transparently, preserving correctness at the cost of the expected
/// bandwidth/throughput improvement.
///
/// @param d_queries   FP32 device pointer: query matrix [numQueries × dim]
/// @param d_vectors   FP32 device pointer: vector matrix [numVectors × dim]
/// @param d_distances FP32 device pointer: output matrix [numQueries × numVectors]
/// @param numQueries  Number of query vectors
/// @param numVectors  Number of database vectors
/// @param dim         Vector dimensionality
/// @param stream      CUDA stream (may be @c nullptr to use the default stream)
/// @return @c cudaSuccess on success; a non-zero @c cudaError_t otherwise.
cudaError_t launchBF16L2DistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float*        d_distances,
    int           numQueries,
    int           numVectors,
    int           dim,
    cudaStream_t  stream);

} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
