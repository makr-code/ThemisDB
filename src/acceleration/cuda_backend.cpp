/**
 * @file cuda_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=33, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "acceleration/cuda_backend.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <thread>

#include "acceleration/batch_validator.h"
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include "acceleration/kernel_invocation.h"
#include "index/cuda_hnsw_graph_traversal.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>

#include "acceleration/raii/cuda_raii.h"

// External CUDA kernel declarations
extern "C" {
void launchL2DistanceKernel(const float *d_queries, const float *d_vectors, float *d_distances, int numQueries,
                            int numVectors, int dim, cudaStream_t stream);

void launchCosineDistanceKernel(const float *d_queries, const float *d_vectors, float *d_distances, int numQueries,
                                int numVectors, int dim, cudaStream_t stream);

void launchInnerProductKernel(const float *d_queries, const float *d_vectors, float *d_distances, int numQueries,
                              int numVectors, int dim, cudaStream_t stream);

void launchTopKKernel(const float *d_distances, int *d_topkIndices, float *d_topkDistances, int numQueries,
                      int numVectors, int k, cudaStream_t stream);

// Geo kernel launchers from cuda/geo_kernels.cu (conform to frozen interface)
int launchGeoDistanceKernel(const double *d_lats1, const double *d_lons1, const double *d_lats2, const double *d_lons2,
                            float *d_distances, int count, themis::acceleration::GeoDistanceFormula formula,
                            void *opaque_stream);

int launchGeoContainmentKernel(const double *d_point_lats, const double *d_point_lons, int numPoints,
                               const double *d_polygon_coords, int numPolygonVertices, uint8_t *d_results,
                               void *opaque_stream);

// Graph kernel launchers from cuda/graph_kernels.cu
void launchGraphBFSInitKernel(const uint32_t *d_startVertices, uint32_t *d_frontier_a, uint32_t *d_frontier_b,
                              uint32_t *d_visited, uint32_t *d_depths, int numVertices, int numStarts,
                              cudaStream_t stream);

void launchGraphBFSExpandKernel(const uint32_t *d_adjacency, const uint32_t *d_frontier_in, uint32_t *d_frontier_out,
                                uint32_t *d_visited, uint32_t *d_depths, int numVertices, int numStarts,
                                uint32_t currentDepth, cudaStream_t stream);

void launchGraphBFSGatherKernel(const uint32_t *d_visited, int numVertices, int numStarts, uint32_t *d_result_vertices,
                                int *d_result_sizes, cudaStream_t stream);

void launchGraphBFInitDistancesKernel(const uint32_t *d_startVertices, float *d_distances, int *d_predecessors,
                                      int numVertices, int numPairs, cudaStream_t stream);

void launchGraphBFRelaxKernel(const uint32_t *d_adjacency, const float *d_weights, float *d_distances,
                              int *d_predecessors, int numVertices, int numPairs, cudaStream_t stream);

// Block-size setters for occupancy tuning — called during initialize() with
// the value returned by cudaOccupancyMaxPotentialBlockSize().
void setGeoKernelBlockSize(int blockSize);
int tuneGeoKernelBlockSize();
void setGraphBFSBlockDim(int blockDim);
int tuneGraphBFSBlockDim();
void setVecKernelBlockDim(int dim);
int tuneVecKernelBlockSize();
}

#endif

namespace themis {
namespace acceleration {

// Maximum k for a single-pass HNSW kernel invocation.
// Mirrors kMaxK in src/acceleration/cuda/cuda_hnsw_kernels.cu.
// For k > kHnswSinglePassMaxK the engine falls through to a multi-pass
// host-merge strategy; this is considered a degraded operation mode.
static constexpr uint32_t kHnswSinglePassMaxK = 1024u;

#ifdef THEMIS_ENABLE_CUDA
namespace {
constexpr auto kCategoryAKernelTimeout = std::chrono::seconds(5);
// FP tolerance for cosine distance boundary checks.
// Cosine distance = 1 - cosine_similarity, valid range [0, 2].
constexpr float kCosineDistanceTolerance = 0.001f;

struct StreamWaitResult {
    bool ok = false;
    AccelerationErrorCode errorCode = AccelerationErrorCode::SynchronizationFailed;
    std::string message = {};
};

StreamWaitResult waitForStreamWithTimeout(cudaStream_t stream, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        const cudaError_t queryErr = cudaStreamQuery(stream);
        if (queryErr == cudaSuccess) {
            return {true, AccelerationErrorCode::Success, ""};
        }
        if (queryErr != cudaErrorNotReady) {
            return {false, AccelerationErrorCode::SynchronizationFailed,
                    "Stream synchronization failed: " + std::string(cudaGetErrorString(queryErr))};
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return {false, AccelerationErrorCode::OperationTimeout,
                    "CUDA stream timeout while waiting for kernel completion"};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool validateDistanceOutputs(const std::vector<float>& distances, bool useL2, std::string& validationError) {
    for (size_t i = 0; i < distances.size(); ++i) {
        const float value = distances[i];
        if (!std::isfinite(value)) {
            validationError = "Distance output contains non-finite value at index " + std::to_string(i);
            return false;
        }
        if (useL2 && value < 0.0F) {
            validationError = "L2 distance output is negative at index " + std::to_string(i);
            return false;
        }
        if (!useL2 && (value < -kCosineDistanceTolerance || value > 2.0F + kCosineDistanceTolerance)) {
            validationError = "Cosine distance output out of [0, 2] range at index " + std::to_string(i);
            return false;
        }
    }
    return true;
}

bool validateTopKOutputs(const std::vector<int>& topkIndices, const std::vector<float>& topkDistances, size_t numVectors,
                         bool useL2, std::string& validationError) {
    if (static_cast<int>(topkIndices.size()) != topkDistances.size()) {
        validationError = "Top-K output size mismatch between indices and distances";
        return false;
    }
    const int maxVectorIndex = static_cast<int>(numVectors);
    for (size_t i = 0; i < topkIndices.size(); ++i) {
        const int index = topkIndices[i];
        if (index < 0 || index >= maxVectorIndex) {
            validationError = "Top-K output index out of range at position " + std::to_string(i);
            return false;
        }
        const float distance = topkDistances[i];
        if (!std::isfinite(distance)) {
            validationError = "Top-K distance contains non-finite value at position " + std::to_string(i);
            return false;
        }
        if (useL2 && distance < 0.0F) {
            validationError = "Top-K L2 distance is negative at position " + std::to_string(i);
            return false;
        }
        if (!useL2 && (distance < -kCosineDistanceTolerance || distance > 2.0F + kCosineDistanceTolerance)) {
            validationError = "Top-K cosine distance out of [0, 2] range at position " + std::to_string(i);
            return false;
        }
    }
    return true;
}
} // namespace
#endif

// ============================================================================
// CUDAVectorBackend Implementation
// ============================================================================

CUDAVectorBackend::~CUDAVectorBackend() {
    shutdown();
}

bool CUDAVectorBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    int deviceCount = 0;
    cudaError_t availErr = cudaGetDeviceCount(&deviceCount);
    return (availErr == cudaSuccess && deviceCount >= 1);
#else
    return false;
#endif
}

BackendCapabilities CUDAVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsVectorOps       = true;
    caps.supportsGraphOps        = false;
    caps.supportsGeoOps          = false;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.supportedMetrics
        = metricBit(DistanceMetric::L2) | metricBit(DistanceMetric::COSINE) | metricBit(DistanceMetric::INNER_PRODUCT);

    if (isAvailable()) {
        cudaDeviceProp prop = {};
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            caps.deviceName     = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits   = prop.multiProcessorCount;
        }
    } else {
        caps.deviceName = "CUDA Device (Not Available)";
    }
#endif
    return caps;
}

bool CUDAVectorBackend::initialize() {
#ifdef THEMIS_ENABLE_CUDA
    if (!isAvailable()) {
        // Enhanced error logging: enumerate devices and provide diagnostic info
        int deviceCount = 0;
        cudaError_t initErr = cudaGetDeviceCount(&deviceCount);

        // Set structured error context
        if (deviceCount == 0) {
            setError(ErrorContextHelpers::createNoDevicesError("CUDA"));
        } else {
            setError(ErrorContext(AccelerationErrorCode::DriverNotInstalled, "CUDA",
                                  "CUDA driver or runtime not accessible: " + std::string(cudaGetErrorString(initErr)),
                                  "Install NVIDIA CUDA driver and runtime"));
        }

        // Keep backward-compatible logging
        std::cerr << getLastError().format() << std::endl;
        return false;
    }

    // Set device with error context
    cudaError_t setDeviceErr = cudaSetDevice(0);
    if (setDeviceErr != cudaSuccess) {
        setError(ErrorContext(AccelerationErrorCode::DeviceSetFailed, "CUDA",
                              "Failed to set device 0: " + std::string(cudaGetErrorString(setDeviceErr)),
                              "Check if device is available and not in exclusive mode"));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    try {
        // v1.1.0: Create CUDA stream with low priority for vLLM co-location using RAII
#ifdef THEMIS_VLLM_COLOCATION
        // Non-blocking stream with low priority (doesn't block vLLM)
        int leastPriority = 0;
        int greatestPriority = 0;
        cudaError_t priorityErr = cudaDeviceGetStreamPriorityRange(&leastPriority, &greatestPriority);
        if (priorityErr != cudaSuccess) {
            setError(ErrorContextHelpers::createQueueError("CUDA", "Failed to get stream priority range: "
                                                                       + std::string(cudaGetErrorString(priorityErr))));
            std::cerr << lastError_.format() << std::endl;
            return false;
        }
        stream_.createWithPriority(leastPriority, cudaStreamNonBlocking);
        std::cout << "CUDA: Created low-priority stream for vLLM co-location (priority=" << leastPriority << ")"
                  << std::endl;
#else
        // Standard stream for non-vLLM deployments (RAII-managed)
        stream_.create();
#endif
    } catch (const std::exception &e) {
        setError(ErrorContextHelpers::createQueueError("CUDA", std::string(e.what())));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    // Query device properties with error handling
    cudaDeviceProp prop;
    cudaError_t propErr = cudaGetDeviceProperties(&prop, 0);
    if (propErr != cudaSuccess) {
        setError(ErrorContext(AccelerationErrorCode::DevicePropertiesQueryFailed, "CUDA",
                              "Failed to query device properties: " + std::string(cudaGetErrorString(propErr)),
                              "Ensure CUDA runtime is properly installed and device is accessible"));
        std::cerr << lastError_.format() << std::endl;
        // stream_ automatically cleaned up by RAII destructor
        return false;
    }

    // Runtime version check
    int runtimeVersion = 0;
    cudaRuntimeGetVersion(&runtimeVersion);

    std::cout << "CUDA Backend initialized successfully:" << std::endl;
    std::cout << "  Device: " << prop.name << std::endl;
    std::cout << "  Compute Capability: " << prop.major << "." << prop.minor << std::endl;
    std::cout << "  Global Memory: " << (prop.totalGlobalMem / (1024 * 1024 * 1024)) << " GB" << std::endl;
    std::cout << "  Multiprocessors: " << prop.multiProcessorCount << std::endl;
    std::cout << "  CUDA Runtime: " << (runtimeVersion / 1000) << "." << ((runtimeVersion % 100) / 10) << std::endl;
#ifdef THEMIS_VLLM_COLOCATION
    std::cout << "  vLLM Co-Location: ENABLED (low-priority stream, max " << THEMIS_MAX_GPU_VRAM_MB << " MB VRAM)"
              << std::endl;
#endif

    // Tune kernel block dimensions via the occupancy API so distance and
    // top-K kernels use the optimal thread count for this device.
    const int vecBlockDim = tuneVecKernelBlockSize();
    std::cout << "  Occupancy-tuned vector block dim: " << vecBlockDim << "x" << vecBlockDim << std::endl;

    // Clear error on success
    clearError();
    initialized_ = true;
    return true;
#else
    setError(ErrorContext(AccelerationErrorCode::FeatureNotSupported, "CUDA",
                          "Not compiled with CUDA support (THEMIS_ENABLE_CUDA not defined)",
                          "Recompile with CUDA support enabled"));
    std::cerr << getLastError().format() << std::endl;
    return false;
#endif
}

void CUDAVectorBackend::shutdown() {
#ifdef THEMIS_ENABLE_CUDA
    if (initialized_) {
        // Clear graph cache before device reset to properly destroy CUDA
        // graph resources (cudaGraphExecDestroy / cudaGraphDestroy) while
        // the device is still valid.
        {
            std::lock_guard<std::mutex> lock(graphCacheMutex_);
            graphCache_.clear();
        }
        // stream_ automatically destroyed by RAII destructor
        cudaDeviceReset();
        initialized_ = false;
    }
#endif
    // Release HNSW engine device resources
    hnswEngine_.reset();
}

// ============================================================================
// HNSW ANN index management
// ============================================================================

bool CUDAVectorBackend::buildHnswAnnIndex(const std::vector<HnswLayerGraph> &layers, const float *vectors,
                                          size_t numVectors, uint32_t dim) {
    if (layers.empty() || vectors == nullptr || numVectors == 0 || dim == 0) {
        return false;
    }

    CudaHnswConfig cfg;
    cfg.dim       = dim;
    cfg.device_id = 0;

    hnswEngine_ = std::make_unique<CudaHnswTraversalEngine>(cfg);

    // Clamp maxBatchSize_ to avoid exceeding BackendCapabilities::maxMemoryBytes.
    // Pool size = maxBatchSize × ceil(numNodes / 8) bytes.  We check against
    // the device's total VRAM reported by getCapabilities().
    size_t effectiveBatchSize = maxBatchSize_;
    const size_t numNodes     = layers[0].num_nodes;
    if (numNodes > 0) {
        const size_t vis_per_q = (numNodes + 7u) / 8u;
        const size_t caps_mem  = getCapabilities().maxMemoryBytes;
        // Reserve ≥ 10% of VRAM for other allocations; cap pool to 90% of VRAM.
        const size_t pool_limit = (caps_mem > 0) ? static_cast<size_t>(caps_mem * 0.9) : SIZE_MAX;
        if (vis_per_q > 0 && effectiveBatchSize > pool_limit / vis_per_q) {
            effectiveBatchSize = pool_limit / vis_per_q;
            if (effectiveBatchSize == 0) {
                effectiveBatchSize = 1;
            }
            std::cout << "CUDAVectorBackend: clamping maxBatchSize from " << maxBatchSize_ << " to "
                      << effectiveBatchSize << " to stay within VRAM budget" << std::endl;
        }
    }
    hnswEngine_->setMaxBatchSize(effectiveBatchSize);

    const bool ok = hnswEngine_->buildIndex(layers, vectors, numVectors);

    // Surface pool allocation failure as a degraded health status so that
    // getHealthStatus() reports "degraded" and callers can react.
    if (ok && !hnswEngine_->hasVisitedPool()) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA-HNSW",
                              "Visited bitset pool allocation failed during buildHnswAnnIndex(); "
                              "per-invocation fallback will be used (degraded performance). "
                              "Reduce setMaxBatchSize() or free GPU memory.",
                              "Call setMaxBatchSize() with a smaller value before buildHnswAnnIndex()"));
    }

    return ok;
}

bool CUDAVectorBackend::isHnswIndexBuilt() const noexcept {
    return hnswEngine_ && hnswEngine_->isBuilt();
}

void CUDAVectorBackend::setMaxBatchSize([[maybe_unused]] size_t n) {
    if (n == 0) {
        n = 1;
    }
    maxBatchSize_ = n;
    // If the HNSW engine is already built, propagate the new setting so that
    // the next buildHnswAnnIndex() picks it up.  A re-build is required to
    // reallocate the visited pool with the new batch size.
    if (hnswEngine_) {
        hnswEngine_->setMaxBatchSize(n);
    }
}

std::vector<std::vector<std::pair<uint32_t, float>>>
CUDAVectorBackend::annBatchSearch(const float *queries, size_t numQueries, size_t k, uint32_t ef) {
    if (!hnswEngine_ || !hnswEngine_->isBuilt()) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA",
                              "HNSW index not built; call buildHnswAnnIndex() before annBatchSearch()",
                              "Build the HNSW index first via buildHnswAnnIndex()"));
        return {};
    }
    if (queries == nullptr || numQueries == 0 || k == 0) {
        return {};
    }

    // In release builds: k > kHnswSinglePassMaxK triggers multi-pass (degraded mode).
    // Surface this as a degraded health status so callers can detect it via
    // getHealthStatus().  In debug builds the kernel itself will __trap() before
    // ever reaching multi-pass, ensuring misuse is caught early.
#if defined(NDEBUG)
    if (k > static_cast<size_t>(kHnswSinglePassMaxK)) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA-HNSW", AccelerationErrorCode::InvalidInputShape,
            "k=" + std::to_string(k) + " exceeds single-pass kMaxK=" + std::to_string(kHnswSinglePassMaxK)
                + "; multi-pass host-merge strategy active (degraded performance)"));
    }
#endif

    auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k), ef);

    // Convert HnswTraversalResult → pair<uint32_t, float>
    std::vector<std::vector<std::pair<uint32_t, float>>> out;
    out.reserve(hnswResults.size());
    for (const auto &queryRes : hnswResults) {
        std::vector<std::pair<uint32_t, float>> row;
        row.reserve(queryRes.size());
        for (const auto &r : queryRes) {
            row.emplace_back(static_cast<uint32_t>(r.id), r.score);
        }
        out.push_back(std::move(row));
    }
    return out;
}

std::vector<float> CUDAVectorBackend::computeDistances(const float *queries, size_t numQueries, size_t dim,
                                                       const float *vectors, size_t numVectors, bool useL2) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA", "CUDA backend not initialized",
                              "Call initialize() before using the backend"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    // Input validation
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError("CUDA", AccelerationErrorCode::InvalidInputShape,
                                                            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0) {
        setError(ErrorContextHelpers::createValidationError("CUDA", AccelerationErrorCode::InvalidInputShape,
                                                            "numQueries, numVectors, and dim must all be > 0"));
        return {};
    }

    cudaStream_t stream = stream_.get();

    const size_t querySize    = numQueries * dim * sizeof(float);
    const size_t vectorSize   = numVectors * dim * sizeof(float);
    const size_t distanceSize = numQueries * numVectors * sizeof(float);

    try {
        // RAII wrappers ensure no device memory leaks on any error path
        raii::CudaDeviceMemory d_queries(querySize);
        raii::CudaDeviceMemory d_vectors(vectorSize);
        raii::CudaDeviceMemory d_distances(distanceSize);

        d_queries.copyFrom(queries, querySize, stream);
        d_vectors.copyFrom(vectors, vectorSize, stream);

        if (useL2) {
            launchL2DistanceKernel(static_cast<const float *>(d_queries.get()),
                                   static_cast<const float *>(d_vectors.get()), static_cast<float *>(d_distances.get()),
                                   static_cast<int>(numQueries), static_cast<int>(numVectors), static_cast<int>(dim),
                                   stream);
        } else {
            launchCosineDistanceKernel(static_cast<const float *>(d_queries.get()),
                                       static_cast<const float *>(d_vectors.get()),
                                       static_cast<float *>(d_distances.get()), static_cast<int>(numQueries),
                                       static_cast<int>(numVectors), static_cast<int>(dim), stream);
        }

        std::vector<float> distances(numQueries * numVectors);
        d_distances.copyTo(distances.data(), distanceSize, stream);

        const StreamWaitResult waitResult
            = waitForStreamWithTimeout(stream, std::chrono::duration_cast<std::chrono::milliseconds>(kCategoryAKernelTimeout));
        if (!waitResult.ok) {
            setError(ErrorContext(waitResult.errorCode, "CUDA", waitResult.message,
                                  "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }
        std::string distanceValidationError = {};
        if (!validateDistanceOutputs(distances, useL2, distanceValidationError)) {
            setError(ErrorContext(AccelerationErrorCode::InputValidationFailed, "CUDA", distanceValidationError,
                                  "Falling back to CPU path is recommended for this batch"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        clearError();
        return distances;
    } catch (const std::exception &e) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                              std::string("Device memory operation failed: ") + e.what(),
                              "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
#else
    return {};
#endif
}

std::vector<std::vector<std::pair<uint32_t, float>>>
CUDAVectorBackend::batchKnnSearch(const float *queries, size_t numQueries, size_t dim, const float *vectors,
                                  size_t numVectors, size_t k, bool useL2) {
    // ── HNSW fast-path: use GPU-accelerated graph traversal when available ──
    // When an HNSW index has been pre-built via buildHnswAnnIndex() the engine
    // stores the graph and vectors on device.  We delegate the search there
    // instead of running the brute-force O(N·d) distance matrix.
    if (hnswEngine_ && hnswEngine_->isBuilt()) {
        // k > kHnswSinglePassMaxK uses multi-pass strategy; mark as degraded
        // in release builds so getHealthStatus() surfaces the condition.
#if defined(NDEBUG)
        if (k > static_cast<size_t>(kHnswSinglePassMaxK)) {
            setError(ErrorContextHelpers::createValidationError(
                "CUDA-HNSW", AccelerationErrorCode::InvalidInputShape,
                "k=" + std::to_string(k) + " exceeds single-pass kMaxK=" + std::to_string(kHnswSinglePassMaxK)
                    + "; multi-pass host-merge strategy active (degraded performance)"));
        }
#endif
        auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k));
        std::vector<std::vector<std::pair<uint32_t, float>>> out;
        out.reserve(hnswResults.size());
        for (const auto &qr : hnswResults) {
            std::vector<std::pair<uint32_t, float>> row;
            row.reserve(qr.size());
            for (const auto &r : qr) {
                row.emplace_back(static_cast<uint32_t>(r.id), r.score);
            }
            out.push_back(std::move(row));
        }
        return out;
    }

#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA", "CUDA backend not initialized",
                              "Call initialize() before using the backend"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    // Input validation
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError("CUDA", AccelerationErrorCode::InvalidInputShape,
                                                            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        setError(ErrorContextHelpers::createValidationError("CUDA", AccelerationErrorCode::InvalidInputShape,
                                                            "numQueries, numVectors, dim, and k must all be > 0"));
        return {};
    }

    // Clamp k to available vectors to prevent out-of-bounds indexing
    const size_t effectiveK = std::min(k, numVectors);

    cudaStream_t stream = stream_.get();

    const size_t querySize    = numQueries * dim * sizeof(float);
    const size_t vectorSize   = numVectors * dim * sizeof(float);
    const size_t distanceSize = numQueries * numVectors * sizeof(float);
    const size_t topkIdxSize  = numQueries * effectiveK * sizeof(int);
    const size_t topkDistSize = numQueries * effectiveK * sizeof(float);

    try {
        // RAII wrappers ensure no device memory leaks on any error path
        raii::CudaDeviceMemory d_queries(querySize);
        raii::CudaDeviceMemory d_vectors(vectorSize);
        raii::CudaDeviceMemory d_distances(distanceSize);
        raii::CudaDeviceMemory d_topkIndices(topkIdxSize);
        raii::CudaDeviceMemory d_topkDistances(topkDistSize);

        d_queries.copyFrom(queries, querySize, stream);
        d_vectors.copyFrom(vectors, vectorSize, stream);

        // Step 1: Compute distances
        if (useL2) {
            launchL2DistanceKernel(static_cast<const float *>(d_queries.get()),
                                   static_cast<const float *>(d_vectors.get()), static_cast<float *>(d_distances.get()),
                                   static_cast<int>(numQueries), static_cast<int>(numVectors), static_cast<int>(dim),
                                   stream);
        } else {
            launchCosineDistanceKernel(static_cast<const float *>(d_queries.get()),
                                       static_cast<const float *>(d_vectors.get()),
                                       static_cast<float *>(d_distances.get()), static_cast<int>(numQueries),
                                       static_cast<int>(numVectors), static_cast<int>(dim), stream);
        }

        // Step 2: Extract top-k
        launchTopKKernel(static_cast<const float *>(d_distances.get()), static_cast<int *>(d_topkIndices.get()),
                         static_cast<float *>(d_topkDistances.get()), static_cast<int>(numQueries),
                         static_cast<int>(numVectors), static_cast<int>(effectiveK), stream);

        std::vector<int> topkIndices(numQueries * effectiveK);
        std::vector<float> topkDistances(numQueries * effectiveK);

        d_topkIndices.copyTo(topkIndices.data(), topkIdxSize, stream);
        d_topkDistances.copyTo(topkDistances.data(), topkDistSize, stream);

        const StreamWaitResult waitResult
            = waitForStreamWithTimeout(stream, std::chrono::duration_cast<std::chrono::milliseconds>(kCategoryAKernelTimeout));
        if (!waitResult.ok) {
            setError(ErrorContext(waitResult.errorCode, "CUDA", waitResult.message,
                                  "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }
        std::string topkValidationError = {};
        if (!validateTopKOutputs(topkIndices, topkDistances, numVectors, useL2, topkValidationError)) {
            setError(ErrorContext(AccelerationErrorCode::InputValidationFailed, "CUDA", topkValidationError,
                                  "Falling back to CPU path is recommended for this batch"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        for (size_t q = 0; q < numQueries; ++q) {
            results[q].reserve(effectiveK);
            for (size_t i = 0; i < effectiveK; ++i) {
                const size_t idx = q * effectiveK + i;
                results[q].emplace_back(static_cast<uint32_t>(topkIndices[idx]), topkDistances[idx]);
            }
        }

        clearError();
        return results;
    } catch (const std::exception &e) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                              std::string("Device memory operation failed: ") + e.what(),
                              "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
#else
    return {};
#endif
}

// ============================================================================
// CUDA Graph Capture Implementation
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

// ---------------------------------------------------------------------------
// CUDAGraphEntry — destructor and move operations
// ---------------------------------------------------------------------------

CUDAGraphEntry::~CUDAGraphEntry() {
    if (exec) {
        cudaGraphExecDestroy(exec);
        exec = nullptr;
    }
    if (graph) {
        cudaGraphDestroy(graph);
        graph = nullptr;
    }
}

CUDAGraphEntry::CUDAGraphEntry(CUDAGraphEntry &&o) noexcept
    : graph(o.graph), exec(o.exec), d_queries(std::move(o.d_queries)), d_vectors(std::move(o.d_vectors)),
      d_distances(std::move(o.d_distances)), d_topkIndices(std::move(o.d_topkIndices)),
      d_topkDistances(std::move(o.d_topkDistances)), lastAccess(o.lastAccess) {
    o.graph = nullptr;
    o.exec  = nullptr;
}

CUDAGraphEntry &CUDAGraphEntry::operator=(CUDAGraphEntry &&o) noexcept {
    if (this != &o) {
        if (exec) {
            cudaGraphExecDestroy(exec);
            exec = nullptr;
        }
        if (graph) {
            cudaGraphDestroy(graph);
            graph = nullptr;
        }

        graph           = o.graph;
        exec            = o.exec;
        d_queries       = std::move(o.d_queries);
        d_vectors       = std::move(o.d_vectors);
        d_distances     = std::move(o.d_distances);
        d_topkIndices   = std::move(o.d_topkIndices);
        d_topkDistances = std::move(o.d_topkDistances);
        lastAccess      = o.lastAccess;

        o.graph = nullptr;
        o.exec  = nullptr;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// CUDAGraphCache
// ---------------------------------------------------------------------------

CUDAGraphEntry *CUDAGraphCache::get(const QueryShape &shape) noexcept {
    auto it = entries_.find(shape);
    if (it == entries_.end())
        return nullptr;
    it->second.lastAccess = ++clock_;
    return &it->second;
}

CUDAGraphEntry &CUDAGraphCache::put(const QueryShape &shape, CUDAGraphEntry entry) {
    // Only evict if this is truly a new key (not a replacement)
    if (static_cast<int>(entries_.size()) > = kMaxEntries && entries_.count(shape) == 0) {
        evictLRU();
    }
    entry.lastAccess = ++clock_;
    auto res         = entries_.insert_or_assign(shape, std::move(entry));
    return res.first->second;
}

void CUDAGraphCache::evictLRU() {
    if (entries_.empty())
        return;
    auto lru = entries_.begin();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.lastAccess < lru->second.lastAccess) {
            lru = it;
        }
    }
    entries_.erase(lru);
}

void CUDAGraphCache::clear() {
    entries_.clear();
}

// ---------------------------------------------------------------------------
// CUDAVectorBackend::batchKnnSearchWithGraph
//
// On the first call for a given QueryShape:
//   1. Pre-allocate dedicated device buffers for the shape.
//   2. Capture the L2/cosine/inner-product kernel + top-K kernel into a
//      cudaGraph_t on a temporary non-blocking stream, then instantiate it.
//   3. Insert the entry into the LRU graph cache.
//
// On subsequent calls with the same shape:
//   1. Look up the cached CUDAGraphEntry.
//   2. Copy the new input data into the pre-allocated device buffers on the
//      main execution stream.
//   3. Replay the instantiated graph (cudaGraphLaunch) on the same stream.
//   4. Copy results back to host and synchronize.
// ---------------------------------------------------------------------------

std::vector<std::vector<std::pair<uint32_t, float>>>
CUDAVectorBackend::batchKnnSearchWithGraph(const float *queries, size_t numQueries, size_t dim, const float *vectors,
                                           size_t numVectors, size_t k, DistanceMetric metric) {
    if (!initialized_) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA", "CUDA backend not initialized",
                              "Call initialize() before using the backend"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError("CUDA", AccelerationErrorCode::InvalidInputShape,
                                                            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        setError(ErrorContextHelpers::createValidationError("CUDA", AccelerationErrorCode::InvalidInputShape,
                                                            "numQueries, numVectors, dim, and k must all be > 0"));
        return {};
    }

    const size_t effectiveK = std::min(k, numVectors);

    const size_t querySize    = numQueries * dim * sizeof(float);
    const size_t vectorSize   = numVectors * dim * sizeof(float);
    const size_t distanceSize = numQueries * numVectors * sizeof(float);
    const size_t topkIdxSize  = numQueries * effectiveK * sizeof(int);
    const size_t topkDistSize = numQueries * effectiveK * sizeof(float);

    const QueryShape shape{static_cast<int>(numQueries), static_cast<int>(numVectors), static_cast<int>(dim),
                           static_cast<int>(effectiveK), metric};

    cudaStream_t mainStream = stream_.get();

    try {
        // ------------------------------------------------------------------
        // Cache lookup (mutex-protected)
        // ------------------------------------------------------------------
        CUDAGraphEntry *entry = nullptr;
        {
            std::lock_guard<std::mutex> lock(graphCacheMutex_);
            entry = graphCache_.get(shape);
        }

        if (entry == nullptr) {
            // ---------------------------------------------------------------
            // Cache miss — capture a new graph for this shape
            // ---------------------------------------------------------------
            CUDAGraphEntry newEntry;

            // Allocate dedicated device buffers
            newEntry.d_queries       = raii::CudaDeviceMemory(querySize);
            newEntry.d_vectors       = raii::CudaDeviceMemory(vectorSize);
            newEntry.d_distances     = raii::CudaDeviceMemory(distanceSize);
            newEntry.d_topkIndices   = raii::CudaDeviceMemory(topkIdxSize);
            newEntry.d_topkDistances = raii::CudaDeviceMemory(topkDistSize);

            // Initialize buffers so the capture is valid (kernels read valid data)
            const auto initializeBuffer = [&](void* buffer, size_t bytes, std::string_view buffer_name) -> bool {
                const cudaError_t memset_err = cudaMemset(buffer, 0, bytes);
                if (memset_err != cudaSuccess) {
                    setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                          "Failed to initialize " + std::string(buffer_name) + ": "
                                              + std::string(cudaGetErrorString(memset_err)),
                                          "Check CUDA device state and available memory"));
                    std::cerr << lastError_.format() << std::endl;
                    return false;
                }
                return true;
            };
            if (!initializeBuffer(newEntry.d_queries.get(), querySize, "query buffer")
                || !initializeBuffer(newEntry.d_vectors.get(), vectorSize, "vector buffer")
                || !initializeBuffer(newEntry.d_distances.get(), distanceSize, "distance buffer")
                || !initializeBuffer(newEntry.d_topkIndices.get(), topkIdxSize, "top-k index buffer")
                || !initializeBuffer(newEntry.d_topkDistances.get(), topkDistSize, "top-k distance buffer")) {
                return {};
            }
            // cudaMemset is synchronous: it blocks the host until the fill is
            // complete, so the buffers are ready before capture begins.

            // Use a dedicated non-blocking stream for the capture
            cudaStream_t captureStream = nullptr;
            cudaError_t csErr          = cudaStreamCreateWithFlags(&captureStream, cudaStreamNonBlocking);
            if (csErr != cudaSuccess) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "Failed to create capture stream: " + std::string(cudaGetErrorString(csErr)),
                                      "Check CUDA driver state"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Begin stream capture — capture kernel launches only
            cudaError_t capErr = cudaStreamBeginCapture(captureStream, cudaStreamCaptureModeGlobal);
            if (capErr != cudaSuccess) {
                cudaStreamDestroy(captureStream);
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaStreamBeginCapture failed: " + std::string(cudaGetErrorString(capErr)),
                                      "Ensure CUDA >= 10.0 and no concurrent capture in progress"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Capture distance kernel
            switch (metric) {
                case DistanceMetric::COSINE:
                    launchCosineDistanceKernel(static_cast<const float *>(newEntry.d_queries.get()),
                                               static_cast<const float *>(newEntry.d_vectors.get()),
                                               static_cast<float *>(newEntry.d_distances.get()),
                                               static_cast<int>(numQueries), static_cast<int>(numVectors),
                                               static_cast<int>(dim), captureStream);
                    break;
                case DistanceMetric::INNER_PRODUCT:
                    launchInnerProductKernel(static_cast<const float *>(newEntry.d_queries.get()),
                                             static_cast<const float *>(newEntry.d_vectors.get()),
                                             static_cast<float *>(newEntry.d_distances.get()),
                                             static_cast<int>(numQueries), static_cast<int>(numVectors),
                                             static_cast<int>(dim), captureStream);
                    break;
                default: // DistanceMetric::L2
                    launchL2DistanceKernel(static_cast<const float *>(newEntry.d_queries.get()),
                                           static_cast<const float *>(newEntry.d_vectors.get()),
                                           static_cast<float *>(newEntry.d_distances.get()),
                                           static_cast<int>(numQueries), static_cast<int>(numVectors),
                                           static_cast<int>(dim), captureStream);
                    break;
            }

            // Capture top-K kernel
            launchTopKKernel(static_cast<const float *>(newEntry.d_distances.get()),
                             static_cast<int *>(newEntry.d_topkIndices.get()),
                             static_cast<float *>(newEntry.d_topkDistances.get()), static_cast<int>(numQueries),
                             static_cast<int>(numVectors), static_cast<int>(effectiveK), captureStream);

            // End capture
            cudaError_t endErr = cudaStreamEndCapture(captureStream, &newEntry.graph);
            cudaStreamDestroy(captureStream);

            if (endErr != cudaSuccess || newEntry.graph == nullptr) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaStreamEndCapture failed: " + std::string(cudaGetErrorString(endErr)),
                                      "Verify CUDA version supports graph capture"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Instantiate the graph
#if defined(CUDART_VERSION) && (CUDART_VERSION >= 12000)
            cudaError_t instErr = cudaGraphInstantiate(&newEntry.exec, newEntry.graph, 0);
#else
            cudaError_t instErr = cudaGraphInstantiate(&newEntry.exec, newEntry.graph, nullptr, nullptr, 0);
#endif
            if (instErr != cudaSuccess || newEntry.exec == nullptr) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaGraphInstantiate failed: " + std::string(cudaGetErrorString(instErr)),
                                      "Check available device memory and CUDA version"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Insert into cache (evicts LRU if needed)
            {
                std::lock_guard<std::mutex> lock(graphCacheMutex_);
                entry = &graphCache_.put(shape, std::move(newEntry));
            }
        }

        // ------------------------------------------------------------------
        // Replay: copy input data → device, launch graph, copy results ← device
        // ------------------------------------------------------------------

        // H2D: copy input data into the pre-allocated device buffers
        if (cudaMemcpyAsync(entry->d_queries.get(), queries, querySize, cudaMemcpyHostToDevice, mainStream) != cudaSuccess ||
            cudaMemcpyAsync(entry->d_vectors.get(), vectors, vectorSize, cudaMemcpyHostToDevice, mainStream) != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::MemoryCopyFailed, "CUDA",
                                  "H2D memcpy failed before graph replay",
                                  "Check available GPU memory"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        // Launch the captured graph (runs after the H2D copies on the same stream)
        cudaError_t launchErr = cudaGraphLaunch(entry->exec, mainStream);
        if (launchErr != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                  "cudaGraphLaunch failed: " + std::string(cudaGetErrorString(launchErr)),
                                  "Inspect CUDA graph validity and available device memory"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        // D2H: copy results from pre-allocated buffers back to host
        std::vector<int> topkIndices(numQueries * effectiveK);
        std::vector<float> topkDistances(numQueries * effectiveK);

        if (cudaMemcpyAsync(topkIndices.data(), entry->d_topkIndices.get(), topkIdxSize, cudaMemcpyDeviceToHost,
                             mainStream) != cudaSuccess ||
            cudaMemcpyAsync(topkDistances.data(), entry->d_topkDistances.get(), topkDistSize, cudaMemcpyDeviceToHost,
                             mainStream) != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::MemoryCopyFailed, "CUDA",
                                  "D2H memcpy failed after graph replay",
                                  "Check GPU/stream state"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        const StreamWaitResult waitResult
            = waitForStreamWithTimeout(mainStream,
                                       std::chrono::duration_cast<std::chrono::milliseconds>(kCategoryAKernelTimeout));
        if (!waitResult.ok) {
            setError(ErrorContext(waitResult.errorCode, "CUDA", waitResult.message,
                                  "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }
        const bool useL2 = (metric == DistanceMetric::L2);
        std::string topkValidationError = {};
        if (!validateTopKOutputs(topkIndices, topkDistances, numVectors, useL2, topkValidationError)) {
            setError(ErrorContext(AccelerationErrorCode::InputValidationFailed, "CUDA", topkValidationError,
                                  "Falling back to non-graph CUDA path is recommended for this batch"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        // Package results
        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        for (size_t q = 0; q < numQueries; ++q) {
            results[q].reserve(effectiveK);
            for (size_t i = 0; i < effectiveK; ++i) {
                const size_t idx = q * effectiveK + i;
                results[q].emplace_back(static_cast<uint32_t>(topkIndices[idx]), topkDistances[idx]);
            }
        }

        clearError();
        return results;

    } catch (const std::exception &e) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                              std::string("Graph capture/replay failed: ") + e.what(),
                              "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
}

#else // !THEMIS_ENABLE_CUDA

std::vector<std::vector<std::pair<uint32_t, float>>>
CUDAVectorBackend::batchKnnSearchWithGraph(const float * /*queries*/, size_t /*numQueries*/, size_t /*dim*/,
                                           const float * /*vectors*/, size_t /*numVectors*/, size_t /*k*/,
                                           DistanceMetric /*metric*/
) {
    return {};
}

