/**
 * ThemisDB GPU-Accelerated Erasure Coding - CUDA Implementation
 * 
 * CUDA kernels for Reed-Solomon erasure coding with Galois field operations
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef THEMIS_ENABLE_CUDA

#include "sharding/gpu_erasure_coder.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstring>

namespace themis {
namespace sharding {

// ═══════════════════════════════════════════════════════════
// CUDA Error Checking
// ═══════════════════════════════════════════════════════════

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            throw std::runtime_error(std::string("CUDA error: ") + cudaGetErrorString(err)); \
        } \
    } while(0)

// ═══════════════════════════════════════════════════════════
// Galois Field (GF(2^8)) Tables and Operations
// ═══════════════════════════════════════════════════════════

// GF(2^8) multiplication table (constant memory for fast access)
__constant__ uint8_t d_gf_mul_table[256][256];
__constant__ uint8_t d_gf_exp_table[512];
__constant__ uint8_t d_gf_log_table[256];

// CPU versions for initialization
namespace {
    uint8_t gf_mul_cpu(uint8_t a, uint8_t b) {
        if (a == 0 || b == 0) return 0;
        
        uint16_t prod = 0;
        for (int i = 0; i < 8; i++) {
            if (b & 1) prod ^= a;
            bool carry = (a & 0x80);
            a <<= 1;
            // GF(2^8) irreducible polynomial: x^8 + x^4 + x^3 + x + 1 (0x11B)
            // For reduction, we use 0x1B (lower 8 bits, since x^8 coefficient is implicit)
            if (carry) a ^= 0x1B;
            b >>= 1;
        }
        return static_cast<uint8_t>(prod);
    }
    
    void init_gf_tables(uint8_t* exp_table, uint8_t* log_table) {
        uint8_t x = 1;
        for (int i = 0; i < 255; i++) {
            exp_table[i] = x;
            log_table[x] = static_cast<uint8_t>(i);
            x = gf_mul_cpu(x, 0x02);  // Generator is 2
        }
        exp_table[255] = exp_table[0];
        log_table[0] = 0;
        
        // Duplicate for wrap-around
        for (int i = 0; i < 256; i++) {
            exp_table[i + 256] = exp_table[i];
        }
    }
}

// ═══════════════════════════════════════════════════════════
// CUDA Kernels
// ═══════════════════════════════════════════════════════════

/**
 * Galois Field multiplication on GPU
 */
__device__ uint8_t gf_mul_device(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) return 0;
    return d_gf_exp_table[d_gf_log_table[a] + d_gf_log_table[b]];
}

/**
 * Encode kernel: Compute parity shards from data shards
 * Each thread processes one byte position across all chunks
 */
__global__ void encodeParityKernel(
    const uint8_t* const* data_chunks,  // Array of pointers to data chunks
    uint8_t** parity_chunks,            // Array of pointers to parity chunks
    const uint8_t* encoding_matrix,     // Encoding matrix (parity_shards x data_shards)
    uint32_t chunk_size,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    int byte_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (byte_idx >= chunk_size) return;
    
    // Each thread computes all parity bytes at position byte_idx
    for (int p = 0; p < parity_shards; p++) {
        uint8_t parity = 0;
        
        // Matrix multiplication in GF(2^8)
        for (int d = 0; d < data_shards; d++) {
            uint8_t coeff = encoding_matrix[p * data_shards + d];
            uint8_t data_byte = data_chunks[d][byte_idx];
            parity ^= gf_mul_device(coeff, data_byte);
        }
        
        parity_chunks[p][byte_idx] = parity;
    }
}

/**
 * Decode kernel: Recover missing data from available chunks
 */
__global__ void decodeDataKernel(
    const uint8_t* const* available_chunks,  // Available chunks
    uint8_t** recovered_chunks,              // Output recovered chunks
    const uint8_t* decoding_matrix,          // Decoding matrix
    const uint32_t* available_indices,       // Indices of available chunks
    const uint32_t* missing_indices,         // Indices of missing chunks
    uint32_t chunk_size,
    uint32_t num_available,
    uint32_t num_missing
) {
    int byte_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (byte_idx >= chunk_size) return;
    
    // Recover each missing chunk
    for (int m = 0; m < num_missing; m++) {
        uint8_t recovered = 0;
        
        // Matrix multiplication in GF(2^8)
        for (int a = 0; a < num_available; a++) {
            uint8_t coeff = decoding_matrix[m * num_available + a];
            uint8_t data_byte = available_chunks[a][byte_idx];
            recovered ^= gf_mul_device(coeff, data_byte);
        }
        
        recovered_chunks[m][byte_idx] = recovered;
    }
}

