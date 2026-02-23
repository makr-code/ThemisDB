/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_backend.cpp                                   ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 BETA                                         ║
    • Quality Score:   65.0/100                                       ║
    • Total Lines:     1433                                           ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6d203e11f  2026-02-21  Freeze ANN & geospatial kernel invocation interfaces; wir... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🚧 Early Development                                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/cuda_backend.h"
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include "acceleration/kernel_invocation.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include "acceleration/raii/cuda_raii.h"

// External CUDA kernel declarations
extern "C" {
void launchL2DistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float* d_distances,
    int numQueries,
    int numVectors,
    int dim,
    cudaStream_t stream
);

void launchCosineDistanceKernel(
    const float* d_queries,
    const float* d_vectors,
    float* d_distances,
    int numQueries,
    int numVectors,
    int dim,
    cudaStream_t stream
);

void launchInnerProductKernel(
    const float* d_queries,
    const float* d_vectors,
    float* d_distances,
    int numQueries,
    int numVectors,
    int dim,
    cudaStream_t stream
);

void launchTopKKernel(
    const float* d_distances,
    int* d_topkIndices,
    float* d_topkDistances,
    int numQueries,
    int numVectors,
    int k,
    cudaStream_t stream
);

// Geo kernel launchers from cuda/geo_kernels.cu (conform to frozen interface)
int launchGeoDistanceKernel(
    const double* d_lats1,
    const double* d_lons1,
    const double* d_lats2,
    const double* d_lons2,
    float* d_distances,
    int count,
    themis::acceleration::GeoDistanceFormula formula,
    void* opaque_stream
);

int launchGeoContainmentKernel(
    const double* d_point_lats,
    const double* d_point_lons,
    int numPoints,
    const double* d_polygon_coords,
    int numPolygonVertices,
    uint8_t* d_results,
    void* opaque_stream
);
}

#endif

namespace themis {
namespace acceleration {

// ============================================================================
// CUDAVectorBackend Implementation
// ============================================================================

CUDAVectorBackend::~CUDAVectorBackend() {
    shutdown();
}

bool CUDAVectorBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    return (err == cudaSuccess && deviceCount > 0);
#else
    return false;
#endif
}

BackendCapabilities CUDAVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsVectorOps = true;
    caps.supportsGraphOps = false;
    caps.supportsGeoOps = false;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync = true;
    caps.supportedPrecisions = PrecisionMode::FP32;
    caps.supportedMetrics = metricBit(DistanceMetric::L2)
                          | metricBit(DistanceMetric::COSINE)
                          | metricBit(DistanceMetric::INNER_PRODUCT);
    
    if (isAvailable()) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            caps.deviceName = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits = prop.multiProcessorCount;
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
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        
        // Set structured error context
        if (deviceCount == 0) {
            setError(ErrorContextHelpers::createNoDevicesError("CUDA"));
        } else {
            setError(ErrorContext(
                AccelerationErrorCode::DriverNotInstalled,
                "CUDA",
                "CUDA driver or runtime not accessible: " + std::string(cudaGetErrorString(err)),
                "Install NVIDIA CUDA driver and runtime"
            ));
        }
        
