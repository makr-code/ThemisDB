#include "llm/lora_framework/lora_layers.h"
#include <spdlog/spdlog.h>
#include <random>
#include <cmath>
#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

// ===== Tensor Implementation =====

Tensor::Tensor(const std::vector<size_t>& shape)
    : shape_(shape) {
    size_t total_size = size();
    data_.resize(total_size, 0.0f);
}

Tensor::Tensor(const std::vector<size_t>& shape, float value)
    : shape_(shape) {
    size_t total_size = size();
    data_.resize(total_size, value);
}

size_t Tensor::size() const {
    size_t s = 1;
    for (auto dim : shape_) s *= dim;
    return s;
}

Tensor Tensor::operator+(const Tensor& other) const {
    if (shape_ != other.shape_) {
        throw std::invalid_argument("Tensor shapes must match for addition");
    }
    
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] + other.data_[i];
    }
    return result;
}

Tensor Tensor::operator-(const Tensor& other) const {
    if (shape_ != other.shape_) {
        throw std::invalid_argument("Tensor shapes must match for subtraction");
    }
    
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] - other.data_[i];
    }
    return result;
}

Tensor Tensor::operator*(float scalar) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] * scalar;
    }
    return result;
}

Tensor Tensor::matmul(const Tensor& other) const {
    // Supports (M, K) @ (K, N) -> (M, N)
    if (shape_.size() != 2 || other.shape_.size() != 2) {
        throw std::invalid_argument("matmul only supports 2D tensors");
    }
    
    size_t M = shape_[0];
    size_t K = shape_[1];
    size_t K2 = other.shape_[0];
    size_t N = other.shape_[1];
    
    if (K != K2) {
        throw std::invalid_argument("Inner dimensions must match for matmul");
    }
    
    Tensor result({M, N});
    
    // Simple matrix multiplication (not optimized)
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            float sum = 0.0f;
            for (size_t k = 0; k < K; ++k) {
                sum += data_[i * K + k] * other.data_[k * N + j];
            }
            result.data_[i * N + j] = sum;
        }
    }
    
    return result;
}

Tensor Tensor::transpose() const {
    if (shape_.size() != 2) {
        throw std::invalid_argument("transpose only supports 2D tensors");
    }
    
    size_t M = shape_[0];
    size_t N = shape_[1];
    
    Tensor result({N, M});
    
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            result.data_[j * M + i] = data_[i * N + j];
        }
    }
    
    return result;
}

void Tensor::fill(float value) {
    std::fill(data_.begin(), data_.end(), value);
}

void Tensor::zero() {
    fill(0.0f);
}

Tensor Tensor::clone() const {
    Tensor result(shape_);
    result.data_ = data_;
    return result;
}

// ===== Tensor Utility Functions =====

namespace tensor_utils {

Tensor randn(const std::vector<size_t>& shape, float mean, float std) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(mean, std);
    
    Tensor result(shape);
    for (auto& val : result.data()) {
        val = dist(gen);
    }
    return result;
}

Tensor xavier_uniform(const std::vector<size_t>& shape) {
    if (shape.size() != 2) {
        throw std::invalid_argument("xavier_uniform only supports 2D tensors");
    }
    
    float fan_in = static_cast<float>(shape[0]);
    float fan_out = static_cast<float>(shape[1]);
    float std = std::sqrt(2.0f / (fan_in + fan_out));
    
    return randn(shape, 0.0f, std);
}

Tensor kaiming_uniform(const std::vector<size_t>& shape, float a) {
    if (shape.size() != 2) {
        throw std::invalid_argument("kaiming_uniform only supports 2D tensors");
    }
    
    float fan_in = static_cast<float>(shape[0]);
    float std = std::sqrt(2.0f / ((1.0f + a * a) * fan_in));
    
    return randn(shape, 0.0f, std);
}

Tensor zeros(const std::vector<size_t>& shape) {
    return Tensor(shape, 0.0f);
}

Tensor ones(const std::vector<size_t>& shape) {
    return Tensor(shape, 1.0f);
}

} // namespace tensor_utils

// ===== LoRALayer =====

