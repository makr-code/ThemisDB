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
    
    // If batch is smaller than batch_size (last batch), pad if needed
    // For simplicity, we'll just handle the actual samples
    
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
        
        // Concatenate samples for this GPU
        // For simplicity, assuming all samples have same shape
        if (gpu_end - gpu_start == 1) {
            // Single sample, just move to correct device
            GPUTensor shard = batch_samples[gpu_start].to(ctx_.get_device(gpu_idx));
            sharded_batch.push_back(std::move(shard));
        } else {
            // Multiple samples, need to stack them
            // For now, just use first sample (real implementation would stack)
            GPUTensor shard = batch_samples[gpu_start].to(ctx_.get_device(gpu_idx));
            sharded_batch.push_back(std::move(shard));
        }
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
