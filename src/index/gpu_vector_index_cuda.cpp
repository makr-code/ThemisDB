#include "index/gpu_vector_index.h"
#include <iostream>
#include <stdexcept>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#endif

namespace themis {
namespace index {

#ifdef THEMIS_ENABLE_CUDA

// CUDA kernels will be in separate .cu file
extern "C" {
    void launchL2DistanceKernel(const float* queries, const float* vectors,
                               float* results, int numQueries, int numVectors,
                               int dimension, cudaStream_t stream);
    
    void launchCosineDistanceKernel(const float* queries, const float* vectors,
                                   float* results, int numQueries, int numVectors,
                                   int dimension, cudaStream_t stream);
    
    void launchInnerProductKernel(const float* queries, const float* vectors,
                                 float* results, int numQueries, int numVectors,
                                 int dimension, cudaStream_t stream);
    
    void launchTopKKernel(const float* distances, uint32_t* indices, float* topKDistances,
                         uint32_t* topKIndices, int numQueries, int numVectors,
                         int k, cudaStream_t stream);
}

// =============================================================================
// CUDAVectorIndexBackend::Impl
// =============================================================================

class CUDAVectorIndexBackend::Impl {
public:
    int deviceId = 0;
    int dimension = 0;
    GPUVectorIndex::Config config;
    bool initialized = false;
    
    // CUDA resources
    cudaStream_t stream = nullptr;
    
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
    
    // Mixed precision settings
    bool useFP16 = false;
    bool useTF32 = false;
    bool useINT8 = false;
    
    // Optimization flags
    bool useFlashAttention = false;
    bool useTensorCores = false;
    bool useUnifiedMem = false;
    
    ~Impl() {
        cleanup();
    }
    
    bool initialize(int dim, const GPUVectorIndex::Config& cfg) {
        dimension = dim;
        config = cfg;
        deviceId = cfg.deviceId;
        
        // Set CUDA device
        cudaError_t err = cudaSetDevice(deviceId);
        if (err != cudaSuccess) {
            std::cerr << "Failed to set CUDA device " << deviceId << ": "
                     << cudaGetErrorString(err) << std::endl;
            return false;
        }
        
        // Check device properties
        cudaDeviceProp prop;
        err = cudaGetDeviceProperties(&prop, deviceId);
        if (err != cudaSuccess) {
            std::cerr << "Failed to get device properties: "
                     << cudaGetErrorString(err) << std::endl;
            return false;
        }
        
        std::cout << "CUDA Device: " << prop.name << std::endl;
        std::cout << "  Compute Capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "  Total VRAM: " << (prop.totalGlobalMem / (1024*1024)) << " MB" << std::endl;
        
        // Check for Tensor Core support (Compute Capability >= 7.0)
        if (prop.major >= 7) {
            useTensorCores = true;
            std::cout << "  Tensor Cores: Available" << std::endl;
        }
        
        // Create CUDA stream
        err = cudaStreamCreate(&stream);
        if (err != cudaSuccess) {
            std::cerr << "Failed to create CUDA stream: "
                     << cudaGetErrorString(err) << std::endl;
            return false;
        }
        
        // Enable mixed precision if configured
        if (config.useMixedPrecision) {
            useFP16 = true;
            useTF32 = (prop.major >= 8); // TF32 available on Ampere (SM 8.x) and newer
            std::cout << "  Mixed Precision: FP16=" << useFP16 << ", TF32=" << useTF32 << std::endl;
        }
        
        initialized = true;
        return true;
    }
    
    void cleanup() {
        if (!initialized) return;
        
        // Free device memory
        if (d_queries) cudaFree(d_queries);
        if (d_vectors) cudaFree(d_vectors);
        if (d_distances) cudaFree(d_distances);
        if (d_indices) cudaFree(d_indices);
        
        // Destroy stream
        if (stream) cudaStreamDestroy(stream);
        
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
            if (d_queries) cudaFree(d_queries);
            cudaError_t err = cudaMalloc(&d_queries, querySize);
            if (err != cudaSuccess) {
                std::cerr << "Failed to allocate query buffer: "
                         << cudaGetErrorString(err) << std::endl;
                return false;
            }
            queryBufferSize = querySize;
        }
        
        if (vectorSize > vectorBufferSize) {
            if (d_vectors) cudaFree(d_vectors);
            cudaError_t err = cudaMalloc(&d_vectors, vectorSize);
            if (err != cudaSuccess) {
                std::cerr << "Failed to allocate vector buffer: "
                         << cudaGetErrorString(err) << std::endl;
                return false;
            }
            vectorBufferSize = vectorSize;
        }
        
        if (distanceSize > distanceBufferSize) {
            if (d_distances) cudaFree(d_distances);
            cudaError_t err = cudaMalloc(&d_distances, distanceSize);
            if (err != cudaSuccess) {
                std::cerr << "Failed to allocate distance buffer: "
                         << cudaGetErrorString(err) << std::endl;
                return false;
            }
            distanceBufferSize = distanceSize;
        }
        
