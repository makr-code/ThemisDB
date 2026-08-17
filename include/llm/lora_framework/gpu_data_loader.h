/**
 * @file gpu_data_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/data_loader.h"
#include "llm/lora_framework/vram_allocator.h"
#include <vector>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief GPU batch for training
 */
struct GPUBatch {
    GPUBatch() = default;
    ~GPUBatch() = default;
    GPUBatch(const GPUBatch&) = delete;
    GPUBatch& operator=(const GPUBatch&) = delete;
    GPUBatch(GPUBatch&&) noexcept = default;
    GPUBatch& operator=(GPUBatch&&) noexcept = default;
    GPUTensor input_ids;      // Tokenized input (batch_size, seq_len)
    GPUTensor attention_mask; // Attention mask (batch_size, seq_len)
    GPUTensor labels;         // Target labels (batch_size, seq_len)
    size_t batch_size = 0;
    size_t seq_len = 0;
    
    bool is_valid() const {
        return batch_size > 0 && seq_len > 0 && 
               input_ids.size() > 0 && labels.size() > 0;
    }
};

/**
 * @brief Configuration for GPU DataLoader
 */
struct GPUDataLoaderConfig {
    size_t batch_size = 4;
    size_t max_sequence_length = 512;
    bool shuffle = true;
    bool pad_to_max_length = true;
    Device target_device = Device::cuda();  // Target GPU device
    bool async_loading = true;              // Enable async data transfer
    size_t prefetch_batches = 2;            // Number of batches to prefetch
    bool pin_cpu_memory = true;             // Use pinned CPU memory for faster transfers
};

/**
 * @brief GPU-optimized DataLoader for LoRA training
 * 
 * Features:
 * - Direct loading to GPU VRAM
 * - Async GPU data transfer pipeline
 * - Batch padding and tokenization on GPU where possible
 * - Memory-efficient prefetching
 * - Support for CUDA, HIP, Vulkan, DirectX backends
 * 
 * Usage:
 * ```cpp
 * GPUDataLoaderConfig config;
 * config.batch_size = 8;
 * config.target_device = Device::cuda();
 * 
 * GPUDataLoader loader(tokenizer, config);
 * loader.loadFromSamples(training_samples);
 * 
 * while (loader.hasNext()) {
 *     auto batch = loader.getNextBatch();
 *     // batch.input_ids is already on GPU!
 * }
 * ```
 */
class GPUDataLoader {
public:
    /**
     * @brief Construct GPU DataLoader
     * @param tokenizer Tokenizer for text processing
     * @param config DataLoader configuration
     * @param allocator Optional VRAM allocator (nullptr = use default)
     */
    explicit GPUDataLoader(
        std::shared_ptr<ITokenizer> tokenizer,
        const GPUDataLoaderConfig& config = GPUDataLoaderConfig{},
        VRAMAllocator* allocator = nullptr
    );
    
    ~GPUDataLoader();
    
    // Disable copy, allow move
    GPUDataLoader(const GPUDataLoader&) = delete;
    GPUDataLoader& operator=(const GPUDataLoader&) = delete;
    GPUDataLoader(GPUDataLoader&&) noexcept;
    GPUDataLoader& operator=(GPUDataLoader&&) noexcept;
    
    /**
     * @brief Load training samples
     * @param samples Vector of instruction-tuning samples
     * @return true on success
     */
    bool loadFromSamples(const std::vector<InstructionDataSample>& samples);
    
    /**
     * @brief Get next batch (already on GPU)
     * @return GPU batch with tensors on target device
     */
    GPUBatch getNextBatch();
    
    /**
     * @brief Check if more batches available
     */
    bool hasNext() const;
    
    /**
     * @brief Reset iterator to beginning
     */
    void reset();
    
    /**
     * @brief Get number of samples
     */
    size_t size() const { return samples_.size(); }
    
    /**
     * @brief Get number of batches
     */
    size_t num_batches() const;
    
    /**
     * @brief Get current batch index
     */
    size_t current_batch_index() const { return current_batch_; }
    
    /**
     * @brief Get configuration
     */
    const GPUDataLoaderConfig& config() const { return config_; }
    
    /**
     * @brief Update batch size dynamically (for adaptive batching)
     * @param new_batch_size New batch size to use for subsequent batches
     * @return true if update successful, false if invalid batch size
     * 
     * Note: This resets the iterator to the current position with new batch size.
     * The change takes effect for the next call to getNextBatch().
     */
    bool updateBatchSize(size_t new_batch_size);
    
    /**
     * @brief Get memory statistics
     */
    struct MemoryStats {
        size_t cpu_memory_bytes = 0;
        size_t gpu_memory_bytes = 0;
        size_t pinned_memory_bytes = 0;
        size_t prefetch_buffer_bytes = 0;
    };
    
    MemoryStats get_memory_stats() const;

private:
    std::shared_ptr<ITokenizer> tokenizer_;
    GPUDataLoaderConfig config_;
    std::unique_ptr<VRAMAllocator> allocator_;
    VRAMAllocator* external_allocator_ = nullptr;  // Non-owning pointer for externally-provided allocator
    
    std::vector<InstructionDataSample> samples_;
    std::vector<size_t> indices_;  // For shuffling
    size_t current_batch_ = 0;
    
    // Async loading infrastructure
    std::thread prefetch_thread_;
    std::queue<GPUBatch> prefetch_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> stop_prefetch_{false};
    std::atomic<bool> prefetch_active_{false};
    
    // Helper methods
    void startPrefetching();
    void stopPrefetching();
    void prefetchWorker();
    GPUBatch prepareBatch(size_t batch_idx);
    std::vector<int> tokenizeSample(const InstructionDataSample& sample);
};

} // namespace lora
} // namespace llm
} // namespace themis

