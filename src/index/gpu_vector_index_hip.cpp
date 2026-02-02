#include "index/gpu_vector_index.h"
#include <iostream>
#include <stdexcept>

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#ifdef THEMIS_ENABLE_ROCBLAS
#include <rocblas/rocblas.h>
#endif
#ifdef THEMIS_ENABLE_RCCL
#include <rccl/rccl.h>
#endif
#endif

namespace themis {
namespace index {

#ifdef THEMIS_ENABLE_HIP

// HIP kernels (compatible with CUDA kernels)
extern "C" {
    void launchHIPL2DistanceKernel(const float* queries, const float* vectors,
                                   float* results, int numQueries, int numVectors,
                                   int dimension, hipStream_t stream);
    
    void launchHIPCosineDistanceKernel(const float* queries, const float* vectors,
                                       float* results, int numQueries, int numVectors,
                                       int dimension, hipStream_t stream);
    
    void launchHIPInnerProductKernel(const float* queries, const float* vectors,
                                     float* results, int numQueries, int numVectors,
                                     int dimension, hipStream_t stream);
}

// =============================================================================
// HIPVectorIndexBackend::Impl
// =============================================================================

class HIPVectorIndexBackend::Impl {
public:
    int deviceId = 0;
    int dimension = 0;
    GPUVectorIndex::Config config;
    bool initialized = false;
    
    // HIP resources
    hipStream_t stream = nullptr;
    
    // Device memory
    float* d_queries = nullptr;
    float* d_vectors = nullptr;
    float* d_distances = nullptr;
    uint32_t* d_indices = nullptr;
    
    // Memory sizes
    size_t queryBufferSize = 0;
    size_t vectorBufferSize = 0;
    size_t distanceBufferSize = 0;
    size_t indexBufferSize = 0;
    
    // rocBLAS handle
#ifdef THEMIS_ENABLE_ROCBLAS
    rocblas_handle rocblasHandle = nullptr;
#endif
    
    // RCCL communicator (for multi-GPU)
#ifdef THEMIS_ENABLE_RCCL
    ncclComm_t ncclComm = nullptr;
    int numDevices = 1;
#endif
    
    // AMD-specific settings
    int waveSize = 64; // Default Wave64
    bool useRocBLAS = false;
    
    ~Impl() {
        cleanup();
    }
    
    bool initialize(int dim, const GPUVectorIndex::Config& cfg) {
        dimension = dim;
        config = cfg;
        deviceId = cfg.deviceId;
        
        // Set HIP device
        hipError_t err = hipSetDevice(deviceId);
        if (err != hipSuccess) {
            std::cerr << "Failed to set HIP device " << deviceId << ": "
                     << hipGetErrorString(err) << std::endl;
            return false;
        }
        
        // Check device properties
        hipDeviceProp_t prop;
        err = hipGetDeviceProperties(&prop, deviceId);
        if (err != hipSuccess) {
            std::cerr << "Failed to get device properties: "
                     << hipGetErrorString(err) << std::endl;
            return false;
        }
        
        std::cout << "HIP Device: " << prop.name << std::endl;
        std::cout << "  Compute Units: " << prop.multiProcessorCount << std::endl;
        std::cout << "  Total VRAM: " << (prop.totalGlobalMem / (1024*1024)) << " MB" << std::endl;
        std::cout << "  Warp Size: " << prop.warpSize << std::endl;
        
        // Detect wave size (AMD architecture specific)
        waveSize = prop.warpSize; // 32 or 64 depending on architecture
        std::cout << "  Wave Size: " << waveSize << std::endl;
        
        // Check for RDNA architecture
        std::string deviceName(prop.name);
        if (deviceName.find("RDNA") != std::string::npos) {
            if (deviceName.find("RDNA3") != std::string::npos) {
                std::cout << "  Architecture: RDNA3 detected" << std::endl;
            } else if (deviceName.find("RDNA2") != std::string::npos) {
                std::cout << "  Architecture: RDNA2 detected" << std::endl;
            }
        }
        
        // Create HIP stream
        err = hipStreamCreate(&stream);
        if (err != hipSuccess) {
            std::cerr << "Failed to create HIP stream: "
                     << hipGetErrorString(err) << std::endl;
            return false;
        }
        
#ifdef THEMIS_ENABLE_ROCBLAS
        // Initialize rocBLAS
        rocblas_status status = rocblas_create_handle(&rocblasHandle);
        if (status == rocblas_status_success) {
            rocblas_set_stream(rocblasHandle, stream);
            useRocBLAS = true;
            std::cout << "  rocBLAS: Enabled" << std::endl;
        } else {
            std::cout << "  rocBLAS: Not available" << std::endl;
        }
#endif
        
        initialized = true;
        return true;
    }
    