#endif // THEMIS_ENABLE_CUDA

// ============================================================================
// CUDAGraphBackend — BFS and Bellman-Ford CUDA Graph Implementation
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

// ---------------------------------------------------------------------------
// CUDAGraphBFSEntry — destructor and move operations
// ---------------------------------------------------------------------------

CUDAGraphBFSEntry::~CUDAGraphBFSEntry() {
    if (exec) {
        cudaGraphExecDestroy(exec);
        exec = nullptr;
    }
    if (graph) {
        cudaGraphDestroy(graph);
        graph = nullptr;
    }
}

CUDAGraphBFSEntry::CUDAGraphBFSEntry(CUDAGraphBFSEntry &&o) noexcept
    : graph(o.graph), exec(o.exec), d_adjacency(std::move(o.d_adjacency)),
      d_startVertices(std::move(o.d_startVertices)), d_frontier_a(std::move(o.d_frontier_a)),
      d_frontier_b(std::move(o.d_frontier_b)), d_visited(std::move(o.d_visited)), d_depths(std::move(o.d_depths)),
      d_result_vertices(std::move(o.d_result_vertices)), d_result_sizes(std::move(o.d_result_sizes)),
      lastAccess(o.lastAccess) {
    o.graph = nullptr;
    o.exec  = nullptr;
}

