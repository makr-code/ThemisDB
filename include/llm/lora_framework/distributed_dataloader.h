/**
 * @file distributed_dataloader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/gpu_tensor.h"
#include <vector>
#include <memory>
#include <functional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Distributed data loader for multi-GPU training
 * 
 * Automatically shards data across GPUs for data-parallel training.
 * Handles:
 * - Data sharding across GPUs
 * - Distributed sampling
 * - Uneven data distribution
 * - Data prefetching
 * 
 * Example:
 * ```cpp
 * MultiGPUContext ctx(4);
 * DistributedDataLoader loader(dataset, 32, ctx);
 * 
 * for (auto& batch : loader) {
 *     // batch is vector of tensors, one per GPU
 *     auto outputs = model.forward(batch);
 * }
 * ```
 */
class DistributedDataLoader {
public:
    /**
     * @brief Dataset interface
     */
    class Dataset {
    public:
        virtual ~Dataset() = default;
        
        /**
         * @brief Get sample at index
         */
        virtual GPUTensor get(size_t index) const = 0;
        
        /**
         * @brief Get dataset size
         */
        virtual size_t size() const = 0;
    };
    
    /**
     * @brief Construct distributed data loader
     * @param dataset Dataset to load from
     * @param batch_size Total batch size (will be divided across GPUs)
     * @param ctx Multi-GPU context
     * @param shuffle Shuffle data each epoch
     * @param drop_last Drop last incomplete batch
     */
    DistributedDataLoader(
        const Dataset& dataset,
        size_t batch_size,
        const MultiGPUContext& ctx,
        bool shuffle = true,
        bool drop_last = false);
    
    ~DistributedDataLoader() = default;
    
    /**
     * @brief Batch iterator
     */
    class Iterator {
    public:
        Iterator(DistributedDataLoader* loader, size_t position);
        
        std::vector<GPUTensor> operator*();
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;
        
    private:
        DistributedDataLoader* loader_;
        size_t position_ = 0;
    };
    
    Iterator begin();
    Iterator end();
    
    /**
     * @brief Get number of batches per epoch
     */
    size_t num_batches() const { return num_batches_; }
    
    /**
     * @brief Get batch size per GPU
     */
    size_t batch_size_per_gpu() const { return batch_size_per_gpu_; }
    
    /**
     * @brief Reset iterator (for new epoch)
     */
    void reset();
    
private:
    const Dataset& dataset_;
    size_t batch_size_ = 0;
    const MultiGPUContext& ctx_;
    bool shuffle_ = false;
    bool drop_last_ = false;
    
    size_t batch_size_per_gpu_ = 0;
    size_t num_batches_ = 0;
    std::vector<size_t> indices_;
    
    void initialize_indices();
    std::vector<GPUTensor> load_batch(size_t batch_idx);
};

/**
 * @brief Simple in-memory dataset
 */
class InMemoryDataset : public DistributedDataLoader::Dataset {
public:
    explicit InMemoryDataset(std::vector<GPUTensor> data);
    ~InMemoryDataset() override = default;
    
    GPUTensor get(size_t index) const override;
    size_t size() const override { return data_.size(); }
    
private:
    std::vector<GPUTensor> data_;
};

} // namespace lora
} // namespace llm
} // namespace themis
