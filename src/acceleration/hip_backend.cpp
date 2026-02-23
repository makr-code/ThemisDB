/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hip_backend.cpp                                    ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     697                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// HIP Backend Implementation for AMD GPUs
// Provides GPU acceleration using AMD ROCm/HIP platform
// Compatible with AMD Radeon GPUs

#include "acceleration/hip_backend.h"
#include "acceleration/batch_validator.h"
#include "acceleration/compute_backend.h"
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"

#ifdef THEMIS_ENABLE_HIP
#include "acceleration/raii/hip_raii.h"
#endif

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstring>
#include <stdexcept>

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>

namespace themis {
namespace acceleration {

// ============================================================================
// HIP Helper Macros
// ============================================================================

#define HIP_CHECK(call) \
    do { \
        hipError_t error = call; \
        if (error != hipSuccess) { \
            std::cerr << "HIP error in " << __FILE__ << ":" << __LINE__ \
                      << " - " << hipGetErrorString(error) << std::endl; \
            return false; \
        } \
    } while(0)

#define HIP_CHECK_THROW(call) \
    do { \
        hipError_t error = call; \
        if (error != hipSuccess) { \
            throw std::runtime_error(std::string("HIP error: ") + hipGetErrorString(error)); \
        } \
    } while(0)

// ============================================================================
// HIP Kernels (similar to CUDA)
// ============================================================================

// Compute L2 distance kernel
__global__ void computeL2DistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ distances,
    int numQueries,
    int numVectors,
    int dim
) {
    int qIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (qIdx >= numQueries || vIdx >= numVectors) return;
    
    const float* query = queries + qIdx * dim;
    const float* vector = vectors + vIdx * dim;
    
    float sum = 0.0f;
    
    #pragma unroll 4
    for (int i = 0; i < dim; i++) {
        float diff = query[i] - vector[i];
        sum += diff * diff;
    }
    
    // Return squared distance to match CPU implementation
    distances[qIdx * numVectors + vIdx] = sum;
}

// Compute Cosine distance kernel
__global__ void computeCosineDistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ distances,
    int numQueries,
    int numVectors,
    int dim
) {
    int qIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (qIdx >= numQueries || vIdx >= numVectors) return;
    
    const float* query = queries + qIdx * dim;
    const float* vector = vectors + vIdx * dim;
    
    float dotProduct = 0.0f;
    float normQuery = 0.0f;
    float normVector = 0.0f;
    
    #pragma unroll 4
    for (int i = 0; i < dim; i++) {
        float q = query[i];
        float v = vector[i];
        dotProduct += q * v;
        normQuery += q * q;
        normVector += v * v;
    }
    
    normQuery = sqrtf(normQuery);
    normVector = sqrtf(normVector);
    
    float cosineSim = (normQuery > 1e-10f && normVector > 1e-10f)
        ? dotProduct / (normQuery * normVector)
        : 0.0f;
    
    distances[qIdx * numVectors + vIdx] = 1.0f - cosineSim;
}

// Compute Inner Product distance kernel
// Inner Product similarity: dot(a, b), distance = max(0, -dot(a, b))
__global__ void computeInnerProductDistanceKernel(
    const float* __restrict__ queries,
    const float* __restrict__ vectors,
    float* __restrict__ distances,
    int numQueries,
    int numVectors,
    int dim
) {
    int qIdx = blockIdx.y * blockDim.y + threadIdx.y;
    int vIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (qIdx >= numQueries || vIdx >= numVectors) return;
    
    const float* query = queries + qIdx * dim;
    const float* vector = vectors + vIdx * dim;
    
    float dotProduct = 0.0f;
    
    #pragma unroll 4
    for (int i = 0; i < dim; i++) {
        dotProduct += query[i] * vector[i];
    }
    
    // Distance is max(0, -dot) for inner product
    distances[qIdx * numVectors + vIdx] = fmaxf(0.0f, -dotProduct);
}