    void cleanup() {
        if (!initialized) return;
        
        // Free device memory
        if (d_queries) hipFree(d_queries);
        if (d_vectors) hipFree(d_vectors);
        if (d_distances) hipFree(d_distances);
        if (d_indices) hipFree(d_indices);
        
#ifdef THEMIS_ENABLE_ROCBLAS
        // Destroy rocBLAS handle
        if (rocblasHandle) {
            rocblas_destroy_handle(rocblasHandle);
        }
#endif
        
#ifdef THEMIS_ENABLE_RCCL
        // Destroy RCCL communicator
        if (ncclComm) {
            ncclCommDestroy(ncclComm);
        }
#endif
        
        // Destroy stream
        if (stream) hipStreamDestroy(stream);
        
        d_queries = nullptr;
        d_vectors = nullptr;
        d_distances = nullptr;
        d_indices = nullptr;
        stream = nullptr;
        
        initialized = false;
    }
    
    bool allocateBuffers(size_t numQueries, size_t numVectors) {
        // Calculate required sizes
        size_t querySize = numQueries * dimension * sizeof(float);
        size_t vectorSize = numVectors * dimension * sizeof(float);
        size_t distanceSize = numQueries * numVectors * sizeof(float);
        size_t indexSize = numQueries * numVectors * sizeof(uint32_t);
        
        // Allocate if needed
        if (querySize > queryBufferSize) {
            if (d_queries) hipFree(d_queries);
            hipError_t err = hipMalloc(&d_queries, querySize);
            if (err != hipSuccess) {
                std::cerr << "Failed to allocate query buffer: "
                         << hipGetErrorString(err) << std::endl;
                return false;
            }
            queryBufferSize = querySize;
        }
        
        if (vectorSize > vectorBufferSize) {
            if (d_vectors) hipFree(d_vectors);
            hipError_t err = hipMalloc(&d_vectors, vectorSize);
            if (err != hipSuccess) {
                std::cerr << "Failed to allocate vector buffer: "
                         << hipGetErrorString(err) << std::endl;
                return false;
            }
            vectorBufferSize = vectorSize;
        }
        
        if (distanceSize > distanceBufferSize) {
            if (d_distances) hipFree(d_distances);
            hipError_t err = hipMalloc(&d_distances, distanceSize);
            if (err != hipSuccess) {
                std::cerr << "Failed to allocate distance buffer: "
                         << hipGetErrorString(err) << std::endl;
                return false;
            }
            distanceBufferSize = distanceSize;
        }
        
        if (indexSize > indexBufferSize) {
            if (d_indices) hipFree(d_indices);
            hipError_t err = hipMalloc(&d_indices, indexSize);
            if (err != hipSuccess) {
                std::cerr << "Failed to allocate index buffer: "
                         << hipGetErrorString(err) << std::endl;
                return false;
            }
            indexBufferSize = indexSize;
        }
        
        return true;
    }
    