        // Keep backward-compatible logging
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Set device with error context
    cudaError_t setDeviceErr = cudaSetDevice(0);
    if (setDeviceErr != cudaSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DeviceSetFailed,
            "CUDA",
            "Failed to set device 0: " + std::string(cudaGetErrorString(setDeviceErr)),
            "Check if device is available and not in exclusive mode"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    try {
        // v1.1.0: Create CUDA stream with low priority for vLLM co-location using RAII
#ifdef THEMIS_VLLM_COLOCATION
        // Non-blocking stream with low priority (doesn't block vLLM)
        int leastPriority, greatestPriority;
        cudaError_t err = cudaDeviceGetStreamPriorityRange(&leastPriority, &greatestPriority);
        if (err != cudaSuccess) {
            setError(ErrorContextHelpers::createQueueError("CUDA", 
                "Failed to get stream priority range: " + std::string(cudaGetErrorString(err))));
            std::cerr << lastError_.format() << std::endl;
            return false;
        }
        stream_.createWithPriority(leastPriority, cudaStreamNonBlocking);
        std::cout << "CUDA: Created low-priority stream for vLLM co-location (priority=" << leastPriority << ")" << std::endl;
#else
        // Standard stream for non-vLLM deployments (RAII-managed)
        stream_.create();
#endif
    } catch (const std::exception& e) {
        setError(ErrorContextHelpers::createQueueError("CUDA", std::string(e.what())));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Query device properties with error handling
    cudaDeviceProp prop;
    cudaError_t propErr = cudaGetDeviceProperties(&prop, 0);
    if (propErr != cudaSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DevicePropertiesQueryFailed,
            "CUDA",
            "Failed to query device properties: " + std::string(cudaGetErrorString(propErr)),
            "Ensure CUDA runtime is properly installed and device is accessible"
        ));
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
    std::cout << "  Global Memory: " << (prop.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;
    std::cout << "  Multiprocessors: " << prop.multiProcessorCount << std::endl;
    std::cout << "  CUDA Runtime: " << (runtimeVersion / 1000) << "." << ((runtimeVersion % 100) / 10) << std::endl;
#ifdef THEMIS_VLLM_COLOCATION
    std::cout << "  vLLM Co-Location: ENABLED (low-priority stream, max " << THEMIS_MAX_GPU_VRAM_MB << " MB VRAM)" << std::endl;
#endif
    
    // Clear error on success
    clearError();
    initialized_ = true;
    return true;
#else
    setError(ErrorContext(
        AccelerationErrorCode::FeatureNotSupported,
        "CUDA",
        "Not compiled with CUDA support (THEMIS_ENABLE_CUDA not defined)",
        "Recompile with CUDA support enabled"
    ));
    std::cerr << lastError_.format() << std::endl;
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
}

std::vector<float> CUDAVectorBackend::computeDistances(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    bool useL2
) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "CUDA",
            "CUDA backend not initialized",
            "Call initialize() before using the backend"
        ));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    // Input validation
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA", AccelerationErrorCode::InvalidInputShape,
            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA", AccelerationErrorCode::InvalidInputShape,
            "numQueries, numVectors, and dim must all be > 0"));
        return {};
    }

    cudaStream_t stream = stream_.get();

    const size_t querySize    = numQueries * dim      * sizeof(float);
    const size_t vectorSize   = numVectors * dim      * sizeof(float);
    const size_t distanceSize = numQueries * numVectors * sizeof(float);

    try {
        // RAII wrappers ensure no device memory leaks on any error path
        raii::CudaDeviceMemory d_queries(querySize);
        raii::CudaDeviceMemory d_vectors(vectorSize);
        raii::CudaDeviceMemory d_distances(distanceSize);

        d_queries.copyFrom(queries, querySize, stream);
        d_vectors.copyFrom(vectors, vectorSize, stream);

        if (useL2) {
            launchL2DistanceKernel(
                static_cast<const float*>(d_queries.get()),
                static_cast<const float*>(d_vectors.get()),
                static_cast<float*>(d_distances.get()),
                static_cast<int>(numQueries), static_cast<int>(numVectors),
                static_cast<int>(dim), stream);
        } else {
            launchCosineDistanceKernel(
                static_cast<const float*>(d_queries.get()),
                static_cast<const float*>(d_vectors.get()),
                static_cast<float*>(d_distances.get()),
                static_cast<int>(numQueries), static_cast<int>(numVectors),
                static_cast<int>(dim), stream);
        }

        std::vector<float> distances(numQueries * numVectors);
        d_distances.copyTo(distances.data(), distanceSize, stream);

        cudaError_t err = cudaStreamSynchronize(stream);
        if (err != cudaSuccess) {
            setError(ErrorContext(
                AccelerationErrorCode::SynchronizationFailed,
                "CUDA",
                "Stream synchronization failed: " + std::string(cudaGetErrorString(err)),
                "Check if the GPU is still responsive"
            ));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        clearError();
        return distances;
    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "CUDA",
            std::string("Device memory operation failed: ") + e.what(),
            "Reduce batch size or free GPU memory"
        ));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
#else
    return {};
#endif
}

