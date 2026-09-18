/**
 * @file lora_layers.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstddef>
#include <unordered_map>

namespace themis {
namespace llm {
namespace lora {

// Forward declaration for tensor type (to be defined in production PR)
class Tensor;

/**
 * @brief Abstract Base for trainable layers
 * 
 * Design Pattern: Composite Pattern
 * Allows treating individual layers and compositions uniformly
 */
class ITrainableLayer {
public:
    /**
     * @brief TBD: Describe ~ITrainableLayer.
     * @return Return value.
     */
    virtual ~ITrainableLayer() = default;
    
    // Forward pass
    [[nodiscard]] virtual Tensor forward(const Tensor& input) = 0;
    
    // Backward pass (gradient computation)
    [[nodiscard]] virtual Tensor backward(const Tensor& grad_output) = 0;
    
    // Parameter access
    [[nodiscard]] virtual std::vector<Tensor*> parameters() = 0;
    
    // Layer metadata
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual size_t parameter_count() const = 0;
    [[nodiscard]] virtual size_t memory_bytes() const = 0;
};

/**
 * @brief LoRA Layer (Low-Rank Adaptation)
 * 
 * W' = W + (B @ A) * scaling
 * B: (in_dim, rank)
 * A: (rank, out_dim)
 */
class LoRALayer : public ITrainableLayer {
public:
    LoRALayer(size_t in_dim, size_t out_dim, size_t rank, float scaling = 1.0f);
    ~LoRALayer() override = default;
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    
    std::vector<Tensor*> parameters() override;
    
    std::string name() const override { return name_; }
    size_t parameter_count() const override;
    size_t memory_bytes() const override;
    
    // Export weights (for storage)
    std::pair<Tensor, Tensor> get_weights() const;
    /**
     * @brief TBD: Describe set_weights.
     * @param[in] B Input parameter.
     * @param[in] A Input parameter.
     */
    void set_weights(const Tensor& B, const Tensor& A);

private:
    std::string name_;
    size_t in_dim_ = 0;
    size_t out_dim_ = 0;
    size_t rank_ = 0;
    float scaling_ = 1.0f;
    
    // Trainable parameters (B and A matrices)
    std::unique_ptr<Tensor> B_;  // (in_dim, rank)
    std::unique_ptr<Tensor> A_;  // (rank, out_dim)
    
    // Cached for backward pass
    std::unique_ptr<Tensor> cached_input_;
    std::unique_ptr<Tensor> cached_BA_;
};

/**
 * @brief Attention-LoRA (LoRA for Attention Weights)
 */
class AttentionLoRA : public ITrainableLayer {
public:
    AttentionLoRA(size_t dim, size_t rank, 
                  bool apply_to_q = true,
                  bool apply_to_k = true,
                  bool apply_to_v = true,
                  bool apply_to_o = true);
    ~AttentionLoRA() override = default;
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    
    std::vector<Tensor*> parameters() override;
    
    std::string name() const override { return "AttentionLoRA"; }
    size_t parameter_count() const override;
    size_t memory_bytes() const override;

private:
    // Query, Key, Value, Output projections
    std::unique_ptr<LoRALayer> q_lora_;
    std::unique_ptr<LoRALayer> k_lora_;
    std::unique_ptr<LoRALayer> v_lora_;
    std::unique_ptr<LoRALayer> o_lora_;
    
    size_t dim_ = 0;
    size_t rank_ = 0;
    bool apply_to_q_ = true;
    bool apply_to_k_ = true;
    bool apply_to_v_ = true;
    bool apply_to_o_ = true;
};

/**
 * @brief Sequential Container (Composite)
 */
class Sequential : public ITrainableLayer {
public:
    Sequential() = default;
    ~Sequential() override = default;
    
    /**
     * @brief TBD: Describe add.
     * @param[in] layer Input parameter.
     */
    void add(std::unique_ptr<ITrainableLayer> layer);
    
    Tensor forward(const Tensor& input) override;
    Tensor backward(const Tensor& grad_output) override;
    
    std::vector<Tensor*> parameters() override;
    
    std::string name() const override { return "Sequential"; }
    size_t parameter_count() const override;
    size_t memory_bytes() const override;

private:
    std::vector<std::unique_ptr<ITrainableLayer>> layers_;
};

/**
 * @brief Tensor class for LoRA training
 * 
 * Supports basic tensor operations needed for training.
 * CPU-only implementation; GPU support can be added in future PRs.
 */
class Tensor {
public:
    Tensor() = default;
    
