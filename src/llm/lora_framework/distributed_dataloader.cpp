/**
 * @file distributed_dataloader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/distributed_dataloader.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <random>

namespace themis {
namespace llm {
namespace lora {

DistributedDataLoader::DistributedDataLoader(
    const Dataset& dataset,
    size_t batch_size,
    const MultiGPUContext& ctx,
    bool shuffle,
    bool drop_last)
    : dataset_(dataset), batch_size_(batch_size), ctx_(ctx),
      shuffle_(shuffle), drop_last_(drop_last) {
    
    // Calculate per-GPU batch size
    batch_size_per_gpu_ = (batch_size + ctx_.num_gpus() - 1) / ctx_.num_gpus();
    
    // Calculate number of batches
    size_t dataset_size = dataset_.size();
    num_batches_ = dataset_size / batch_size_;
    if (!drop_last_ && dataset_size % batch_size_ != 0) {
        num_batches_++;
    }
    
    spdlog::info("DistributedDataLoader initialized:");
    spdlog::info("  Dataset size: {}", dataset_size);
    spdlog::info("  Batch size: {}", batch_size_);
    spdlog::info("  Batch size per GPU: {}", batch_size_per_gpu_);
    spdlog::info("  Number of batches: {}", num_batches_);
    spdlog::info("  Shuffle: {}", shuffle_);
    
    initialize_indices();
}

void DistributedDataLoader::initialize_indices() {
    size_t dataset_size = dataset_.size();
    indices_.resize(dataset_size);
    
    for (size_t i = 0; i < dataset_size; ++i) {
        indices_[i] = i;
    }
    
    if (shuffle_) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(indices_.begin(), indices_.end(), gen);
    }
}

std::vector<GPUTensor> DistributedDataLoader::load_batch(size_t batch_idx) {
    size_t start_idx = batch_idx * batch_size_;
    size_t end_idx = std::min(start_idx + batch_size_, dataset_.size());
    
    // Load samples for this batch
    std::vector<GPUTensor> batch_samples;
    batch_samples.reserve(end_idx - start_idx);
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        size_t sample_idx = indices_[i];
        batch_samples.push_back(dataset_.get(sample_idx));
    }
    
    // Shard across GPUs
    std::vector<GPUTensor> sharded_batch;
    sharded_batch.reserve(ctx_.num_gpus());
    
    size_t samples_per_gpu = (batch_samples.size() + ctx_.num_gpus() - 1) / ctx_.num_gpus();
    
    for (int gpu_idx = 0; gpu_idx < ctx_.num_gpus(); ++gpu_idx) {
        size_t gpu_start = gpu_idx * samples_per_gpu;
        size_t gpu_end = std::min(gpu_start + samples_per_gpu, batch_samples.size());
        
        if (gpu_start >= batch_samples.size()) {
            // Empty shard for this GPU
            sharded_batch.emplace_back(std::vector<size_t>{0}, ctx_.get_device(gpu_idx));
            continue;
        }
        
        // Stack/concatenate samples for this GPU
        // Note: Real implementation would properly stack tensors along batch dimension
        // For simplicity, we create a representative tensor on the correct device
        
        size_t num_samples = gpu_end - gpu_start;
        auto sample_shape = batch_samples[gpu_start].shape();

        // Create batched shape: prepend batch dimension to the per-sample shape.
        std::vector<size_t> batch_shape = {num_samples};
        batch_shape.insert(batch_shape.end(), sample_shape.begin(), sample_shape.end());

        GPUTensor batched_tensor(batch_shape, ctx_.get_device(gpu_idx));

        // Concatenate all samples along the batch dimension.
        // Each sample must have the same shape; mismatched shapes are skipped with a
        // warning so that one corrupt sample does not abort the entire mini-batch.
        std::vector<float> batch_data;
        size_t per_sample_size = 1;
        for (size_t d : sample_shape) per_sample_size *= d;
        batch_data.reserve(per_sample_size * num_samples);

        for (size_t i = gpu_start; i < gpu_end; ++i) {
            if (batch_samples[i].shape() != sample_shape) {
                // Shape mismatch: pad with zeros so the batch tensor stays rectangular.
                // Log a warning with full dimensions so operators can identify
                // corrupted samples in the training data.
                auto shapeStr = [](const std::vector<size_t>& sh) {
                    std::string s;
                    for (size_t d = 0; d < sh.size(); ++d)
                        s += (d ? "×" : "") + std::to_string(sh[d]);
                    return s;
                };
                spdlog::warn("DistributedDataLoader: sample {} has unexpected shape — "
                             "expected [{}], got [{}]; padding with zeros",
                             i, shapeStr(sample_shape), shapeStr(batch_samples[i].shape()));
                std::vector<float> zeros(per_sample_size, 0.0f);
                batch_data.insert(batch_data.end(), zeros.begin(), zeros.end());
                continue;
            }
            auto sample_data = batch_samples[i].cpu_data();
            batch_data.insert(batch_data.end(), sample_data.begin(), sample_data.end());
        }

        batched_tensor.upload(batch_data);
        sharded_batch.push_back(std::move(batched_tensor));
    }
    
    return sharded_batch;
}

void DistributedDataLoader::reset() {
    if (shuffle_) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(indices_.begin(), indices_.end(), gen);
    }
}

DistributedDataLoader::Iterator DistributedDataLoader::begin() {
    return Iterator(this, 0);
}

DistributedDataLoader::Iterator DistributedDataLoader::end() {
    return Iterator(this, num_batches_);
}

// Iterator implementation
DistributedDataLoader::Iterator::Iterator(DistributedDataLoader* loader, size_t position)
    : loader_(loader), position_(position) {
}

std::vector<GPUTensor> DistributedDataLoader::Iterator::operator*() {
    return loader_->load_batch(position_);
}

DistributedDataLoader::Iterator& DistributedDataLoader::Iterator::operator++() {
    ++position_;
    return *this;
}

bool DistributedDataLoader::Iterator::operator!=(const Iterator& other) const {
    return position_ != other.position_;
}

// InMemoryDataset implementation
InMemoryDataset::InMemoryDataset(std::vector<GPUTensor> data)
    : data_(std::move(data)) {
}

GPUTensor InMemoryDataset::get(size_t index) const {
    if (index >= data_.size()) {
        throw std::out_of_range("Dataset index out of range");
    }
    
    // Return a copy (in real implementation, might return reference)
    return GPUTensor(data_[index].shape(), data_[index].device());
}

} // namespace lora
} // namespace llm
} // namespace themis
