/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_dataloader.h                           ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:16:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
        size_t position_;
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
    size_t batch_size_;
    const MultiGPUContext& ctx_;
    bool shuffle_;
    bool drop_last_;
    
    size_t batch_size_per_gpu_;
    size_t num_batches_;
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
    
    GPUTensor get(size_t index) const override;
    size_t size() const override { return data_.size(); }
    
private:
    std::vector<GPUTensor> data_;
};

} // namespace lora
} // namespace llm
} // namespace themis