    /**
     * @brief Constructor with shape (allocates memory)
     * @param[in] shape Input parameter.
     * @return Return value.
     */
    explicit Tensor(const std::vector<size_t>& shape);
    
    // Constructor with shape and initial value
    Tensor(const std::vector<size_t>& shape, float value);
    
    // Getters
    const std::vector<size_t>& shape() const { return shape_; }
    /**
     * @brief TBD: Describe size.
     * @return Return value.
     */
    size_t size() const;
    const std::vector<float>& data() const { return data_; }
    /**
     * @brief TBD: Describe data.
     * @return Return value.
     * @details Implements data without additional internal calls.
     */
    std::vector<float>& data() { return data_; }
    
    // Element access
    float& operator[](size_t idx) { return data_[idx]; }
    const float& operator[](size_t idx) const { return data_[idx]; }
    
    // Basic operations
    Tensor operator+(const Tensor& other) const;
    Tensor operator-(const Tensor& other) const;
    Tensor operator*(float scalar) const;
    
    /**
     * @brief Matrix multiplication
     * @param[in] other Input parameter.
     * @return Return value.
     */
    Tensor matmul(const Tensor& other) const;
    
    /**
     * @brief Transpose (for 2D tensors)
     * @return Return value.
     */
    Tensor transpose() const;
    
    /**
     * @brief Utilities
     * @param[in] value Input parameter.
     */
    void fill(float value);
    /**
     * @brief TBD: Describe zero.
     */
    void zero();
    /**
     * @brief TBD: Describe clone.
     * @return Return value.
     */
    Tensor clone() const;
    
    // Gradient storage (for training)
    std::unique_ptr<Tensor> grad;
    bool requires_grad = false;

private:
    std::vector<size_t> shape_;
    std::vector<float> data_;
};

// Tensor utility functions
namespace tensor_utils {
    // Random initialization
    Tensor randn(const std::vector<size_t>& shape, float mean = 0.0f, float std = 1.0f);
    /**
     * @brief TBD: Describe xavier_uniform.
     * @param[in] shape Input parameter.
     * @return Return value.
     */
    Tensor xavier_uniform(const std::vector<size_t>& shape);
    Tensor kaiming_uniform(const std::vector<size_t>& shape, float a = 0.0f);
    
    /**
     * @brief Zero initialization
     * @param[in] shape Input parameter.
     * @return Return value.
     */
    Tensor zeros(const std::vector<size_t>& shape);
    /**
     * @brief TBD: Describe ones.
     * @param[in] shape Input parameter.
     * @return Return value.
     */
    Tensor ones(const std::vector<size_t>& shape);
} // namespace tensor_utils

/**
 * @brief Simple SGD optimizer for LoRA training
 * 
 * Implements basic Stochastic Gradient Descent with momentum (optional).
 * Can be extended to Adam in future PRs.
 */
class SGDOptimizer {
public:
    explicit SGDOptimizer(float learning_rate = 0.001f, float momentum = 0.0f, float weight_decay = 0.0f);
    
    /**
     * @brief Register parameters to optimize
     * @param[in] params Input parameter.
     */
    void add_parameters(const std::vector<Tensor*>& params);
    
    /**
     * @brief Perform optimization step (update parameters using gradients)
     */
    void step();
    
    /**
     * @brief Zero out all gradients
     */
    void zero_grad();
    
    // Getters/Setters
    float learning_rate() const { return learning_rate_; }
    /**
     * @brief TBD: Describe set_learning_rate.
     * @param[in] lr Input parameter.
     * @details Implements set_learning_rate without additional internal calls.
     */
    void set_learning_rate(float lr) { learning_rate_ = lr; }

private:
    float learning_rate_ = 0.0f;
    float momentum_ = 0.0f;
    float weight_decay_ = 0.0f;
    std::vector<Tensor*> parameters_;
    
    // Momentum buffers (for momentum > 0)
    std::unordered_map<Tensor*, Tensor> momentum_buffers_;
};

/**
 * @brief Adam (Adaptive Moment Estimation) optimizer
 * 
 * Implements Adam optimization algorithm with adaptive learning rates.
 * Reference: https://arxiv.org/abs/1412.6980
 * 
 * Update rule:
 * m_t = β1 * m_{t-1} + (1 - β1) * g_t        // First moment
 * v_t = β2 * v_{t-1} + (1 - β2) * g_t²       // Second moment
 * m̂_t = m_t / (1 - β1^t)                     // Bias-corrected first moment
 * v̂_t = v_t / (1 - β2^t)                     // Bias-corrected second moment
 * θ_t = θ_{t-1} - α * m̂_t / (√v̂_t + ε)     // Parameter update
 */