// ═══════════════════════════════════════════════════════════
// CUDA Implementation Class
// ═══════════════════════════════════════════════════════════

class CUDAErasureCoderImpl : public GPUErasureCoderImpl {
public:
    CUDAErasureCoderImpl(ErasureCodingAlgorithm algorithm)
        : algorithm_(algorithm) {}
    
    ~CUDAErasureCoderImpl() override {
        shutdown();
    }
    
    bool initialize(const GPUConfig& config) override {
        try {
            config_ = config;
            
            // Set CUDA device
            CUDA_CHECK(cudaSetDevice(config.device_id));
            
            // Get device properties
            cudaDeviceProp prop;
            CUDA_CHECK(cudaGetDeviceProperties(&prop, config.device_id));
            spdlog::info("CUDA device {}: {} (compute {}.{})", 
                        config.device_id, prop.name, prop.major, prop.minor);
            
            // Initialize Galois field tables
            initGFTables();
            
            // Create CUDA streams for async operations
            if (config.async_compute) {
                streams_.resize(config.cuda_streams);
                for (auto& stream : streams_) {
                    CUDA_CHECK(cudaStreamCreate(&stream));
                }
            }
            
            // Allocate pinned host memory if requested
            if (config.use_pinned_memory) {
                CUDA_CHECK(cudaMallocHost(&pinned_buffer_, config.pinned_buffer_size));
            }
            
            initialized_ = true;
            return true;
            
        } catch (const std::exception& e) {
            spdlog::error("CUDA initialization failed: {}", e.what());
            return false;
        }
    }
    
    void shutdown() override {
        if (!initialized_) return;
        
        // Destroy streams
        for (auto stream : streams_) {
            if (stream) {
                cudaStreamDestroy(stream);
            }
        }
        streams_.clear();
        
        // Free pinned memory
        if (pinned_buffer_) {
            cudaFreeHost(pinned_buffer_);
            pinned_buffer_ = nullptr;
        }
        
        initialized_ = false;
    }
    
    std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override {
        if (!initialized_) {
            throw std::runtime_error("CUDA erasure coder not initialized");
        }
        
        // Calculate chunk size
        size_t chunk_size = (data.size() + data_shards - 1) / data_shards;
        size_t total_size = chunk_size * data_shards;
        
        // Split data into chunks (pad if necessary)
        std::vector<std::vector<uint8_t>> data_chunks(data_shards);
        for (uint32_t i = 0; i < data_shards; i++) {
            size_t offset = i * chunk_size;
            size_t size = std::min(chunk_size, data.size() - offset);
            data_chunks[i].resize(chunk_size, 0);
            if (offset < data.size()) {
                std::memcpy(data_chunks[i].data(), data.data() + offset, size);
            }
        }
        
        // Allocate device memory for data chunks
        std::vector<uint8_t*> d_data_ptrs(data_shards);
        for (uint32_t i = 0; i < data_shards; i++) {
            CUDA_CHECK(cudaMalloc(&d_data_ptrs[i], chunk_size));
            CUDA_CHECK(cudaMemcpy(d_data_ptrs[i], data_chunks[i].data(), 
                                 chunk_size, cudaMemcpyHostToDevice));
        }
        
        // Allocate device memory for parity chunks
        std::vector<uint8_t*> d_parity_ptrs(parity_shards);
        for (uint32_t i = 0; i < parity_shards; i++) {
            CUDA_CHECK(cudaMalloc(&d_parity_ptrs[i], chunk_size));
        }
        
        // Create encoding matrix (Vandermonde matrix)
        std::vector<uint8_t> encoding_matrix = createEncodingMatrix(data_shards, parity_shards);
        uint8_t* d_encoding_matrix;
        CUDA_CHECK(cudaMalloc(&d_encoding_matrix, encoding_matrix.size()));
        CUDA_CHECK(cudaMemcpy(d_encoding_matrix, encoding_matrix.data(), 
                             encoding_matrix.size(), cudaMemcpyHostToDevice));
        
        // Copy pointer arrays to device
        uint8_t** d_data_array;
        uint8_t** d_parity_array;
        CUDA_CHECK(cudaMalloc(&d_data_array, data_shards * sizeof(uint8_t*)));
        CUDA_CHECK(cudaMalloc(&d_parity_array, parity_shards * sizeof(uint8_t*)));
        CUDA_CHECK(cudaMemcpy(d_data_array, d_data_ptrs.data(), 
                             data_shards * sizeof(uint8_t*), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_parity_array, d_parity_ptrs.data(), 
                             parity_shards * sizeof(uint8_t*), cudaMemcpyHostToDevice));
        
        // Launch kernel
        int threads_per_block = 256;
        int num_blocks = (chunk_size + threads_per_block - 1) / threads_per_block;
        
        cudaStream_t stream = config_.async_compute && !streams_.empty() ? streams_[0] : 0;
        encodeParityKernel<<<num_blocks, threads_per_block, 0, stream>>>(
            d_data_array, d_parity_array, d_encoding_matrix,
            chunk_size, data_shards, parity_shards
        );
        
        CUDA_CHECK(cudaStreamSynchronize(stream));
        
        // Copy parity chunks back to host
        std::vector<std::vector<uint8_t>> parity_chunks(parity_shards);
        for (uint32_t i = 0; i < parity_shards; i++) {
            parity_chunks[i].resize(chunk_size);
            CUDA_CHECK(cudaMemcpy(parity_chunks[i].data(), d_parity_ptrs[i], 
                                 chunk_size, cudaMemcpyDeviceToHost));
        }
        
        // Free device memory
        for (auto ptr : d_data_ptrs) cudaFree(ptr);
        for (auto ptr : d_parity_ptrs) cudaFree(ptr);
        cudaFree(d_encoding_matrix);
        cudaFree(d_data_array);
        cudaFree(d_parity_array);
        
        // Combine data and parity chunks
        std::vector<std::vector<uint8_t>> result;
        result.reserve(data_shards + parity_shards);
        result.insert(result.end(), data_chunks.begin(), data_chunks.end());
        result.insert(result.end(), parity_chunks.begin(), parity_chunks.end());
        
        return result;
    }
    