    std::vector<float> computeDistances(const float* queries, size_t numQueries,
                                       const float* vectors, size_t numVectors,
                                       GPUVectorIndex::DistanceMetric metric) {
        if (!initialized || !allocateBuffers(numQueries, numVectors)) {
            return {};
        }
        
        // Copy data to device
        hipError_t err;
        err = hipMemcpyAsync(d_queries, queries, numQueries * dimension * sizeof(float),
                            hipMemcpyHostToDevice, stream);
        if (err != hipSuccess) {
            std::cerr << "Failed to copy queries to device: "
                     << hipGetErrorString(err) << std::endl;
            return {};
        }
        
        err = hipMemcpyAsync(d_vectors, vectors, numVectors * dimension * sizeof(float),
                            hipMemcpyHostToDevice, stream);
        if (err != hipSuccess) {
            std::cerr << "Failed to copy vectors to device: "
                     << hipGetErrorString(err) << std::endl;
            return {};
        }
        
        // Launch distance kernel
        switch (metric) {
            case GPUVectorIndex::DistanceMetric::L2:
                launchHIPL2DistanceKernel(d_queries, d_vectors, d_distances,
                                         numQueries, numVectors, dimension, stream);
                break;
            case GPUVectorIndex::DistanceMetric::COSINE:
                launchHIPCosineDistanceKernel(d_queries, d_vectors, d_distances,
                                             numQueries, numVectors, dimension, stream);
                break;
            case GPUVectorIndex::DistanceMetric::INNER_PRODUCT:
                launchHIPInnerProductKernel(d_queries, d_vectors, d_distances,
                                           numQueries, numVectors, dimension, stream);
                break;
        }
        
        // Check for kernel errors
        err = hipGetLastError();
        if (err != hipSuccess) {
            std::cerr << "Kernel launch failed: " << hipGetErrorString(err) << std::endl;
            return {};
        }
        
        // Copy results back
        std::vector<float> results(numQueries * numVectors);
        err = hipMemcpyAsync(results.data(), d_distances,
                            numQueries * numVectors * sizeof(float),
                            hipMemcpyDeviceToHost, stream);
        if (err != hipSuccess) {
            std::cerr << "Failed to copy results from device: "
                     << hipGetErrorString(err) << std::endl;
            return {};
        }
        
        // Wait for completion
        hipStreamSynchronize(stream);
        
        return results;
    }
};

#else // !THEMIS_ENABLE_HIP

// Stub implementation when HIP is not available
class HIPVectorIndexBackend::Impl {
public:
    bool initialize(int, const GPUVectorIndex::Config&) {
        std::cerr << "HIP support not compiled in\n";
        return false;
    }
    void cleanup() {}
};

#endif // THEMIS_ENABLE_HIP

// =============================================================================
// HIPVectorIndexBackend public interface
// =============================================================================

HIPVectorIndexBackend::HIPVectorIndexBackend()
    : pImpl(std::make_unique<Impl>()) {
}

HIPVectorIndexBackend::~HIPVectorIndexBackend() = default;

bool HIPVectorIndexBackend::initialize(int dimension, const GPUVectorIndex::Config& config) {
    return pImpl->initialize(dimension, config);
}

void HIPVectorIndexBackend::shutdown() {
#ifdef THEMIS_ENABLE_HIP
    pImpl->cleanup();
#endif
}

bool HIPVectorIndexBackend::enableRocBLAS(bool enable) {
#ifdef THEMIS_ENABLE_HIP
    pImpl->useRocBLAS = enable;
    return true;
#else
    (void)enable;
    return false;
#endif
}

void HIPVectorIndexBackend::optimizeForRDNA2() {
    std::cout << "Applying RDNA2 optimizations...\n";
    // RDNA2 typically uses Wave32
    setWaveSize(32);
}

void HIPVectorIndexBackend::optimizeForRDNA3() {
    std::cout << "Applying RDNA3 optimizations...\n";
    // RDNA3 can use Wave32 or Wave64
    setWaveSize(32);
}

void HIPVectorIndexBackend::setWaveSize(int waveSize) {
#ifdef THEMIS_ENABLE_HIP
    pImpl->waveSize = waveSize;
    std::cout << "Wave size set to: " << waveSize << std::endl;
#else
    (void)waveSize;
#endif
}

bool HIPVectorIndexBackend::enableRCCL(int numDevices) {
#ifdef THEMIS_ENABLE_RCCL
    pImpl->numDevices = numDevices;
    // TODO: Initialize RCCL communicator
    return false;
#else
    (void)numDevices;
    return false;
#endif
}

void HIPVectorIndexBackend::ringAllReduce(float* data, size_t size) {
#ifdef THEMIS_ENABLE_RCCL
    // TODO: Implement RCCL ring all-reduce
    (void)data; (void)size;
#else
    (void)data; (void)size;
#endif
}

void HIPVectorIndexBackend::collectiveBroadcast(const float* src, float* dst, size_t size, int rootDevice) {
#ifdef THEMIS_ENABLE_RCCL
    // TODO: Implement RCCL broadcast
    (void)src; (void)dst; (void)size; (void)rootDevice;
#else
    (void)src; (void)dst; (void)size; (void)rootDevice;
#endif
}

std::vector<float> HIPVectorIndexBackend::computeDistances(
    const float* queries, size_t numQueries,
    const float* vectors, size_t numVectors,
    size_t dim, GPUVectorIndex::DistanceMetric metric) {
#ifdef THEMIS_ENABLE_HIP
    return pImpl->computeDistances(queries, numQueries, vectors, numVectors, metric);
#else
    (void)queries; (void)numQueries; (void)vectors; (void)numVectors;
    (void)dim; (void)metric;
    return {};
#endif
}

std::vector<std::vector<std::pair<uint32_t, float>>> HIPVectorIndexBackend::batchSearch(
    const float* queries, size_t numQueries,
    const float* vectors, size_t numVectors,
    size_t dim, size_t k, GPUVectorIndex::DistanceMetric metric) {
    // TODO: Implement batch search with top-k selection
    (void)queries; (void)numQueries; (void)vectors; (void)numVectors;
    (void)dim; (void)k; (void)metric;
    return {};
}

} // namespace index
} // namespace themis