std::vector<std::vector<std::pair<uint32_t, float>>> CUDAVectorBackend::batchKnnSearch(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    bool useL2
) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "CUDA",
            "CUDA backend not initialized",
            "Call initialize() before using the backend"
        ));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    // Input validation
    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA", AccelerationErrorCode::InvalidInputShape,
            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA", AccelerationErrorCode::InvalidInputShape,
            "numQueries, numVectors, dim, and k must all be > 0"));
        return {};
    }

    // Clamp k to available vectors to prevent out-of-bounds indexing
    const size_t effectiveK = std::min(k, numVectors);

    cudaStream_t stream = stream_.get();

    const size_t querySize    = numQueries  * dim         * sizeof(float);
    const size_t vectorSize   = numVectors  * dim         * sizeof(float);
    const size_t distanceSize = numQueries  * numVectors  * sizeof(float);
    const size_t topkIdxSize  = numQueries  * effectiveK  * sizeof(int);
    const size_t topkDistSize = numQueries  * effectiveK  * sizeof(float);

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
            launchL2DistanceKernel(
                static_cast<const float*>(d_queries.get()),
                static_cast<const float*>(d_vectors.get()),
                static_cast<float*>(d_distances.get()),
                static_cast<int>(numQueries), static_cast<int>(numVectors),
                static_cast<int>(dim), stream);
        } else {
            launchCosineDistanceKernel(
                static_cast<const float*>(d_queries.get()),
                static_cast<const float*>(d_vectors.get()),
                static_cast<float*>(d_distances.get()),
                static_cast<int>(numQueries), static_cast<int>(numVectors),
                static_cast<int>(dim), stream);
        }

        // Step 2: Extract top-k
        launchTopKKernel(
            static_cast<const float*>(d_distances.get()),
            static_cast<int*>(d_topkIndices.get()),
            static_cast<float*>(d_topkDistances.get()),
            static_cast<int>(numQueries), static_cast<int>(numVectors),
            static_cast<int>(effectiveK), stream);

        std::vector<int>   topkIndices  (numQueries * effectiveK);
        std::vector<float> topkDistances(numQueries * effectiveK);

        d_topkIndices  .copyTo(topkIndices.data(),   topkIdxSize,  stream);
        d_topkDistances.copyTo(topkDistances.data(), topkDistSize, stream);

        cudaError_t err = cudaStreamSynchronize(stream);
        if (err != cudaSuccess) {
            setError(ErrorContext(
                AccelerationErrorCode::SynchronizationFailed,
                "CUDA",
                "Stream synchronization failed: " + std::string(cudaGetErrorString(err)),
                "Check if the GPU is still responsive"
            ));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        for (size_t q = 0; q < numQueries; ++q) {
            results[q].reserve(effectiveK);
            for (size_t i = 0; i < effectiveK; ++i) {
                const size_t idx = q * effectiveK + i;
                results[q].emplace_back(
                    static_cast<uint32_t>(topkIndices[idx]),
                    topkDistances[idx]
                );
            }
        }

        clearError();
        return results;
    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "CUDA",
            std::string("Device memory operation failed: ") + e.what(),
            "Reduce batch size or free GPU memory"
        ));
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
    if (exec)  { cudaGraphExecDestroy(exec);  exec  = nullptr; }
    if (graph) { cudaGraphDestroy(graph);     graph = nullptr; }
}

CUDAGraphEntry::CUDAGraphEntry(CUDAGraphEntry&& o) noexcept
    : graph(o.graph), exec(o.exec),
      d_queries    (std::move(o.d_queries)),
      d_vectors    (std::move(o.d_vectors)),
      d_distances  (std::move(o.d_distances)),
      d_topkIndices(std::move(o.d_topkIndices)),
      d_topkDistances(std::move(o.d_topkDistances)),
      lastAccess(o.lastAccess)
{
    o.graph = nullptr;
    o.exec  = nullptr;
}

