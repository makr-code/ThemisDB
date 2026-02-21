/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_backend.cpp                                   ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🔴 ALPHA                                        ║
    • Quality Score:   35.0/100                                       ║
    • Total Lines:     626                                            ║
    • Open Issues:     TODOs: 0, Stubs: 13                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error in " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(err) << std::endl; \
            return {}; \
        } \
    } while(0)

#define CUDA_CHECK_BOOL(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error in " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(err) << std::endl; \
            return false; \
        } \
    } while(0)

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
        std::cerr << "CUDA backend not initialized" << std::endl;
        return {};
    }
    
    // Use RAII-managed stream
    cudaStream_t stream = stream_.get();
    
    // Allocate device memory
    float *d_queries, *d_vectors, *d_distances;
    size_t querySize = numQueries * dim * sizeof(float);
    size_t vectorSize = numVectors * dim * sizeof(float);
    size_t distanceSize = numQueries * numVectors * sizeof(float);
    
    CUDA_CHECK(cudaMalloc(&d_queries, querySize));
    CUDA_CHECK(cudaMalloc(&d_vectors, vectorSize));
    CUDA_CHECK(cudaMalloc(&d_distances, distanceSize));
    
    // Copy data to device
    CUDA_CHECK(cudaMemcpyAsync(d_queries, queries, querySize, cudaMemcpyHostToDevice, stream));
    CUDA_CHECK(cudaMemcpyAsync(d_vectors, vectors, vectorSize, cudaMemcpyHostToDevice, stream));
    
    // Launch kernel
    if (useL2) {
        launchL2DistanceKernel(d_queries, d_vectors, d_distances,
                              numQueries, numVectors, dim, stream);
    } else {
        launchCosineDistanceKernel(d_queries, d_vectors, d_distances,
                                  numQueries, numVectors, dim, stream);
    }
    
    // Copy results back
    std::vector<float> distances(numQueries * numVectors);
    CUDA_CHECK(cudaMemcpyAsync(distances.data(), d_distances, distanceSize,
                               cudaMemcpyDeviceToHost, stream));
    
    // Synchronize
    CUDA_CHECK(cudaStreamSynchronize(stream));
    
    // Cleanup
    cudaFree(d_queries);
    cudaFree(d_vectors);
    cudaFree(d_distances);
    
    return distances;
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
        std::cerr << "CUDA backend not initialized" << std::endl;
        return {};
    }
    
    cudaStream_t stream = stream_.get();
    
    // Allocate device memory
    float *d_queries, *d_vectors, *d_distances;
    int *d_topkIndices;
    float *d_topkDistances;
    
    size_t querySize = numQueries * dim * sizeof(float);
    size_t vectorSize = numVectors * dim * sizeof(float);
    size_t distanceSize = numQueries * numVectors * sizeof(float);
    size_t topkIdxSize = numQueries * k * sizeof(int);
    size_t topkDistSize = numQueries * k * sizeof(float);
    
    CUDA_CHECK(cudaMalloc(&d_queries, querySize));
    CUDA_CHECK(cudaMalloc(&d_vectors, vectorSize));
    CUDA_CHECK(cudaMalloc(&d_distances, distanceSize));
    CUDA_CHECK(cudaMalloc(&d_topkIndices, topkIdxSize));
    CUDA_CHECK(cudaMalloc(&d_topkDistances, topkDistSize));
    
    // Copy data to device
    CUDA_CHECK(cudaMemcpyAsync(d_queries, queries, querySize, cudaMemcpyHostToDevice, stream));
    CUDA_CHECK(cudaMemcpyAsync(d_vectors, vectors, vectorSize, cudaMemcpyHostToDevice, stream));
    
    // Step 1: Compute distances
    if (useL2) {
        launchL2DistanceKernel(d_queries, d_vectors, d_distances,
                              numQueries, numVectors, dim, stream);
    } else {
        launchCosineDistanceKernel(d_queries, d_vectors, d_distances,
                                  numQueries, numVectors, dim, stream);
    }
    
    // Step 2: Extract top-k
    launchTopKKernel(d_distances, d_topkIndices, d_topkDistances,
                    numQueries, numVectors, k, stream);
    
    // Copy results back
    std::vector<int> topkIndices(numQueries * k);
    std::vector<float> topkDistances(numQueries * k);
    
    CUDA_CHECK(cudaMemcpyAsync(topkIndices.data(), d_topkIndices, topkIdxSize,
                               cudaMemcpyDeviceToHost, stream));
    CUDA_CHECK(cudaMemcpyAsync(topkDistances.data(), d_topkDistances, topkDistSize,
                               cudaMemcpyDeviceToHost, stream));
    
    // Synchronize
    CUDA_CHECK(cudaStreamSynchronize(stream));
    
    // Cleanup device memory
    cudaFree(d_queries);
    cudaFree(d_vectors);
    cudaFree(d_distances);
    cudaFree(d_topkIndices);
    cudaFree(d_topkDistances);
    
    // Convert to output format
    std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
    for (size_t q = 0; q < numQueries; ++q) {
        results[q].reserve(k);
        for (size_t i = 0; i < k; ++i) {
            size_t idx = q * k + i;
            results[q].emplace_back(
                static_cast<uint32_t>(topkIndices[idx]),
                topkDistances[idx]
            );
        }
    }
    
    return results;
#else
    return {};
#endif
}

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
// CUDAGeoBackend Stub Implementation
// ============================================================================

CUDAGeoBackend::~CUDAGeoBackend() {
    shutdown();
}

bool CUDAGeoBackend::isAvailable() const noexcept {
#ifdef THEMIS_ENABLE_CUDA
    return false; // Stub
#else
    return false;
#endif
}

BackendCapabilities CUDAGeoBackend::getCapabilities() const {
    BackendCapabilities caps;
#ifdef THEMIS_ENABLE_CUDA
    caps.supportsGeoOps = true;
    caps.deviceName = "CUDA Device (Stub)";
#endif
    return caps;
}

bool CUDAGeoBackend::initialize() {
#ifdef THEMIS_ENABLE_CUDA
    initialized_ = false; // Stub
    return initialized_;
#else
    return false;
#endif
}

void CUDAGeoBackend::shutdown() {
#ifdef THEMIS_ENABLE_CUDA
    initialized_ = false;
#endif
}

std::vector<float> CUDAGeoBackend::batchDistances(
    const double* /*latitudes1*/,
    const double* /*longitudes1*/,
    const double* /*latitudes2*/,
    const double* /*longitudes2*/,
    size_t /*count*/,
    bool /*useHaversine*/
) {
    return {}; // Stub
}

std::vector<bool> CUDAGeoBackend::batchPointInPolygon(
    const double* /*pointLats*/,
    const double* /*pointLons*/,
    size_t /*numPoints*/,
    const double* /*polygonCoords*/,
    size_t /*numPolygonVertices*/
) {
    return {}; // Stub
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
    // Inner-product not yet implemented for CUDA — slot remains null (CPU fallback)
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

} // namespace acceleration
} // namespace themis
