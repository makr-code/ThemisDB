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
    • Maturity Level:  🔴 ALPHA                                        ║
    • Quality Score:   35.0/100                                       ║
    • Total Lines:     622                                            ║
    • Open Issues:     TODOs: 0, Stubs: 13                            ║
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

} // namespace acceleration
} // namespace themis
