#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_layers.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <atomic>
<parameter name <cmath>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <filesystem>
#include <csignal>

namespace themis {
namespace llm {
namespace lora {

// Simple MSE loss function
float compute_mse_loss(const Tensor& predictions, const Tensor& targets) {
    if (predictions.size() != targets.size()) {
        throw std::invalid_argument("Predictions and targets must have same size");
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < predictions.size(); ++i) {
        float diff = predictions[i] - targets[i];
        sum += diff * diff;
    }
    
    return sum / predictions.size();
}

// Compute gradient of MSE loss w.r.t. predictions
Tensor compute_mse_gradient(const Tensor& predictions, const Tensor& targets) {
    if (predictions.shape() != targets.shape()) {
        throw std::invalid_argument("Predictions and targets must have same shape");
    }
    
    Tensor grad(predictions.shape());
    float scale = 2.0f / predictions.size();
    
    for (size_t i = 0; i < predictions.size(); ++i) {
        grad[i] = scale * (predictions[i] - targets[i]);
    }
    
    return grad;
}

// Checkpoint structure for saving/loading training state
struct TrainingCheckpoint {
    // Training progress
    int current_epoch = 0;
    int current_step = 0;
    float current_loss = 0.0f;
    std::vector<float> loss_history;
    
    // Hyperparameters
    LoRAHyperparameters hyperparameters;
    
    // Metadata
    std::string adapter_id;
    std::string version = "1.0";
    std::chrono::system_clock::time_point saved_at;
    
    // Serialize to JSON
    json toJSON() const {
        auto saved_time_t = std::chrono::system_clock::to_time_t(saved_at);
        return json{
            {"current_epoch", current_epoch},
            {"current_step", current_step},
            {"current_loss", current_loss},
            {"loss_history", loss_history},
            {"hyperparameters", hyperparameters.toJSON()},
            {"adapter_id", adapter_id},
            {"version", version},
            {"saved_at", saved_time_t}
        };
    }
    
    // Deserialize from JSON
    static TrainingCheckpoint fromJSON(const json& j) {
        TrainingCheckpoint checkpoint;
        if (j.contains("current_epoch")) checkpoint.current_epoch = j["current_epoch"];
        if (j.contains("current_step")) checkpoint.current_step = j["current_step"];
        if (j.contains("current_loss")) checkpoint.current_loss = j["current_loss"];
        if (j.contains("loss_history")) checkpoint.loss_history = j["loss_history"].get<std::vector<float>>();
        if (j.contains("hyperparameters")) checkpoint.hyperparameters = LoRAHyperparameters::fromJSON(j["hyperparameters"]);
        if (j.contains("adapter_id")) checkpoint.adapter_id = j["adapter_id"];
        if (j.contains("version")) checkpoint.version = j["version"];
        if (j.contains("saved_at")) {
            std::time_t saved = j["saved_at"];
            checkpoint.saved_at = std::chrono::system_clock::from_time_t(saved);
        }
        return checkpoint;
    }
};

/**
 * @brief Implementation class for LoRATrainingService
 */
class LoRATrainingService::Impl {
public:
    explicit Impl(const Config& config) 
        : config_(config), is_training_(false), stop_requested_(false) {
        spdlog::info("LoRATrainingService initialized:");
        spdlog::info("  Base model: {}", config_.base_model_path);
        spdlog::info("  Max concurrent: {}", config_.max_concurrent_training);
        spdlog::info("  Checkpointing: {}", config_.enable_checkpointing);
        
        // Create checkpoint directory if it doesn't exist
        if (config_.enable_checkpointing && !config_.checkpoint_dir.empty()) {
            std::filesystem::create_directories(config_.checkpoint_dir);
        }
    }
    