    std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override {
        // GPU decode not yet fully implemented
        // The base GPUErasureCoder class will catch this exception and 
        // automatically fall back to CPU-based decoding
        throw std::runtime_error("GPU decode not implemented");
    }
    
    std::vector<std::vector<std::vector<uint8_t>>> batchEncode(
        const std::vector<std::vector<uint8_t>>& data_blocks,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override {
        // Batch encode using multiple CUDA streams
        std::vector<std::vector<std::vector<uint8_t>>> results;
        results.reserve(data_blocks.size());
        
        for (const auto& block : data_blocks) {
            results.push_back(encode(block, data_shards, parity_shards));
        }
        
        return results;
    }
    
    bool isAvailable() const override {
        return initialized_;
    }

private:
    ErasureCodingAlgorithm algorithm_;
    GPUConfig config_;
    bool initialized_ = false;
    std::vector<cudaStream_t> streams_;
    void* pinned_buffer_ = nullptr;
    
    void initGFTables() {
        // Initialize on host
        uint8_t exp_table[512];
        uint8_t log_table[256];
        init_gf_tables(exp_table, log_table);
        
        // Copy to device constant memory
        CUDA_CHECK(cudaMemcpyToSymbol(d_gf_exp_table, exp_table, sizeof(exp_table)));
        CUDA_CHECK(cudaMemcpyToSymbol(d_gf_log_table, log_table, sizeof(log_table)));
    }
    
    std::vector<uint8_t> createEncodingMatrix(uint32_t data_shards, uint32_t parity_shards) {
        // Create Vandermonde matrix for Reed-Solomon encoding
        std::vector<uint8_t> matrix(parity_shards * data_shards);
        
        for (uint32_t p = 0; p < parity_shards; p++) {
            for (uint32_t d = 0; d < data_shards; d++) {
                // Vandermonde: matrix[p][d] = (d+1)^(p+data_shards)
                uint8_t base = static_cast<uint8_t>(d + 1);
                uint8_t power = static_cast<uint8_t>(p + data_shards);
                uint8_t result = 1;
                for (uint32_t i = 0; i < power; i++) {
                    result = gf_mul_cpu(result, base);
                }
                matrix[p * data_shards + d] = result;
            }
        }
        
        return matrix;
    }
};

// ═══════════════════════════════════════════════════════════
// Factory Function
// ═══════════════════════════════════════════════════════════

std::unique_ptr<GPUErasureCoderImpl> createCUDAErasureCoder(
    const GPUConfig& config,
    ErasureCodingAlgorithm algorithm
) {
    return std::make_unique<CUDAErasureCoderImpl>(algorithm);
}

} // namespace sharding
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