CUDAGraphBFSEntry &CUDAGraphBFSEntry::operator=(CUDAGraphBFSEntry &&o) noexcept {
    if (this != &o) {
        if (exec) {
            cudaGraphExecDestroy(exec);
            exec = nullptr;
        }
        if (graph) {
            cudaGraphDestroy(graph);
            graph = nullptr;
        }

        graph             = o.graph;
        exec              = o.exec;
        d_adjacency       = std::move(o.d_adjacency);
        d_startVertices   = std::move(o.d_startVertices);
        d_frontier_a      = std::move(o.d_frontier_a);
        d_frontier_b      = std::move(o.d_frontier_b);
        d_visited         = std::move(o.d_visited);
        d_depths          = std::move(o.d_depths);
        d_result_vertices = std::move(o.d_result_vertices);
        d_result_sizes    = std::move(o.d_result_sizes);
        lastAccess        = o.lastAccess;

        o.graph = nullptr;
        o.exec  = nullptr;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// CUDAGraphBFSCache
// ---------------------------------------------------------------------------

CUDAGraphBFSEntry *CUDAGraphBFSCache::get(const GraphBFSShape &shape) noexcept {
    auto it = entries_.find(shape);
    if (it == entries_.end())
        return nullptr;
    it->second.lastAccess = ++clock_;
    return &it->second;
}

CUDAGraphBFSEntry &CUDAGraphBFSCache::put(const GraphBFSShape &shape, CUDAGraphBFSEntry entry) {
    if (static_cast<int>(entries_.size()) > = kMaxEntries && entries_.count(shape) == 0) {
        evictLRU();
    }
    entry.lastAccess = ++clock_;
    auto res         = entries_.insert_or_assign(shape, std::move(entry));
    return res.first->second;
}

void CUDAGraphBFSCache::evictLRU() {
    if (entries_.empty())
        return;
    auto lru = entries_.begin();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.lastAccess < lru->second.lastAccess)
            lru = it;
    }
    entries_.erase(lru);
}

