/**
 * @file gpu_training_loop.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=8, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gpu_training_loop.h"
#include "llm/lora_framework/base_model_adapter.h"
#include "llm/lora_framework/cuda_kernels.h"
#include "llm/lora_framework/hip_kernels.h"
#include "llm/lora_framework/vulkan_kernels.h"
#include "llm/lora_framework/directx_kernels.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <limits>
#include <memory>
#include <vector>
#include <thread>

namespace themis {
namespace llm {
namespace lora {

// Default hidden dimension for transformer models (used as fallback)
constexpr size_t DEFAULT_HIDDEN_DIM = 768;

GPUTrainingLoop::GPUTrainingLoop(const GPUTrainingConfig& config)
    : config_(config) {
    
    spdlog::info("GPUTrainingLoop initialized:");
    spdlog::info("  Epochs: {}", config_.num_epochs);
    spdlog::info("  Learning rate: {}", config_.learning_rate);
    spdlog::info("  Device: {}", static_cast<int>(config_.device.type));
    spdlog::info("  Mixed precision: {}", config_.use_mixed_precision);
    spdlog::info("  Multi-GPU: {}", config_.use_multi_gpu);
    spdlog::info("  Adaptive batching: {}", config_.enable_adaptive_batching);
    
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
    is_training_.store(other.is_training_.load(std::memory_order_acquire), std::memory_order_release);
    stop_requested_.store(other.stop_requested_.load(std::memory_order_acquire), std::memory_order_release);
    
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
        
        is_training_.store(other.is_training_.load(std::memory_order_acquire), std::memory_order_release);
        stop_requested_.store(other.stop_requested_.load(std::memory_order_acquire), std::memory_order_release);
        
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

void GPUTrainingLoop::setBaseModel(const BaseModelAdapter* base_model) {
    base_model_ = base_model;
    
    if (base_model_ && base_model_->isLoaded()) {
        // Create GPU embedding layer from base model
        const float* embedding_matrix = base_model_->getEmbeddingMatrix();
        
        if (embedding_matrix) {
            size_t vocab_size = base_model_->getVocabSize();
            size_t hidden_dim = base_model_->getHiddenSize();
            
            spdlog::info("Creating GPU embedding layer from base model:");
            spdlog::info("  Vocab size: {}", vocab_size);
            spdlog::info("  Hidden dim: {}", hidden_dim);
            
            gpu_embedding_layer_ = std::make_unique<GPUEmbeddingLayer>(
                embedding_matrix,
                vocab_size,
                hidden_dim,
                config_.device
            );
            
            spdlog::info("GPU embedding layer created successfully");
        } else {
            spdlog::warn("Base model loaded but embedding matrix not available");
        }
    } else {
        spdlog::info("No base model set, will use hash-based embeddings");
    }
}

bool GPUTrainingLoop::train() {
    if (is_training_.load(std::memory_order_acquire)) {
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
        
        // Initialize adaptive batching (NEW)
        initializeAdaptiveBatching();
        // Initialize checkpointing if enabled
        initializeCheckpointing();
        
        // Setup metrics
        const size_t batches_per_epoch = data_loader_->num_batches();
        const size_t total_steps_raw =
            static_cast<size_t>(std::max(0, config_.num_epochs)) * batches_per_epoch;
        current_metrics_.total_epochs = config_.num_epochs;
        current_metrics_.total_steps =
            (total_steps_raw > static_cast<size_t>(std::numeric_limits<int>::max()))
                ? std::numeric_limits<int>::max()
                : static_cast<int>(total_steps_raw);
        current_metrics_.learning_rate = config_.learning_rate;
        
        spdlog::info("Starting GPU training:");
        spdlog::info("  Total epochs: {}", current_metrics_.total_epochs);
        spdlog::info("  Total steps: {}", current_metrics_.total_steps);
        spdlog::info("  Batch size: {}", data_loader_->config().batch_size);
        
        // Training loop
        float total_loss = 0.0f;
        size_t total_steps = 0;
        
        for (int epoch = 0; epoch < config_.num_epochs; ++epoch) {
            if (stop_requested_.load(std::memory_order_acquire)) {
                spdlog::info("Training stopped at epoch {}/{}", epoch + 1, config_.num_epochs);
                break;
            }
            
            float epoch_loss = trainEpoch(epoch);
            total_loss += epoch_loss;
            total_steps += batches_per_epoch;
            
            spdlog::info("Epoch {}/{} completed, avg loss: {:.6f}", 
                        epoch + 1, config_.num_epochs, epoch_loss);
        }
        
        const size_t safe_total_steps = std::max<size_t>(1, total_steps);
        final_loss_ = total_loss / static_cast<float>(safe_total_steps);
        current_metrics_.status = "completed";
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        spdlog::info("Training completed:");
        spdlog::info("  Final loss: {:.6f}", final_loss_);
        spdlog::info("  Duration: {}s", duration.count());
        
        is_training_.store(false, std::memory_order_release);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Training failed: {}", e.what());
        current_metrics_.status = "failed";
        is_training_.store(false, std::memory_order_release);
        return false;
    }
}

void GPUTrainingLoop::stop() {
    if (!is_training_.load(std::memory_order_acquire)) {
        return;
    }
    
    spdlog::info("Stopping training...");
    stop_requested_.store(true, std::memory_order_release);
    
    // Wait for training to complete
    while (is_training_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    spdlog::info("Training stopped");
}

GPUTrainingMetrics GPUTrainingLoop::getMetrics() const {
    GPUTrainingMetrics metrics = current_metrics_;
    
    // Update GPU memory info
    if (vram_allocator_) {
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
    
    spdlog::info("Optimizer initialized with {} parameters",static_cast<int>(params.size()));
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

void GPUTrainingLoop::initializeAdaptiveBatching() {
    if (!config_.enable_adaptive_batching) {
        return;
    }
    
    AdaptiveBatcher::Config batcher_config;
    batcher_config.min_batch_size = config_.min_batch_size;
    batcher_config.max_batch_size = config_.max_batch_size;
    batcher_config.target_vram_utilization_pct = 85;
    batcher_config.enable_dynamic_batching = true;
    
    adaptive_batcher_ = std::make_unique<AdaptiveBatcher>(
        batcher_config, gpu_memory_manager_
    );
    
    gpu_monitor_ = std::make_unique<GPUUtilizationMonitor>(config_.device);
    
    spdlog::info("Adaptive batching enabled:");
    spdlog::info("  Batch size range: [{}, {}]",
                 batcher_config.min_batch_size,
                 batcher_config.max_batch_size);
    spdlog::info("  Target VRAM utilization: {}%", 
                 batcher_config.target_vram_utilization_pct);
}

void GPUTrainingLoop::initializeCheckpointing() {
    if (!config_.enable_gradient_checkpointing) {
        spdlog::info("Gradient checkpointing disabled");
        return;
    }
    
    // Count total layers
    int total_layers = static_cast<int>(layers_.size());
    if (multi_gpu_layer_) {
        total_layers = 1;  // Multi-GPU layer counts as one
    }
    
    if (total_layers == 0) {
        spdlog::warn("No layers to checkpoint, disabling gradient checkpointing");
        config_.enable_gradient_checkpointing = false;
        return;
    }
    
    // Create checkpoint configuration
    CheckpointConfig checkpoint_config;
    checkpoint_config.strategy = config_.checkpoint_strategy;
    checkpoint_config.checkpoint_frequency = config_.checkpoint_frequency;
    checkpoint_config.total_layers = total_layers;
    checkpoint_config.checkpoint_lora = true;
    
    checkpointer_ = std::make_unique<GradientCheckpointer>(checkpoint_config);
    
    // Apply checkpointing to layers
    int checkpointed_count = 0;
    for (size_t i = 0; i < layers_.size(); ++i) {
        if (checkpointer_->shouldCheckpoint(static_cast<int>(i))) {
            layers_[i]->set_checkpointing(true);
            layers_[i]->set_layer_id(static_cast<int>(i));
            checkpointer_->setLayerType(static_cast<int>(i), LayerType::LORA);
            checkpointed_count++;
            spdlog::debug("Enabled checkpointing for layer {}", i);
        }
    }
    
    // Estimate memory savings
    size_t avg_activation_size = 4 * 1024 * 1024;  // 4MB per layer estimate
    size_t estimated_savings = checkpointer_->estimateMemorySavings(avg_activation_size);
    float compute_overhead = checkpointer_->estimateComputeOverhead();
    
    spdlog::info("Gradient checkpointing initialized:");
    spdlog::info("  Strategy: {}", static_cast<int>(config_.checkpoint_strategy));
    spdlog::info("  Total layers: {}", total_layers);
    spdlog::info("  Checkpointed layers: {}", checkpointed_count);
    spdlog::info("  Estimated memory savings: {:.2f} GB", 
                 estimated_savings / (1024.0 * 1024.0 * 1024.0));
    spdlog::info("  Estimated compute overhead: {:.1f}%", compute_overhead);
}

float GPUTrainingLoop::trainEpoch(int epoch) {
    current_metrics_.current_epoch = epoch + 1;
    
    data_loader_->reset();
    
    float epoch_loss = 0.0f;
    size_t step = 0;
    const size_t batches_per_epoch = data_loader_->num_batches();
    const size_t max_int = static_cast<size_t>(std::numeric_limits<int>::max());
    
    while (data_loader_->hasNext()) {
        if (stop_requested_.load(std::memory_order_acquire)) {
            break;
        }
        
        // Adjust batch size dynamically (NEW - now functional!)
        if (adaptive_batcher_ && step % 10 == 0) {
            size_t optimal_batch = adaptive_batcher_->computeOptimalBatchSize(
                data_loader_->config().max_sequence_length
            );
            
            // Update batch size dynamically if different from current
            if (optimal_batch != data_loader_->config().batch_size) {
                if (data_loader_->updateBatchSize(optimal_batch)) {
                    spdlog::info("Dynamically adjusted batch size to {}", optimal_batch);
                } else {
                    spdlog::debug("Optimal batch size: {} (current: {})", 
                                 optimal_batch, data_loader_->config().batch_size);
                }
            }
            
            // Check for underutilization every 50 steps
            if (step % 50 == 0 && gpu_monitor_ && gpu_monitor_->isAvailable()) {
                auto metrics = gpu_monitor_->queryMetrics();
                adaptive_batcher_->updateUtilization(metrics.gpu_utilization_pct / 100.0f);
                
                if (gpu_monitor_->isUnderutilized()) {
                    auto recommendations = gpu_monitor_->getOptimizationRecommendations();
                    for (const auto& rec : recommendations) {
                        spdlog::info("Optimization: {}", rec);
                    }
                    
                    adaptive_batcher_->increaseBatchSizeIfPossible();
                }
            }
        }
        
        try {
            auto batch = data_loader_->getNextBatch();
            
            if (!batch.is_valid()) {
                spdlog::warn("Invalid batch at step {}", step);
                continue;
            }
            
            float batch_loss = trainStep(batch);
            epoch_loss += batch_loss;
            
            const size_t global_step_raw =
                static_cast<size_t>(std::max(0, epoch)) * batches_per_epoch + step;
            const int global_step = static_cast<int>(std::min(global_step_raw, max_int));
            updateMetrics(epoch, global_step, batch_loss);
            
            if (step % 10 == 0) {
                spdlog::debug("Epoch {}/{}, Step {}/{}, Loss: {:.6f}",
                             epoch + 1, config_.num_epochs,
                             step + 1, batches_per_epoch,
                             batch_loss);
            }
            
        } catch (const std::bad_alloc&) {
            // Handle OOM (NEW)
            if (adaptive_batcher_) {
                adaptive_batcher_->handleOOMEvent();
                spdlog::warn("OOM handled, retrying with batch size: {}", 
                            adaptive_batcher_->getCurrentBatchSize());
                continue;  // Retry with smaller batch
            } else {
                throw;  // Re-throw if no adaptive batching
            }
        } catch (const std::runtime_error& e) {
            // Check for CUDA/HIP OOM errors
            std::string error_msg = e.what();
            if (error_msg.find("out of memory") != std::string::npos ||
                error_msg.find("OOM") != std::string::npos) {
                if (adaptive_batcher_) {
                    adaptive_batcher_->handleOOMEvent();
                    spdlog::warn("OOM handled ({}), retrying with batch size: {}", 
                                error_msg, adaptive_batcher_->getCurrentBatchSize());
                    continue;
                }
            }
            throw;  // Re-throw other errors
        }
        
        ++step;
    }
    
    return epoch_loss / static_cast<float>(std::max<size_t>(1, step));
}

float GPUTrainingLoop::trainStep(const GPUBatch& batch) {
    // Create embeddings from token IDs
    // Use embedding layer's dimension if available, otherwise use default
    size_t hidden_dim = gpu_embedding_layer_ ? gpu_embedding_layer_->hidden_dim() : DEFAULT_HIDDEN_DIM;
    
    GPUTensor input_embeddings = createEmbeddingsOnGPU(
        batch.input_ids, hidden_dim, config_.device, gpu_embedding_layer_.get()
    );
    GPUTensor target_embeddings = createEmbeddingsOnGPU(
        batch.labels, hidden_dim, config_.device, gpu_embedding_layer_.get()
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
    // PERMANENT FALLBACK NOTE (GPUTensor multi-GPU dispatch):
    // Under THEMIS_HAS_GPU_TENSOR_SMART_PTR, std::shared_ptr<GPUTensor> ownership
    // is used to forward the split input through multi_gpu_layer_->forward() with
    // move-only semantics — the shared_ptr wrapper gives reference-counted lifetime
    // without requiring GPUTensor copy construction.
    // Fallback (default — THEMIS_HAS_GPU_TENSOR_SMART_PTR not set): single-GPU
    // path only; all training runs on layers_[0] regardless of multi_gpu_layer_.
#ifdef THEMIS_HAS_GPU_TENSOR_SMART_PTR
    if (multi_gpu_layer_ && multi_gpu_layer_->num_gpus() > 0) {
        // Wrap the input embedding in a shared_ptr for move-only dispatch.
        // multi_gpu_layer_->forward() accepts a shared_ptr<GPUTensor> to allow
        // reference-counted fanout across GPU devices without copying the tensor.
        auto input_ptr = std::make_shared<GPUTensor>(std::move(input_embeddings));
        predictions = multi_gpu_layer_->forward(input_ptr);
    } else {
        predictions = std::move(layers_[0]->forward(input_embeddings));
    }
#else
    // PERMANENT FALLBACK NOTE: single-GPU forward path.
    predictions = std::move(layers_[0]->forward(input_embeddings));
#endif // THEMIS_HAS_GPU_TENSOR_SMART_PTR
    
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
        std::vector<GPUTensor> grad_outputs;
        grad_outputs.reserve(1);
        grad_outputs.push_back(std::move(grad_output));
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
        // GPU-native gradient unscaling (no CPU transfers)
        std::vector<GPUTensor*> gradients;
        
        // Get gradients from layers
        if (multi_gpu_layer_) {
            gradients = multi_gpu_layer_->get_layer(0).gradients();
        } else {
            gradients = layers_[0]->gradients();
        }
        
        // Check for overflow before unscaling
        bool has_overflow = false;
        for (auto* grad : gradients) {
            if (grad && grad->has_inf_or_nan()) {
                has_overflow = true;
                spdlog::warn("Gradient overflow detected in mixed precision training");
                break;
            }
        }
        
        if (!has_overflow) {
            // Unscale gradients on GPU
            float inv_scale = 1.0f / mixed_precision_trainer_->get_loss_scale();
            for (auto* grad : gradients) {
                if (grad && grad->size() > 0) {
                    grad->multiply_inplace(inv_scale);
                }
            }
            spdlog::debug("GPU gradient unscaling completed (scale: {})", 
                         mixed_precision_trainer_->get_loss_scale());
        } else {
            should_step = false;
        }
        
        // Update loss scale based on overflow
        mixed_precision_trainer_->update_loss_scale(has_overflow);
    }
    
    // Optimizer step
    if (should_step) {
        optimizer_->step();
    }
    
    // Calibrate memory estimation periodically (every 100 steps)
    if (adaptive_batcher_ && current_metrics_.current_step % 100 == 0) {
        if (vram_allocator_) {
            auto stats = vram_allocator_->get_stats();
            size_t used_vram = stats.allocated_bytes;
            
            // Calibrate based on actual memory usage
            adaptive_batcher_->calibrateMemoryEstimation(
                used_vram,
                batch.seq_len,
                batch.batch_size
            );
        }
    }
    
    // Check memory usage periodically
    if (current_metrics_.current_step % 100 == 0) {
        checkMemoryUsage();
        
        // Log checkpoint statistics if checkpointing is enabled
        if (checkpointer_) {
            auto checkpoint_stats = checkpointer_->getStats();
            spdlog::info("Checkpoint stats: {:.1f}% memory reduction, {:.1f}% compute overhead, {}ms recompute time",
                        checkpoint_stats.memory_reduction_pct, 
                        checkpoint_stats.compute_overhead_pct,
                        checkpoint_stats.recomputation_time_ms);
        }
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
    const Device& device,
    GPUEmbeddingLayer* embedding_layer
) {
    // Get batch size and sequence length from token_ids shape
    auto shape = token_ids.shape();
    if (static_cast<int>(shape.size()) < 2) {
        throw std::invalid_argument("token_ids must be at least 2D (batch_size, seq_len)");
    }
    
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    
    // Use real embeddings from base model if available
    if (embedding_layer) {
        spdlog::debug("Using real embeddings from base model");
        
        // Get embeddings from embedding layer: [batch_size, seq_len, hidden_dim]
        GPUTensor embeddings_3d = embedding_layer->forward(token_ids);
        
        // Average over sequence dimension to get [batch_size, hidden_dim]
        // Use GPU kernel if CUDA is available, otherwise fall back to CPU
        GPUTensor embeddings({batch_size, hidden_dim}, device);
        
#ifdef THEMIS_ENABLE_CUDA
        if (device.type == DeviceType::CUDA) {
            // ✅ Use CUDA kernel for sequence averaging (NO CPU transfers!)
            spdlog::debug("Using CUDA kernel for sequence averaging on GPU");
            
            cudaError_t err = cuda::launch_sequence_mean_kernel(
                static_cast<float*>(embeddings.gpu_ptr()),
                static_cast<const float*>(embeddings_3d.gpu_ptr()),
                batch_size,
                seq_len,
                hidden_dim,
                nullptr  // Use default stream
            );
            
            if (err != cudaSuccess) {
                spdlog::error("CUDA sequence mean kernel failed: {}", cudaGetErrorString(err));
                throw std::runtime_error("CUDA sequence mean kernel failed");
            }
            
            return embeddings;
        }
#endif

#ifdef THEMIS_ENABLE_HIP
        if (device.type == DeviceType::HIP) {
            // ✅ Use HIP kernel for sequence averaging (NO CPU transfers!)
            spdlog::debug("Using HIP kernel for sequence averaging on GPU");
            
            hipError_t err = hip::launch_sequence_mean_kernel(
                static_cast<float*>(embeddings.gpu_ptr()),
                static_cast<const float*>(embeddings_3d.gpu_ptr()),
                batch_size,
                seq_len,
                hidden_dim,
                nullptr  // Use default stream
            );
            
            if (err != hipSuccess) {
                spdlog::error("HIP sequence mean kernel failed: {}", hipGetErrorString(err));
                throw std::runtime_error("HIP sequence mean kernel failed");
            }
            
            return embeddings;
        }
#endif
        
        // Vulkan backend: Note - shader implementation not available in this build
        // Fall through to CPU fallback
        
        // DirectX backend: Note - shader implementation not available in this build
        // Fall through to CPU fallback
        
        // CPU fallback: Download, average, and upload
        spdlog::debug("Using CPU fallback for sequence averaging");
        auto embeddings_data = embeddings_3d.cpu_data();
        std::vector<float> averaged_data(batch_size * hidden_dim, 0.0f);
        
        // Reordered loops for better cache locality (sequential memory access)
        for (size_t i = 0; i < batch_size; ++i) {
            for (size_t k = 0; k < seq_len; ++k) {
                for (size_t j = 0; j < hidden_dim; ++j) {
                    averaged_data[i * hidden_dim + j] += 
                        embeddings_data[i * seq_len * hidden_dim + k * hidden_dim + j];
                }
            }
            // Normalize by sequence length
            for (size_t j = 0; j < hidden_dim; ++j) {
                averaged_data[i * hidden_dim + j] /= seq_len;
            }
        }
        
        embeddings.upload(averaged_data);
        return embeddings;
    }
    
    // Fallback: Hash-based embeddings (standalone mode)
    spdlog::debug("Using hash-based embeddings (standalone mode)");
    
    // Create embedding tensor
    GPUTensor embeddings({batch_size, hidden_dim}, device);
    
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
    
    const Device& device = predictions.device();
    
    // Use GPU kernels for CUDA and HIP backends
    if (device.type == DeviceType::CUDA || device.type == DeviceType::HIP) {
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
        const size_t n = predictions.size();
        // Step 1: Parallel reduction on GPU
        int threads = THEMIS_GPU_REDUCTION_BLOCK_SIZE;
        int blocks = std::min(THEMIS_GPU_MAX_BLOCKS, static_cast<int>((n + threads - 1) / threads));
        
        // Allocate temporary buffer for partial sums
        GPUTensor partial_sums({static_cast<size_t>(blocks)}, device);
        
        // Launch kernel for parallel reduction
#ifdef THEMIS_ENABLE_CUDA
        if (device.type == DeviceType::CUDA) {
            auto err = cuda::launch_mse_loss_reduction_kernel(
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                static_cast<float*>(partial_sums.gpu_ptr()),
                static_cast<int>(n),
                blocks,
                nullptr  // use default stream
            );
            if (err != cudaSuccess) {
                throw std::runtime_error("CUDA MSE loss reduction kernel failed: " + 
                                       std::string(cudaGetErrorString(err)));
            }
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device.type == DeviceType::HIP) {
            auto err = hip::launch_mse_loss_reduction_kernel(
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                static_cast<float*>(partial_sums.gpu_ptr()),
                static_cast<int>(n),
                blocks,
                nullptr  // use default stream
            );
            if (err != hipSuccess) {
                throw std::runtime_error("HIP MSE loss reduction kernel failed: " + 
                                       std::string(hipGetErrorString(err)));
            }
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
            auto err = cuda::launch_mse_gradient_kernel(
                static_cast<float*>(grad.gpu_ptr()),
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                scale,
                static_cast<int>(n),
                nullptr  // use default stream
            );
            if (err != cudaSuccess) {
                throw std::runtime_error("CUDA MSE gradient kernel failed: " + 
                                       std::string(cudaGetErrorString(err)));
            }
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device.type == DeviceType::HIP) {
            auto err = hip::launch_mse_gradient_kernel(
                static_cast<float*>(grad.gpu_ptr()),
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                scale,
                static_cast<int>(n),
                nullptr  // use default stream
            );
            if (err != hipSuccess) {
                throw std::runtime_error("HIP MSE gradient kernel failed: " + 
                                       std::string(hipGetErrorString(err)));
            }
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

/**
 * @brief Fused MSE loss and gradient computation
 * 
 * Computes both loss and gradient in a single GPU kernel pass.
 * This is more efficient than calling computeMSELossGPU and computeMSEGradientGPU separately
 * because it reads predictions/targets only once instead of twice.
 * 
 * Expected performance: 1.3-1.5x faster than separate calls
 * Memory bandwidth reduction: ~50%
 * 
 * @param predictions Prediction tensor
 * @param targets Target tensor
 * @param grad_output Output gradient tensor (will be allocated)
 * @return MSE loss value
 */
