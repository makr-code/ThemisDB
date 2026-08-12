/**
 * @file hip_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=12, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

#include <cfloat>
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

// Forward declarations for block-size setters defined in hip/ann_kernels.hip
// and hip/geo_kernels.hip.  Called during initialize() with the occupancy-tuned
// block size so that kernel launchers use the optimal thread count.
extern "C" void hipSetTopKBlockSize(int blockSize);
extern "C" void hipSetGeoKernelBlockSize(int blockSize);

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

// ============================================================================
// Device-side max-heap helpers for top-K selection
//
// The heap is stored as two parallel arrays (distances + indices) of capacity
// `cap`.  heap[0] always holds the maximum distance (max-heap invariant), so
// we can cheaply check whether a new candidate improves the result set and
// evict the worst element in O(log cap).
// ============================================================================

__device__ __forceinline__ void hipHeapSiftDown(
    float* __restrict__    dists,
    uint32_t* __restrict__ ids,
    int top, int size)
{
    while (true) {
        int largest = top;
        int left    = 2 * top + 1;
        int right   = 2 * top + 2;
        if (left  < size && dists[left]  > dists[largest]) largest = left;
        if (right < size && dists[right] > dists[largest]) largest = right;
        if (largest == top) break;
        float    td = dists[top]; dists[top]  = dists[largest]; dists[largest]  = td;
        uint32_t ti = ids[top];   ids[top]    = ids[largest];   ids[largest]    = ti;
        top = largest;
    }
}

// Insert (d, id) into a max-heap of capacity cap.
// If the heap has room it grows; otherwise, if d < heap[0] (the current max),
// replace the root and sift down.
__device__ __forceinline__ void hipHeapPushCapped(
    float* __restrict__    dists,
    uint32_t* __restrict__ ids,
    int* __restrict__      size_ptr,
    int cap,
    float d,
    uint32_t id)
{
    if (*size_ptr < cap) {
        int pos = *size_ptr;
        dists[pos] = d;
        ids[pos]   = id;
        ++(*size_ptr);
        // Sift up to maintain max-heap invariant
        while (pos > 0) {
            int parent = (pos - 1) / 2;
            if (dists[parent] < dists[pos]) {
                float    td = dists[parent]; dists[parent] = dists[pos]; dists[pos] = td;
                uint32_t ti = ids[parent];   ids[parent]   = ids[pos];   ids[pos]   = ti;
                pos = parent;
            } else {
                break;
            }
        }
    } else if (d < dists[0]) {
        dists[0] = d;
        ids[0]   = id;
        hipHeapSiftDown(dists, ids, 0, cap);
    }
}

// Sort a max-heap in ascending order (heap-sort descending then reverse).
__device__ __forceinline__ void hipHeapSort(
    float* __restrict__    dists,
    uint32_t* __restrict__ ids,
    int size)
{
    for (int end = size - 1; end > 0; --end) {
        float    td = dists[0]; dists[0] = dists[end]; dists[end] = td;
        uint32_t ti = ids[0];   ids[0]   = ids[end];   ids[end]   = ti;
        hipHeapSiftDown(dists, ids, 0, end);
    }
}

// Top-K selection kernel using parallel reduction
// Selects k nearest neighbors for each query.
//
// Algorithm selection:
//   k <= 32  — insertion-sort style (O(k²) init + O(n·k) sweep); low overhead for tiny k.
//   k >  32  — max-heap selection (O(k log k) build + O(n log k) sweep); efficient for large k.
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

    const int effectiveN = (numVectors < k) ? numVectors : k;

    if (k <= 32) {
        // ── Insertion-sort path (efficient for very small k) ─────────────────
        // Initialize with first k elements
        for (int i = 0; i < effectiveN; i++) {
            queryIndices[i] = static_cast<uint32_t>(i);
            queryTopK[i] = queryDistances[i];
        }
        // Bubble-sort the initial k elements into ascending order
        for (int i = 0; i < effectiveN - 1; i++) {
            for (int j = 0; j < effectiveN - i - 1; j++) {
                if (queryTopK[j] > queryTopK[j + 1]) {
                    float    td = queryTopK[j];   queryTopK[j]   = queryTopK[j + 1]; queryTopK[j + 1]   = td;
                    uint32_t ti = queryIndices[j]; queryIndices[j] = queryIndices[j + 1]; queryIndices[j + 1] = ti;
                }
            }
        }
        // Sweep remaining elements via insertion into sorted prefix
        for (int i = k; i < numVectors; i++) {
            float dist = queryDistances[i];
            if (dist < queryTopK[k - 1]) {
                int pos = k - 1;
                while (pos > 0 && dist < queryTopK[pos - 1]) pos--;
                for (int j = k - 1; j > pos; j--) {
                    queryTopK[j]   = queryTopK[j - 1];
                    queryIndices[j] = queryIndices[j - 1];
                }
                queryTopK[pos]   = dist;
                queryIndices[pos] = static_cast<uint32_t>(i);
            }
        }
    } else {
        // ── Max-heap path (O(k log k + n log k)) — efficient for large k ─────
        int heapSize = 0;
        // Build initial max-heap from first min(k, numVectors) elements
        for (int i = 0; i < effectiveN; i++) {
            hipHeapPushCapped(queryTopK, queryIndices, &heapSize, k,
                              queryDistances[i], static_cast<uint32_t>(i));
        }
        // Sweep remaining elements: replace heap root when we find a closer vector
        for (int i = k; i < numVectors; i++) {
            hipHeapPushCapped(queryTopK, queryIndices, &heapSize, k,
                              queryDistances[i], static_cast<uint32_t>(i));
        }
        // Sort the heap into ascending distance order
        hipHeapSort(queryTopK, queryIndices, heapSize);
        // Zero-fill trailing slots when numVectors < k
        for (int i = heapSize; i < k; i++) {
            queryTopK[i]   = FLT_MAX;
            queryIndices[i] = static_cast<uint32_t>(-1);
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

    // Occupancy-tuned block size for 1-D kernels (top-K selection, etc.).
    // Set during initialize() via hipOccupancyMaxPotentialBlockSize(); falls
    // back to 256 (safe for all ROCm-supported devices) if the query fails.
    // AMD GCN devices with 64-thread wavefronts use 64 as their baseline.
    int occupancyTunedBlockSize = 256;

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

    // ── Occupancy-based block size tuning ─────────────────────────────────────
    // Start with a wave-size-aware baseline: AMD GCN devices have 64-thread
    // wavefronts, so using 256 threads per block would leave half the wavefront
    // slots idle.  Use 64 as the starting point for those devices.
    int baseBlockSize = (impl_->deviceProps.warpSize == 64) ? 64 : 256;

    // Query the HIP occupancy API for the top-K selection kernel.
    int minGridSize   = 0;
    int tunedBlockSize = baseBlockSize;
    hipError_t occErr = hipOccupancyMaxPotentialBlockSize(
        &minGridSize, &tunedBlockSize, topKSelectionKernel, 0, 0);
    if (occErr == hipSuccess && tunedBlockSize > 0) {
        // Round down to the nearest multiple of warpSize (never go below it).
        const int warpSize = impl_->deviceProps.warpSize;
        tunedBlockSize = (tunedBlockSize / warpSize) * warpSize;
        if (tunedBlockSize < warpSize) tunedBlockSize = warpSize;
        impl_->occupancyTunedBlockSize = tunedBlockSize;
    } else {
        impl_->occupancyTunedBlockSize = baseBlockSize;
    }
    std::cout << "  Occupancy-tuned block size: " << impl_->occupancyTunedBlockSize << std::endl;

    // Propagate the tuned block size to the external HIP kernel launchers.
    hipSetTopKBlockSize(impl_->occupancyTunedBlockSize);
    hipSetGeoKernelBlockSize(impl_->occupancyTunedBlockSize);
    
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
        
        // Launch top-k selection kernel using the occupancy-tuned block size
        int threadsPerBlock = impl_->occupancyTunedBlockSize;
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

// =============================================================================
// Non-HIP fallback dispatch tables
//
// When THEMIS_ENABLE_HIP is not defined the class methods below return empty
// dispatch tables so that BackendRegistry falls back to the CPU table.
// This mirrors the cuda_backend.cpp pattern for THEMIS_ENABLE_CUDA.
// =============================================================================
#ifndef THEMIS_ENABLE_HIP

namespace themis {
namespace acceleration {

ANNKernelDispatch HIPVectorBackend::populateANNDispatch() const {
    return {}; // No HIP — all null; BackendRegistry falls back to CPU table
}

GeoKernelDispatch HIPGeoBackend::populateGeoDispatch() const {
    return {}; // No HIP — all null; BackendRegistry falls back to CPU table
}

} // namespace acceleration
} // namespace themis

#endif // !THEMIS_ENABLE_HIP

