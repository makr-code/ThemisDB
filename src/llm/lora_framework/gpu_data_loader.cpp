/**
 * @file gpu_data_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gpu_data_loader.h"
#include "acceleration/compute_backend.h"
#include "utils/thread_join_utils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <random>

namespace themis {
namespace llm {
namespace lora {

GPUDataLoader::GPUDataLoader(
    std::shared_ptr<ITokenizer> tokenizer,
    const GPUDataLoaderConfig& config,
    VRAMAllocator* allocator
) : tokenizer_(tokenizer), config_(config), external_allocator_(allocator) {
    
    // Create allocator if not provided
    if (!external_allocator_) {
        acceleration::BackendType backend = acceleration::BackendType::CUDA;
        if (config_.target_device.type == DeviceType::HIP) {
            backend = acceleration::BackendType::HIP;
        } else if (config_.target_device.type == DeviceType::VULKAN) {
            backend = acceleration::BackendType::VULKAN;
        } else if (config_.target_device.type == DeviceType::DIRECTX) {
            backend = acceleration::BackendType::DIRECTX;
        }
        
        allocator_ = std::make_unique<VRAMAllocator>(backend);
    }
    
    spdlog::info("GPUDataLoader initialized:");
    spdlog::info("  Batch size: {}", config_.batch_size);
    spdlog::info("  Max sequence length: {}", config_.max_sequence_length);
    spdlog::info("  Target device: {}", static_cast<int>(config_.target_device.type));
    spdlog::info("  Async loading: {}", config_.async_loading);
    spdlog::info("  Prefetch batches: {}", config_.prefetch_batches);
}

GPUDataLoader::~GPUDataLoader() {
    stopPrefetching();
    // allocator_ is automatically cleaned up by unique_ptr
}

GPUDataLoader::GPUDataLoader(GPUDataLoader&& other) noexcept
    : tokenizer_(std::move(other.tokenizer_))
    , config_(other.config_)
    , allocator_(std::move(other.allocator_))
    , external_allocator_(other.external_allocator_)
    , samples_(std::move(other.samples_))
    , indices_(std::move(other.indices_))
    , current_batch_(other.current_batch_)
{
    other.external_allocator_ = nullptr;
}

GPUDataLoader& GPUDataLoader::operator=(GPUDataLoader&& other) noexcept {
    if (this != &other) {
        stopPrefetching();
        
        // unique_ptr automatically handles cleanup
        tokenizer_ = std::move(other.tokenizer_);
        config_ = other.config_;
        allocator_ = std::move(other.allocator_);
        external_allocator_ = other.external_allocator_;
        samples_ = std::move(other.samples_);
        indices_ = std::move(other.indices_);
        current_batch_ = other.current_batch_;
        
        other.external_allocator_ = nullptr;
    }
    return *this;
}

bool GPUDataLoader::loadFromSamples(const std::vector<InstructionDataSample>& samples) {
    if (samples.empty()) {
        spdlog::error("Cannot load empty sample list");
        return false;
    }
    
    samples_ = samples;
    
    // Initialize indices for shuffling
    indices_.resize(samples_.size());
    std::iota(indices_.begin(), indices_.end(), 0);
    
    if (config_.shuffle) {
        std::random_device rd = {};
        std::mt19937 gen(rd());
        std::shuffle(indices_.begin(), indices_.end(), gen);
        spdlog::debug("Shuffled {} samples", indices_.size());
    }
    
    current_batch_ = 0;
    
    // Start async prefetching if enabled
    if (config_.async_loading) {
        startPrefetching();
    }
    
    spdlog::info("Loaded {} samples, {} batches", samples_.size(), num_batches());
    return true;
}

GPUBatch GPUDataLoader::getNextBatch() {
    if (!hasNext()) {
        spdlog::warn("No more batches available");
        return GPUBatch{};
    }
    
    GPUBatch batch = {};
    
    if (config_.async_loading && prefetch_active_.load(std::memory_order_acquire)) {
        // Get from prefetch queue
        std::unique_lock<std::mutex> lock(queue_mutex_);
        // B2-blocking_no_timeout: wait_for bounds the block so a stalled prefetch thread
        // cannot deadlock the consumer indefinitely.
        static constexpr std::chrono::seconds kPrefetchConsumeTimeout{5};
        if (!queue_cv_.wait_for(lock, kPrefetchConsumeTimeout, [this] {
                return !prefetch_queue_.empty() || stop_prefetch_.load(std::memory_order_acquire);
            })) {
            spdlog::warn("GPUDataLoader::getNextBatch: timed out waiting for prefetch queue after {} s; "
                         "falling back to synchronous load", kPrefetchConsumeTimeout.count());
            batch = prepareBatch(current_batch_);
        } else if (!prefetch_queue_.empty()) {
            batch = std::move(prefetch_queue_.front());
            prefetch_queue_.pop();
            queue_cv_.notify_one();  // Notify prefetch thread
        }
    } else {
        // Synchronous loading
        batch = prepareBatch(current_batch_);
    }
    
    current_batch_++;
    return batch;
}

bool GPUDataLoader::hasNext() const {
    return current_batch_ < num_batches();
}

void GPUDataLoader::reset() {
    current_batch_ = 0;
    
    // Reshuffle if enabled
    if (config_.shuffle && !indices_.empty()) {
        std::random_device rd = {};
        std::mt19937 gen(rd());
        std::shuffle(indices_.begin(), indices_.end(), gen);
    }
    
    // Restart prefetching
    if (config_.async_loading) {
        stopPrefetching();
        startPrefetching();
    }
}

size_t GPUDataLoader::num_batches() const {
    if (samples_.empty()) {
      return 0;
    }
    return (static_cast<int>(samples_.size()) + config_.batch_size - 1) / config_.batch_size;
}

GPUDataLoader::MemoryStats GPUDataLoader::get_memory_stats() const {
    MemoryStats stats;
    
    // Get stats from whichever allocator is in use
    VRAMAllocator* active_allocator = external_allocator_ ? external_allocator_ : allocator_.get();
    if (active_allocator) {
        auto allocator_stats = active_allocator->get_stats();
        stats.gpu_memory_bytes = allocator_stats.allocated_bytes;
    }
    
    // Estimate prefetch buffer size
    stats.prefetch_buffer_bytes = config_.prefetch_batches * config_.batch_size * 
                                  config_.max_sequence_length * sizeof(int);
    
    return stats;
}

void GPUDataLoader::startPrefetching() {
    if (prefetch_active_.load(std::memory_order_acquire)) {
        return;
    }
    
    stop_prefetch_.store(false, std::memory_order_release);
    prefetch_active_.store(true, std::memory_order_release);
    prefetch_thread_ = std::thread(&GPUDataLoader::prefetchWorker, this);
    
    spdlog::debug("Started prefetch thread");
}

void GPUDataLoader::stopPrefetching() {
    if (!prefetch_active_.load(std::memory_order_acquire)) {
        return;
    }
    
    stop_prefetch_.store(true, std::memory_order_release);
    queue_cv_.notify_all();
    
    if (prefetch_thread_.joinable()) {
        if (!themis::utils::joinThreadWithin(prefetch_thread_)) {
            spdlog::warn("Prefetch thread did not join within timeout, continuing shutdown");
        }
    }
    
    prefetch_active_.store(false);
    
    // Clear queue
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!prefetch_queue_.empty()) {
        prefetch_queue_.pop();
    }
    
    spdlog::debug("Stopped prefetch thread");
}

void GPUDataLoader::prefetchWorker() {
    size_t batch_idx = current_batch_;
    
    while (!stop_prefetch_.load(std::memory_order_acquire)) {
        // Check if we have room in the queue
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            // B2-blocking_no_timeout: wait_for prevents permanent stall if consumer thread dies.
            static constexpr std::chrono::seconds kPrefetchProduceTimeout{10};
            queue_cv_.wait_for(lock, kPrefetchProduceTimeout, [this, &batch_idx] {
                return prefetch_queue_.size() < config_.prefetch_batches ||
                       stop_prefetch_.load(std::memory_order_acquire);
            });
            
            if (stop_prefetch_.load(std::memory_order_acquire)) {
                break;
            }
        }
        
        // Check if we've reached the end
        if (batch_idx >= num_batches()) {
            break;
        }
        
        // Prepare batch
        GPUBatch batch = prepareBatch(batch_idx);
        
        // Add to queue
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            prefetch_queue_.push(std::move(batch));
            queue_cv_.notify_one();  // Notify consumer
        }
        
        batch_idx++;
    }
}

GPUBatch GPUDataLoader::prepareBatch([[maybe_unused]] size_t batch_idx) {
    GPUBatch batch;
    
    // Calculate batch bounds
    size_t start_idx = batch_idx * config_.batch_size;
    size_t end_idx = std::min(start_idx + config_.batch_size, samples_.size());
    size_t actual_batch_size = end_idx - start_idx;
    
    batch.batch_size = actual_batch_size;
    batch.seq_len = config_.max_sequence_length;
    
    // Tokenize samples
    std::vector<std::vector<int>> tokenized_samples;
    tokenized_samples.reserve(actual_batch_size);
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        size_t sample_idx = indices_[i];
        auto tokens = tokenizeSample(samples_[sample_idx]);
        tokenized_samples.push_back(std::move(tokens));
    }
    
    // Pad/truncate to max_sequence_length
    std::vector<float> input_ids_data;
    std::vector<float> attention_mask_data;
    std::vector<float> labels_data;
    
    size_t total_size = actual_batch_size * config_.max_sequence_length;
    input_ids_data.reserve(total_size);
    attention_mask_data.reserve(total_size);
    labels_data.reserve(total_size);
    
    for (size_t i = 0; i < actual_batch_size; ++i) {
        const auto& tokens = tokenized_samples[i];
        size_t token_count = std::min(tokens.size(), config_.max_sequence_length);
        
        // Copy tokens
        for (size_t j = 0; j < token_count; ++j) {
            input_ids_data.push_back(static_cast<float>(tokens[j]));
            attention_mask_data.push_back(1.0f);
            // Labels are shifted input for causal LM
            labels_data.push_back(j + 1 < tokens.size() ? 
                                 static_cast<float>(tokens[j + 1]) : 
                                 static_cast<float>(tokenizer_->eos_token_id()));
        }
        
        // Pad if needed
        if (config_.pad_to_max_length) {
            for (size_t j = token_count; j < config_.max_sequence_length; ++j) {
                input_ids_data.push_back(static_cast<float>(tokenizer_->pad_token_id()));
                attention_mask_data.push_back(0.0f);
                labels_data.push_back(static_cast<float>(tokenizer_->pad_token_id()));
            }
        }
    }
    
    // Create GPU tensors and upload data
    batch.input_ids = GPUTensor({actual_batch_size, config_.max_sequence_length}, config_.target_device);
    batch.attention_mask = GPUTensor({actual_batch_size, config_.max_sequence_length}, config_.target_device);
    batch.labels = GPUTensor({actual_batch_size, config_.max_sequence_length}, config_.target_device);
    
    batch.input_ids.upload(input_ids_data);
    batch.attention_mask.upload(attention_mask_data);
    batch.labels.upload(labels_data);
    
    return batch;
}

std::vector<int> GPUDataLoader::tokenizeSample(const InstructionDataSample& sample) {
    // Format sample as instruction-following template
    std::string formatted = "### Instruction:\n" + sample.instruction;
    
    if (!sample.input.empty()) {
        formatted += "\n\n### Input:\n" + sample.input;
    }
    
    formatted += "\n\n### Response:\n" + sample.output;
    
    // Tokenize
    return tokenizer_->encode(formatted);
}

bool GPUDataLoader::updateBatchSize([[maybe_unused]] size_t new_batch_size) {
    if (new_batch_size == 0) {
        spdlog::warn("Cannot update batch size to 0");
        return false;
    }
    
    if (new_batch_size == config_.batch_size) {
        // No change needed
        return true;
    }
    
    spdlog::info("Updating batch size from {} to {}", config_.batch_size, new_batch_size);
    
    // Update configuration
    config_.batch_size = new_batch_size;
    
    // Note: The current batch index (current_batch_) is preserved to maintain
    // training progress through the dataset. The new batch size takes effect
    // on the next getNextBatch() call, which will generate batches using the
    // updated size from the current position onward.
    
    return true;
}

} // namespace lora
} // namespace llm
} // namespace themis

