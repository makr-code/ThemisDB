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

/**
 * @brief Launch FP16 L2 Distance Kernel.
 * @param[in] d_queries Input parameter.
 * @param[in] d_vectors Input parameter.
 * @param[in,out] d_distances Input/output parameter.
 * @param[in] numQueries Input parameter.
 * @param[in] numVectors Input parameter.
 * @param[in] dim Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 */
cudaError_t launchFP16L2DistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float*        d_distances,
    int           numQueries,
    int           numVectors,
    int           dim,
    cudaStream_t  stream);

/**
 * @brief Launch BF16 L2 Distance Kernel.
 * @param[in] d_queries Input parameter.
 * @param[in] d_vectors Input parameter.
 * @param[in,out] d_distances Input/output parameter.
 * @param[in] numQueries Input parameter.
 * @param[in] numVectors Input parameter.
 * @param[in] dim Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 */
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