void CUDAGraphBFSCache::clear() {
    entries_.clear();
}

// ---------------------------------------------------------------------------
// CUDAGraphSPEntry — destructor and move operations
// ---------------------------------------------------------------------------

CUDAGraphSPEntry::~CUDAGraphSPEntry() {
    if (exec) {
        cudaGraphExecDestroy(exec);
        exec = nullptr;
    }
    if (graph) {
        cudaGraphDestroy(graph);
        graph = nullptr;
    }
}

CUDAGraphSPEntry::CUDAGraphSPEntry(CUDAGraphSPEntry &&o) noexcept
    : graph(o.graph), exec(o.exec), d_adjacency(std::move(o.d_adjacency)), d_weights(std::move(o.d_weights)),
      d_startVertices(std::move(o.d_startVertices)), d_distances(std::move(o.d_distances)),
      d_predecessors(std::move(o.d_predecessors)), lastAccess(o.lastAccess) {
    o.graph = nullptr;
    o.exec  = nullptr;
}

CUDAGraphSPEntry &CUDAGraphSPEntry::operator=(CUDAGraphSPEntry &&o) noexcept {
    if (this != &o) {
        if (exec) {
            cudaGraphExecDestroy(exec);
            exec = nullptr;
        }
        if (graph) {
            cudaGraphDestroy(graph);
            graph = nullptr;
        }

        graph           = o.graph;
        exec            = o.exec;
        d_adjacency     = std::move(o.d_adjacency);
        d_weights       = std::move(o.d_weights);
        d_startVertices = std::move(o.d_startVertices);
        d_distances     = std::move(o.d_distances);
        d_predecessors  = std::move(o.d_predecessors);
        lastAccess      = o.lastAccess;

        o.graph = nullptr;
        o.exec  = nullptr;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// CUDAGraphSPCache
// ---------------------------------------------------------------------------

CUDAGraphSPEntry *CUDAGraphSPCache::get(const GraphSPShape &shape) noexcept {
    auto it = entries_.find(shape);
    if (it == entries_.end())
        return nullptr;
    it->second.lastAccess = ++clock_;
    return &it->second;
}

CUDAGraphSPEntry &CUDAGraphSPCache::put(const GraphSPShape &shape, CUDAGraphSPEntry entry) {
    if (static_cast<int>(entries_.size()) > = kMaxEntries && entries_.count(shape) == 0) {
        evictLRU();
    }
    entry.lastAccess = ++clock_;
    auto res         = entries_.insert_or_assign(shape, std::move(entry));
    return res.first->second;
}

void CUDAGraphSPCache::evictLRU() {
    if (entries_.empty())
        return;
    auto lru = entries_.begin();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.lastAccess < lru->second.lastAccess)
            lru = it;
    }
    entries_.erase(lru);
}