CUDAGraphEntry& CUDAGraphEntry::operator=(CUDAGraphEntry&& o) noexcept {
    if (this != &o) {
        if (exec)  { cudaGraphExecDestroy(exec);  exec  = nullptr; }
        if (graph) { cudaGraphDestroy(graph);     graph = nullptr; }

        graph       = o.graph;
        exec        = o.exec;
        d_queries   = std::move(o.d_queries);
        d_vectors   = std::move(o.d_vectors);
        d_distances = std::move(o.d_distances);
        d_topkIndices    = std::move(o.d_topkIndices);
        d_topkDistances  = std::move(o.d_topkDistances);
        lastAccess  = o.lastAccess;

        o.graph = nullptr;
        o.exec  = nullptr;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// CUDAGraphCache
// ---------------------------------------------------------------------------

CUDAGraphEntry* CUDAGraphCache::get(const QueryShape& shape) noexcept {
    auto it = entries_.find(shape);
    if (it == entries_.end()) return nullptr;
    it->second.lastAccess = ++clock_;
    return &it->second;
}

CUDAGraphEntry& CUDAGraphCache::put(const QueryShape& shape, CUDAGraphEntry entry) {
    // Only evict if this is truly a new key (not a replacement)
    if (entries_.size() >= kMaxEntries && entries_.count(shape) == 0) {
        evictLRU();
    }
    entry.lastAccess = ++clock_;
    auto res = entries_.insert_or_assign(shape, std::move(entry));
    return res.first->second;
}

void CUDAGraphCache::evictLRU() {
    if (entries_.empty()) return;
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
CUDAVectorBackend::batchKnnSearchWithGraph(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    DistanceMetric metric
) {
    if (!initialized_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "CUDA",
            "CUDA backend not initialized",
            "Call initialize() before using the backend"
        ));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    if (queries == nullptr || vectors == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA", AccelerationErrorCode::InvalidInputShape,
            "queries and vectors pointers must be non-null"));
        return {};
    }
    if (numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA", AccelerationErrorCode::InvalidInputShape,
            "numQueries, numVectors, dim, and k must all be > 0"));
        return {};
    }

    const size_t effectiveK = std::min(k, numVectors);

    const size_t querySize    = numQueries * dim       * sizeof(float);
    const size_t vectorSize   = numVectors * dim       * sizeof(float);
    const size_t distanceSize = numQueries * numVectors * sizeof(float);
    const size_t topkIdxSize  = numQueries * effectiveK * sizeof(int);
    const size_t topkDistSize = numQueries * effectiveK * sizeof(float);

    const QueryShape shape{
        static_cast<int>(numQueries),
        static_cast<int>(numVectors),
        static_cast<int>(dim),
        static_cast<int>(effectiveK),
        metric
    };

    cudaStream_t mainStream = stream_.get();

    try {
        // ------------------------------------------------------------------
        // Cache lookup (mutex-protected)
        // ------------------------------------------------------------------
        CUDAGraphEntry* entry = nullptr;
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
            newEntry.d_queries      = raii::CudaDeviceMemory(querySize);
            newEntry.d_vectors      = raii::CudaDeviceMemory(vectorSize);
            newEntry.d_distances    = raii::CudaDeviceMemory(distanceSize);
            newEntry.d_topkIndices  = raii::CudaDeviceMemory(topkIdxSize);
            newEntry.d_topkDistances= raii::CudaDeviceMemory(topkDistSize);

            // Initialize buffers so the capture is valid (kernels read valid data)
            cudaMemset(newEntry.d_queries.get(),    0, querySize);
            cudaMemset(newEntry.d_vectors.get(),    0, vectorSize);
            cudaMemset(newEntry.d_distances.get(),  0, distanceSize);
            cudaMemset(newEntry.d_topkIndices.get(),   0, topkIdxSize);
            cudaMemset(newEntry.d_topkDistances.get(), 0, topkDistSize);
            // cudaMemset is synchronous: it blocks the host until the fill is
            // complete, so the buffers are ready before capture begins.

            // Use a dedicated non-blocking stream for the capture
            cudaStream_t captureStream = nullptr;
            cudaError_t csErr = cudaStreamCreateWithFlags(&captureStream,
                                                          cudaStreamNonBlocking);
            if (csErr != cudaSuccess) {
                setError(ErrorContext(
                    AccelerationErrorCode::AllocationFailed, "CUDA",
                    "Failed to create capture stream: " +
                        std::string(cudaGetErrorString(csErr)),
                    "Check CUDA driver state"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Begin stream capture — capture kernel launches only
            cudaError_t capErr = cudaStreamBeginCapture(
                captureStream, cudaStreamCaptureModeGlobal);
            if (capErr != cudaSuccess) {
                cudaStreamDestroy(captureStream);
                setError(ErrorContext(
                    AccelerationErrorCode::AllocationFailed, "CUDA",
                    "cudaStreamBeginCapture failed: " +
                        std::string(cudaGetErrorString(capErr)),
                    "Ensure CUDA >= 10.0 and no concurrent capture in progress"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Capture distance kernel
            switch (metric) {
                case DistanceMetric::COSINE:
                    launchCosineDistanceKernel(
                        static_cast<const float*>(newEntry.d_queries.get()),
                        static_cast<const float*>(newEntry.d_vectors.get()),
                        static_cast<float*>(newEntry.d_distances.get()),
                        static_cast<int>(numQueries), static_cast<int>(numVectors),
                        static_cast<int>(dim), captureStream);
                    break;
                case DistanceMetric::INNER_PRODUCT:
                    launchInnerProductKernel(
                        static_cast<const float*>(newEntry.d_queries.get()),
                        static_cast<const float*>(newEntry.d_vectors.get()),
                        static_cast<float*>(newEntry.d_distances.get()),
                        static_cast<int>(numQueries), static_cast<int>(numVectors),
                        static_cast<int>(dim), captureStream);
                    break;
                default: // DistanceMetric::L2
                    launchL2DistanceKernel(
                        static_cast<const float*>(newEntry.d_queries.get()),
                        static_cast<const float*>(newEntry.d_vectors.get()),
                        static_cast<float*>(newEntry.d_distances.get()),
                        static_cast<int>(numQueries), static_cast<int>(numVectors),
                        static_cast<int>(dim), captureStream);
                    break;
            }

            // Capture top-K kernel
            launchTopKKernel(
                static_cast<const float*>(newEntry.d_distances.get()),
                static_cast<int*>(newEntry.d_topkIndices.get()),
                static_cast<float*>(newEntry.d_topkDistances.get()),
                static_cast<int>(numQueries), static_cast<int>(numVectors),
                static_cast<int>(effectiveK), captureStream);

            // End capture
            cudaError_t endErr = cudaStreamEndCapture(captureStream, &newEntry.graph);
            cudaStreamDestroy(captureStream);

            if (endErr != cudaSuccess || newEntry.graph == nullptr) {
                setError(ErrorContext(
                    AccelerationErrorCode::AllocationFailed, "CUDA",
                    "cudaStreamEndCapture failed: " +
                        std::string(cudaGetErrorString(endErr)),
                    "Verify CUDA version supports graph capture"));
                std::cerr << lastError_.format() << std::endl;
                return {};
            }

            // Instantiate the graph
#if defined(CUDART_VERSION) && (CUDART_VERSION >= 12000)
            cudaError_t instErr = cudaGraphInstantiate(
                &newEntry.exec, newEntry.graph, 0);
#else
            cudaError_t instErr = cudaGraphInstantiate(
                &newEntry.exec, newEntry.graph, nullptr, nullptr, 0);
#endif
            if (instErr != cudaSuccess || newEntry.exec == nullptr) {
                setError(ErrorContext(
                    AccelerationErrorCode::AllocationFailed, "CUDA",
                    "cudaGraphInstantiate failed: " +
                        std::string(cudaGetErrorString(instErr)),
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
        cudaMemcpyAsync(entry->d_queries.get(),  queries, querySize,
                        cudaMemcpyHostToDevice, mainStream);
        cudaMemcpyAsync(entry->d_vectors.get(), vectors, vectorSize,
                        cudaMemcpyHostToDevice, mainStream);

        // Launch the captured graph (runs after the H2D copies on the same stream)
        cudaError_t launchErr = cudaGraphLaunch(entry->exec, mainStream);
        if (launchErr != cudaSuccess) {
            setError(ErrorContext(
                AccelerationErrorCode::AllocationFailed, "CUDA",
                "cudaGraphLaunch failed: " +
                    std::string(cudaGetErrorString(launchErr)),
                "Inspect CUDA graph validity and available device memory"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        // D2H: copy results from pre-allocated buffers back to host
        std::vector<int>   topkIndices  (numQueries * effectiveK);
        std::vector<float> topkDistances(numQueries * effectiveK);

        cudaMemcpyAsync(topkIndices.data(),
                        entry->d_topkIndices.get(), topkIdxSize,
                        cudaMemcpyDeviceToHost, mainStream);
        cudaMemcpyAsync(topkDistances.data(),
                        entry->d_topkDistances.get(), topkDistSize,
                        cudaMemcpyDeviceToHost, mainStream);

        cudaError_t syncErr = cudaStreamSynchronize(mainStream);
        if (syncErr != cudaSuccess) {
            setError(ErrorContext(
                AccelerationErrorCode::SynchronizationFailed, "CUDA",
                "Stream synchronization failed: " +
                    std::string(cudaGetErrorString(syncErr)),
                "Check if the GPU is still responsive"));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        // Package results
        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        for (size_t q = 0; q < numQueries; ++q) {
            results[q].reserve(effectiveK);
            for (size_t i = 0; i < effectiveK; ++i) {
                const size_t idx = q * effectiveK + i;
                results[q].emplace_back(
                    static_cast<uint32_t>(topkIndices[idx]),
                    topkDistances[idx]
                );
            }
        }

        clearError();
        return results;

    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed, "CUDA",
            std::string("Graph capture/replay failed: ") + e.what(),
            "Reduce batch size or free GPU memory"));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
}

#else // !THEMIS_ENABLE_CUDA

std::vector<std::vector<std::pair<uint32_t, float>>>
CUDAVectorBackend::batchKnnSearchWithGraph(
    const float* /*queries*/, size_t /*numQueries*/, size_t /*dim*/,
    const float* /*vectors*/, size_t /*numVectors*/, size_t /*k*/,
    DistanceMetric /*metric*/
) {
    return {};
}

#endif // THEMIS_ENABLE_CUDA

// ============================================================================
// CUDAGraphBackend Stub Implementation
// ============================================================================

CUDAGraphBackend::~CUDAGraphBackend() {
    shutdown();
}

bool CUDAGraphBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    return false; // Stub
#else
    return false;
#endif
}

BackendCapabilities CUDAGraphBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsGraphOps = true;
    caps.deviceName = "CUDA Device (Stub)";
#endif
    return caps;
}

bool CUDAGraphBackend::initialize() {
#ifdef THEMIS_ENABLE_CUDA
    initialized_ = false; // Stub
    return initialized_;
#else
    return false;
#endif
}

void CUDAGraphBackend::shutdown() {
#ifdef THEMIS_ENABLE_CUDA
    initialized_ = false;
#endif
}

std::vector<std::vector<uint32_t>> CUDAGraphBackend::batchBFS(
    const uint32_t* /*adjacency*/,
    size_t /*numVertices*/,
    const uint32_t* /*startVertices*/,
    size_t /*numStarts*/,
    uint32_t /*maxDepth*/
) {
    return {}; // Stub
}

std::vector<std::vector<uint32_t>> CUDAGraphBackend::batchShortestPath(
    const uint32_t* /*adjacency*/,
    const float* /*weights*/,
    size_t /*numVertices*/,
    const uint32_t* /*startVertices*/,
    const uint32_t* /*endVertices*/,
    size_t /*numPairs*/
) {
    return {}; // Stub
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
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    return (err == cudaSuccess && deviceCount > 0);
#else
    return false;
#endif
}

BackendCapabilities CUDAGeoBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsGeoOps = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync = true;
    caps.supportedPrecisions = PrecisionMode::FP32;
    if (isAvailable()) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            caps.deviceName = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits = prop.multiProcessorCount;
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
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        if (deviceCount == 0) {
            setError(ErrorContextHelpers::createNoDevicesError("CUDA-Geo"));
        } else {
            setError(ErrorContext(
                AccelerationErrorCode::DriverNotInstalled,
                "CUDA-Geo",
                "CUDA driver or runtime not accessible: " + std::string(cudaGetErrorString(err)),
                "Install NVIDIA CUDA driver and runtime"
            ));
        }
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    cudaError_t setDeviceErr = cudaSetDevice(0);
    if (setDeviceErr != cudaSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DeviceSetFailed,
            "CUDA-Geo",
            "Failed to set device 0: " + std::string(cudaGetErrorString(setDeviceErr)),
            "Check if device is available and not in exclusive mode"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    try {
        stream_.create();
    } catch (const std::exception& e) {
        setError(ErrorContextHelpers::createQueueError("CUDA-Geo", std::string(e.what())));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    cudaDeviceProp prop;
    cudaError_t propErr = cudaGetDeviceProperties(&prop, 0);
    if (propErr != cudaSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DevicePropertiesQueryFailed,
            "CUDA-Geo",
            "Failed to query device properties: " + std::string(cudaGetErrorString(propErr)),
            "Ensure CUDA runtime is properly installed and device is accessible"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    std::cout << "CUDA Geo Backend initialized successfully:" << std::endl;
    std::cout << "  Device: " << prop.name << std::endl;

    clearError();
    initialized_ = true;
    return true;
#else
    setError(ErrorContext(
        AccelerationErrorCode::FeatureNotSupported,
        "CUDA-Geo",
        "Not compiled with CUDA support (THEMIS_ENABLE_CUDA not defined)",
        "Recompile with CUDA support enabled"
    ));
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

std::vector<float> CUDAGeoBackend::batchDistances(
    const double* latitudes1,
    const double* longitudes1,
    const double* latitudes2,
    const double* longitudes2,
    size_t count,
    bool useHaversine
) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "CUDA-Geo",
            "CUDA geo backend not initialized",
            "Call initialize() before using the backend"
        ));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    if (latitudes1 == nullptr || longitudes1 == nullptr ||
        latitudes2 == nullptr || longitudes2 == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA-Geo", AccelerationErrorCode::InvalidInputShape,
            "input coordinate pointers must be non-null"));
        return {};
    }
    if (count == 0) {
        return {};
    }

    const GeoDistanceFormula formula = useHaversine
        ? GeoDistanceFormula::HAVERSINE
        : GeoDistanceFormula::VINCENTY;

    cudaStream_t stream = stream_.get();

    const size_t coordSize = count * sizeof(double);
    const size_t distSize  = count * sizeof(float);

    try {
        raii::CudaDeviceMemory d_lats1(coordSize);
        raii::CudaDeviceMemory d_lons1(coordSize);
        raii::CudaDeviceMemory d_lats2(coordSize);
        raii::CudaDeviceMemory d_lons2(coordSize);
        raii::CudaDeviceMemory d_distances(distSize);

        d_lats1.copyFrom(latitudes1,  coordSize, stream);
        d_lons1.copyFrom(longitudes1, coordSize, stream);
        d_lats2.copyFrom(latitudes2,  coordSize, stream);
        d_lons2.copyFrom(longitudes2, coordSize, stream);

        const int rc = launchGeoDistanceKernel(
            static_cast<const double*>(d_lats1.get()),
            static_cast<const double*>(d_lons1.get()),
            static_cast<const double*>(d_lats2.get()),
            static_cast<const double*>(d_lons2.get()),
            static_cast<float*>(d_distances.get()),
            static_cast<int>(count),
            formula,
            stream);

        if (rc != 0) {
            setError(ErrorContext(
                AccelerationErrorCode::KernelLaunchFailed,
                "CUDA-Geo",
                "launchGeoDistanceKernel failed with code " + std::to_string(rc),
                "Check CUDA device state"
            ));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<float> result(count);
        d_distances.copyTo(result.data(), distSize, stream);

        cudaError_t err = cudaStreamSynchronize(stream);
        if (err != cudaSuccess) {
            setError(ErrorContext(
                AccelerationErrorCode::SynchronizationFailed,
                "CUDA-Geo",
                "Stream synchronization failed: " + std::string(cudaGetErrorString(err)),
                "Check if the GPU is still responsive"
            ));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        clearError();
        return result;
    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "CUDA-Geo",
            std::string("Device memory operation failed: ") + e.what(),
            "Reduce batch size or free GPU memory"
        ));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }
#else
    return {};
#endif
}

std::vector<bool> CUDAGeoBackend::batchPointInPolygon(
    const double* pointLats,
    const double* pointLons,
    size_t numPoints,
    const double* polygonCoords,
    size_t numPolygonVertices
) {
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendNotInitialized,
            "CUDA-Geo",
            "CUDA geo backend not initialized",
            "Call initialize() before using the backend"
        ));
        std::cerr << lastError_.format() << std::endl;
        return {};
    }

    if (pointLats == nullptr || pointLons == nullptr || polygonCoords == nullptr) {
        setError(ErrorContextHelpers::createValidationError(
            "CUDA-Geo", AccelerationErrorCode::InvalidInputShape,
            "input coordinate pointers must be non-null"));
        return {};
    }
    if (numPoints == 0 || numPolygonVertices < 3) {
        return {};
    }

    cudaStream_t stream = stream_.get();

    const size_t pointSize   = numPoints          * sizeof(double);
    const size_t polySize    = numPolygonVertices * 2 * sizeof(double);
    const size_t resultSize  = numPoints          * sizeof(uint8_t);

    try {
        raii::CudaDeviceMemory d_pointLats(pointSize);
        raii::CudaDeviceMemory d_pointLons(pointSize);
        raii::CudaDeviceMemory d_polyCoords(polySize);
        raii::CudaDeviceMemory d_results(resultSize);

        d_pointLats.copyFrom(pointLats,      pointSize, stream);
        d_pointLons.copyFrom(pointLons,      pointSize, stream);
        d_polyCoords.copyFrom(polygonCoords, polySize,  stream);

        const int rc = launchGeoContainmentKernel(
            static_cast<const double*>(d_pointLats.get()),
            static_cast<const double*>(d_pointLons.get()),
            static_cast<int>(numPoints),
            static_cast<const double*>(d_polyCoords.get()),
            static_cast<int>(numPolygonVertices),
            static_cast<uint8_t*>(d_results.get()),
            stream);

        if (rc != 0) {
            setError(ErrorContext(
                AccelerationErrorCode::KernelLaunchFailed,
                "CUDA-Geo",
                "launchGeoContainmentKernel failed with code " + std::to_string(rc),
                "Check CUDA device state"
            ));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<uint8_t> rawResults(numPoints);
        d_results.copyTo(rawResults.data(), resultSize, stream);

        cudaError_t err = cudaStreamSynchronize(stream);
        if (err != cudaSuccess) {
            setError(ErrorContext(
                AccelerationErrorCode::SynchronizationFailed,
                "CUDA-Geo",
                "Stream synchronization failed: " + std::string(cudaGetErrorString(err)),
                "Check if the GPU is still responsive"
            ));
            std::cerr << lastError_.format() << std::endl;
            return {};
        }

        std::vector<bool> result(numPoints);
        for (size_t i = 0; i < numPoints; ++i) {
            result[i] = (rawResults[i] != 0);
        }

        clearError();
        return result;
    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::AllocationFailed,
            "CUDA-Geo",
            std::string("Device memory operation failed: ") + e.what(),
            "Reduce batch size or free GPU memory"
        ));
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
// Adapts the legacy void-return CUDA launchers from cuda/vector_kernels.cu
// to the frozen ANNDistanceFn / ANNTopKFn signatures (return int, 0=success).
// Under THEMIS_ENABLE_CUDA the wrappers call the real kernels; otherwise all
// slots remain null so the BackendRegistry falls back to the CPU table.
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

namespace {

static int cuda_ann_l2_dispatch(
    const float* d_queries, const float* d_vectors, float* d_distances,
    int numQueries, int numVectors, int dim, void* opaque_stream)
{
    launchL2DistanceKernel(d_queries, d_vectors, d_distances,
                           numQueries, numVectors, dim,
                           static_cast<cudaStream_t>(opaque_stream));
    return static_cast<int>(cudaGetLastError());
}

static int cuda_ann_cosine_dispatch(
    const float* d_queries, const float* d_vectors, float* d_distances,
    int numQueries, int numVectors, int dim, void* opaque_stream)
{
    launchCosineDistanceKernel(d_queries, d_vectors, d_distances,
                               numQueries, numVectors, dim,
                               static_cast<cudaStream_t>(opaque_stream));
    return static_cast<int>(cudaGetLastError());
}

static int cuda_ann_inner_product_dispatch(
    const float* d_queries, const float* d_vectors, float* d_distances,
    int numQueries, int numVectors, int dim, void* opaque_stream)
{
    launchInnerProductKernel(d_queries, d_vectors, d_distances,
                             numQueries, numVectors, dim,
                             static_cast<cudaStream_t>(opaque_stream));
    return static_cast<int>(cudaGetLastError());
}

static int cuda_ann_topk_dispatch(
    const float* d_distances, uint32_t* d_topk_indices, float* d_topk_dists,
    int numQueries, int numVectors, int topK, void* opaque_stream)
{
    // Guard: the legacy launcher uses int* for indices.  Verify they are the
    // same size so the reinterpret_cast below is safe.
    static_assert(sizeof(uint32_t) == sizeof(int),
                  "uint32_t and int must have the same size for index cast");
    launchTopKKernel(d_distances,
                     reinterpret_cast<int*>(d_topk_indices), d_topk_dists,
                     numQueries, numVectors, topK,
                     static_cast<cudaStream_t>(opaque_stream));
    return static_cast<int>(cudaGetLastError());
}

} // anonymous namespace

#endif // THEMIS_ENABLE_CUDA

namespace acceleration {

ANNKernelDispatch CUDAVectorBackend::populateANNDispatch() const {
#ifdef THEMIS_ENABLE_CUDA
    ANNKernelDispatch d;
    d.launchL2Distance   = cuda_ann_l2_dispatch;
    d.launchCosine       = cuda_ann_cosine_dispatch;
    d.launchInnerProduct = cuda_ann_inner_product_dispatch;
    d.launchTopK         = cuda_ann_topk_dispatch;
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
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    return (err == cudaSuccess && deviceCount > 0);
#else
    return false;
#endif
}

BackendCapabilities CUDAMatrixBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsMatrixOps     = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync         = true;
    // FP16 from SM 7.0 (Volta); BF16 from SM 8.0 (Ampere).
    // Advertise both — the kernel selection inside dispatchMatmul handles
    // the actual hardware capability at runtime via cuBLAS.
    caps.supportedPrecisions   = PrecisionMode::FP32
                               | PrecisionMode::FP16
                               | PrecisionMode::BF16;
    if (isAvailable()) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            caps.deviceName    = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits  = prop.multiProcessorCount;
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

int CUDAMatrixBackend::matmul(const MatrixKernelParams& params, void* opaque_stream)
{
#ifdef THEMIS_ENABLE_CUDA
    if (!initialized_) return 1;
    cudaStream_t stream = opaque_stream
        ? static_cast<cudaStream_t>(opaque_stream)
        : static_cast<cudaStream_t>(stream_.get());
    return tensor_core::dispatchMatmul(params, stream);
#else
    (void)params; (void)opaque_stream;
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
