/**
 * @file opencl_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

#ifdef THEMIS_ENABLE_OPENCL
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include "acceleration/raii/opencl_raii.h"
#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_OPENCL

/**
 * OpenCL Vector Backend for broad hardware compatibility.
 *
 * Provides GPU-accelerated vector operations using the OpenCL 1.2+ API.
 * Compatible with any OpenCL-capable device: NVIDIA, AMD, Intel, ARM, Qualcomm.
 *
 * Features:
 * - L2 (squared) and Cosine distance computation on GPU
 * - Batch KNN search with parallel distance computation
 * - RAII-managed OpenCL resources (automatic cleanup)
 * - Structured error reporting via ErrorContext
 *
 * Hardware Requirements:
 * - Any device with an OpenCL 1.2+ ICD installed
 * - Prefers GPU; falls back to CPU OpenCL device when no GPU is found
 */
class OpenCLVectorBackend : public IVectorBackend {
public:
    OpenCLVectorBackend() = default;
    ~OpenCLVectorBackend() = default;

    BackendType type() const noexcept override;
    const char* name() const noexcept override;
    bool isAvailable() const noexcept override;
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dimension,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override;

    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dimension,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override;

private:
    cl_platform_id platform_ = nullptr;
    cl_device_id   device_   = nullptr;

    // RAII-managed OpenCL resources (automatic cleanup)
    raii::OpenCLContext context_;
    raii::OpenCLQueue   queue_;
    raii::OpenCLProgram program_;
    raii::OpenCLKernel  l2Kernel_;
    raii::OpenCLKernel  cosineKernel_;

    bool initialized_ = false;
};

#else // THEMIS_ENABLE_OPENCL not defined

// Stub implementation when OpenCL is not available
/** @brief Stub implementation when OpenCL is not available. */
class OpenCLVectorBackend : public IVectorBackend {
public:
    BackendType type() const noexcept override;
    const char* name() const noexcept override;
    bool isAvailable() const noexcept override;
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dimension,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override;

    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dimension,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override;

    // -----------------------------------------------------------------------
    // Stub-path injection — active only when THEMIS_ENABLE_OPENCL is not defined.
    // -----------------------------------------------------------------------
    using ComputeDistancesFn = std::function<std::vector<float>(
        const float* query, size_t query_count, size_t dim,
        const float* db, size_t db_count, bool use_l2)>;

    /// Inject a computeDistances implementation for the non-OpenCL stub path.
    /// Pass empty fn to restore fail-closed stub default (returns {}).
    static void setComputeDistancesFn(ComputeDistancesFn fn);
};

#endif // THEMIS_ENABLE_OPENCL

// Factory function — creates an OpenCLVectorBackend instance.
// Returns a stub (isAvailable() == false) when OpenCL is not compiled in.
std::unique_ptr<IVectorBackend> createOpenCLBackend();

} // namespace acceleration
} // namespace themis
