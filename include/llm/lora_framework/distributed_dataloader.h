/**
 * @file distributed_dataloader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class DistributedDataLoader {
public:
    class Dataset {
    public:
        /**
         * @brief Dataset.
         * @return Return value.
         */
        virtual ~Dataset() = default;
        
        /**
         * @brief Get.
         * @param[in] index Input parameter.
         * @return Return value.
         */
        virtual GPUTensor get(size_t index) const = 0;
        
        /**
         * @brief Size.
         * @return Return value.
         */
        virtual size_t size() const = 0;
    };
    
    DistributedDataLoader(
        const Dataset& dataset,
        size_t batch_size,
        const MultiGPUContext& ctx,
        bool shuffle = true,
        bool drop_last = false);
    
    ~DistributedDataLoader() = default;
    
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
    
    /**
     * @brief Begin.
     * @return Return value.
     */
    Iterator begin();
    /**
     * @brief End.
     * @return Return value.
     */
    Iterator end();
    
    size_t num_batches() const { return num_batches_; }
    
    size_t batch_size_per_gpu() const { return batch_size_per_gpu_; }
    
    /**
     * @brief Reset the modification detection flag.
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
    
    /**
     * @brief Initialize indices.
     */
    void initialize_indices();
    /**
     * @brief Load batch.
     * @param[in] batch_idx Input parameter.
     * @return Return value.
     */
    std::vector<GPUTensor> load_batch(size_t batch_idx);
};

class InMemoryDataset : public DistributedDataLoader::Dataset {
public:
    /**
     * @brief In Memory Dataset.
     * @param[in] data Input parameter.
     * @return Return value.
     */
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
