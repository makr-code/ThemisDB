#include "llm/lora_framework/gpu_training_loop.h"
#include "llm/lora_framework/cuda_kernels.h"
#include "llm/lora_framework/hip_kernels.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

GPUTrainingLoop::GPUTrainingLoop(const GPUTrainingConfig& config)
    : config_(config) {
    
    spdlog::info("GPUTrainingLoop initialized:");
    spdlog::info("  Epochs: {}", config_.num_epochs);
    spdlog::info("  Learning rate: {}", config_.learning_rate);
    spdlog::info("  Device: {}", static_cast<int>(config_.device.type));
    spdlog::info("  Mixed precision: {}", config_.use_mixed_precision);
    spdlog::info("  Multi-GPU: {}", config_.use_multi_gpu);
    
    initializeMemoryManagement();
}

GPUTrainingLoop::~GPUTrainingLoop() {
    stop();
}

GPUTrainingLoop::GPUTrainingLoop(GPUTrainingLoop&& other) noexcept
    : config_(other.config_)
    , data_loader_(std::move(other.data_loader_))
    , layers_(std::move(other.layers_))
    , multi_gpu_layer_(other.multi_gpu_layer_)
    , optimizer_(std::move(other.optimizer_))
    , mixed_precision_trainer_(other.mixed_precision_trainer_)
    , vram_allocator_(std::move(other.vram_allocator_))
    , gpu_memory_manager_(other.gpu_memory_manager_)
    , current_metrics_(other.current_metrics_)
    , callback_(other.callback_)
    , final_loss_(other.final_loss_)
{
    is_training_.store(other.is_training_.load());
    stop_requested_.store(other.stop_requested_.load());
    
    other.multi_gpu_layer_ = nullptr;
    other.mixed_precision_trainer_ = nullptr;
    other.gpu_memory_manager_ = nullptr;
}

GPUTrainingLoop& GPUTrainingLoop::operator=(GPUTrainingLoop&& other) noexcept {
    if (this != &other) {
        stop();
        
        config_ = other.config_;
        data_loader_ = std::move(other.data_loader_);
        layers_ = std::move(other.layers_);
        multi_gpu_layer_ = other.multi_gpu_layer_;
        optimizer_ = std::move(other.optimizer_);
        mixed_precision_trainer_ = other.mixed_precision_trainer_;
        vram_allocator_ = std::move(other.vram_allocator_);
        gpu_memory_manager_ = other.gpu_memory_manager_;
        current_metrics_ = other.current_metrics_;
        callback_ = other.callback_;
        final_loss_ = other.final_loss_;
        
        is_training_.store(other.is_training_.load());
        stop_requested_.store(other.stop_requested_.load());
        
        other.multi_gpu_layer_ = nullptr;
        other.mixed_precision_trainer_ = nullptr;
        other.gpu_memory_manager_ = nullptr;
    }
    return *this;
}

void GPUTrainingLoop::setDataLoader(std::unique_ptr<GPUDataLoader> loader) {
    data_loader_ = std::move(loader);
    spdlog::info("Data loader set: {} samples, {} batches", 
                 data_loader_->size(), data_loader_->num_batches());
}

void GPUTrainingLoop::addLayer(GPULoRALayer* layer) {
    if (layer) {
        layers_.push_back(layer);
        spdlog::debug("Added GPU LoRA layer: {} parameters", layer->parameter_count());
    }
}

void GPUTrainingLoop::setMultiGPULayer(MultiGPULoRALayer* layer) {
    multi_gpu_layer_ = layer;
    spdlog::info("Multi-GPU layer set: {} GPUs", layer ? layer->num_gpus() : 0);
}

void GPUTrainingLoop::setMixedPrecisionTrainer(MixedPrecisionTrainer* trainer) {
    mixed_precision_trainer_ = trainer;
    spdlog::info("Mixed precision trainer set: enabled={}", 
                 trainer ? trainer->is_enabled() : false);
}

void GPUTrainingLoop::registerCallback(GPUTrainingCallback callback) {
    callback_ = callback;
}