// Top-K selection kernel using parallel reduction
// Selects k nearest neighbors for each query
// Note: Uses bubble sort for simplicity. For k > 32, consider heap-based or radix select.
__global__ void topKSelectionKernel(
    const float* __restrict__ distances,
    uint32_t* __restrict__ indices,
    float* __restrict__ topKDistances,
    int numQueries,
    int numVectors,
    int k
) {
    int qIdx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (qIdx >= numQueries) return;
    
    const float* queryDistances = distances + qIdx * numVectors;
    uint32_t* queryIndices = indices + qIdx * k;
    float* queryTopK = topKDistances + qIdx * k;
    
    // Initialize with first k elements
    for (int i = 0; i < k && i < numVectors; i++) {
        queryIndices[i] = i;
        queryTopK[i] = queryDistances[i];
    }
    
    // Sort initial k elements (bubble sort works well for small k < 32)
    // TODO: For larger k, consider heap-based selection or radix select
    for (int i = 0; i < k - 1; i++) {
        for (int j = 0; j < k - i - 1; j++) {
            if (queryTopK[j] > queryTopK[j + 1]) {
                float tmpDist = queryTopK[j];
                queryTopK[j] = queryTopK[j + 1];
                queryTopK[j + 1] = tmpDist;
                
                uint32_t tmpIdx = queryIndices[j];
                queryIndices[j] = queryIndices[j + 1];
                queryIndices[j + 1] = tmpIdx;
            }
        }
    }
    
    // Process remaining elements
    for (int i = k; i < numVectors; i++) {
        float dist = queryDistances[i];
        
        // If this distance is smaller than largest in top-k, insert it
        if (dist < queryTopK[k - 1]) {
            int insertPos = k - 1;
            
            // Find insertion position
            while (insertPos > 0 && dist < queryTopK[insertPos - 1]) {
                insertPos--;
            }
            
            // Shift elements
            for (int j = k - 1; j > insertPos; j--) {
                queryTopK[j] = queryTopK[j - 1];
                queryIndices[j] = queryIndices[j - 1];
            }
            
            // Insert new element
            queryTopK[insertPos] = dist;
            queryIndices[insertPos] = i;
        }
    }
}

// ============================================================================
// HIPBackendImpl - Internal implementation
// ============================================================================

struct HIPBackendImpl {
    bool initialized = false;
    int deviceId = 0;
    raii::HipStream stream;  // RAII-managed stream (automatic cleanup)
    HIPVectorBackend::HIPConfig config;
    
    // Device properties
    hipDeviceProp_t deviceProps;
    
    // Destructor no longer needs manual cleanup - RAII handles it
    ~HIPBackendImpl() = default;
};

// ============================================================================
// HIPVectorBackend Implementation
// ============================================================================

HIPVectorBackend::HIPVectorBackend()
    : impl_(std::make_unique<HIPBackendImpl>()) {
}

HIPVectorBackend::HIPVectorBackend(const HIPConfig& config)
    : impl_(std::make_unique<HIPBackendImpl>()) {
    impl_->config = config;
}

HIPVectorBackend::~HIPVectorBackend() {
    shutdown();
}

const char* HIPVectorBackend::name() const noexcept {
    return "HIP";
}

BackendType HIPVectorBackend::type() const noexcept {
    return BackendType::HIP;
}

bool HIPVectorBackend::isAvailable() const noexcept {
    int deviceCount = 0;
    hipError_t error = hipGetDeviceCount(&deviceCount);
    return (error == hipSuccess && deviceCount > 0);
}

BackendCapabilities HIPVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
    caps.supportsVectorOps = true;
    caps.supportsGraphOps = false;
    caps.supportsGeoOps = false;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync = true;
    caps.supportedPrecisions = PrecisionMode::FP32;
    caps.supportedMetrics = metricBit(DistanceMetric::L2)
                          | metricBit(DistanceMetric::COSINE)
                          | metricBit(DistanceMetric::INNER_PRODUCT);
    
    if (impl_->initialized) {
        caps.deviceName = std::string(impl_->deviceProps.name) + " (HIP)";
        caps.maxMemoryBytes = impl_->deviceProps.totalGlobalMem;
        caps.computeUnits = impl_->deviceProps.multiProcessorCount;
    } else {
        caps.deviceName = "AMD GPU (HIP - not initialized)";
    }
    
    return caps;
}