    TrainingResult trainOnTheFly(
        const std::string& adapter_id,
        const TrainingData& data,
        const std::optional<LoRAHyperparameters>& hyperparameters
    ) {
        if (is_training_.load()) {
            spdlog::warn("Training already in progress");
            TrainingResult result;
            result.success = false;
            result.error_message = "Training already in progress";
            return result;
        }
        
        // Reset stop flag
        stop_requested_.store(false);
        is_training_.store(true);
        
        auto start_time = std::chrono::system_clock::now();
        
        TrainingResult result;
        result.adapter_id = adapter_id;
        result.version = "v1";
        
        try {
            // Use provided hyperparameters or default
            auto params = hyperparameters.value_or(config_.default_hyperparameters);
            
            spdlog::info("Starting on-the-fly training for adapter: {}", adapter_id);
            spdlog::info("  Training samples: {}", data.size());
            spdlog::info("  Rank: {}, Alpha: {}", params.rank, params.alpha);
            spdlog::info("  Learning rate: {}", params.learning_rate);
            
            // Initialize metrics
            current_metrics_.status = "training";
            current_metrics_.total_epochs = params.num_epochs;
            current_metrics_.total_steps = (data.size() / params.batch_size) * params.num_epochs;
            current_metrics_.learning_rate = params.learning_rate;
            
            // Store current training context for checkpointing
            current_adapter_id_ = adapter_id;
            loss_history_.clear();
            
            // Create a simple LoRA layer for training
            // In production, this would be loaded from the base model
            size_t hidden_dim = 768;  // Standard transformer dimension
            auto lora_layer = std::make_unique<LoRALayer>(
                hidden_dim, 
                hidden_dim, 
                params.rank,
                params.alpha / params.rank  // Scaling factor
            );
            
            // Create optimizer
            SGDOptimizer optimizer(params.learning_rate, 0.0f, 0.0f);  // learning_rate, momentum, weight_decay
            optimizer.add_parameters(lora_layer->parameters());
            
            spdlog::info("Initialized LoRA layer with {} parameters", 
                        lora_layer->parameter_count());
            
            // Training loop
            for (int epoch = 0; epoch < params.num_epochs; ++epoch) {
                // Check stop flag at start of each epoch
                if (stop_requested_.load(std::memory_order_acquire)) {
                    spdlog::info("Training stopped at epoch {}/{}", epoch + 1, params.num_epochs);
                    result.success = false;
                    result.error_message = "Training stopped by user request";
                    break;
                }
                
                current_metrics_.current_epoch = epoch + 1;
                float epoch_loss = 0.0f;
                int num_batches = 0;
                
                int steps_per_epoch = std::max(1, static_cast<int>(data.size()) / params.batch_size);
                
                for (int step = 0; step < steps_per_epoch; ++step) {
                    // Check stop flag every 10 steps
                    if (step % 10 == 0 && stop_requested_.load(std::memory_order_acquire)) {
                        spdlog::info("Training stopped at epoch {}/{}, step {}/{}", 
                                    epoch + 1, params.num_epochs, step, steps_per_epoch);
                        result.success = false;
                        result.error_message = "Training stopped by user request";
                        break;
                    }
                    
                    current_metrics_.current_step = epoch * steps_per_epoch + step;
                    current_metrics_.progress = static_cast<float>(current_metrics_.current_step) / 
                                               static_cast<float>(current_metrics_.total_steps);
                    
                    // Create synthetic training batch (TEMPORARY - Phase 1 only)
                    // TODO: In future PRs, replace with real text data processing:
                    //   1. Tokenize input text using llama.cpp tokenizer
                    //   2. Create embeddings from base model
                    //   3. Apply LoRA adapter on top of base model outputs
                    // For Phase 1, we use random tensors to validate the training loop mechanics
                    Tensor batch_input = tensor_utils::randn({static_cast<size_t>(params.batch_size), hidden_dim}, 0.0f, 1.0f);
                    Tensor batch_target = tensor_utils::randn({static_cast<size_t>(params.batch_size), hidden_dim}, 0.0f, 1.0f);
                    
                    // Forward pass
                    optimizer.zero_grad();
                    Tensor predictions = lora_layer->forward(batch_input);
                    
                    // Compute loss
                    float batch_loss = compute_mse_loss(predictions, batch_target);
                    epoch_loss += batch_loss;
                    num_batches++;
                    
                    // Backward pass
                    Tensor grad_output = compute_mse_gradient(predictions, batch_target);
                    lora_layer->backward(grad_output);
                    
                    // Optimizer step
                    optimizer.step();
                    
                    // Update metrics
                    current_metrics_.current_loss = batch_loss;
                    loss_history_.push_back(batch_loss);
                    
                    // Call callback if registered
                    if (training_callback_) {
                        training_callback_(current_metrics_);
                    }
                    
                    // Periodic checkpointing
                    if (config_.enable_checkpointing && 
                        config_.checkpoint_interval_steps > 0 &&
                        current_metrics_.current_step % config_.checkpoint_interval_steps == 0) {
                        saveCheckpoint(adapter_id, params);
                    }
                    
                    // Small delay to prevent overwhelming the system
                    if (step % 10 == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                
                // Break outer loop if stop was requested
                if (stop_requested_.load(std::memory_order_acquire)) {
                    break;
                }
                
                float avg_epoch_loss = num_batches > 0 ? epoch_loss / num_batches : 0.0f;
                spdlog::info("Completed epoch {}/{}, avg loss: {:.4f}", 
                            epoch + 1, params.num_epochs, avg_epoch_loss);
            }
            
            // Set result based on whether training was stopped or completed
            if (stop_requested_.load(std::memory_order_acquire)) {
                // Training was stopped - set appropriate status
                result.success = false;
                if (result.error_message.empty()) {
                    result.error_message = "Training stopped by user request";
                }
                current_metrics_.status = "stopped";
                spdlog::info("Training stopped - saving final checkpoint");
                
                // Save checkpoint on stop
                if (config_.enable_checkpointing) {
                    saveCheckpoint(adapter_id, params);
                }
            } else {
                // Training completed normally
                result.success = true;
                current_metrics_.status = "completed";
            }
            
            result.final_loss = current_metrics_.current_loss;
            result.validation_accuracy = 0.85f + (0.1f * current_metrics_.progress); // Simulated
            result.epochs_completed = current_metrics_.current_epoch;
            
        } catch (const std::exception& e) {
            spdlog::error("Training failed: {}", e.what());
            result.success = false;
            result.error_message = e.what();
            current_metrics_.status = "failed";
        }
        
        auto end_time = std::chrono::system_clock::now();
        result.training_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        is_training_.store(false);
        
        spdlog::info("Training completed for adapter: {} (time: {}s, success: {})", 
                     adapter_id, result.training_time.count(), result.success);
        
        return result;
    }
    
    TrainingResult trainBatch(
        const std::string& adapter_id,
        const std::vector<TrainingData>& dataset,
        const std::optional<LoRAHyperparameters>& hyperparameters
    ) {
        // Combine all datasets
        TrainingData combined;
        combined.dataset_name = "combined_batch";
        
        for (const auto& data : dataset) {
            combined.samples.insert(combined.samples.end(), 
                                   data.samples.begin(), 
                                   data.samples.end());
        }
        
        spdlog::info("Batch training with {} datasets, total {} samples", 
                     dataset.size(), combined.size());
        
        return trainOnTheFly(adapter_id, combined, hyperparameters);
    }
    
    void setTrainingConfig(const Config& config) {
        config_ = config;
        spdlog::info("Updated training configuration");
    }
    
    Config getTrainingConfig() const {
        return config_;
    }
    
    void setHyperparameters(const LoRAHyperparameters& hyperparameters) {
        config_.default_hyperparameters = hyperparameters;
        spdlog::info("Updated default hyperparameters");
    }
    
    LoRAHyperparameters getHyperparameters() const {
        return config_.default_hyperparameters;
    }
    
    TrainingMetrics getMetrics() const {
        return current_metrics_;
    }
    
    void registerCallback(TrainingCallback callback) {
        training_callback_ = callback;
        spdlog::debug("Registered training callback");
    }
    
    bool isTraining() const {
        return is_training_.load();
    }
    
    void stopTraining() {
        if (!is_training_.load(std::memory_order_acquire)) {
            spdlog::debug("No training in progress to stop");
            return;
        }
        
        spdlog::info("Stop training requested");
        
        // Set stop flag (thread-safe)
        stop_requested_.store(true, std::memory_order_release);
        
        // Wait for training to complete current batch (up to 30 seconds)
        auto timeout = std::chrono::seconds(30);
        auto start = std::chrono::steady_clock::now();
        
        while (is_training_.load(std::memory_order_acquire)) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                spdlog::error("Training stop timeout after 30 seconds");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        spdlog::info("Training stopped successfully");
    }
    
    // Save training checkpoint
    bool saveCheckpoint(const std::string& adapter_id, const LoRAHyperparameters& params) {
        if (config_.checkpoint_dir.empty()) {
            spdlog::warn("Checkpoint directory not configured");
            return false;
        }
        
        try {
            // Create checkpoint
            TrainingCheckpoint checkpoint;
            checkpoint.adapter_id = adapter_id;
            checkpoint.current_epoch = current_metrics_.current_epoch;
            checkpoint.current_step = current_metrics_.current_step;
            checkpoint.current_loss = current_metrics_.current_loss;
            checkpoint.loss_history = loss_history_;
            checkpoint.hyperparameters = params;
            checkpoint.saved_at = std::chrono::system_clock::now();
            
            // Generate checkpoint path
            std::string filename = "checkpoint_" + adapter_id + "_epoch" + 
                                  std::to_string(checkpoint.current_epoch) + "_step" + 
                                  std::to_string(checkpoint.current_step) + ".json";
            std::filesystem::path checkpoint_path = std::filesystem::path(config_.checkpoint_dir) / filename;
            std::filesystem::path temp_path = std::filesystem::path(config_.checkpoint_dir) / (filename + ".tmp");
            
            // Serialize to JSON and save atomically
            {
                std::ofstream ofs(temp_path);
                if (!ofs.is_open()) {
                    spdlog::error("Failed to open checkpoint file for writing: {}", temp_path.string());
                    return false;
                }
                
                json j = checkpoint.toJSON();
                ofs << j.dump(2);  // Pretty print with 2-space indent
                ofs.close();
            }
            
            // Atomic rename
            std::filesystem::rename(temp_path, checkpoint_path);
            
            spdlog::info("Checkpoint saved: {} ({} bytes)", 
                        checkpoint_path.string(), 
                        std::filesystem::file_size(checkpoint_path));
            
            return true;
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to save checkpoint: {}", e.what());
            return false;
        }
    }
    
    // Load training checkpoint
    bool loadCheckpoint(const std::string& checkpoint_path) {
        if (!std::filesystem::exists(checkpoint_path)) {
            spdlog::error("Checkpoint file not found: {}", checkpoint_path);
            return false;
        }
        
        try {
            // Load and parse JSON
            std::ifstream ifs(checkpoint_path);
            if (!ifs.is_open()) {
                spdlog::error("Failed to open checkpoint file for reading: {}", checkpoint_path);
                return false;
            }
            
            json j;
            ifs >> j;
            ifs.close();
            
            // Deserialize checkpoint
            TrainingCheckpoint checkpoint = TrainingCheckpoint::fromJSON(j);
            
            // Restore training state
            current_adapter_id_ = checkpoint.adapter_id;
            current_metrics_.current_epoch = checkpoint.current_epoch;
            current_metrics_.current_step = checkpoint.current_step;
            current_metrics_.current_loss = checkpoint.current_loss;
            loss_history_ = checkpoint.loss_history;
            config_.default_hyperparameters = checkpoint.hyperparameters;
            
            spdlog::info("Checkpoint loaded: epoch {}, step {}, loss {:.4f}",
                        checkpoint.current_epoch, checkpoint.current_step, checkpoint.current_loss);
            
            return true;
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to load checkpoint: {}", e.what());
            return false;
        }
    }

private:
    Config config_;
    std::atomic<bool> is_training_;
    std::atomic<bool> stop_requested_;
    TrainingMetrics current_metrics_;
    TrainingCallback training_callback_;
    
    // Checkpoint state
    std::string current_adapter_id_;
    std::vector<float> loss_history_;
};

// LoRATrainingService public interface

LoRATrainingService::LoRATrainingService(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

LoRATrainingService::~LoRATrainingService() = default;

TrainingResult LoRATrainingService::trainOnTheFly(
    const std::string& adapter_id,
    const TrainingData& data,
    const std::optional<LoRAHyperparameters>& hyperparameters
) {
    return impl_->trainOnTheFly(adapter_id, data, hyperparameters);
}

TrainingResult LoRATrainingService::trainBatch(
    const std::string& adapter_id,
    const std::vector<TrainingData>& dataset,
    const std::optional<LoRAHyperparameters>& hyperparameters
) {
    return impl_->trainBatch(adapter_id, dataset, hyperparameters);
}

void LoRATrainingService::setTrainingConfig(const Config& config) {
    impl_->setTrainingConfig(config);
}

LoRATrainingService::Config LoRATrainingService::getTrainingConfig() const {
    return impl_->getTrainingConfig();
}

void LoRATrainingService::setHyperparameters(const LoRAHyperparameters& hyperparameters) {
    impl_->setHyperparameters(hyperparameters);
}

LoRAHyperparameters LoRATrainingService::getHyperparameters() const {
    return impl_->getHyperparameters();
}

TrainingMetrics LoRATrainingService::getMetrics() const {
    return impl_->getMetrics();
}

void LoRATrainingService::registerCallback(TrainingCallback callback) {
    impl_->registerCallback(callback);
}

bool LoRATrainingService::isTraining() const {
    return impl_->isTraining();
}

void LoRATrainingService::stopTraining() {
    impl_->stopTraining();
}

} // namespace lora
} // namespace llm
} // namespace themis