void CUDAGraphSPCache::clear() {
    entries_.clear();
}

#endif // THEMIS_ENABLE_CUDA

// ---------------------------------------------------------------------------
// CUDAGraphBackend lifecycle
// ---------------------------------------------------------------------------

CUDAGraphBackend::~CUDAGraphBackend() {
    shutdown();
}

bool CUDAGraphBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    int deviceCount = 0;
    cudaError_t availErr = cudaGetDeviceCount(&deviceCount);
    return (availErr == cudaSuccess && deviceCount >= 1);
#else
    return false;
#endif
}

BackendCapabilities CUDAGraphBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsGraphOps        = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    if (isAvailable()) {
        cudaDeviceProp prop = {};
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            caps.deviceName     = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits   = prop.multiProcessorCount;
        }
    } else {
        caps.deviceName = "CUDA Device (Not Available)";
    }
#endif
    return caps;
}

bool CUDAGraphBackend::initialize() {
#ifdef THEMIS_ENABLE_CUDA
    if (!isAvailable()) {
        int deviceCount = 0;
        cudaError_t initErr = cudaGetDeviceCount(&deviceCount);
        if (deviceCount == 0) {
            setError(ErrorContextHelpers::createNoDevicesError("CUDA"));
        } else {
            setError(ErrorContext(AccelerationErrorCode::DriverNotInstalled, "CUDA",
                                  "CUDA driver or runtime not accessible: " + std::string(cudaGetErrorString(initErr)),
                                  "Install NVIDIA CUDA driver and runtime"));
        }
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    cudaError_t setDeviceErr = cudaSetDevice(0);
    if (setDeviceErr != cudaSuccess) {
        setError(ErrorContext(AccelerationErrorCode::DeviceSetFailed, "CUDA",
                              "Failed to set device 0: " + std::string(cudaGetErrorString(setDeviceErr)),
                              "Check if device is available and not in exclusive mode"));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    try {
        stream_.create();
    } catch (const std::exception &e) {
        setError(ErrorContextHelpers::createQueueError("CUDA", std::string(e.what())));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    // Tune BFS/SP kernel block dimensions via the occupancy API.
    const int bfsBlockDim = tuneGraphBFSBlockDim();
    std::cout << "CUDA Graph Backend: occupancy-tuned BFS block dim = " << bfsBlockDim << std::endl;

    clearError();
    initialized_ = true;
    return true;
#else
    setError(ErrorContext(AccelerationErrorCode::FeatureNotSupported, "CUDA",
                          "Not compiled with CUDA support (THEMIS_ENABLE_CUDA not defined)",
                          "Recompile with CUDA support enabled"));
    std::cerr << lastError_.format() << std::endl;
    return false;
#endif
}

void CUDAGraphBackend::shutdown() {
#ifdef THEMIS_ENABLE_CUDA
    if (initialized_) {
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            bfsCache_.clear();
            spCache_.clear();
        }
        // stream_ automatically destroyed by RAII destructor
        initialized_ = false;
    }
#endif
}

// ---------------------------------------------------------------------------
// CUDAGraphBackend::batchBFS
//
// On the first call for a given (numVertices, numStarts, maxDepth) shape:
//   1. Pre-allocate dedicated device buffers.
//   2. Capture: init kernel + maxDepth expand kernels + gather kernel.
//   3. Insert the captured graph entry into the LRU BFS cache.
//
// On subsequent calls with the same shape:
//   1. Look up the cached CUDAGraphBFSEntry.
//   2. Copy adjacency + startVertices data to device (H2D on mainStream).
//   3. Replay the instantiated graph.
//   4. Copy result_vertices + result_sizes back to host (D2H on mainStream).
// ---------------------------------------------------------------------------

std::vector<std::vector<uint32_t>> CUDAGraphBackend::batchBFS(const uint32_t *adjacency, size_t numVertices,
                                                              const uint32_t *startVertices, size_t numStarts,
                                                              uint32_t maxDepth) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validateGraphBFSBatch(name(), adjacency, numVertices, startVertices, numStarts, sink)) {
        return {};
    }

#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA",
                              "CUDA graph backend not initialized", "Call initialize() before using the backend"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    const GraphBFSShape shape{static_cast<int>(numVertices), static_cast<int>(numStarts), static_cast<int>(maxDepth)};

    const size_t adjSize    = numVertices * numVertices * sizeof(uint32_t);
    const size_t svSize     = numStarts * sizeof(uint32_t);
    const size_t frontierSz = numStarts * numVertices * sizeof(uint32_t);
    const size_t resultsSz  = numStarts * numVertices * sizeof(uint32_t);
    const size_t sizesSz    = numStarts * sizeof(int);

    cudaStream_t mainStream = stream_.get();

    try {
        // ------------------------------------------------------------------
        // Cache lookup
        // ------------------------------------------------------------------
        CUDAGraphBFSEntry *entry = nullptr;
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            entry = bfsCache_.get(shape);
        }

        if (entry == nullptr) {
            // ---------------------------------------------------------------
            // Cache miss — capture a new graph for this shape
            // ---------------------------------------------------------------
            CUDAGraphBFSEntry newEntry;

            newEntry.d_adjacency       = raii::CudaDeviceMemory(adjSize);
            newEntry.d_startVertices   = raii::CudaDeviceMemory(svSize);
            newEntry.d_frontier_a      = raii::CudaDeviceMemory(frontierSz);
            newEntry.d_frontier_b      = raii::CudaDeviceMemory(frontierSz);
            newEntry.d_visited         = raii::CudaDeviceMemory(frontierSz);
            newEntry.d_depths          = raii::CudaDeviceMemory(frontierSz);
            newEntry.d_result_vertices = raii::CudaDeviceMemory(resultsSz);
            newEntry.d_result_sizes    = raii::CudaDeviceMemory(sizesSz);

            // Zero-fill so captures start from a known state
            cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
            cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
            cudaMemset(newEntry.d_frontier_a.get(), 0, frontierSz);
            cudaMemset(newEntry.d_frontier_b.get(), 0, frontierSz);
            cudaMemset(newEntry.d_visited.get(), 0, frontierSz);
            cudaMemset(newEntry.d_depths.get(), 0, frontierSz);
            cudaMemset(newEntry.d_result_vertices.get(), 0, resultsSz);
            cudaMemset(newEntry.d_result_sizes.get(), 0, sizesSz);

            // Create a dedicated non-blocking capture stream
            cudaStream_t captureStream = nullptr;
            cudaError_t csErr          = cudaStreamCreateWithFlags(&captureStream, cudaStreamNonBlocking);
            if (csErr != cudaSuccess) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "Failed to create capture stream: " + std::string(cudaGetErrorString(csErr)),
                                      "Check CUDA driver state"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            cudaError_t capErr = cudaStreamBeginCapture(captureStream, cudaStreamCaptureModeGlobal);
            if (capErr != cudaSuccess) {
                cudaStreamDestroy(captureStream);
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaStreamBeginCapture failed: " + std::string(cudaGetErrorString(capErr)),
                                      "Ensure CUDA >= 10.0 and no concurrent capture in progress"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Capture: init kernel
            launchGraphBFSInitKernel(static_cast<const uint32_t *>(newEntry.d_startVertices.get()),
                                     static_cast<uint32_t *>(newEntry.d_frontier_a.get()),
                                     static_cast<uint32_t *>(newEntry.d_frontier_b.get()),
                                     static_cast<uint32_t *>(newEntry.d_visited.get()),
                                     static_cast<uint32_t *>(newEntry.d_depths.get()), static_cast<int>(numVertices),
                                     static_cast<int>(numStarts), captureStream);

            // Capture: maxDepth expand kernels, alternating frontier buffers
            for (uint32_t d = 0; d < maxDepth; ++d) {
                const bool even             = (d % 2 == 0);
                const uint32_t *frontier_in = even ? static_cast<const uint32_t *>(newEntry.d_frontier_a.get())
                                                   : static_cast<const uint32_t *>(newEntry.d_frontier_b.get());
                uint32_t *frontier_out      = even ? static_cast<uint32_t *>(newEntry.d_frontier_b.get())
                                                   : static_cast<uint32_t *>(newEntry.d_frontier_a.get());

                launchGraphBFSExpandKernel(
                    static_cast<const uint32_t *>(newEntry.d_adjacency.get()), frontier_in, frontier_out,
                    static_cast<uint32_t *>(newEntry.d_visited.get()), static_cast<uint32_t *>(newEntry.d_depths.get()),
                    static_cast<int>(numVertices), static_cast<int>(numStarts), d + 1, captureStream);
            }

            // Capture: gather kernel
            launchGraphBFSGatherKernel(static_cast<const uint32_t *>(newEntry.d_visited.get()),
                                       static_cast<int>(numVertices), static_cast<int>(numStarts),
                                       static_cast<uint32_t *>(newEntry.d_result_vertices.get()),
                                       static_cast<int *>(newEntry.d_result_sizes.get()), captureStream);

            // End capture
            cudaError_t endErr = cudaStreamEndCapture(captureStream, &newEntry.graph);
            cudaStreamDestroy(captureStream);

            if (endErr != cudaSuccess || newEntry.graph == nullptr) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaStreamEndCapture failed: " + std::string(cudaGetErrorString(endErr)),
                                      "Verify CUDA version supports graph capture"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Instantiate
#if defined(CUDART_VERSION) && (CUDART_VERSION >= 12000)
            cudaError_t instErr = cudaGraphInstantiate(&newEntry.exec, newEntry.graph, 0);
#else
            cudaError_t instErr = cudaGraphInstantiate(&newEntry.exec, newEntry.graph, nullptr, nullptr, 0);
#endif
            if (instErr != cudaSuccess || newEntry.exec == nullptr) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaGraphInstantiate failed: " + std::string(cudaGetErrorString(instErr)),
                                      "Check available device memory and CUDA version"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            std::lock_guard<std::mutex> lock(cacheMutex_);
            entry = &bfsCache_.put(shape, std::move(newEntry));
        }

        // ------------------------------------------------------------------
        // Replay: copy inputs → device, launch graph, copy results ← device
        // ------------------------------------------------------------------
        if (cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream) != cudaSuccess ||
            cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream) != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::MemoryCopyFailed, "CUDA",
                                  "H2D memcpy failed before BFS graph replay",
                                  "Check available GPU memory"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        cudaError_t launchErr = cudaGraphLaunch(entry->exec, mainStream);
        if (launchErr != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                  "cudaGraphLaunch (BFS) failed: " + std::string(cudaGetErrorString(launchErr)),
                                  "Inspect CUDA graph validity and available device memory"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<uint32_t> h_result_vertices(numStarts * numVertices);
        std::vector<int> h_result_sizes(numStarts);

        if (cudaMemcpyAsync(h_result_vertices.data(), entry->d_result_vertices.get(), resultsSz, cudaMemcpyDeviceToHost,
                             mainStream) != cudaSuccess ||
            cudaMemcpyAsync(h_result_sizes.data(), entry->d_result_sizes.get(), sizesSz, cudaMemcpyDeviceToHost,
                             mainStream) != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::MemoryCopyFailed, "CUDA",
                                  "D2H memcpy failed after BFS graph replay",
                                  "Check GPU/stream state"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        cudaError_t syncErr = cudaStreamSynchronize(mainStream);
        if (syncErr != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::SynchronizationFailed, "CUDA",
                                  "Stream synchronization failed (BFS): " + std::string(cudaGetErrorString(syncErr)),
                                  "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        // Package results
        std::vector<std::vector<uint32_t>> results(numStarts);
        for (size_t s = 0; s < numStarts; ++s) {
            const int cnt = h_result_sizes[s];
            results[s].assign(h_result_vertices.begin() + static_cast<ptrdiff_t>(s * numVertices),
                              h_result_vertices.begin() + static_cast<ptrdiff_t>(s * numVertices + cnt));
        }

        clearError();
        return results;

    } catch (const std::exception &e) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                              std::string("BFS graph capture/replay failed: ") + e.what(),
                              "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

#else
    return {};
#endif
}

// ---------------------------------------------------------------------------
// CUDAGraphBackend::batchShortestPath
//
// Uses Bellman-Ford with CUDA Graph Capture, keyed on (numVertices, numPairs).
// Captured graph: init + (numVertices-1) relax iterations.
// Path reconstruction (predecessor tracing) is performed on the host after
// copying distances and predecessors back.
// ---------------------------------------------------------------------------

std::vector<std::vector<uint32_t>> CUDAGraphBackend::batchShortestPath(const uint32_t *adjacency, const float *weights,
                                                                       size_t numVertices,
                                                                       const uint32_t *startVertices,
                                                                       const uint32_t *endVertices, size_t numPairs) {
    clearError();
    auto sink = [this](ErrorContext e) { setError(std::move(e)); };
    if (!BatchValidator::validateShortestPathBatch(name(), adjacency, weights, numVertices, startVertices, endVertices,
                                                   numPairs, sink)) {
        return {};
    }

#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA",
                              "CUDA graph backend not initialized", "Call initialize() before using the backend"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    const GraphSPShape shape{static_cast<int>(numVertices), static_cast<int>(numPairs)};

    const size_t adjSize  = numVertices * numVertices * sizeof(uint32_t);
    const size_t wgtSize  = numVertices * numVertices * sizeof(float);
    const size_t svSize   = numPairs * sizeof(uint32_t);
    const size_t distSize = numPairs * numVertices * sizeof(float);
    const size_t predSize = numPairs * numVertices * sizeof(int);

    cudaStream_t mainStream = stream_.get();

    try {
        // ------------------------------------------------------------------
        // Cache lookup
        // ------------------------------------------------------------------
        CUDAGraphSPEntry *entry = nullptr;
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            entry = spCache_.get(shape);
        }

        if (entry == nullptr) {
            // ---------------------------------------------------------------
            // Cache miss — capture a new Bellman-Ford graph for this shape
            // ---------------------------------------------------------------
            CUDAGraphSPEntry newEntry;

            newEntry.d_adjacency     = raii::CudaDeviceMemory(adjSize);
            newEntry.d_weights       = raii::CudaDeviceMemory(wgtSize);
            newEntry.d_startVertices = raii::CudaDeviceMemory(svSize);
            newEntry.d_distances     = raii::CudaDeviceMemory(distSize);
            newEntry.d_predecessors  = raii::CudaDeviceMemory(predSize);

            cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
            cudaMemset(newEntry.d_weights.get(), 0, wgtSize);
            cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
            cudaMemset(newEntry.d_distances.get(), 0, distSize);
            cudaMemset(newEntry.d_predecessors.get(), 0, predSize);

            cudaStream_t captureStream = nullptr;
            cudaError_t csErr          = cudaStreamCreateWithFlags(&captureStream, cudaStreamNonBlocking);
            if (csErr != cudaSuccess) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "Failed to create capture stream: " + std::string(cudaGetErrorString(csErr)),
                                      "Check CUDA driver state"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            cudaError_t capErr = cudaStreamBeginCapture(captureStream, cudaStreamCaptureModeGlobal);
            if (capErr != cudaSuccess) {
                cudaStreamDestroy(captureStream);
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaStreamBeginCapture failed: " + std::string(cudaGetErrorString(capErr)),
                                      "Ensure CUDA >= 10.0 and no concurrent capture in progress"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Capture: init distances kernel
            launchGraphBFInitDistancesKernel(static_cast<const uint32_t *>(newEntry.d_startVertices.get()),
                                             static_cast<float *>(newEntry.d_distances.get()),
                                             static_cast<int *>(newEntry.d_predecessors.get()),
                                             static_cast<int>(numVertices), static_cast<int>(numPairs), captureStream);

            // Capture: numVertices-1 relaxation passes (Bellman-Ford guarantee)
            for (size_t iter = 0; iter + 1 < numVertices; ++iter) {
                launchGraphBFRelaxKernel(static_cast<const uint32_t *>(newEntry.d_adjacency.get()),
                                         static_cast<const float *>(newEntry.d_weights.get()),
                                         static_cast<float *>(newEntry.d_distances.get()),
                                         static_cast<int *>(newEntry.d_predecessors.get()),
                                         static_cast<int>(numVertices), static_cast<int>(numPairs), captureStream);
            }

            cudaError_t endErr = cudaStreamEndCapture(captureStream, &newEntry.graph);
            cudaStreamDestroy(captureStream);

            if (endErr != cudaSuccess || newEntry.graph == nullptr) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaStreamEndCapture failed: " + std::string(cudaGetErrorString(endErr)),
                                      "Verify CUDA version supports graph capture"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

#if defined(CUDART_VERSION) && (CUDART_VERSION >= 12000)
            cudaError_t instErr = cudaGraphInstantiate(&newEntry.exec, newEntry.graph, 0);
#else
            cudaError_t instErr = cudaGraphInstantiate(&newEntry.exec, newEntry.graph, nullptr, nullptr, 0);
#endif
            if (instErr != cudaSuccess || newEntry.exec == nullptr) {
                setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                      "cudaGraphInstantiate failed: " + std::string(cudaGetErrorString(instErr)),
                                      "Check available device memory and CUDA version"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            std::lock_guard<std::mutex> lock(cacheMutex_);
            entry = &spCache_.put(shape, std::move(newEntry));
        }

        // ------------------------------------------------------------------
        // Replay
        // ------------------------------------------------------------------
        if (cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream) != cudaSuccess ||
            cudaMemcpyAsync(entry->d_weights.get(), weights, wgtSize, cudaMemcpyHostToDevice, mainStream) != cudaSuccess ||
            cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream) != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::MemoryCopyFailed, "CUDA",
                                  "H2D memcpy failed before SP graph replay",
                                  "Check available GPU memory"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        cudaError_t launchErr = cudaGraphLaunch(entry->exec, mainStream);
        if (launchErr != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                                  "cudaGraphLaunch (SP) failed: " + std::string(cudaGetErrorString(launchErr)),
                                  "Inspect CUDA graph validity and available device memory"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<float> h_distances(numPairs * numVertices);
        std::vector<int> h_predecessors(numPairs * numVertices);

        if (cudaMemcpyAsync(h_distances.data(), entry->d_distances.get(), distSize, cudaMemcpyDeviceToHost,
                             mainStream) != cudaSuccess ||
            cudaMemcpyAsync(h_predecessors.data(), entry->d_predecessors.get(), predSize, cudaMemcpyDeviceToHost,
                             mainStream) != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::MemoryCopyFailed, "CUDA",
                                  "D2H memcpy failed after SP graph replay",
                                  "Check GPU/stream state"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        cudaError_t syncErr = cudaStreamSynchronize(mainStream);
        if (syncErr != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::SynchronizationFailed, "CUDA",
                                  "Stream synchronization failed (SP): " + std::string(cudaGetErrorString(syncErr)),
                                  "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        // Path reconstruction on the host: trace predecessors from endVertex back
        // to startVertex and reverse the path.
        std::vector<std::vector<uint32_t>> results(numPairs);
        for (size_t p = 0; p < numPairs; ++p) {
            const uint32_t endV   = endVertices[p];
            const uint32_t startV = startVertices[p];
            const float dist      = h_distances[p * numVertices + endV];

            if (dist >= 1e37f) {
                // endVertex not reachable from startVertex
                continue;
            }

            std::vector<uint32_t> path;
            uint32_t cur = endV;
            // Guard against predecessor cycles (maximum path length < numVertices)
            for (size_t step = 0; step < numVertices; ++step) {
                path.push_back(cur);
                if (cur == startV)
                    break;
                const int pred = h_predecessors[p * numVertices + cur];
                if (pred < 0 || static_cast<size_t>(pred) >= numVertices)
                    break;
                cur = static_cast<uint32_t>(pred);
            }
            std::reverse(path.begin(), path.end());
            results[p] = std::move(path);
        }

        clearError();
        return results;

    } catch (const std::exception &e) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA",
                              std::string("SP graph capture/replay failed: ") + e.what(),
                              "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

#else
    return {};
#endif
}

// ============================================================================
// CUDAGeoBackend Implementation
// ============================================================================

CUDAGeoBackend::~CUDAGeoBackend() {
    shutdown();
}

bool CUDAGeoBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    int deviceCount = 0;
    cudaError_t availErr = cudaGetDeviceCount(&deviceCount);
    return (availErr == cudaSuccess && deviceCount >= 1);
#else
    return false;
#endif
}

BackendCapabilities CUDAGeoBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsGeoOps          = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    if (isAvailable()) {
        cudaDeviceProp prop = {};
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            caps.deviceName     = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits   = prop.multiProcessorCount;
        }
    } else {
        caps.deviceName = "CUDA Device (Not Available)";
    }
#endif
    return caps;
}

bool CUDAGeoBackend::initialize() {
#ifdef THEMIS_ENABLE_CUDA
    if (!isAvailable()) {
        int deviceCount = 0;
        cudaError_t initErr = cudaGetDeviceCount(&deviceCount);
        if (deviceCount == 0) {
            setError(ErrorContextHelpers::createNoDevicesError("CUDA-Geo"));
        } else {
            setError(ErrorContext(AccelerationErrorCode::DriverNotInstalled, "CUDA-Geo",
                                  "CUDA driver or runtime not accessible: " + std::string(cudaGetErrorString(initErr)),
                                  "Install NVIDIA CUDA driver and runtime"));
        }
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    cudaError_t setDeviceErr = cudaSetDevice(0);
    if (setDeviceErr != cudaSuccess) {
        setError(ErrorContext(AccelerationErrorCode::DeviceSetFailed, "CUDA-Geo",
                              "Failed to set device 0: " + std::string(cudaGetErrorString(setDeviceErr)),
                              "Check if device is available and not in exclusive mode"));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    try {
        stream_.create();
    } catch (const std::exception &e) {
        setError(ErrorContextHelpers::createQueueError("CUDA-Geo", std::string(e.what())));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    cudaDeviceProp prop;
    cudaError_t propErr = cudaGetDeviceProperties(&prop, 0);
    if (propErr != cudaSuccess) {
        setError(ErrorContext(AccelerationErrorCode::DevicePropertiesQueryFailed, "CUDA-Geo",
                              "Failed to query device properties: " + std::string(cudaGetErrorString(propErr)),
                              "Ensure CUDA runtime is properly installed and device is accessible"));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    std::cout << "CUDA Geo Backend initialized successfully:" << std::endl;
    std::cout << "  Device: " << prop.name << std::endl;

    // Tune geo kernel block size via the occupancy API.
    const int geoBlockSize = tuneGeoKernelBlockSize();
    std::cout << "  Occupancy-tuned geo block size: " << geoBlockSize << std::endl;

    clearError();
    initialized_ = true;
    return true;
#else
    setError(ErrorContext(AccelerationErrorCode::FeatureNotSupported, "CUDA-Geo",
                          "Not compiled with CUDA support (THEMIS_ENABLE_CUDA not defined)",
                          "Recompile with CUDA support enabled"));
    std::cerr << lastError_.format() << std::endl;
    return false;
#endif
}

void CUDAGeoBackend::shutdown() {
#ifdef THEMIS_ENABLE_CUDA
    if (initialized_) {
        // stream_ automatically destroyed by RAII destructor
        initialized_ = false;
    }
#endif
}

std::vector<float> CUDAGeoBackend::batchDistances(const double *latitudes1, const double *longitudes1,
                                                  const double *latitudes2, const double *longitudes2, size_t count,
                                                  bool useHaversine) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA-Geo",
                              "CUDA geo backend not initialized", "Call initialize() before using the backend"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    if (latitudes1 == nullptr || longitudes1 == nullptr || latitudes2 == nullptr || longitudes2 == nullptr) {
        setError(ErrorContextHelpers::createValidationError("CUDA-Geo", AccelerationErrorCode::InvalidInputShape,
                                                            "input coordinate pointers must be non-null"));
        return {};
    }
    if (count == 0) {
        return {};
    }

    const GeoDistanceFormula formula = useHaversine ? GeoDistanceFormula::HAVERSINE : GeoDistanceFormula::VINCENTY;

    cudaStream_t stream = stream_.get();

    const size_t coordSize = count * sizeof(double);
    const size_t distSize  = count * sizeof(float);

    try {
        raii::CudaDeviceMemory d_lats1(coordSize);
        raii::CudaDeviceMemory d_lons1(coordSize);
        raii::CudaDeviceMemory d_lats2(coordSize);
        raii::CudaDeviceMemory d_lons2(coordSize);
        raii::CudaDeviceMemory d_distances(distSize);

        d_lats1.copyFrom(latitudes1, coordSize, stream);
        d_lons1.copyFrom(longitudes1, coordSize, stream);
        d_lats2.copyFrom(latitudes2, coordSize, stream);
        d_lons2.copyFrom(longitudes2, coordSize, stream);

        const int rc = launchGeoDistanceKernel(
            static_cast<const double *>(d_lats1.get()), static_cast<const double *>(d_lons1.get()),
            static_cast<const double *>(d_lats2.get()), static_cast<const double *>(d_lons2.get()),
            static_cast<float *>(d_distances.get()), static_cast<int>(count), formula, stream);

        if (rc != 0) {
            setError(ErrorContext(AccelerationErrorCode::KernelLaunchFailed, "CUDA-Geo",
                                  "launchGeoDistanceKernel failed with code " + std::to_string(rc),
                                  "Check CUDA device state"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<float> result(count);
        d_distances.copyTo(result.data(), distSize, stream);

        cudaError_t err = cudaStreamSynchronize(stream);
        if (err != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::SynchronizationFailed, "CUDA-Geo",
                                  "Stream synchronization failed: " + std::string(cudaGetErrorString(err)),
                                  "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        clearError();
        return result;
    } catch (const std::exception &e) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA-Geo",
                              std::string("Device memory operation failed: ") + e.what(),
                              "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
#else
    return {};
#endif
}

std::vector<bool> CUDAGeoBackend::batchPointInPolygon(const double *pointLats, const double *pointLons,
                                                      size_t numPoints, const double *polygonCoords,
                                                      size_t numPolygonVertices) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(AccelerationErrorCode::BackendNotInitialized, "CUDA-Geo",
                              "CUDA geo backend not initialized", "Call initialize() before using the backend"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    if (pointLats == nullptr || pointLons == nullptr || polygonCoords == nullptr) {
        setError(ErrorContextHelpers::createValidationError("CUDA-Geo", AccelerationErrorCode::InvalidInputShape,
                                                            "input coordinate pointers must be non-null"));
        return {};
    }
    if (numPoints == 0 || numPolygonVertices < 3) {
        return {};
    }

    cudaStream_t stream = stream_.get();

    const size_t pointSize  = numPoints * sizeof(double);
    const size_t polySize   = numPolygonVertices * 2 * sizeof(double);
    const size_t resultSize = numPoints * sizeof(uint8_t);

    try {
        raii::CudaDeviceMemory d_pointLats(pointSize);
        raii::CudaDeviceMemory d_pointLons(pointSize);
        raii::CudaDeviceMemory d_polyCoords(polySize);
        raii::CudaDeviceMemory d_results(resultSize);

        d_pointLats.copyFrom(pointLats, pointSize, stream);
        d_pointLons.copyFrom(pointLons, pointSize, stream);
        d_polyCoords.copyFrom(polygonCoords, polySize, stream);

        const int rc = launchGeoContainmentKernel(
            static_cast<const double *>(d_pointLats.get()), static_cast<const double *>(d_pointLons.get()),
            static_cast<int>(numPoints), static_cast<const double *>(d_polyCoords.get()),
            static_cast<int>(numPolygonVertices), static_cast<uint8_t *>(d_results.get()), stream);

        if (rc != 0) {
            setError(ErrorContext(AccelerationErrorCode::KernelLaunchFailed, "CUDA-Geo",
                                  "launchGeoContainmentKernel failed with code " + std::to_string(rc),
                                  "Check CUDA device state"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<uint8_t> rawResults(numPoints);
        d_results.copyTo(rawResults.data(), resultSize, stream);

        cudaError_t err = cudaStreamSynchronize(stream);
        if (err != cudaSuccess) {
            setError(ErrorContext(AccelerationErrorCode::SynchronizationFailed, "CUDA-Geo",
                                  "Stream synchronization failed: " + std::string(cudaGetErrorString(err)),
                                  "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<bool> result(numPoints);
        for (size_t i = 0; i < numPoints; ++i) {
            result[i] = (rawResults[i] != 0);
        }

        clearError();
        return result;
    } catch (const std::exception &e) {
        setError(ErrorContext(AccelerationErrorCode::AllocationFailed, "CUDA-Geo",
                              std::string("Device memory operation failed: ") + e.what(),
                              "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
#else
    return {};
#endif
}

} // namespace acceleration

// ============================================================================
// CUDAVectorBackend::populateANNDispatch
//
// Wires the frozen-interface launchers compiled in cuda/ann_kernels.cu into
// the ANNKernelDispatch table.  Under THEMIS_ENABLE_CUDA all four slots are
// populated; otherwise all slots remain null so the BackendRegistry falls back
// to the CPU table.
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

// Forward declarations for launchers compiled in cuda/ann_kernels.cu.
// These conform to the ANNDistanceFn / ANNTopKFn typedefs in
// include/acceleration/kernel_invocation.h (INTERFACE_VERSION 100).
extern "C" {
int cuda_launchL2DistanceKernel(const float *, const float *, float *, int, int, int, void *);
int cuda_launchCosineDistanceKernel(const float *, const float *, float *, int, int, int, void *);
int cuda_launchInnerProductKernel(const float *, const float *, float *, int, int, int, void *);
int cuda_launchTopKKernel(const float *, uint32_t *, float *, int, int, int, void *);
} // extern "C"

#endif // THEMIS_ENABLE_CUDA

namespace acceleration {

ANNKernelDispatch CUDAVectorBackend::populateANNDispatch() const {
#ifdef THEMIS_ENABLE_CUDA
    ANNKernelDispatch d;
    d.launchL2Distance   = &cuda_launchL2DistanceKernel;
    d.launchCosine       = &cuda_launchCosineDistanceKernel;
    d.launchInnerProduct = &cuda_launchInnerProductKernel;
    d.launchTopK         = &cuda_launchTopKKernel;
    return d;
#else
    return {}; // No CUDA — all null; BackendRegistry falls back to CPU table
#endif
}

GeoKernelDispatch CUDAGeoBackend::populateGeoDispatch() const {
#ifdef THEMIS_ENABLE_CUDA
    GeoKernelDispatch d;
    d.launchDistance    = launchGeoDistanceKernel;
    d.launchContainment = launchGeoContainmentKernel;
    return d;
#else
    return {}; // No CUDA — all null; BackendRegistry falls back to CPU table
#endif
}

// =============================================================================
// CUDAMatrixBackend Implementation
// =============================================================================

CUDAMatrixBackend::~CUDAMatrixBackend() {
    shutdown();
}

bool CUDAMatrixBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    int deviceCount = 0;
    cudaError_t availErr = cudaGetDeviceCount(&deviceCount);
    return (availErr == cudaSuccess && deviceCount >= 1);
#else
    return false;
#endif
}

BackendCapabilities CUDAMatrixBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsMatrixOps       = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync           = true;
    // FP16 from SM 7.0 (Volta); BF16 from SM 8.0 (Ampere).
    // Advertise both — the kernel selection inside dispatchMatmul handles
    // the actual hardware capability at runtime via cuBLAS.
    caps.supportedPrecisions = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::BF16;
    if (isAvailable()) {
        cudaDeviceProp prop = {};
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            caps.deviceName     = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits   = prop.multiProcessorCount;
            // INT8 Tensor Core acceleration requires Turing (SM 7.5+).
            const int sm = prop.major * 10 + prop.minor;
            if (sm >= 75) {
                caps.supportedPrecisions = caps.supportedPrecisions | PrecisionMode::INT8;
            }
        }
    } else {
        caps.deviceName = "CUDA Device (Not Available)";
    }
#endif
    return caps;
}

bool CUDAMatrixBackend::initialize() {
#ifdef THEMIS_ENABLE_CUDA
    if (!isAvailable()) {
        return false;
    }
    initialized_ = true;
    return true;
#else
    return false;
#endif
}

void CUDAMatrixBackend::shutdown() {
#ifdef THEMIS_ENABLE_CUDA
    initialized_ = false;
#endif
}

int CUDAMatrixBackend::matmul(const MatrixKernelParams &params, void *opaque_stream) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_)
        return 1;
    cudaStream_t stream
        = opaque_stream ? static_cast<cudaStream_t>(opaque_stream) : static_cast<cudaStream_t>(stream_.get());
    return tensor_core::dispatchMatmul(params, stream);
#else
    return 1; // CUDA not available
#endif
}

MatrixKernelDispatch CUDAMatrixBackend::populateMatrixDispatch() const {
#ifdef THEMIS_ENABLE_CUDA
    MatrixKernelDispatch d;
    d.launchMatmul = tensor_core::dispatchMatmul;
    return d;
#else
    return {}; // No CUDA — null; BackendRegistry falls back to CPU table
#endif
}

} // namespace acceleration
} // namespace themis