bool HIPVectorBackend::initialize() {
    if (impl_->initialized) {
        setError(ErrorContext(
            AccelerationErrorCode::BackendAlreadyInitialized,
            "HIP",
            "Backend is already initialized",
            "Call shutdown() before reinitializing"
        ));
        return true;  // Not an error, just already initialized
    }
    
    std::cout << "HIP Backend: Initializing..." << std::endl;
    
    int deviceCount = 0;
    hipError_t countErr = hipGetDeviceCount(&deviceCount);
    
    if (countErr != hipSuccess || deviceCount == 0) {
        // Set structured error context
        if (deviceCount == 0) {
            setError(ErrorContextHelpers::createNoDevicesError("HIP"));
        } else {
            setError(ErrorContextHelpers::createDriverError("HIP"));
        }
        
        // Keep backward-compatible logging
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Select device
    int deviceId = impl_->config.deviceId;
    if (deviceId < 0 || deviceId >= deviceCount) {
        // Auto-select device with most compute units
        int maxCUs = 0;
        deviceId = 0;
        
        for (int i = 0; i < deviceCount; i++) {
            hipDeviceProp_t prop;
            if (hipGetDeviceProperties(&prop, i) == hipSuccess) {
                std::cout << "Device " << i << ": " << prop.name 
                          << " (" << prop.multiProcessorCount << " CUs, " 
                          << prop.gcnArchName << ")" << std::endl;
                
                if (prop.multiProcessorCount > maxCUs) {
                    maxCUs = prop.multiProcessorCount;
                    deviceId = i;
                }
            }
        }
    }
    
    impl_->deviceId = deviceId;
    
    hipError_t setDeviceErr = hipSetDevice(impl_->deviceId);
    if (setDeviceErr != hipSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DeviceSetFailed,
            "HIP",
            "Failed to set device " + std::to_string(impl_->deviceId) + ": " + hipGetErrorString(setDeviceErr),
            "Check if device exists and is not in exclusive mode"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    hipError_t propErr = hipGetDeviceProperties(&impl_->deviceProps, impl_->deviceId);
    if (propErr != hipSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DevicePropertiesQueryFailed,
            "HIP",
            "Failed to query device properties: " + std::string(hipGetErrorString(propErr)),
            "Ensure ROCm runtime is properly installed"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // ROCm runtime version
    int runtimeVersion = 0;
    hipRuntimeGetVersion(&runtimeVersion);
    
    std::cout << "HIP Backend: Selected device " << impl_->deviceId 
              << " (" << impl_->deviceProps.name << ")" << std::endl;
    std::cout << "  Compute Units: " << impl_->deviceProps.multiProcessorCount << std::endl;
    std::cout << "  Global Memory: " << (impl_->deviceProps.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;
    std::cout << "  Warp Size: " << impl_->deviceProps.warpSize << std::endl;
    std::cout << "  GCN Arch: " << impl_->deviceProps.gcnArchName << std::endl;
    std::cout << "  ROCm Runtime: " << (runtimeVersion / 10000000) << "." 
              << ((runtimeVersion / 100000) % 100) << std::endl;
    
    // Auto-detect wave size if not specified
    if (impl_->config.waveSize == 0) {
        impl_->config.waveSize = impl_->deviceProps.warpSize;
        std::cout << "  Auto-detected Wave Size: " << impl_->config.waveSize << std::endl;
    }
    
    // Create stream for async operations using RAII
    try {
        impl_->stream.create();
    } catch (const std::exception& e) {
        setError(ErrorContextHelpers::createQueueError("HIP", e.what()));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Clear error on success
    clearError();
    impl_->initialized = true;
    return true;
}

void HIPVectorBackend::shutdown() {
    if (impl_->initialized) {
        // stream automatically destroyed by RAII
        impl_->initialized = false;
    }
}

std::vector<float> HIPVectorBackend::computeDistances(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    bool useL2
) {
    auto sink = [this](ErrorContext e){ setError(std::move(e)); };
    if (!BatchValidator::validateVectorBatch(name(), queries, numQueries, dim,
                                             vectors, numVectors, sink)) {
        return {};
    }

    if (!impl_->initialized) {
        std::cerr << "HIP backend not initialized" << std::endl;
        return {};
    }
    
    // Allocate device memory
    float *d_queries = nullptr, *d_vectors = nullptr, *d_distances = nullptr;
    
    size_t queriesSize = numQueries * dim * sizeof(float);
    size_t vectorsSize = numVectors * dim * sizeof(float);
    size_t distancesSize = numQueries * numVectors * sizeof(float);
    
    try {
        HIP_CHECK_THROW(hipMalloc(&d_queries, queriesSize));
        HIP_CHECK_THROW(hipMalloc(&d_vectors, vectorsSize));
        HIP_CHECK_THROW(hipMalloc(&d_distances, distancesSize));
        
        // Copy data to device
        HIP_CHECK_THROW(hipMemcpy(d_queries, queries, queriesSize, hipMemcpyHostToDevice));
        HIP_CHECK_THROW(hipMemcpy(d_vectors, vectors, vectorsSize, hipMemcpyHostToDevice));
        
        // Launch kernel
        dim3 blockSize(16, 16);
        dim3 gridSize(
            (numVectors + blockSize.x - 1) / blockSize.x,
            (numQueries + blockSize.y - 1) / blockSize.y
        );
        
        if (useL2) {
            hipLaunchKernelGGL(computeL2DistanceKernel, gridSize, blockSize, 0, impl_->stream.get(),
                d_queries, d_vectors, d_distances,
                numQueries, numVectors, dim
            );
        } else {
            // Cosine distance kernel (default for non-L2)
            hipLaunchKernelGGL(computeCosineDistanceKernel, gridSize, blockSize, 0, impl_->stream.get(),
                d_queries, d_vectors, d_distances,
                numQueries, numVectors, dim
            );
        }
        
        HIP_CHECK_THROW(hipStreamSynchronize(impl_->stream.get()));
        
        // Copy results back
        std::vector<float> distances(numQueries * numVectors);
        HIP_CHECK_THROW(hipMemcpy(distances.data(), d_distances, distancesSize, hipMemcpyDeviceToHost));
        
        // Cleanup
        hipFree(d_queries);
        hipFree(d_vectors);
        hipFree(d_distances);
        
        return distances;
        
    } catch (const std::exception& e) {
        if (d_queries) hipFree(d_queries);
        if (d_vectors) hipFree(d_vectors);
        if (d_distances) hipFree(d_distances);
        std::cerr << "HIP computeDistances error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<std::vector<std::pair<uint32_t, float>>> HIPVectorBackend::batchKnnSearch(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    bool useL2
) {
    // Delegate to metric-aware version
    return batchKnnSearchWithMetric(queries, numQueries, dim, vectors, numVectors, k, 
                                     useL2 ? DistanceMetric::L2 : DistanceMetric::COSINE);
}

std::vector<std::vector<std::pair<uint32_t, float>>> HIPVectorBackend::batchKnnSearchWithMetric(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    DistanceMetric metric
) {
    if (!impl_->initialized) {
        std::cerr << "HIP backend not initialized" << std::endl;
        return {};
    }

    // Validate pointers and reject unsafe batches
    auto sink = [this](ErrorContext e){ setError(std::move(e)); };
    if (!BatchValidator::validateVectorBatch(name(), queries, numQueries, dim,
                                             vectors, numVectors, sink)) {
        return {};
    }
    if (!BatchValidator::validateK(name(), k, sink)) {
        return {};
    }

    size_t effectiveK = std::min(k, numVectors);
    
    // Allocate device memory
    float *d_queries = nullptr, *d_vectors = nullptr, *d_distances = nullptr;
    uint32_t *d_indices = nullptr;
    float *d_topKDistances = nullptr;
    
    size_t queriesSize = numQueries * dim * sizeof(float);
    size_t vectorsSize = numVectors * dim * sizeof(float);
    size_t distancesSize = numQueries * numVectors * sizeof(float);
    size_t topKSize = numQueries * effectiveK * sizeof(float);
    size_t indicesSize = numQueries * effectiveK * sizeof(uint32_t);
    
    try {
        HIP_CHECK_THROW(hipMalloc(&d_queries, queriesSize));
        HIP_CHECK_THROW(hipMalloc(&d_vectors, vectorsSize));
        HIP_CHECK_THROW(hipMalloc(&d_distances, distancesSize));
        HIP_CHECK_THROW(hipMalloc(&d_indices, indicesSize));
        HIP_CHECK_THROW(hipMalloc(&d_topKDistances, topKSize));
        
        // Copy data to device
        HIP_CHECK_THROW(hipMemcpy(d_queries, queries, queriesSize, hipMemcpyHostToDevice));
        HIP_CHECK_THROW(hipMemcpy(d_vectors, vectors, vectorsSize, hipMemcpyHostToDevice));
        
        // Launch distance kernel based on metric
        dim3 blockSize(16, 16);
        dim3 gridSize(
            (numVectors + blockSize.x - 1) / blockSize.x,
            (numQueries + blockSize.y - 1) / blockSize.y
        );
        
        switch (metric) {
            case DistanceMetric::L2:
                hipLaunchKernelGGL(computeL2DistanceKernel, gridSize, blockSize, 0, impl_->stream.get(),
                    d_queries, d_vectors, d_distances,
                    numQueries, numVectors, dim
                );
                break;
            case DistanceMetric::COSINE:
                hipLaunchKernelGGL(computeCosineDistanceKernel, gridSize, blockSize, 0, impl_->stream.get(),
                    d_queries, d_vectors, d_distances,
                    numQueries, numVectors, dim
                );
                break;
            case DistanceMetric::INNER_PRODUCT:
                hipLaunchKernelGGL(computeInnerProductDistanceKernel, gridSize, blockSize, 0, impl_->stream.get(),
                    d_queries, d_vectors, d_distances,
                    numQueries, numVectors, dim
                );
                break;
        }
        
        // Launch top-k selection kernel
        int threadsPerBlock = 256;
        int numBlocks = (numQueries + threadsPerBlock - 1) / threadsPerBlock;
        
        hipLaunchKernelGGL(topKSelectionKernel, dim3(numBlocks), dim3(threadsPerBlock), 0, impl_->stream.get(),
            d_distances, d_indices, d_topKDistances,
            numQueries, numVectors, effectiveK
        );
        
        HIP_CHECK_THROW(hipStreamSynchronize(impl_->stream.get()));
        
        // Copy results back
        std::vector<uint32_t> indices(numQueries * effectiveK);
        std::vector<float> topKDistances(numQueries * effectiveK);
        HIP_CHECK_THROW(hipMemcpy(indices.data(), d_indices, indicesSize, hipMemcpyDeviceToHost));
        HIP_CHECK_THROW(hipMemcpy(topKDistances.data(), d_topKDistances, topKSize, hipMemcpyDeviceToHost));
        
        // Cleanup
        hipFree(d_queries);
        hipFree(d_vectors);
        hipFree(d_distances);
        hipFree(d_indices);
        hipFree(d_topKDistances);
        
        // Format results
        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        for (size_t q = 0; q < numQueries; q++) {
            results[q].reserve(effectiveK);
            for (size_t i = 0; i < effectiveK; i++) {
                results[q].emplace_back(indices[q * effectiveK + i], topKDistances[q * effectiveK + i]);
            }
        }
        
        return results;
        
    } catch (const std::exception& e) {
        if (d_queries) hipFree(d_queries);
        if (d_vectors) hipFree(d_vectors);
        if (d_distances) hipFree(d_distances);
        if (d_indices) hipFree(d_indices);
        if (d_topKDistances) hipFree(d_topKDistances);
        std::cerr << "HIP batchKnnSearch error: " << e.what() << std::endl;
        return {};
    }
}

// HIP-specific methods

HIPVectorBackend::DeviceInfo HIPVectorBackend::getDeviceInfo() const {
    DeviceInfo info;
    if (impl_->initialized) {
        info.name = impl_->deviceProps.name;
        info.computeUnits = impl_->deviceProps.multiProcessorCount;
        info.totalMemory = impl_->deviceProps.totalGlobalMem;
        info.waveSize = impl_->deviceProps.warpSize;
        info.gcnArchName = impl_->deviceProps.gcnArchName;
        
        // Note: Assuming FP16 and Int8 support for all ROCm-supported GPUs (RDNA2+, CDNA+)
        // All GPUs supported by ROCm 5.0+ have these capabilities
        info.supportsFP16 = true;
        info.supportsInt8 = true;
    }
    return info;
}

void HIPVectorBackend::setConfig(const HIPConfig& config) {
    impl_->config = config;
}

HIPVectorBackend::HIPConfig HIPVectorBackend::getConfig() const {
    return impl_->config;
}

std::vector<HIPVectorBackend::DeviceInfo> HIPVectorBackend::getAvailableDevices() {
    std::vector<DeviceInfo> devices;
    
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        return devices;
    }
    
    for (int i = 0; i < deviceCount; i++) {
        hipDeviceProp_t prop;
        if (hipGetDeviceProperties(&prop, i) == hipSuccess) {
            DeviceInfo info;
            info.name = prop.name;
            info.computeUnits = prop.multiProcessorCount;
            info.totalMemory = prop.totalGlobalMem;
            info.waveSize = prop.warpSize;
            info.gcnArchName = prop.gcnArchName;
            // All ROCm 5.0+ supported GPUs have FP16 and Int8 capabilities
            info.supportsFP16 = true;
            info.supportsInt8 = true;
            devices.push_back(info);
        }
    }
    
    return devices;
}

std::string HIPVectorBackend::getHIPVersion() {
    int runtimeVersion = 0;
    if (hipRuntimeGetVersion(&runtimeVersion) == hipSuccess) {
        int major = runtimeVersion / 10000000;
        int minor = (runtimeVersion / 100000) % 100;
        int patch = runtimeVersion % 100000;
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
    return "unknown";
}

std::string HIPVectorBackend::getROCmVersion() {
    // ROCm version is typically extracted from HIP version
    return getHIPVersion();
}

// Forward declarations for launchers compiled in hip/ann_kernels.hip.
// These conform to the ANNDistanceFn / ANNTopKFn typedefs in
// include/acceleration/kernel_invocation.h (INTERFACE_VERSION 100).
extern "C" {
int hip_launchL2DistanceKernel(const float*, const float*, float*,
                                int, int, int, void*);
int hip_launchCosineDistanceKernel(const float*, const float*, float*,
                                    int, int, int, void*);
int hip_launchInnerProductKernel(const float*, const float*, float*,
                                  int, int, int, void*);
int hip_launchTopKKernel(const float*, uint32_t*, float*,
                          int, int, int, void*);
} // extern "C"

ANNKernelDispatch HIPVectorBackend::populateANNDispatch() const {
    ANNKernelDispatch d;
    d.launchL2Distance   = &hip_launchL2DistanceKernel;
    d.launchCosine       = &hip_launchCosineDistanceKernel;
    d.launchInnerProduct = &hip_launchInnerProductKernel;
    d.launchTopK         = &hip_launchTopKKernel;
    return d;
}

// ============================================================================
// HIPGeoBackend Implementation
// ============================================================================

// Forward declarations for launchers compiled in hip/geo_kernels.hip.
// These conform to the GeoDistanceFn / GeoContainmentFn typedefs in
// include/acceleration/kernel_invocation.h (INTERFACE_VERSION 100).
extern "C" {
int hip_launchGeoDistanceKernel(const double*, const double*, const double*, const double*,
                                 float*, int, GeoDistanceFormula, void*);
int hip_launchGeoContainmentKernel(const double*, const double*, int,
                                    const double*, int, uint8_t*, void*);
} // extern "C"

HIPGeoBackend::~HIPGeoBackend() {
    shutdown();
}

bool HIPGeoBackend::isAvailable() const noexcept {
    int deviceCount = 0;
    hipError_t err = hipGetDeviceCount(&deviceCount);
    return (err == hipSuccess && deviceCount > 0);
}

BackendCapabilities HIPGeoBackend::getCapabilities() const {
    BackendCapabilities caps;
    caps.supportsGeoOps = true;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync = true;
    caps.supportedPrecisions = PrecisionMode::FP32;

    if (isAvailable()) {
        hipDeviceProp_t prop;
        if (hipGetDeviceProperties(&prop, 0) == hipSuccess) {
            caps.deviceName = std::string(prop.name) + " (HIP)";
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits = prop.multiProcessorCount;
        }
    } else {
        caps.deviceName = "AMD GPU (HIP - not available)";
    }
    return caps;
}

bool HIPGeoBackend::initialize() {
    if (initialized_) {
        return true;
    }

    if (!isAvailable()) {
        int deviceCount = 0;
        hipError_t err = hipGetDeviceCount(&deviceCount);
        if (deviceCount == 0) {
            setError(ErrorContextHelpers::createNoDevicesError("HIP"));
        } else {
            setError(ErrorContextHelpers::createDriverError("HIP"));
        }
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    hipError_t setDeviceErr = hipSetDevice(0);
    if (setDeviceErr != hipSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DeviceSetFailed,
            "HIP",
            "Failed to set device 0: " + std::string(hipGetErrorString(setDeviceErr)),
            "Check if device is available and not in exclusive mode"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    try {
        stream_.create();
    } catch (const std::exception& e) {
        setError(ErrorContextHelpers::createQueueError("HIP", e.what()));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    hipDeviceProp_t prop;
    hipError_t propErr = hipGetDeviceProperties(&prop, 0);
    if (propErr != hipSuccess) {
        setError(ErrorContext(
            AccelerationErrorCode::DevicePropertiesQueryFailed,
            "HIP",
            "Failed to query device properties: " + std::string(hipGetErrorString(propErr)),
            "Ensure ROCm runtime is properly installed and device is accessible"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }

    std::cout << "HIP Geo Backend initialized successfully:" << std::endl;
    std::cout << "  Device: " << prop.name << std::endl;
    std::cout << "  GCN Arch: " << prop.gcnArchName << std::endl;
    std::cout << "  Global Memory: " << (prop.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;

    clearError();
    initialized_ = true;
    return true;
}

void HIPGeoBackend::shutdown() {
    if (initialized_) {
        // stream_ automatically destroyed by RAII destructor
        initialized_ = false;
    }
}

std::vector<float> HIPGeoBackend::batchDistances(
    const double* latitudes1,
    const double* longitudes1,
    const double* latitudes2,
    const double* longitudes2,
    size_t count,
    bool useHaversine
) {
    if (!initialized_) {
        std::cerr << "HIP Geo backend not initialized" << std::endl;
        return {};
    }
    if (count == 0) return {};

    hipStream_t stream = stream_.get();

    double *d_lats1 = nullptr, *d_lons1 = nullptr;
    double *d_lats2 = nullptr, *d_lons2 = nullptr;
    float  *d_distances = nullptr;

    const size_t coordBytes = count * sizeof(double);
    const size_t distBytes  = count * sizeof(float);

    try {
        HIP_CHECK_THROW(hipMalloc(&d_lats1,     coordBytes));
        HIP_CHECK_THROW(hipMalloc(&d_lons1,     coordBytes));
        HIP_CHECK_THROW(hipMalloc(&d_lats2,     coordBytes));
        HIP_CHECK_THROW(hipMalloc(&d_lons2,     coordBytes));
        HIP_CHECK_THROW(hipMalloc(&d_distances, distBytes));

        HIP_CHECK_THROW(hipMemcpyAsync(d_lats1, latitudes1,  coordBytes, hipMemcpyHostToDevice, stream));
        HIP_CHECK_THROW(hipMemcpyAsync(d_lons1, longitudes1, coordBytes, hipMemcpyHostToDevice, stream));
        HIP_CHECK_THROW(hipMemcpyAsync(d_lats2, latitudes2,  coordBytes, hipMemcpyHostToDevice, stream));
        HIP_CHECK_THROW(hipMemcpyAsync(d_lons2, longitudes2, coordBytes, hipMemcpyHostToDevice, stream));

        // HAVERSINE: fast, ~0.5% error; suitable for most use-cases.
        // VINCENTY:  iterative ellipsoidal model, higher precision for nearly-antipodal points.
        const GeoDistanceFormula formula = useHaversine
            ? GeoDistanceFormula::HAVERSINE
            : GeoDistanceFormula::VINCENTY;

        const int rc = hip_launchGeoDistanceKernel(
            d_lats1, d_lons1, d_lats2, d_lons2,
            d_distances, static_cast<int>(count),
            formula, static_cast<void*>(stream));

        if (rc != 0) {
            throw std::runtime_error("hip_launchGeoDistanceKernel failed with code " +
                                     std::to_string(rc));
        }

        std::vector<float> distances(count);
        HIP_CHECK_THROW(hipMemcpyAsync(distances.data(), d_distances, distBytes,
                                       hipMemcpyDeviceToHost, stream));
        HIP_CHECK_THROW(hipStreamSynchronize(stream));

        hipFree(d_lats1);
        hipFree(d_lons1);
        hipFree(d_lats2);
        hipFree(d_lons2);
        hipFree(d_distances);

        return distances;

    } catch (const std::exception& e) {
        if (d_lats1)     hipFree(d_lats1);
        if (d_lons1)     hipFree(d_lons1);
        if (d_lats2)     hipFree(d_lats2);
        if (d_lons2)     hipFree(d_lons2);
        if (d_distances) hipFree(d_distances);
        std::cerr << "HIP batchDistances error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<bool> HIPGeoBackend::batchPointInPolygon(
    const double* pointLats,
    const double* pointLons,
    size_t numPoints,
    const double* polygonCoords,
    size_t numPolygonVertices
) {
    if (!initialized_) {
        std::cerr << "HIP Geo backend not initialized" << std::endl;
        return {};
    }
    if (numPoints == 0) return {};

    hipStream_t stream = stream_.get();

    double  *d_point_lats = nullptr, *d_point_lons = nullptr;
    double  *d_polygon_coords = nullptr;
    uint8_t *d_results = nullptr;

    const size_t pointBytes  = numPoints          * sizeof(double);
    const size_t polyBytes   = numPolygonVertices * 2 * sizeof(double);
    const size_t resultBytes = numPoints          * sizeof(uint8_t);

    try {
        HIP_CHECK_THROW(hipMalloc(&d_point_lats,    pointBytes));
        HIP_CHECK_THROW(hipMalloc(&d_point_lons,    pointBytes));
        HIP_CHECK_THROW(hipMalloc(&d_polygon_coords, polyBytes));
        HIP_CHECK_THROW(hipMalloc(&d_results,        resultBytes));

        HIP_CHECK_THROW(hipMemcpyAsync(d_point_lats,     pointLats,     pointBytes, hipMemcpyHostToDevice, stream));
        HIP_CHECK_THROW(hipMemcpyAsync(d_point_lons,     pointLons,     pointBytes, hipMemcpyHostToDevice, stream));
        HIP_CHECK_THROW(hipMemcpyAsync(d_polygon_coords, polygonCoords, polyBytes,  hipMemcpyHostToDevice, stream));

        const int rc = hip_launchGeoContainmentKernel(
            d_point_lats, d_point_lons, static_cast<int>(numPoints),
            d_polygon_coords, static_cast<int>(numPolygonVertices),
            d_results, static_cast<void*>(stream));

        if (rc != 0) {
            throw std::runtime_error("hip_launchGeoContainmentKernel failed with code " +
                                     std::to_string(rc));
        }

        std::vector<uint8_t> rawResults(numPoints);
        HIP_CHECK_THROW(hipMemcpyAsync(rawResults.data(), d_results, resultBytes,
                                       hipMemcpyDeviceToHost, stream));
        HIP_CHECK_THROW(hipStreamSynchronize(stream));

        hipFree(d_point_lats);
        hipFree(d_point_lons);
        hipFree(d_polygon_coords);
        hipFree(d_results);

        std::vector<bool> results(numPoints);
        for (size_t i = 0; i < numPoints; ++i) {
            results[i] = (rawResults[i] != 0);
        }
        return results;

    } catch (const std::exception& e) {
        if (d_point_lats)     hipFree(d_point_lats);
        if (d_point_lons)     hipFree(d_point_lons);
        if (d_polygon_coords) hipFree(d_polygon_coords);
        if (d_results)        hipFree(d_results);
        std::cerr << "HIP batchPointInPolygon error: " << e.what() << std::endl;
        return {};
    }
}

GeoKernelDispatch HIPGeoBackend::populateGeoDispatch() const {
    GeoKernelDispatch d;
    d.launchDistance    = &hip_launchGeoDistanceKernel;
    d.launchContainment = &hip_launchGeoContainmentKernel;
    return d;
}

} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_HIP