        if (indexSize > indexBufferSize) {
            if (d_indices) cudaFree(d_indices);
            cudaError_t err = cudaMalloc(&d_indices, indexSize);
            if (err != cudaSuccess) {
                std::cerr << "Failed to allocate index buffer: "
                         << cudaGetErrorString(err) << std::endl;
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
        cudaError_t err;
        err = cudaMemcpyAsync(d_queries, queries, numQueries * dimension * sizeof(float),
                             cudaMemcpyHostToDevice, stream);
        if (err != cudaSuccess) {
            std::cerr << "Failed to copy queries to device: "
                     << cudaGetErrorString(err) << std::endl;
            return {};
        }
        
        err = cudaMemcpyAsync(d_vectors, vectors, numVectors * dimension * sizeof(float),
                             cudaMemcpyHostToDevice, stream);
        if (err != cudaSuccess) {
            std::cerr << "Failed to copy vectors to device: "
                     << cudaGetErrorString(err) << std::endl;
            return {};
        }
        
        // Launch distance kernel
        switch (metric) {
            case GPUVectorIndex::DistanceMetric::L2:
                launchL2DistanceKernel(d_queries, d_vectors, d_distances,
                                      numQueries, numVectors, dimension, stream);
                break;
            case GPUVectorIndex::DistanceMetric::COSINE:
                launchCosineDistanceKernel(d_queries, d_vectors, d_distances,
                                          numQueries, numVectors, dimension, stream);
                break;
            case GPUVectorIndex::DistanceMetric::INNER_PRODUCT:
                launchInnerProductKernel(d_queries, d_vectors, d_distances,
                                        numQueries, numVectors, dimension, stream);
                break;
        }
        
        // Check for kernel errors
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "Kernel launch failed: " << cudaGetErrorString(err) << std::endl;
            return {};
        }
        
        // Copy results back
        std::vector<float> results(numQueries * numVectors);
        err = cudaMemcpyAsync(results.data(), d_distances,
                             numQueries * numVectors * sizeof(float),
                             cudaMemcpyDeviceToHost, stream);
        if (err != cudaSuccess) {
            std::cerr << "Failed to copy results from device: "
                     << cudaGetErrorString(err) << std::endl;
            return {};
        }
        
        // Wait for completion
        cudaStreamSynchronize(stream);
        
        return results;
    }
};

#else // !THEMIS_ENABLE_CUDA

// Stub implementation when CUDA is not available
class CUDAVectorIndexBackend::Impl {
public:
    bool initialize(int, const GPUVectorIndex::Config&) {
        std::cerr << "CUDA support not compiled in\n";
        return false;
    }
    void cleanup() {}
};

#endif // THEMIS_ENABLE_CUDA

// =============================================================================
// CUDAVectorIndexBackend public interface
// =============================================================================

CUDAVectorIndexBackend::CUDAVectorIndexBackend()
    : pImpl(std::make_unique<Impl>()) {
}

CUDAVectorIndexBackend::~CUDAVectorIndexBackend() = default;

bool CUDAVectorIndexBackend::initialize(int dimension, const GPUVectorIndex::Config& config) {
    return pImpl->initialize(dimension, config);
}

void CUDAVectorIndexBackend::shutdown() {
#ifdef THEMIS_ENABLE_CUDA
    pImpl->cleanup();
#endif
}

bool CUDAVectorIndexBackend::enableMixedPrecision(bool useFP16, bool useTF32, bool useINT8) {
#ifdef THEMIS_ENABLE_CUDA
    pImpl->useFP16 = useFP16;
    pImpl->useTF32 = useTF32;
    pImpl->useINT8 = useINT8;
    return true;
#else
    (void)useFP16; (void)useTF32; (void)useINT8;
    return false;
#endif
}

void CUDAVectorIndexBackend::enableFlashAttentionOptimization(bool enable) {
#ifdef THEMIS_ENABLE_CUDA
    pImpl->useFlashAttention = enable;
#else
    (void)enable;
#endif
}

bool CUDAVectorIndexBackend::hasTensorCoreSupport() const {
#ifdef THEMIS_ENABLE_CUDA
    return pImpl->useTensorCores;
#else
    return false;
#endif
}

void CUDAVectorIndexBackend::enableTensorCores(bool enable) {
#ifdef THEMIS_ENABLE_CUDA
    pImpl->useTensorCores = enable;
#else
    (void)enable;
#endif
}

void CUDAVectorIndexBackend::optimizeMemoryCoalescing() {
    // Memory coalescing is handled automatically in kernel implementations
}

bool CUDAVectorIndexBackend::enableUnifiedMemory(bool enable) {
#ifdef THEMIS_ENABLE_CUDA
    pImpl->useUnifiedMem = enable;
    return true;
#else
    (void)enable;
    return false;
#endif
}

bool CUDAVectorIndexBackend::createComputeGraph() {
    // TODO: Implement CUDA graph creation
    return false;
}

void CUDAVectorIndexBackend::executeComputeGraph() {
    // TODO: Implement CUDA graph execution
}

std::vector<float> CUDAVectorIndexBackend::computeDistances(
    const float* queries, size_t numQueries,
    const float* vectors, size_t numVectors,
    size_t dim, GPUVectorIndex::DistanceMetric metric) {
#ifdef THEMIS_ENABLE_CUDA
    return pImpl->computeDistances(queries, numQueries, vectors, numVectors, metric);
#else
    (void)queries; (void)numQueries; (void)vectors; (void)numVectors;
    (void)dim; (void)metric;
    return {};
#endif
}

std::vector<std::vector<std::pair<uint32_t, float>>> CUDAVectorIndexBackend::batchSearch(
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