float computeFusedMSELossGradientGPU(
    const GPUTensor& predictions, 
    const GPUTensor& targets,
    GPUTensor& grad_output
) {
    if (predictions.shape() != targets.shape()) {
        throw std::invalid_argument("Predictions and targets must have same shape");
    }
    
    // Create gradient tensor on same device
    grad_output = GPUTensor(predictions.shape(), predictions.device());
    
    const Device& device = predictions.device();
    
    // Use fused GPU kernels for CUDA and HIP backends
    if (device.type == DeviceType::CUDA || device.type == DeviceType::HIP) {
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
        const size_t n = predictions.size();
        // Parallel reduction on GPU
        int threads = 256;
        int blocks = std::min(1024, static_cast<int>((n + threads - 1) / threads));
        
        // Allocate temporary buffer for partial sums
        GPUTensor partial_sums({static_cast<size_t>(blocks)}, device);
        
        // Launch fused kernel - computes both loss and gradient
#ifdef THEMIS_ENABLE_CUDA
        if (device.type == DeviceType::CUDA) {
            cuda::fused::launch_fused_mse_loss_gradient(
                static_cast<float*>(grad_output.gpu_ptr()),
                static_cast<float*>(partial_sums.gpu_ptr()),
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                static_cast<int>(n),
                blocks,
                nullptr  // use default stream
            );
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device.type == DeviceType::HIP) {
            hip::fused::launch_fused_mse_loss_gradient(
                static_cast<float*>(grad_output.gpu_ptr()),
                static_cast<float*>(partial_sums.gpu_ptr()),
                static_cast<const float*>(predictions.gpu_ptr()),
                static_cast<const float*>(targets.gpu_ptr()),
                static_cast<int>(n),
                blocks,
                nullptr  // use default stream
            );
        }
#endif
        
        // Final reduction on CPU (small array, ~4KB transfer)
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
        std::vector<float> grad_data(pred_data.size());
        float scale = 2.0f / static_cast<float>(pred_data.size());
        
        for (size_t i = 0; i < pred_data.size(); ++i) {
            float diff = pred_data[i] - target_data[i];
            sum += diff * diff;
            grad_data[i] = scale * diff;
        }
        
        grad_output.upload(grad_data);
        return static_cast<bool>(sum / static_cast<float < static_cast<int>((pred_data.size())));
#endif
    } else {
        // CPU fallback or other backends
        auto pred_data = predictions.cpu_data();
        auto target_data = targets.cpu_data();
        
        float sum = 0.0f;
        std::vector<float> grad_data(pred_data.size());
        float scale = 2.0f / static_cast<float>(pred_data.size());
        
        for (size_t i = 0; i < pred_data.size(); ++i) {
            float diff = pred_data[i] - target_data[i];
            sum += diff * diff;
            grad_data[i] = scale * diff;
        }
        
        grad_output.upload(grad_data);
        return sum / pred_data.size();
    }
}

} // namespace lora
} // namespace llm
} // namespace themis

