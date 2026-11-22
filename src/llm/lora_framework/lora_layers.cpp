/**
 * @file lora_layers.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_layers.h"
#include "utils/type_conversion.h"

#ifndef THEMIS_NO_SPDLOG
#include <spdlog/spdlog.h>
#else
namespace spdlog {
    template<typename... Args>
    /**
     * @brief Debug.
     * @param[in] param Input parameter.
     * @param[in] param Input parameter.
     * @details Implements debug without additional internal calls.
     */
    inline void debug(const char*, ...) {}
    template<typename... Args>
    /**
     * @brief Info.
     * @param[in] param Input parameter.
     * @param[in] param Input parameter.
     * @details Implements info without additional internal calls.
     */
    inline void info(const char*, ...) {}
}
#endif

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
    for (auto dim : shape_) {
      s *= dim;
    }
    return s;
}

Tensor Tensor::operator+(const Tensor& other) const {
    if (shape_ != other.shape_) {
        throw std::invalid_argument("Tensor shapes must match for addition");
    }
    
    /**
     * @brief Result.
     * @param[in] shape_ Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief Result.
     * @param[in] shape_ Input parameter.
     * @return Return value.
     */
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] - other.data_[i];
    }
    return result;
}

Tensor Tensor::operator*(float scalar) const {
    /**
     * @brief Result.
     * @param[in] shape_ Input parameter.
     * @return Return value.
     */
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
    
    // Keep the reduction order for each output element while traversing the
    // right-hand matrix in row-major order.
    for (size_t i = 0; i < M; ++i) {
        for (size_t k = 0; k < K; ++k) {
            const float lhs_value = data_[i * K + k];
            for (size_t j = 0; j < N; ++j) {
                result.data_[i * N + j] += lhs_value * other.data_[k * N + j];
            }
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

/**
 * @brief Fill.
 * @param[in] value Input parameter.
 * @details Calls: begin(), end().
 */
void Tensor::fill(float value) {
    std::fill(data_.begin(), data_.end(), value);
}

/**
 * @brief Zero.
 * @details Calls: fill().
 */
void Tensor::zero() {
    fill(0.0f);
}

Tensor Tensor::clone() const {
    /**
     * @brief Result.
     * @param[in] shape_ Input parameter.
     * @return Return value.
     */
    Tensor result(shape_);
    result.data_ = data_;
    return result;
}

// ===== Tensor Utility Functions =====

namespace tensor_utils {

/**
 * @brief Randn.
 * @param[in] shape Input parameter.
 * @param[in] mean Input parameter.
 * @param[in] std Input parameter.
 * @return Return value.
 * @details Calls: gen(), dist(), result(), data().
 */
Tensor randn(const std::vector<size_t>& shape, float mean, float std) {
    // Use thread-local random number generator for thread safety
    // Note: random_device is used once per thread to seed the generator
    thread_local std::mt19937 gen(std::random_device{}());
    std::normal_distribution<float> dist(mean, std);
    
    Tensor result(shape);
    for (auto& val : result.data()) {
        val = dist(gen);
    }
    return result;
}

/**
 * @brief Xavier uniform.
 * @param[in] shape Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: size(), std::sqrt(), randn().
 */
Tensor xavier_uniform(const std::vector<size_t>& shape) {
    if (shape.size() != 2) {
        throw std::invalid_argument("xavier_uniform only supports 2D tensors");
    }
    
    float fan_in = static_cast<float>(shape[0]);
    float fan_out = static_cast<float>(shape[1]);
    float std = std::sqrt(2.0f / (fan_in + fan_out));
    
    return randn(shape, 0.0f, std);
}

/**
 * @brief Kaiming uniform.
 * @param[in] shape Input parameter.
 * @param[in] a Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: size(), std::sqrt(), randn().
 */
Tensor kaiming_uniform(const std::vector<size_t>& shape, float a) {
    if (shape.size() != 2) {
        throw std::invalid_argument("kaiming_uniform only supports 2D tensors");
    }
    
    float fan_in = static_cast<float>(shape[0]);
    float bound = std::sqrt(6.0f / ((1.0f + a * a) * fan_in));
    
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(-bound, bound);

    Tensor result(shape);
    for (auto& val : result.data()) {
        val = dist(gen);
    }
    return result;
}

/**
 * @brief Zeros.
 * @param[in] shape Input parameter.
 * @return Return value.
 * @details Calls: Tensor().
 */
Tensor zeros(const std::vector<size_t>& shape) {
    return Tensor(shape, 0.0f);
}

/**
 * @brief Ones.
 * @param[in] shape Input parameter.
 * @return Return value.
 * @details Calls: Tensor().
 */
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
    
    if (in_dim == 0 || out_dim == 0 || rank == 0) {
        throw std::invalid_argument(
            "LoRALayer: in_dim, out_dim, and rank must all be > 0 (got in_dim=" +
            std::to_string(in_dim) + ", out_dim=" + std::to_string(out_dim) +
            ", rank=" + std::to_string(rank) + ")");
    }
    
    name_ = "LoRALayer_" + std::to_string(in_dim) + "x" + std::to_string(out_dim) + 
            "_r" + std::to_string(rank);
    
    spdlog::debug("Created {}: in={}, out={}, rank={}, scaling={}",
                name_, in_dim_, out_dim_, rank_, scaling_);
    
    // Initialize B and A matrices with proper initialization
    // B: (in_dim, rank) - Kaiming initialization
    B_ = std::make_unique<Tensor>(tensor_utils::kaiming_uniform({in_dim, rank}));
    B_->requires_grad = true;
    B_->grad = std::make_unique<Tensor>(std::vector<size_t>{in_dim, rank}, 0.0f);
    
    // A: (rank, out_dim) - Zero initialization (as per LoRA paper)
    A_ = std::make_unique<Tensor>(tensor_utils::zeros({rank, out_dim}));
    A_->requires_grad = true;
    A_->grad = std::make_unique<Tensor>(std::vector<size_t>{rank, out_dim}, 0.0f);
    
    spdlog::debug("{}: Initialized with {} parameters", name_, parameter_count());
}

/**
 * @brief Forward.
 * @param[in] input Input parameter.
 * @return Return value.
 * @details Calls: spdlog::debug(), shape(), size(), clone(), matmul().
 */
Tensor LoRALayer::forward(const Tensor& input) {
    return forwardImpl(input, nullptr);
}

Tensor LoRALayer::forwardOwned(Tensor&& input) {
    return forwardImpl(input, &input);
}

Tensor LoRALayer::forwardImpl(const Tensor& input, Tensor* owned_input) {
    spdlog::debug("{}: forward with input shape ({}, {})", 
                  name_, input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
    
    // Apply the low-rank factors in sequence to avoid materializing B @ A.
    Tensor low_rank = input.matmul(*B_);
    Tensor output = low_rank.matmul(*A_);
    for (auto& value : output.data()) {
        value *= scaling_;
    }

    // Cache both inputs required by the backward pass only after the forward
    // calculation succeeds, moving the low-rank projection without copying it.
    auto cached_projection = std::make_unique<Tensor>(std::move(low_rank));
    auto cached_input = owned_input
        ? std::make_unique<Tensor>(std::move(*owned_input))
        : std::make_unique<Tensor>(input.clone());
    cached_input_ = std::move(cached_input);
    cached_BA_ = std::move(cached_projection);
    
    return output;
}

/**
 * @brief Backward.
 * @param[in] grad_output Input parameter.
 * @return Return value.
 * @details Reuses the projection cached by forward() to compute parameter and
 *          input gradients without materializing transposed parameter matrices.
 */
Tensor LoRALayer::backward(const Tensor& grad_output) {
    spdlog::debug("{}: backward with grad_output shape ({}, {})",
                  name_, grad_output.shape()[0], grad_output.shape().size() > 1 ? grad_output.shape()[1] : 0);
    
    if (!cached_input_ || !cached_BA_) {
        throw std::logic_error("LoRALayer::backward requires a successful forward pass");
    }

    const size_t batch = cached_input_->shape()[0];
    if (grad_output.shape() != std::vector<size_t>{batch, out_dim_}) {
        throw std::invalid_argument("LoRALayer::backward gradient shape does not match the cached input");
    }

    if (!A_->grad || A_->grad->shape() != std::vector<size_t>{rank_, out_dim_}) {
        A_->grad = std::make_unique<Tensor>(std::vector<size_t>{rank_, out_dim_});
    }
    if (!B_->grad || B_->grad->shape() != std::vector<size_t>{in_dim_, rank_}) {
        B_->grad = std::make_unique<Tensor>(std::vector<size_t>{in_dim_, rank_});
    }
    Tensor& grad_A = *A_->grad;
    Tensor& grad_B = *B_->grad;
    Tensor grad_input({batch, in_dim_});
    Tensor projected_grad({batch, rank_});

    if (batch == 1) {
        const float* input_data = cached_input_->data().data();
        const float* projection_data = cached_BA_->data().data();
        const float* grad_output_data = grad_output.data().data();
        const float* a_data = A_->data().data();
        const float* b_data = B_->data().data();
        float* grad_a_data = grad_A.data().data();
        float* grad_b_data = grad_B.data().data();
        float* grad_input_data = grad_input.data().data();
        float* projected_grad_data = projected_grad.data().data();

        for (size_t r = 0; r < rank_; ++r) {
            float sum = 0.0f;
            const float* a_row = a_data + r * out_dim_;
            for (size_t o = 0; o < out_dim_; ++o) {
                sum += grad_output_data[o] * a_row[o];
            }
            projected_grad_data[r] = sum * scaling_;

            const float projected_input = projection_data[r] * scaling_;
            float* grad_a_row = grad_a_data + r * out_dim_;
            for (size_t o = 0; o < out_dim_; ++o) {
                grad_a_row[o] = projected_input * grad_output_data[o];
            }
        }

        for (size_t d = 0; d < in_dim_; ++d) {
            const float input_value = input_data[d];
            const float* b_row = b_data + d * rank_;
            float input_gradient = 0.0f;
            for (size_t r = 0; r < rank_; ++r) {
                const float rank_gradient = projected_grad_data[r];
                grad_b_data[d * rank_ + r] = input_value * rank_gradient;
                input_gradient += rank_gradient * b_row[r];
            }
            grad_input_data[d] = input_gradient;
        }
        return grad_input;
    }

    // Compute (grad_output * scaling) @ A.T without constructing A.T.
    for (size_t b = 0; b < batch; ++b) {
        for (size_t r = 0; r < rank_; ++r) {
            float sum = 0.0f;
            for (size_t o = 0; o < out_dim_; ++o) {
                sum += grad_output[b * out_dim_ + o] * (*A_)[r * out_dim_ + o];
            }
            projected_grad[b * rank_ + r] = sum * scaling_;
        }
    }

    // grad_A = cached(input @ B).T @ grad_output * scaling.
    for (size_t r = 0; r < rank_; ++r) {
        for (size_t o = 0; o < out_dim_; ++o) {
            float sum = 0.0f;
            for (size_t b = 0; b < batch; ++b) {
                sum += (*cached_BA_)[b * rank_ + r] * grad_output[b * out_dim_ + o];
            }
            grad_A[r * out_dim_ + o] = sum * scaling_;
        }
    }

    // grad_B = input.T @ projected_grad; grad_input = projected_grad @ B.T.
    for (size_t d = 0; d < in_dim_; ++d) {
        for (size_t r = 0; r < rank_; ++r) {
            float sum = 0.0f;
            for (size_t b = 0; b < batch; ++b) {
                sum += (*cached_input_)[b * in_dim_ + d] * projected_grad[b * rank_ + r];
            }
            grad_B[d * rank_ + r] = sum;
        }
    }
    for (size_t b = 0; b < batch; ++b) {
        for (size_t d = 0; d < in_dim_; ++d) {
            float sum = 0.0f;
            for (size_t r = 0; r < rank_; ++r) {
                sum += projected_grad[b * rank_ + r] * (*B_)[d * rank_ + r];
            }
            grad_input[b * in_dim_ + d] = sum;
        }
    }

    return grad_input;
}

/**
 * @brief Parameters.
 * @return Return value.
 * @details Calls: get().
 */
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

/**
 * @brief Set weights.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: shape(), clone(), spdlog::debug().
 */
void LoRALayer::set_weights(const Tensor& B, const Tensor& A) {
    if (B.shape() != B_->shape() || A.shape() != A_->shape()) {
        throw std::invalid_argument("Weight shapes do not match");
    }
    // Replace underlying tensors using make_unique for proper RAII
    B_ = std::make_unique<Tensor>(B.clone());
    A_ = std::make_unique<Tensor>(A.clone());
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
    
    if (dim == 0 || rank == 0) {
        throw std::invalid_argument(
            "AttentionLoRA: dim and rank must be > 0 (got dim=" +
            std::to_string(dim) + ", rank=" + std::to_string(rank) + ")");
    }
    
    spdlog::debug("Created AttentionLoRA: dim={}, rank={}, q={}, k={}, v={}, o={}",
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
    
    spdlog::debug("AttentionLoRA: Initialized with {} parameters", parameter_count());
}

/**
 * @brief Forward.
 * @param[in] input Input parameter.
 * @return Return value.
 * @details Calls: spdlog::debug(), shape(), size(), clone().
 */
Tensor AttentionLoRA::forward(const Tensor& input) {
    spdlog::debug("AttentionLoRA: forward with input shape ({}, {})",
                  input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
    
    Tensor output;
    bool has_output = false;

    // Apply LoRA layers sequentially (simplified version). The first layer can
    // consume input directly because each LoRALayer caches its own backward input.
    if (apply_to_q_ && q_lora_) {
        output = q_lora_->forward(input);
        has_output = true;
    }
    if (apply_to_k_ && k_lora_) {
        if (has_output) {
            output = k_lora_->forwardOwned(std::move(output));
        } else {
            output = k_lora_->forward(input);
        }
        has_output = true;
    }
    if (apply_to_v_ && v_lora_) {
        if (has_output) {
            output = v_lora_->forwardOwned(std::move(output));
        } else {
            output = v_lora_->forward(input);
        }
        has_output = true;
    }
    if (apply_to_o_ && o_lora_) {
        if (has_output) {
            output = o_lora_->forwardOwned(std::move(output));
        } else {
            output = o_lora_->forward(input);
        }
        has_output = true;
    }

    return has_output ? std::move(output) : input.clone();
}

/**
 * @brief Backward.
 * @param[in] grad_output Input parameter.
 * @return Return value.
 * @details Calls: spdlog::debug(), shape(), size(), clone().
 */
Tensor AttentionLoRA::backward(const Tensor& grad_output) {
    spdlog::debug("AttentionLoRA: backward with grad_output shape ({}, {})",
                  grad_output.shape()[0], grad_output.shape().size() > 1 ? grad_output.shape()[1] : 0);
    
    Tensor grad;
    bool has_grad = false;

    // Backward pass in reverse order
    if (apply_to_o_ && o_lora_) {
        grad = o_lora_->backward(has_grad ? grad : grad_output);
        has_grad = true;
    }
    if (apply_to_v_ && v_lora_) {
        grad = v_lora_->backward(has_grad ? grad : grad_output);
        has_grad = true;
    }
    if (apply_to_k_ && k_lora_) {
        grad = k_lora_->backward(has_grad ? grad : grad_output);
        has_grad = true;
    }
    if (apply_to_q_ && q_lora_) {
        grad = q_lora_->backward(has_grad ? grad : grad_output);
        has_grad = true;
    }

    return has_grad ? std::move(grad) : grad_output.clone();
}

/**
 * @brief Parameters.
 * @return Return value.
 * @details Calls: insert(), end(), begin(), spdlog::debug(), size().
 */
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
    
    spdlog::debug("AttentionLoRA: Collected {} parameters",params.size());
    return params;
}

size_t AttentionLoRA::parameter_count() const {
    size_t count = 0;
    if (apply_to_q_) {
      count += (dim_ * rank_) + (rank_ * dim_);
    }
    if (apply_to_k_) {
      count += (dim_ * rank_) + (rank_ * dim_);
    }
    if (apply_to_v_) {
      count += (dim_ * rank_) + (rank_ * dim_);
    }
    if (apply_to_o_) {
      count += (dim_ * rank_) + (rank_ * dim_);
    }
    return count;
}

size_t AttentionLoRA::memory_bytes() const {
    return parameter_count() * sizeof(float);
}

/**
 * @brief ===== Sequential =====
 * @param[in] layer Input parameter.
 * @details Calls: spdlog::info(), name(), push_back(), std::move().
 */

void Sequential::add(std::unique_ptr<ITrainableLayer> layer) {
    spdlog::debug("Sequential: Adding layer {}", layer->name());
    layers_.push_back(std::move(layer));
}

/**
 * @brief Forward.
 * @param[in] input Input parameter.
 * @return Return value.
 * @details Calls: spdlog::debug(), size(), clone().
 */
Tensor Sequential::forward(const Tensor& input) {
    spdlog::debug("Sequential: forward through {} layers",layers_.size());

    if (layers_.empty()) {
        return input.clone();
    }

    Tensor output = layers_.front()->forward(input);
    for (size_t i = 1; i < layers_.size(); ++i) {
        output = layers_[i]->forward(output);
    }
    
    return output;
}

/**
 * @brief Backward.
 * @param[in] grad_output Input parameter.
 * @return Return value.
 * @details Calls: spdlog::debug(), size(), clone(), rbegin(), rend().
 */
Tensor Sequential::backward(const Tensor& grad_output) {
    spdlog::debug("Sequential: backward through {} layers",layers_.size());

    if (layers_.empty()) {
        return grad_output.clone();
    }

    Tensor grad = layers_.back()->backward(grad_output);
    for (auto it = layers_.rbegin() + 1; it != layers_.rend(); ++it) {
        grad = (*it)->backward(grad);
    }
    
    return grad;
}

/**
 * @brief Parameters.
 * @return Return value.
 * @details Calls: spdlog::debug(), size(), insert(), end(), begin().
 */
std::vector<Tensor*> Sequential::parameters() {
    spdlog::debug("Sequential: collecting parameters from {} layers",layers_.size());
    
    std::vector<Tensor*> params = {};

    for (auto& layer : layers_) {
        auto layer_params = layer->parameters();
        params.insert(params.end(), layer_params.begin(), layer_params.end());
    }
    
    spdlog::debug("Sequential: Collected {} total parameters",params.size());
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

/**
 * @brief Add parameters.
 * @param[in] params Input parameter.
 * @details Calls: insert(), end(), begin(), spdlog::debug(), size().
 */
void SGDOptimizer::add_parameters(const std::vector<Tensor*>& params) {
    parameters_.insert(parameters_.end(), params.begin(), params.end());
    spdlog::debug("SGDOptimizer: Added {} parameters, total={}", 
                  params.size(),parameters_.size());
}

/**
 * @brief Step.
 * @details Calls: find(), end(), Tensor(), shape(), data(), size(), spdlog::debug().
 */
void SGDOptimizer::step() {
    for (auto* param : parameters_) {
        if (!param || !param->requires_grad) {
            continue;
        }
        
        // Apply momentum if specified
        if (momentum_ > 0.0f) {
            // Initialize momentum buffer if not exists
            if (momentum_buffers_.find(param) == momentum_buffers_.end()) {
                momentum_buffers_[param] = Tensor(param->shape(), 0.0f);
            }
            
            Tensor& momentum_buffer = momentum_buffers_[param];
            
            // Update momentum: v = momentum * v + grad (with optional weight decay)
            // Update parameters: param = param - lr * v (with optional weight decay)
            for (size_t i = 0; i < momentum_buffer.data().size(); ++i) {
                float grad_with_decay = param->grad ? (*param->grad)[i] : 0.0f;
                if (weight_decay_ > 0.0f) {
                    grad_with_decay += weight_decay_ * param->data()[i];
                }
                momentum_buffer[i] = momentum_ * momentum_buffer[i] + grad_with_decay;
                param->data()[i] -= learning_rate_ * momentum_buffer[i];
            }
        } else {
            // Standard SGD: param = param - lr * grad (with optional weight decay)
            for (size_t i = 0; i < param->data().size(); ++i) {
                float grad_with_decay = param->grad ? (*param->grad)[i] : 0.0f;
                if (weight_decay_ > 0.0f) {
                    grad_with_decay += weight_decay_ * param->data()[i];
                }
                param->data()[i] -= learning_rate_ * grad_with_decay;
            }
        }
    }
    
    spdlog::debug("SGDOptimizer: Updated {} parameters",parameters_.size());
}

/**
 * @brief Zero grad.
 * @details Calls: zero(), spdlog::debug(), size().
 */
void SGDOptimizer::zero_grad() {
    for (auto* param : parameters_) {
        if (param && param->requires_grad && param->grad) {
            param->grad->zero();
        }
    }
    spdlog::debug("SGDOptimizer: Zeroed gradients for {} parameters",parameters_.size());
}

// ===== Adam Optimizer =====

AdamOptimizer::AdamOptimizer(float learning_rate, float beta1, float beta2, float epsilon, float weight_decay)
    : learning_rate_(learning_rate)
    , beta1_(beta1)
    , beta2_(beta2)
    , epsilon_(epsilon)
    , weight_decay_(weight_decay)
    , step_count_(0) {
    spdlog::info("Created AdamOptimizer: lr={}, beta1={}, beta2={}, epsilon={}, weight_decay={}",
                 learning_rate_, beta1_, beta2_, epsilon_, weight_decay_);
}

/**
 * @brief Add parameters.
 * @param[in] params Input parameter.
 * @details Calls: insert(), end(), begin(), spdlog::debug(), size().
 */
void AdamOptimizer::add_parameters(const std::vector<Tensor*>& params) {
    parameters_.insert(parameters_.end(), params.begin(), params.end());
    spdlog::debug("AdamOptimizer: Added {} parameters, total={}",
                  params.size(),parameters_.size());
}

/**
 * @brief Step.
 * @details Calls: std::pow(), themis::utils::conversion::clamp_double_to_float(), themis::utils::conversion::safe_size_to_int32(), find(), end(), Tensor(), shape(), data().
 */
void AdamOptimizer::step() {
    step_count_++;
    
    // Compute bias correction terms
    float bias_correction1 = 1.0f - static_cast<float>(std::pow(themis::utils::conversion::clamp_double_to_float(beta1_), 
                                                               themis::utils::conversion::safe_size_to_int32(step_count_)));
    float bias_correction2 = 1.0f - static_cast<float>(std::pow(themis::utils::conversion::clamp_double_to_float(beta2_), 
                                                               themis::utils::conversion::safe_size_to_int32(step_count_)));
    
    for (auto* param : parameters_) {
        if (!param || !param->requires_grad) {
            continue;
        }
        
        // Initialize moment buffers if not exists
        if (m_buffers_.find(param) == m_buffers_.end()) {
            m_buffers_[param] = Tensor(param->shape(), 0.0f);
        }
        if (v_buffers_.find(param) == v_buffers_.end()) {
            v_buffers_[param] = Tensor(param->shape(), 0.0f);
        }
        
        Tensor& m = m_buffers_[param];
        Tensor& v = v_buffers_[param];
        
        // Update parameters using Adam algorithm
        for (size_t i = 0; i < param->data().size(); ++i) {
            float grad = param->grad ? (*param->grad)[i] : 0.0f;
            
            // Apply weight decay to gradient (L2 regularization)
            if (weight_decay_ > 0.0f) {
                grad += weight_decay_ * param->data()[i];
            }
            
            // Update biased first moment estimate (momentum)
            // m_t = β1 * m_{t-1} + (1 - β1) * g_t
            m[i] = beta1_ * m[i] + (1.0f - beta1_) * grad;
            
            // Update biased second moment estimate (RMSprop)
            // v_t = β2 * v_{t-1} + (1 - β2) * g_t²
            v[i] = beta2_ * v[i] + (1.0f - beta2_) * grad * grad;
            
            // Compute bias-corrected first moment estimate
            // m̂_t = m_t / (1 - β1^t)
            float m_hat = m[i] / bias_correction1;
            
            // Compute bias-corrected second moment estimate
            // v̂_t = v_t / (1 - β2^t)
            float v_hat = v[i] / bias_correction2;
            
            // Update parameters
            // θ_t = θ_{t-1} - α * m̂_t / (√v̂_t + ε)
            param->data()[i] -= learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);
        }
    }
    
    spdlog::debug("AdamOptimizer: Updated {} parameters at step {}",parameters_.size(), step_count_);
}

/**
 * @brief Zero grad.
 * @details Calls: zero(), spdlog::debug(), size().
 */
void AdamOptimizer::zero_grad() {
    for (auto* param : parameters_) {
        if (param && param->requires_grad) {
            if (param->grad) {
                param->grad->zero();
            }
        }
    }
    spdlog::debug("AdamOptimizer: Zeroed gradients for {} parameters",parameters_.size());
}

// ===== AdamW Optimizer =====

AdamWOptimizer::AdamWOptimizer(float learning_rate, float beta1, float beta2, float epsilon, float weight_decay)
    : learning_rate_(learning_rate)
    , beta1_(beta1)
    , beta2_(beta2)
    , epsilon_(epsilon)
    , weight_decay_(weight_decay)
    , step_count_(0) {
    spdlog::info("Created AdamWOptimizer: lr={}, beta1={}, beta2={}, epsilon={}, weight_decay={}",
                 learning_rate_, beta1_, beta2_, epsilon_, weight_decay_);
}

/**
 * @brief Add parameters.
 * @param[in] params Input parameter.
 * @details Calls: insert(), end(), begin(), spdlog::debug(), size().
 */
void AdamWOptimizer::add_parameters(const std::vector<Tensor*>& params) {
    parameters_.insert(parameters_.end(), params.begin(), params.end());
    spdlog::debug("AdamWOptimizer: Added {} parameters, total={}",
                  params.size(),parameters_.size());
}

/**
 * @brief Step.
 * @details Calls: std::pow(), find(), end(), Tensor(), shape(), data(), size(), std::sqrt().
 */
void AdamWOptimizer::step() {
    step_count_++;
    
    // Compute bias correction terms
    float bias_correction1 = 1.0f - static_cast<float>(std::pow(beta1_, step_count_));
    float bias_correction2 = 1.0f - static_cast<float>(std::pow(beta2_, step_count_));
    
    for (auto* param : parameters_) {
        if (!param || !param->requires_grad) {
            continue;
        }
        
        // Initialize moment buffers if not exists
        if (m_buffers_.find(param) == m_buffers_.end()) {
            m_buffers_[param] = Tensor(param->shape(), 0.0f);
        }
        if (v_buffers_.find(param) == v_buffers_.end()) {
            v_buffers_[param] = Tensor(param->shape(), 0.0f);
        }
        
        Tensor& m = m_buffers_[param];
        Tensor& v = v_buffers_[param];
        
        // Update parameters using AdamW algorithm (decoupled weight decay)
        for (size_t i = 0; i < param->data().size(); ++i) {
            float grad = param->grad ? (*param->grad)[i] : 0.0f;
            
            // Update biased first moment estimate (momentum)
            // m_t = β1 * m_{t-1} + (1 - β1) * g_t
            m[i] = beta1_ * m[i] + (1.0f - beta1_) * grad;
            
            // Update biased second moment estimate (RMSprop)
            // v_t = β2 * v_{t-1} + (1 - β2) * g_t²
            v[i] = beta2_ * v[i] + (1.0f - beta2_) * grad * grad;
            
            // Compute bias-corrected first moment estimate
            // m̂_t = m_t / (1 - β1^t)
            float m_hat = m[i] / bias_correction1;
            
            // Compute bias-corrected second moment estimate
            // v̂_t = v_t / (1 - β2^t)
            float v_hat = v[i] / bias_correction2;
            
            // AdamW update with decoupled weight decay
            // θ_t = θ_{t-1} - α * (m̂_t / (√v̂_t + ε) + λ * θ_{t-1})
            float adam_update = learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);
            float weight_decay_update = learning_rate_ * weight_decay_ * param->data()[i];
            
            param->data()[i] -= adam_update + weight_decay_update;
        }
    }
    
    spdlog::debug("AdamWOptimizer: Updated {} parameters at step {}",parameters_.size(), step_count_);
}

/**
 * @brief Zero grad.
 * @details Calls: zero(), spdlog::debug(), size().
 */
void AdamWOptimizer::zero_grad() {
    for (auto* param : parameters_) {
        if (param && param->requires_grad && param->grad) {
            param->grad->zero();
        }
    }
    spdlog::debug("AdamWOptimizer: Zeroed gradients for {} parameters",parameters_.size());
}

} // namespace lora
} // namespace llm
} // namespace themis