LoRALayer::LoRALayer(size_t in_dim, size_t out_dim, size_t rank, float scaling)
    : in_dim_(in_dim)
    , out_dim_(out_dim)
    , rank_(rank)
    , scaling_(scaling) {
    
    name_ = "LoRALayer_" + std::to_string(in_dim) + "x" + std::to_string(out_dim) + 
            "_r" + std::to_string(rank);
    
    spdlog::info("Created {}: in={}, out={}, rank={}, scaling={}",
                name_, in_dim_, out_dim_, rank_, scaling_);
    
    // Initialize B and A matrices with proper initialization
    // B: (in_dim, rank) - Kaiming initialization
    B_ = std::make_unique<Tensor>(tensor_utils::kaiming_uniform({in_dim, rank}));
    B_->requires_grad = true;
    B_->grad = Tensor({in_dim, rank}, 0.0f);
    
    // A: (rank, out_dim) - Zero initialization (as per LoRA paper)
    A_ = std::make_unique<Tensor>(tensor_utils::zeros({rank, out_dim}));
    A_->requires_grad = true;
    A_->grad = Tensor({rank, out_dim}, 0.0f);
    
    spdlog::debug("{}: Initialized with {} parameters", name_, parameter_count());
}

Tensor LoRALayer::forward(const Tensor& input) {
    spdlog::debug("{}: forward with input shape ({}, {})", 
                  name_, input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
    
    // Cache input for backward pass
    cached_input_ = input.clone();
    
    // Compute BA = B @ A
    cached_BA_ = B_->matmul(*A_);
    
    // Compute output = input @ BA * scaling
    Tensor output = input.matmul(cached_BA_);
    output = output * scaling_;
    
    return output;
}

Tensor LoRALayer::backward(const Tensor& grad_output) {
    spdlog::debug("{}: backward with grad_output shape ({}, {})",
                  name_, grad_output.shape()[0], grad_output.shape().size() > 1 ? grad_output.shape()[1] : 0);
    
    // Scale gradient by scaling factor
    Tensor scaled_grad = grad_output * scaling_;
    
    // Compute gradients w.r.t. A: grad_A = B.T @ (input.T @ scaled_grad)
    // Simplified: grad_A = B.T @ (scaled_grad.T @ input).T
    Tensor input_T = cached_input_.transpose();
    Tensor grad_A_partial = input_T.matmul(scaled_grad);
    Tensor B_T = B_->transpose();
    A_->grad = B_T.matmul(grad_A_partial);
    
    // Compute gradients w.r.t. B: grad_B = (scaled_grad @ A.T) @ input.T
    Tensor A_T = A_->transpose();
    Tensor grad_B_partial = scaled_grad.matmul(A_T);
    B_->grad = grad_B_partial.matmul(input_T.transpose());
    
    // Compute gradient w.r.t. input: grad_input = scaled_grad @ (BA).T
    Tensor BA_T = cached_BA_.transpose();
    Tensor grad_input = scaled_grad.matmul(BA_T);
    
    return grad_input;
}

std::vector<Tensor*> LoRALayer::parameters() {
    return {B_.get(), A_.get()};
}

size_t LoRALayer::parameter_count() const {
    return (in_dim_ * rank_) + (rank_ * out_dim_);
}

size_t LoRALayer::memory_bytes() const {
    return parameter_count() * sizeof(float);
}

std::pair<Tensor, Tensor> LoRALayer::get_weights() const {
    return {B_->clone(), A_->clone()};
}

void LoRALayer::set_weights(const Tensor& B, const Tensor& A) {
    if (B.shape() != B_->shape() || A.shape() != A_->shape()) {
        throw std::invalid_argument("Weight shapes do not match");
    }
    *B_ = B.clone();
    *A_ = A.clone();
    spdlog::debug("{}: Weights updated", name_);
}

// ===== AttentionLoRA =====

AttentionLoRA::AttentionLoRA(size_t dim, size_t rank,
                             bool apply_to_q,
                             bool apply_to_k,
                             bool apply_to_v,
                             bool apply_to_o)
    : dim_(dim)
    , rank_(rank)
    , apply_to_q_(apply_to_q)
    , apply_to_k_(apply_to_k)
    , apply_to_v_(apply_to_v)
    , apply_to_o_(apply_to_o) {
    
    spdlog::info("Created AttentionLoRA: dim={}, rank={}, q={}, k={}, v={}, o={}",
                dim, rank, apply_to_q, apply_to_k, apply_to_v, apply_to_o);
    
    // Initialize LoRA layers for Q, K, V, O
    if (apply_to_q_) {
        q_lora_ = std::make_unique<LoRALayer>(dim, dim, rank);
        spdlog::debug("AttentionLoRA: Created Q LoRA layer");
    }
    if (apply_to_k_) {
        k_lora_ = std::make_unique<LoRALayer>(dim, dim, rank);
        spdlog::debug("AttentionLoRA: Created K LoRA layer");
    }
    if (apply_to_v_) {
        v_lora_ = std::make_unique<LoRALayer>(dim, dim, rank);
        spdlog::debug("AttentionLoRA: Created V LoRA layer");
    }
    if (apply_to_o_) {
        o_lora_ = std::make_unique<LoRALayer>(dim, dim, rank);
        spdlog::debug("AttentionLoRA: Created O LoRA layer");
    }
    
    spdlog::info("AttentionLoRA: Initialized with {} parameters", parameter_count());
}

Tensor AttentionLoRA::forward(const Tensor& input) {
    spdlog::debug("AttentionLoRA: forward with input shape ({}, {})",
                  input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
    
    Tensor output = input.clone();
    
    // Apply LoRA layers sequentially (simplified version)
    // In a real implementation, Q, K, V, O would be applied separately in attention mechanism
    if (apply_to_q_ && q_lora_) {
        output = q_lora_->forward(output);
    }
    if (apply_to_k_ && k_lora_) {
        output = k_lora_->forward(output);
    }
    if (apply_to_v_ && v_lora_) {
        output = v_lora_->forward(output);
    }
    if (apply_to_o_ && o_lora_) {
        output = o_lora_->forward(output);
    }
    
    return output;
}

Tensor AttentionLoRA::backward(const Tensor& grad_output) {
    spdlog::debug("AttentionLoRA: backward with grad_output shape ({}, {})",
                  grad_output.shape()[0], grad_output.shape().size() > 1 ? grad_output.shape()[1] : 0);
    
    Tensor grad = grad_output.clone();
    
    // Backward pass in reverse order
    if (apply_to_o_ && o_lora_) {
        grad = o_lora_->backward(grad);
    }
    if (apply_to_v_ && v_lora_) {
        grad = v_lora_->backward(grad);
    }
    if (apply_to_k_ && k_lora_) {
        grad = k_lora_->backward(grad);
    }
    if (apply_to_q_ && q_lora_) {
        grad = q_lora_->backward(grad);
    }
    
    return grad;
}

std::vector<Tensor*> AttentionLoRA::parameters() {
    std::vector<Tensor*> params;
    
    if (apply_to_q_ && q_lora_) {
        auto q_params = q_lora_->parameters();
        params.insert(params.end(), q_params.begin(), q_params.end());
    }
    if (apply_to_k_ && k_lora_) {
        auto k_params = k_lora_->parameters();
        params.insert(params.end(), k_params.begin(), k_params.end());
    }
    if (apply_to_v_ && v_lora_) {
        auto v_params = v_lora_->parameters();
        params.insert(params.end(), v_params.begin(), v_params.end());
    }
    if (apply_to_o_ && o_lora_) {
        auto o_params = o_lora_->parameters();
        params.insert(params.end(), o_params.begin(), o_params.end());
    }
    
    spdlog::debug("AttentionLoRA: Collected {} parameters", params.size());
    return params;
}

size_t AttentionLoRA::parameter_count() const {
    size_t count = 0;
    if (apply_to_q_) count += (dim_ * rank_) + (rank_ * dim_);
    if (apply_to_k_) count += (dim_ * rank_) + (rank_ * dim_);
    if (apply_to_v_) count += (dim_ * rank_) + (rank_ * dim_);
    if (apply_to_o_) count += (dim_ * rank_) + (rank_ * dim_);
    return count;
}

size_t AttentionLoRA::memory_bytes() const {
    return parameter_count() * sizeof(float);
}

// ===== Sequential =====

void Sequential::add(std::unique_ptr<ITrainableLayer> layer) {
    spdlog::info("Sequential: Adding layer {}", layer->name());
    layers_.push_back(std::move(layer));
}

Tensor Sequential::forward(const Tensor& input) {
    spdlog::debug("Sequential: forward through {} layers", layers_.size());
    
    Tensor output = input.clone();
    for (auto& layer : layers_) {
        output = layer->forward(output);
    }
    
    return output;
}

Tensor Sequential::backward(const Tensor& grad_output) {
    spdlog::debug("Sequential: backward through {} layers", layers_.size());
    
    Tensor grad = grad_output.clone();
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        grad = (*it)->backward(grad);
    }
    
    return grad;
}

std::vector<Tensor*> Sequential::parameters() {
    spdlog::debug("Sequential: collecting parameters from {} layers", layers_.size());
    
    std::vector<Tensor*> params;
    for (auto& layer : layers_) {
        auto layer_params = layer->parameters();
        params.insert(params.end(), layer_params.begin(), layer_params.end());
    }
    
    spdlog::debug("Sequential: Collected {} total parameters", params.size());
    return params;
}

size_t Sequential::parameter_count() const {
    size_t total = 0;
    for (const auto& layer : layers_) {
        total += layer->parameter_count();
    }
    return total;
}

size_t Sequential::memory_bytes() const {
    size_t total = 0;
    for (const auto& layer : layers_) {
        total += layer->memory_bytes();
    }
    return total;
}

// ===== SGD Optimizer =====

SGDOptimizer::SGDOptimizer(float learning_rate, float momentum, float weight_decay)
    : learning_rate_(learning_rate)
    , momentum_(momentum)
    , weight_decay_(weight_decay) {
    spdlog::info("Created SGDOptimizer: lr={}, momentum={}, weight_decay={}",
                 learning_rate_, momentum_, weight_decay_);
}

void SGDOptimizer::add_parameters(const std::vector<Tensor*>& params) {
    parameters_.insert(parameters_.end(), params.begin(), params.end());
    spdlog::debug("SGDOptimizer: Added {} parameters, total={}", 
                  params.size(), parameters_.size());
}

void SGDOptimizer::step() {
    for (auto* param : parameters_) {
        if (!param || !param->requires_grad) {
            continue;
        }
        
        // Apply weight decay if specified (L2 regularization)
        if (weight_decay_ > 0.0f) {
            for (size_t i = 0; i < param->data().size(); ++i) {
                param->grad[i] += weight_decay_ * param->data()[i];
            }
        }
        
        // Apply momentum if specified
        if (momentum_ > 0.0f) {
            // Initialize momentum buffer if not exists
            if (momentum_buffers_.find(param) == momentum_buffers_.end()) {
                momentum_buffers_[param] = Tensor(param->shape(), 0.0f);
            }
            
            Tensor& momentum_buffer = momentum_buffers_[param];
            
            // Update momentum: v = momentum * v + grad
            for (size_t i = 0; i < momentum_buffer.data().size(); ++i) {
                momentum_buffer[i] = momentum_ * momentum_buffer[i] + param->grad[i];
            }
            
            // Update parameters: param = param - lr * v
            for (size_t i = 0; i < param->data().size(); ++i) {
                param->data()[i] -= learning_rate_ * momentum_buffer[i];
            }
        } else {
            // Standard SGD: param = param - lr * grad
            for (size_t i = 0; i < param->data().size(); ++i) {
                param->data()[i] -= learning_rate_ * param->grad[i];
            }
        }
    }
    
    spdlog::debug("SGDOptimizer: Updated {} parameters", parameters_.size());
}

void SGDOptimizer::zero_grad() {
    for (auto* param : parameters_) {
        if (param && param->requires_grad) {
            param->grad.zero();
        }
    }
    spdlog::debug("SGDOptimizer: Zeroed gradients for {} parameters", parameters_.size());
}

} // namespace lora
} // namespace llm
} // namespace themis