bool GPUTrainingLoop::train() {
    if (is_training_.load()) {
        spdlog::warn("Training already in progress");
        return false;
    }
    
    if (!data_loader_) {
        spdlog::error("No data loader set");
        return false;
    }
    
    if (layers_.empty() && !multi_gpu_layer_) {
        spdlog::error("No layers to train");
        return false;
    }
    
    is_training_.store(true);
    stop_requested_.store(false);
    current_metrics_.status = "training";
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Initialize optimizer
        initializeOptimizer();
        
        // Setup metrics
        current_metrics_.total_epochs = config_.num_epochs;
        current_metrics_.total_steps = config_.num_epochs * data_loader_->num_batches();
        current_metrics_.learning_rate = config_.learning_rate;
        
        spdlog::info("Starting GPU training:");
        spdlog::info("  Total epochs: {}", current_metrics_.total_epochs);
        spdlog::info("  Total steps: {}", current_metrics_.total_steps);
        spdlog::info("  Batch size: {}", data_loader_->config().batch_size);
        
        // Training loop
        float total_loss = 0.0f;
        int total_steps = 0;
        
        for (int epoch = 0; epoch < config_.num_epochs; ++epoch) {
            if (stop_requested_.load()) {
                spdlog::info("Training stopped at epoch {}/{}", epoch + 1, config_.num_epochs);
                break;
            }
            
            float epoch_loss = trainEpoch(epoch);
            total_loss += epoch_loss;
            total_steps += data_loader_->num_batches();
            
            spdlog::info("Epoch {}/{} completed, avg loss: {:.6f}", 
                        epoch + 1, config_.num_epochs, epoch_loss);
        }
        
        final_loss_ = total_loss / std::max(1, total_steps);
        current_metrics_.status = "completed";
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        spdlog::info("Training completed:");
        spdlog::info("  Final loss: {:.6f}", final_loss_);
        spdlog::info("  Duration: {}s", duration.count());
        
        is_training_.store(false);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Training failed: {}", e.what());
        current_metrics_.status = "failed";
        is_training_.store(false);
        return false;
    }
}

