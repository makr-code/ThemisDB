#include "llm/lora_framework/gpu_lora_layers.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cmath>

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// GPULoRALayer Implementation
// ============================================================================

GPULoRALayer::GPULoRALayer(size_t in_dim, size_t out_dim, size_t rank, 
                           float scaling, const Device& device)
    : in_dim_(in_dim)
    , out_dim_(out_dim)
    , rank_(rank)
    , scaling_(scaling)
    , device_(device) {
    
    // Initialize B with Kaiming uniform
    B_ = std::make_unique<GPUTensor>(
        gpu_tensor_utils::kaiming_uniform({in_dim, rank}, 0.0f, device)
    );
    B_->requires_grad = true;
    
    // Initialize A with zeros (standard LoRA initialization)
    A_ = std::make_unique<GPUTensor>(
        gpu_tensor_utils::zeros({rank, out_dim}, device)
    );
    A_->requires_grad = true;
    
    spdlog::debug("GPULoRALayer created: in_dim={}, out_dim={}, rank={}, scaling={}, device={}",
                  in_dim_, out_dim_, rank_, scaling_, static_cast<int>(device.type));
}

GPUTensor GPULoRALayer::forward(const GPUTensor& input) {
    // Verify input is on the same device
    if (input.device() != device_) {
        throw std::runtime_error("Input device mismatch in GPULoRALayer::forward");
    }
    
    // Cache input for backward pass
    cached_input_ = input.clone();
    
    // Forward: output = input @ B @ A * scaling
    // Step 1: h = input @ B
    cached_h_ = input.matmul(*B_);
    
    // Step 2: output = h @ A
    auto output = cached_h_.matmul(*A_);
    
    // Step 3: Scale output
    if (std::abs(scaling_ - 1.0f) > 1e-6f) {
        output = output * scaling_;
    }
    
    return output;
}

GPUTensor GPULoRALayer::backward(const GPUTensor& grad_output) {
    // Verify grad_output is on the same device
    if (grad_output.device() != device_) {
        throw std::runtime_error("Gradient device mismatch in GPULoRALayer::backward");
    }
    
    // Apply scaling to gradient
    GPUTensor scaled_grad = (std::abs(scaling_ - 1.0f) > 1e-6f) 
                            ? grad_output * scaling_
                            : grad_output.clone();
    
    // Compute gradients
    // grad_A = cached_h^T @ scaled_grad
    auto cached_h_t = cached_h_.transpose();
    A_->ensure_grad();
    *(A_->grad) = cached_h_t.matmul(scaled_grad);
    
    // grad_B = cached_input^T @ (scaled_grad @ A^T)
    auto A_t = A_->transpose();
    auto grad_h = scaled_grad.matmul(A_t);
    auto cached_input_t = cached_input_.transpose();
    B_->ensure_grad();
    *(B_->grad) = cached_input_t.matmul(grad_h);
    
    // grad_input = (scaled_grad @ A^T) @ B^T
    auto B_t = B_->transpose();
    auto grad_input = grad_h.matmul(B_t);
    
    return grad_input;
}

std::vector<GPUTensor*> GPULoRALayer::parameters() {
    return {B_.get(), A_.get()};
}

std::vector<GPUTensor*> GPULoRALayer::gradients() {
    return {B_->grad.get(), A_->grad.get()};
}

void GPULoRALayer::zero_grad() {
    if (B_->grad) {
        B_->grad->zero();
    }
    if (A_->grad) {
        A_->grad->zero();
    }
}

std::pair<GPUTensor, GPUTensor> GPULoRALayer::get_weights() const {
    return {B_->clone(), A_->clone()};
}

void GPULoRALayer::set_weights(const GPUTensor& B, const GPUTensor& A) {
    // Verify shapes
    if (B.shape() != std::vector<size_t>{in_dim_, rank_}) {
        throw std::invalid_argument("B shape mismatch in set_weights");
    }
    if (A.shape() != std::vector<size_t>{rank_, out_dim_}) {
        throw std::invalid_argument("A shape mismatch in set_weights");
    }
    
    // Move to correct device if needed and set
    *B_ = B.device() == device_ ? B.clone() : B.to(device_);
    *A_ = A.device() == device_ ? A.clone() : A.to(device_);
    
    B_->requires_grad = true;
    A_->requires_grad = true;
}

void GPULoRALayer::to(const Device& target_device) {
    if (device_ == target_device) {
        return;
    }
    
    // Move parameters
    B_->to_inplace(target_device);
    A_->to_inplace(target_device);
    
    // Update device
    device_ = target_device;
    
    spdlog::debug("GPULoRALayer moved to device: {}", static_cast<int>(device_.type));
}