class AdamOptimizer {
public:
    /**
     * @brief TBD: Describe ~AdamOptimizer.
     * @return Return value.
     */
    virtual ~AdamOptimizer() = default;
    /**
     * @brief Construct Adam optimizer
     * @param learning_rate Learning rate (α), default 1e-4
     * @param beta1 Exponential decay rate for first moment (β1), default 0.9
     * @param beta2 Exponential decay rate for second moment (β2), default 0.999
     * @param epsilon Numerical stability constant (ε), default 1e-8
     * @param weight_decay Weight decay (L2 penalty), default 0.0
     */
    explicit AdamOptimizer(
        float learning_rate = 1e-4f,
        float beta1 = 0.9f,
        float beta2 = 0.999f,
        float epsilon = 1e-8f,
        float weight_decay = 0.0f
    );
    
    /**
     * @brief Register parameters to optimize
     * @param[in] params Input parameter.
     */
    void add_parameters(const std::vector<Tensor*>& params);
    
    /**
     * @brief Perform optimization step (update parameters using gradients)
     */
    void step();
    
    /**
     * @brief Zero out all gradients
     */
    void zero_grad();
    
    // Getters/Setters
    float learning_rate() const { return learning_rate_; }
    /**
     * @brief TBD: Describe set_learning_rate.
     * @param[in] lr Input parameter.
     * @details Implements set_learning_rate without additional internal calls.
     */
    void set_learning_rate(float lr) { learning_rate_ = lr; }
    int step_count() const { return step_count_; }

private:
    float learning_rate_ = 0.0f;
    float beta1_ = 0.0f;
    float beta2_ = 0.0f;
    float epsilon_ = 0.0f;
    float weight_decay_ = 0.0f;
    int step_count_ = 0;
    std::vector<Tensor*> parameters_;
    
    // First moment estimates (momentum)
    std::unordered_map<Tensor*, Tensor> m_buffers_;
    
    // Second moment estimates (RMSprop)
    std::unordered_map<Tensor*, Tensor> v_buffers_;
};

/**
 * @brief AdamW optimizer (Adam with decoupled weight decay)
 * 
 * Implements AdamW variant with proper weight decay decoupling.
 * Reference: https://arxiv.org/abs/1711.05101
 * 
 * Better generalization than standard Adam for LLM fine-tuning.
 * Weight decay is applied directly to parameters, not through gradients.
 */
class AdamWOptimizer {
public:
    /**
     * @brief TBD: Describe ~AdamWOptimizer.
     * @return Return value.
     */
    virtual ~AdamWOptimizer() = default;
    /**
     * @brief Construct AdamW optimizer
     * @param learning_rate Learning rate (α), default 1e-4
     * @param beta1 Exponential decay rate for first moment (β1), default 0.9
     * @param beta2 Exponential decay rate for second moment (β2), default 0.999
     * @param epsilon Numerical stability constant (ε), default 1e-8
     * @param weight_decay Decoupled weight decay (λ), default 0.01
     */
    explicit AdamWOptimizer(
        float learning_rate = 1e-4f,
        float beta1 = 0.9f,
        float beta2 = 0.999f,
        float epsilon = 1e-8f,
        float weight_decay = 0.01f
    );
    
    /**
     * @brief Register parameters to optimize
     * @param[in] params Input parameter.
     */
    void add_parameters(const std::vector<Tensor*>& params);
    
    /**
     * @brief Perform optimization step (update parameters using gradients)
     */
    void step();
    
    /**
     * @brief Zero out all gradients
     */
    void zero_grad();
    
    // Getters/Setters
    float learning_rate() const { return learning_rate_; }
    /**
     * @brief TBD: Describe set_learning_rate.
     * @param[in] lr Input parameter.
     * @details Implements set_learning_rate without additional internal calls.
     */
    void set_learning_rate(float lr) { learning_rate_ = lr; }
    int step_count() const { return step_count_; }

private:
    float learning_rate_ = 0.0f;
    float beta1_ = 0.0f;
    float beta2_ = 0.0f;
    float epsilon_ = 0.0f;
    float weight_decay_ = 0.0f;
    int step_count_ = 0;
    std::vector<Tensor*> parameters_;
    
    // First moment estimates (momentum)
    std::unordered_map<Tensor*, Tensor> m_buffers_;
    
    // Second moment estimates (RMSprop)
    std::unordered_map<Tensor*, Tensor> v_buffers_;
};

} // namespace lora
} // namespace llm
} // namespace themis