void GPUTrainingLoop::stop() {
    if (!is_training_.load()) {
        return;
    }
    
    spdlog::info("Stopping training...");
    stop_requested_.store(true);
    
    // Wait for training to complete
    while (is_training_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    spdlog::info("Training stopped");
}

GPUTrainingMetrics GPUTrainingLoop::getMetrics() const {
    GPUTrainingMetrics metrics = current_metrics_;
    
    // Update GPU memory info
    if (gpu_memory_manager_) {
        metrics.gpu_memory_used = gpu_memory_manager_->getTotalVRAM();
        metrics.gpu_memory_available = gpu_memory_manager_->getFreeVRAM();
    } else if (vram_allocator_) {
        auto stats = vram_allocator_->get_stats();
        metrics.gpu_memory_used = stats.allocated_bytes;
        metrics.gpu_memory_available = stats.free_bytes;
    }
    
    return metrics;
}

void GPUTrainingLoop::initializeOptimizer() {
    // Collect parameters from all layers
    std::vector<GPUTensor*> params;
    
    if (multi_gpu_layer_) {
        // Multi-GPU: collect from first GPU (parameters are synchronized)
        auto& layer = multi_gpu_layer_->get_layer(0);
        auto layer_params = layer.parameters();
        params.insert(params.end(), layer_params.begin(), layer_params.end());
    } else {
        // Single-GPU: collect from all layers
        for (auto* layer : layers_) {
            auto layer_params = layer->parameters();
            params.insert(params.end(), layer_params.begin(), layer_params.end());
        }
    }
    
    optimizer_ = std::make_unique<GPUSGDOptimizer>(
        config_.learning_rate,
        config_.momentum,
        config_.weight_decay
    );
    
    optimizer_->add_parameters(params);
    
    spdlog::info("Optimizer initialized with {} parameters", params.size());
}

void GPUTrainingLoop::initializeMemoryManagement() {
    // Create VRAM allocator
    acceleration::BackendType backend = acceleration::BackendType::CUDA;
    if (config_.device.type == DeviceType::HIP) {
        backend = acceleration::BackendType::HIP;
    } else if (config_.device.type == DeviceType::VULKAN) {
        backend = acceleration::BackendType::VULKAN;
    } else if (config_.device.type == DeviceType::DIRECTX) {
        backend = acceleration::BackendType::DIRECTX;
    }
    
    size_t pool_size = config_.max_vram_bytes;
    vram_allocator_ = std::make_unique<VRAMAllocator>(backend, pool_size);
    
    if (vram_allocator_->is_available()) {
        auto stats = vram_allocator_->get_stats();
        spdlog::info("VRAM allocator initialized:");
        spdlog::info("  Total VRAM: {:.2f} GB", stats.total_bytes / (1024.0 * 1024.0 * 1024.0));
        spdlog::info("  Free VRAM: {:.2f} GB", stats.free_bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

float GPUTrainingLoop::trainEpoch(int epoch) {
    current_metrics_.current_epoch = epoch + 1;
    
    data_loader_->reset();
    
    float epoch_loss = 0.0f;
    int step = 0;
    
    while (data_loader_->hasNext()) {
        if (stop_requested_.load()) {
            break;
        }
        
        auto batch = data_loader_->getNextBatch();
        
        if (!batch.is_valid()) {
            spdlog::warn("Invalid batch at step {}", step);
            continue;
        }
        
        float batch_loss = trainStep(batch);
        epoch_loss += batch_loss;
        
        int global_step = epoch * data_loader_->num_batches() + step;
        updateMetrics(epoch, global_step, batch_loss);
        
        if (step % 10 == 0) {
            spdlog::debug("Epoch {}/{}, Step {}/{}, Loss: {:.6f}",
                         epoch + 1, config_.num_epochs,
                         step + 1, data_loader_->num_batches(),
                         batch_loss);
        }
        
        step++;
    }
    
    return epoch_loss / std::max(1, step);
}

float GPUTrainingLoop::trainStep(const GPUBatch& batch) {
    // Create embeddings from token IDs
    size_t hidden_dim = 768;  // Standard transformer dimension
    GPUTensor input_embeddings = createEmbeddingsOnGPU(
        batch.input_ids, hidden_dim, config_.device
    );
    GPUTensor target_embeddings = createEmbeddingsOnGPU(
        batch.labels, hidden_dim, config_.device
    );
    
    // Apply mixed precision if enabled
    if (mixed_precision_trainer_ && mixed_precision_trainer_->is_enabled()) {
        // Convert to lower precision (done internally by GPUTensor with appropriate dtype)
        // For now, we keep FP32 and let mixed_precision_trainer_ handle scaling
    }
    
    // Zero gradients
    if (multi_gpu_layer_) {
        multi_gpu_layer_->zero_grad();
    } else {
        for (auto* layer : layers_) {
            layer->zero_grad();
        }
    }
    
    // Forward pass
    GPUTensor predictions;
    if (multi_gpu_layer_) {
        // Multi-GPU forward (data parallel)
        // Split batch across GPUs
        std::vector<GPUTensor> inputs = {input_embeddings};  // Simplified for single GPU case
        auto outputs = multi_gpu_layer_->forward(inputs);
        predictions = std::move(outputs[0]);
    } else {
        // Single-GPU forward
        predictions = layers_[0]->forward(input_embeddings);
    }
    
    // Compute loss
    float loss = computeMSELossGPU(predictions, target_embeddings);
    
    // Scale loss for mixed precision
    if (mixed_precision_trainer_ && mixed_precision_trainer_->is_enabled()) {
        loss = mixed_precision_trainer_->scale_loss(loss);
    }
    
    // Backward pass
    GPUTensor grad_output = computeMSEGradientGPU(predictions, target_embeddings);
    
    if (multi_gpu_layer_) {
        // Multi-GPU backward
        std::vector<GPUTensor> grad_outputs = {grad_output};
        multi_gpu_layer_->backward(grad_outputs);
        
        // Synchronize gradients across GPUs
        multi_gpu_layer_->synchronize_gradients();
    } else {
        // Single-GPU backward
        layers_[0]->backward(grad_output);
    }
    
    // Unscale gradients if mixed precision
    bool should_step = true;
    if (mixed_precision_trainer_ && mixed_precision_trainer_->is_enabled()) {
        // TODO: Implement proper gradient unscaling for GPU tensors
        // Current limitation: MixedPrecisionTrainer::unscale_gradients expects std::vector<Tensor*>
        // but we have std::vector<GPUTensor*>. Need to either:
        // 1. Create an adapter/wrapper to convert GPUTensor* to Tensor*
        // 2. Add a GPU-specific unscale_gradients method to MixedPrecisionTrainer
        // 3. Implement gradient unscaling directly in GPU training loop
        // For now, we skip unscaling which may lead to gradient overflow in FP16 mode.
        // This is acceptable for initial implementation but should be fixed for production.
        
        spdlog::debug("Mixed precision gradient unscaling skipped (not yet implemented for GPU tensors)");
        should_step = true;  // Proceed with optimizer step despite skipped unscaling
    }
    
    // Optimizer step
    if (should_step) {
        optimizer_->step();
    }
    
    // Check memory usage periodically
    if (current_metrics_.current_step % 100 == 0) {
        checkMemoryUsage();
    }
    
    return loss;
}

void GPUTrainingLoop::updateMetrics(int epoch, int step, float loss) {
    current_metrics_.current_epoch = epoch + 1;
    current_metrics_.current_step = step;
    current_metrics_.current_loss = loss;
    current_metrics_.progress = static_cast<float>(step) / 
                                static_cast<float>(current_metrics_.total_steps);
    
    if (callback_) {
        callback_(current_metrics_);
    }
}

void GPUTrainingLoop::checkMemoryUsage() {
    if (vram_allocator_) {
        auto stats = vram_allocator_->get_stats();
        float usage_pct = 100.0f * stats.allocated_bytes / std::max(stats.total_bytes, size_t(1));
        
        if (usage_pct > 90.0f) {
            spdlog::warn("High VRAM usage: {:.1f}% ({:.2f} GB / {:.2f} GB)",
                        usage_pct,
                        stats.allocated_bytes / (1024.0 * 1024.0 * 1024.0),
                        stats.total_bytes / (1024.0 * 1024.0 * 1024.0));
        }
    }
}

// Helper functions

GPUTensor createEmbeddingsOnGPU(
    const GPUTensor& token_ids,
    size_t hidden_dim,
    const Device& device
) {
    // Get batch size and sequence length from token_ids shape
    auto shape = token_ids.shape();
    if (shape.size() < 2) {
        throw std::invalid_argument("token_ids must be at least 2D (batch_size, seq_len)");
    }
    
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    
    // Create embedding tensor
    GPUTensor embeddings({batch_size, hidden_dim}, device);
    
    // TODO: Replace with actual embedding lookup from base model
    // Current implementation uses hash-based embeddings as a placeholder.
    // In production, this should:
    // 1. Look up token embeddings from the base model's embedding layer
    // 2. Perform embedding lookup directly on GPU (no CPU transfer)
    // 3. Handle different model architectures (Llama, GPT, etc.)
    // 
    // Hash-based embeddings are only suitable for testing the training loop mechanics,
    // but will produce poor training quality. This is a known limitation of the
    // initial implementation and should be addressed before production use.
    
    auto token_data = token_ids.cpu_data();
    std::vector<float> embedding_data(batch_size * hidden_dim);
    
    for (size_t i = 0; i < batch_size; ++i) {
        for (size_t j = 0; j < hidden_dim; ++j) {
            size_t token_idx = j % seq_len;
            int token_id = static_cast<int>(token_data[i * seq_len + token_idx]);
            embedding_data[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
        }
    }
    
    embeddings.upload(embedding_data);
    return embeddings;
}

float computeMSELossGPU(const GPUTensor& predictions, const GPUTensor& targets) {
    if (predictions.shape() != targets.shape()) {
        throw std::invalid_argument("Predictions and targets must have same shape");
    }
    
    size_t n = predictions.size();
    const Device& device = predictions.device();
    
    // Use GPU kernels for CUDA and HIP backends
    if (device.type == DeviceType::CUDA || device.type == DeviceType::HIP) {
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
        // Step 1: Parallel reduction on GPU
        int threads = 256;
        int blocks = std::min(1024, static_cast<int>((n + threads - 1) / threads));
        
        // Allocate temporary buffer for partial sums
        GPUTensor partial_sums({static_cast<size_t>(blocks)}, device);
        
        // Launch kernel for parallel reduction
#ifdef THEMIS_ENABLE_CUDA
        if (device.type == DeviceType::CUDA) {
            cuda::launch_mse_loss_reduction_kernel(
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                static_cast<float*>(partial_sums.gpu_ptr()),
                static_cast<int>(n),
                blocks,
                nullptr  // use default stream
            );
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device.type == DeviceType::HIP) {
            hip::launch_mse_loss_reduction_kernel(
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                static_cast<float*>(partial_sums.gpu_ptr()),
                static_cast<int>(n),
                blocks,
                nullptr  // use default stream
            );
        }
#endif
        
        // Step 2: Final reduction on CPU (small array, ~4KB transfer)
        auto partial_data = partial_sums.cpu_data();
        float sum = 0.0f;
        for (float val : partial_data) {
            sum += val;
        }
        
        return sum / n;
#else
        // Fallback to CPU if GPU not compiled
        auto pred_data = predictions.cpu_data();
        auto target_data = targets.cpu_data();
        
        float sum = 0.0f;
        for (size_t i = 0; i < pred_data.size(); ++i) {
            float diff = pred_data[i] - target_data[i];
            sum += diff * diff;
        }
        
        return sum / pred_data.size();
#endif
    } else {
        // CPU fallback or other backends
        auto pred_data = predictions.cpu_data();
        auto target_data = targets.cpu_data();
        
        float sum = 0.0f;
        for (size_t i = 0; i < pred_data.size(); ++i) {
            float diff = pred_data[i] - target_data[i];
            sum += diff * diff;
        }
        
        return sum / pred_data.size();
    }
}

GPUTensor computeMSEGradientGPU(const GPUTensor& predictions, const GPUTensor& targets) {
    if (predictions.shape() != targets.shape()) {
        throw std::invalid_argument("Predictions and targets must have same shape");
    }
    
    // Create gradient tensor on same device
    GPUTensor grad(predictions.shape(), predictions.device());
    
    size_t n = predictions.size();
    float scale = 2.0f / n;
    const Device& device = predictions.device();
    
    // Use GPU kernels for CUDA and HIP backends
    if (device.type == DeviceType::CUDA || device.type == DeviceType::HIP) {
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
        // All computation on GPU, no CPU transfers!
#ifdef THEMIS_ENABLE_CUDA
        if (device.type == DeviceType::CUDA) {
            cuda::launch_mse_gradient_kernel(
                static_cast<float*>(grad.gpu_ptr()),
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                scale,
                static_cast<int>(n),
                nullptr  // use default stream
            );
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device.type == DeviceType::HIP) {
            hip::launch_mse_gradient_kernel(
                static_cast<float*>(grad.gpu_ptr()),
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                scale,
                static_cast<int>(n),
                nullptr  // use default stream
            );
        }
#endif
        
        return grad;
#else
        // Fallback to CPU if GPU not compiled
        auto pred_data = predictions.cpu_data();
        auto target_data = targets.cpu_data();
        
        std::vector<float> grad_data(pred_data.size());
        
        for (size_t i = 0; i < pred_data.size(); ++i) {
            grad_data[i] = scale * (pred_data[i] - target_data[i]);
        }
        
        grad.upload(grad_data);
        return grad;
#endif
    } else {
        // CPU fallback or other backends
        auto pred_data = predictions.cpu_data();
        auto target_data = targets.cpu_data();
        
        std::vector<float> grad_data(pred_data.size());
        
        for (size_t i = 0; i < pred_data.size(); ++i) {
            grad_data[i] = scale * (pred_data[i] - target_data[i]);
        }
        
        grad.upload(grad_data);
        return grad;
    }
}

} // namespace lora
} // namespace llm
} // namespace themis
