#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstddef>

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
    virtual ~ITrainableLayer() = default;
    
    // Forward pass
    virtual Tensor forward(const Tensor& input) = 0;
    
    // Backward pass (gradient computation)
    virtual Tensor backward(const Tensor& grad_output) = 0;
    
    // Parameter access
    virtual std::vector<Tensor*> parameters() = 0;
    
    // Layer metadata
    virtual std::string name() const = 0;
    virtual size_t parameter_count() const = 0;
    virtual size_t memory_bytes() const = 0;
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
    void set_weights(const Tensor& B, const Tensor& A);

private:
    std::string name_;
    size_t in_dim_;
    size_t out_dim_;
    size_t rank_;
    float scaling_;
    
    // Trainable parameters (B and A matrices)
    std::unique_ptr<Tensor> B_;  // (in_dim, rank)
    std::unique_ptr<Tensor> A_;  // (rank, out_dim)
    
    // Cached for backward pass
    Tensor cached_input_;
    Tensor cached_BA_;
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
    
    size_t dim_;
    size_t rank_;
    bool apply_to_q_;
    bool apply_to_k_;
    bool apply_to_v_;
    bool apply_to_o_;
};

/**
 * @brief Sequential Container (Composite)
 */
class Sequential : public ITrainableLayer {
public:
    Sequential() = default;
    ~Sequential() override = default;
    
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
    
    // Constructor with shape (allocates memory)
    explicit Tensor(const std::vector<size_t>& shape);
    
    // Constructor with shape and initial value
    Tensor(const std::vector<size_t>& shape, float value);
    
    // Getters
    const std::vector<size_t>& shape() const { return shape_; }
    size_t size() const;
    const std::vector<float>& data() const { return data_; }
    std::vector<float>& data() { return data_; }
    
    // Element access
    float& operator[](size_t idx) { return data_[idx]; }
    const float& operator[](size_t idx) const { return data_[idx]; }
    
    // Basic operations
    Tensor operator+(const Tensor& other) const;
    Tensor operator-(const Tensor& other) const;
    Tensor operator*(float scalar) const;
    
    // Matrix multiplication
    Tensor matmul(const Tensor& other) const;
    
    // Transpose (for 2D tensors)
    Tensor transpose() const;
    
    // Utilities
    void fill(float value);
    void zero();
    Tensor clone() const;
    
    // Gradient storage (for training)
    Tensor grad;
    bool requires_grad = false;

private:
    std::vector<size_t> shape_;
    std::vector<float> data_;
};

// Tensor utility functions
namespace tensor_utils {
    // Random initialization
    Tensor randn(const std::vector<size_t>& shape, float mean = 0.0f, float std = 1.0f);
    Tensor xavier_uniform(const std::vector<size_t>& shape);
    Tensor kaiming_uniform(const std::vector<size_t>& shape, float a = 0.0f);
    
    // Zero initialization
    Tensor zeros(const std::vector<size_t>& shape);
    Tensor ones(const std::vector<size_t>& shape);
} // namespace tensor_utils

} // namespace lora
} // namespace llm
} // namespace themis