// ============================================================================
// GPUSGDOptimizer Implementation
// ============================================================================

GPUSGDOptimizer::GPUSGDOptimizer(float learning_rate, float momentum, float weight_decay)
    : learning_rate_(learning_rate)
    , momentum_(momentum)
    , weight_decay_(weight_decay) {
    
    if (learning_rate_ <= 0.0f) {
        throw std::invalid_argument("Learning rate must be positive");
    }
    if (momentum_ < 0.0f || momentum_ >= 1.0f) {
        throw std::invalid_argument("Momentum must be in [0, 1)");
    }
    if (weight_decay_ < 0.0f) {
        throw std::invalid_argument("Weight decay must be non-negative");
    }
    
    spdlog::debug("GPUSGDOptimizer created: lr={}, momentum={}, weight_decay={}",
                  learning_rate_, momentum_, weight_decay_);
}

void GPUSGDOptimizer::add_parameters(const std::vector<GPUTensor*>& params) {
    parameters_.insert(parameters_.end(), params.begin(), params.end());
    
    // Initialize momentum buffers if needed
    if (momentum_ > 0.0f) {
        for (auto* param : params) {
            auto buffer = std::make_unique<GPUTensor>(
                gpu_tensor_utils::zeros(param->shape(), param->device())
            );
            momentum_buffers_.push_back(std::move(buffer));
        }
    }
    
    spdlog::debug("GPUSGDOptimizer: {} parameters registered", parameters_.size());
}

void GPUSGDOptimizer::step() {
    for (size_t i = 0; i < parameters_.size(); ++i) {
        auto* param = parameters_[i];
        
        if (!param->grad) {
            spdlog::warn("Parameter {} has no gradient, skipping", i);
            continue;
        }
        
        // Get gradient
        auto grad = param->grad->clone();
        
        // Add weight decay (L2 regularization)
        if (weight_decay_ > 0.0f) {
            // grad = grad + weight_decay * param
            auto decay_term = *param * weight_decay_;
            grad = grad + decay_term;
        }
        
        // Apply momentum if enabled
        if (momentum_ > 0.0f) {
            auto* v = momentum_buffers_[i].get();
            
            // v = momentum * v + (1 - momentum) * grad
            auto momentum_term = *v * momentum_;
            auto grad_term = grad * (1.0f - momentum_);
            *v = momentum_term + grad_term;
            
            // Use momentum buffer for update
            grad = v->clone();
        }
        
        // Update parameter: param = param - lr * grad
        auto update = grad * learning_rate_;
        *param = *param - update;
    }
}

void GPUSGDOptimizer::zero_grad() {
    for (auto* param : parameters_) {
        if (param->grad) {
            param->grad->zero();
        }
    }
}

// ============================================================================
// GPULoRATrainer Implementation
// ============================================================================

GPULoRATrainer::GPULoRATrainer(GPULoRALayer* layer, GPUSGDOptimizer* optimizer)
    : layer_(layer)
    , optimizer_(optimizer) {
    
    if (!layer_ || !optimizer_) {
        throw std::invalid_argument("Layer and optimizer cannot be null");
    }
}

float GPULoRATrainer::train_step(const GPUTensor& input, const GPUTensor& target) {
    // Zero gradients
    optimizer_->zero_grad();
    
    // Forward pass
    auto output = layer_->forward(input);
    
    // Compute loss
    float loss = compute_mse_loss(output, target);
    
    // Compute gradient
    auto grad_output = compute_mse_grad(output, target);
    
    // Backward pass
    layer_->backward(grad_output);
    
    // Update parameters
    optimizer_->step();
    
    return loss;
}

float GPULoRATrainer::eval_step(const GPUTensor& input, const GPUTensor& target) {
    // Forward pass only (no gradient computation)
    auto output = layer_->forward(input);
    
    // Compute loss
    float loss = compute_mse_loss(output, target);
    
    return loss;
}

float GPULoRATrainer::compute_mse_loss(const GPUTensor& output, const GPUTensor& target) {
    // MSE = mean((output - target)^2)
    auto diff = output - target;
    auto squared = diff.mul(diff);
    
    // Download and compute mean on CPU
    auto squared_data = squared.cpu_data();
    float sum = 0.0f;
    for (auto val : squared_data) {
        sum += val;
    }
    
    return sum / squared_data.size();
}

GPUTensor GPULoRATrainer::compute_mse_grad(const GPUTensor& output, const GPUTensor& target) {
    // grad_MSE = 2 * (output - target) / n
    auto diff = output - target;
    float scale = 2.0f / output.size();
    return diff * scale;
}

} // namespace lora
} // namespace llm
} // namespace themis
