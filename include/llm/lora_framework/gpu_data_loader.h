/**
 * @file gpu_data_loader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class GPUDataLoader {
public:
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
     * @brief Load From Samples.
     * @param[in] samples Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromSamples(const std::vector<InstructionDataSample>& samples);
    
    /**
     * @brief Get Next Batch.
     * @return Return value.
     */
    GPUBatch getNextBatch();
    
    /**
     * @brief Has Next.
     * @return True when the operation succeeds.
     */
    bool hasNext() const;
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
    size_t size() const { return samples_.size(); }
    
    /**
     * @brief Num batches.
     * @return Return value.
     */
    size_t num_batches() const;
    
    size_t current_batch_index() const { return current_batch_; }
    
    const GPUDataLoaderConfig& config() const { return config_; }
    
    /**
     * @brief Update Batch Size.
     * @param[in] new_batch_size Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateBatchSize(size_t new_batch_size);
    
    struct MemoryStats {
        size_t cpu_memory_bytes = 0;
        size_t gpu_memory_bytes = 0;
        size_t pinned_memory_bytes = 0;
        size_t prefetch_buffer_bytes = 0;
    };
    
    /**
     * @brief Get memory stats.
     * @return Return value.
     */
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
    /**
     * @brief Start Prefetching.
     */
    void startPrefetching();
    /**
     * @brief Stop Prefetching.
     */
    void stopPrefetching();
    /**
     * @brief Prefetch Worker.
     */
    void prefetchWorker();
    /**
     * @brief Prepare Batch.
     * @param[in] batch_idx Input parameter.
     * @return Return value.
     */
    GPUBatch prepareBatch(size_t batch_idx);
    /**
     * @brief Tokenize Sample.
     * @param[in] sample Input parameter.
     * @return Return value.
     */
    std::vector<int> tokenizeSample(const InstructionDataSample& sample);
};

} // namespace lora
} // namespace llm
} // namespace themis

