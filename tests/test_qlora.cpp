#include <gtest/gtest.h>
#include "llm/lora_framework/quantized_model.h"
#include "llm/lora_framework/lora_layers.h"
#include <memory>
#include <cmath>

using namespace themis::llm::lora;

/**
 * @file test_qlora.cpp
 * @brief Tests for QLoRA (Quantized LoRA) training
 */

class QLoRATest : public ::testing::Test {
protected:
    void SetUp() override {
        // Standard dimensions for testing
        in_dim_ = 128;
        out_dim_ = 128;
        rank_ = 8;
        
        // Create test configuration
        config_.quantization_type = QuantizationType::NF4;
        config_.block_size = 64;
        config_.use_double_quantization = false;
        config_.layer_by_layer = true;
    }
    
    size_t in_dim_;
    size_t out_dim_;
    size_t rank_;
    QuantizedModelConfig config_;
};

// ===== QuantizedLayerWeights Tests =====

TEST_F(QLoRATest, QuantizedLayerWeights_Construction) {
    // Create a test tensor
    Tensor weights = tensor_utils::randn({in_dim_, out_dim_});
    
    // Quantize it
    QuantizedLayerWeights q_weights(weights, config_);
    
    // Check memory reduction
    size_t original_bytes = weights.size() * sizeof(float);
    size_t quantized_bytes = q_weights.memory_bytes();
    
    EXPECT_LT(quantized_bytes, original_bytes / 2);
    
    float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
    EXPECT_GT(reduction, 0.60f) << "Reduction: " << (reduction * 100) << "%";
}

TEST_F(QLoRATest, QuantizedLayerWeights_Dequantization) {
    // Create and quantize
    Tensor weights = tensor_utils::randn({in_dim_, out_dim_}, 0.0f, 1.0f);
    QuantizedLayerWeights q_weights(weights, config_);
    
    // Dequantize
    Tensor reconstructed = q_weights.dequantize();
    
    // Check shape preservation
    EXPECT_EQ(reconstructed.shape(), weights.shape());
    
    // Check approximate reconstruction
    float mse = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
        float diff = weights[i] - reconstructed[i];
        mse += diff * diff;
    }
    mse /= weights.size();
    
    // NF4 should have reasonable reconstruction error
    EXPECT_LT(mse, 0.05f) << "MSE: " << mse;
}

// ===== QuantizedModel Tests =====

TEST_F(QLoRATest, QuantizedModel_AddLayers) {
    QuantizedModel model(config_);
    
    // Add multiple layers
    Tensor layer1 = tensor_utils::randn({128, 128});
    Tensor layer2 = tensor_utils::randn({128, 256});
    Tensor layer3 = tensor_utils::randn({256, 128});
    
    model.add_layer("layer1", layer1);
    model.add_layer("layer2", layer2);
    model.add_layer("layer3", layer3);
    
    EXPECT_EQ(model.num_layers(), 3);
    
    auto names = model.layer_names();
    EXPECT_EQ(names.size(), 3);
}

TEST_F(QLoRATest, QuantizedModel_GetLayer) {
    QuantizedModel model(config_);
    Tensor weights = tensor_utils::randn({128, 128});
    model.add_layer("test_layer", weights);
    
    auto layer = model.get_layer("test_layer");
    EXPECT_NE(layer, nullptr);
    
    auto missing = model.get_layer("missing_layer");
    EXPECT_EQ(missing, nullptr);
}

TEST_F(QLoRATest, QuantizedModel_DequantizeLayer) {
    QuantizedModel model(config_);
    Tensor weights = tensor_utils::randn({128, 128});
    model.add_layer("test_layer", weights);
    
    Tensor dequantized = model.dequantize_layer("test_layer");
    EXPECT_EQ(dequantized.shape(), weights.shape());
}

TEST_F(QLoRATest, QuantizedModel_MemoryUsage) {
    QuantizedModel model(config_);
    
    // Add layers
    for (int i = 0; i < 5; ++i) {
        Tensor weights = tensor_utils::randn({128, 128});
        model.add_layer("layer" + std::to_string(i), weights);
    }
    
    size_t quantized_bytes = model.memory_bytes();
    size_t original_bytes = 5 * 128 * 128 * sizeof(float);
    
    // Should have significant memory reduction
    float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
    EXPECT_GT(reduction, 0.60f) << "Reduction: " << (reduction * 100) << "%";
}

// ===== QLoRALayer Tests =====

TEST_F(QLoRATest, QLoRALayer_Construction) {
    // Create without base weights
    QLoRALayer layer(in_dim_, out_dim_, rank_);
    
    EXPECT_EQ(layer.parameter_count(), (in_dim_ * rank_) + (rank_ * out_dim_));
    EXPECT_EQ(layer.name(), "QLoRALayer");
}

TEST_F(QLoRATest, QLoRALayer_ForwardWithoutBase) {
    // QLoRA layer without base weights (just LoRA)
    QLoRALayer layer(in_dim_, out_dim_, rank_);
    
    Tensor input({1, in_dim_});
    Tensor output = layer.forward(input);
    
    EXPECT_EQ(output.shape().size(), 2);
    EXPECT_EQ(output.shape()[0], 1);
    EXPECT_EQ(output.shape()[1], out_dim_);
}

TEST_F(QLoRATest, QLoRALayer_ForwardWithBase) {
    // Create quantized base weights
    Tensor base_weights = tensor_utils::randn({in_dim_, out_dim_}, 0.0f, 0.1f);
    auto q_weights = std::make_shared<QuantizedLayerWeights>(base_weights, config_);
    
    // Create QLoRA layer with base
    QLoRALayer layer(in_dim_, out_dim_, rank_, q_weights);
    
    Tensor input({1, in_dim_});
    input.fill(1.0f);  // Simple input for testing
    
    Tensor output = layer.forward(input);
    
    EXPECT_EQ(output.shape().size(), 2);
    EXPECT_EQ(output.shape()[0], 1);
    EXPECT_EQ(output.shape()[1], out_dim_);
}

TEST_F(QLoRATest, QLoRALayer_Backward) {
    QLoRALayer layer(in_dim_, out_dim_, rank_);
    
    // Forward pass
    Tensor input({1, in_dim_});
    input.fill(1.0f);
    Tensor output = layer.forward(input);
    
    // Backward pass
    Tensor grad_output({1, out_dim_});
    grad_output.fill(1.0f);
    Tensor grad_input = layer.backward(grad_output);
    
    // Check gradient shapes
    EXPECT_EQ(grad_input.shape(), input.shape());
    
    // Check that gradients were computed for parameters
    auto params = layer.parameters();
    EXPECT_EQ(params.size(), 2);  // B and A
    // Note: In real implementation, check params[0]->grad and params[1]->grad
}

TEST_F(QLoRATest, QLoRALayer_Parameters) {
    QLoRALayer layer(in_dim_, out_dim_, rank_);
    
    auto params = layer.parameters();
    EXPECT_EQ(params.size(), 2);  // B and A matrices
}

TEST_F(QLoRATest, QLoRALayer_MemoryReduction) {
    // Create quantized base weights
    Tensor base_weights = tensor_utils::randn({in_dim_, out_dim_});
    auto q_weights = std::make_shared<QuantizedLayerWeights>(base_weights, config_);
    
    // QLoRA layer
    QLoRALayer qlora_layer(in_dim_, out_dim_, rank_, q_weights);
    
    // Regular LoRA layer (for comparison)
    LoRALayer lora_layer(in_dim_, out_dim_, rank_);
    
    // QLoRA memory includes quantized base + LoRA adapters
    size_t qlora_bytes = qlora_layer.memory_bytes();
    
    // Regular LoRA would need full precision base (hypothetical)
    size_t full_base_bytes = in_dim_ * out_dim_ * sizeof(float);
    size_t lora_bytes = lora_layer.memory_bytes();
    size_t full_memory = full_base_bytes + lora_bytes;
    
    // QLoRA should use significantly less memory
    EXPECT_LT(qlora_bytes, full_memory / 2);
}

// ===== Training Tests =====

TEST_F(QLoRATest, QLoRALayer_SimpleTraining) {
    // Test that QLoRA can train (gradients flow correctly)
    QLoRALayer layer(32, 32, 4);
    SGDOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());
    
    // Simple training loop
    Tensor input({1, 32});
    input.fill(1.0f);
    Tensor target({1, 32});
    target.fill(0.5f);
    
    float initial_loss = 0.0f;
    float final_loss = 0.0f;
    
    for (int step = 0; step < 10; ++step) {
        optimizer.zero_grad();
        
        // Forward
        Tensor output = layer.forward(input);
        
        // Loss (MSE)
        Tensor diff = output - target;
        float loss = 0.0f;
        for (size_t i = 0; i < diff.size(); ++i) {
            loss += diff[i] * diff[i];
        }
        loss /= diff.size();
        
        if (step == 0) initial_loss = loss;
        if (step == 9) final_loss = loss;
        
        // Backward
        Tensor grad = diff * (2.0f / diff.size());
        layer.backward(grad);
        
        // Update
        optimizer.step();
    }
    
    // Loss should decrease
    EXPECT_LT(final_loss, initial_loss);
}

// ===== Utility Function Tests =====

TEST_F(QLoRATest, Utils_EstimateMemory_NF4) {
    size_t num_params = 1024 * 1024;  // 1M parameters
    
    size_t nf4_bytes = quantized_model_utils::estimate_memory_usage(
        num_params, QuantizationType::NF4, 64, false);
    
    size_t original_bytes = num_params * sizeof(float);
    
    // Should be roughly 1/8th (4 bits per param + overhead)
    EXPECT_LT(nf4_bytes, original_bytes / 4);
}

TEST_F(QLoRATest, Utils_EstimateMemory_INT8) {
    size_t num_params = 1024 * 1024;
    
    size_t int8_bytes = quantized_model_utils::estimate_memory_usage(
        num_params, QuantizationType::INT8, 64, false);
    
    size_t original_bytes = num_params * sizeof(float);
    
    // Should be roughly 1/4th (8 bits per param + overhead)
    EXPECT_LT(int8_bytes, original_bytes / 2);
}

TEST_F(QLoRATest, Utils_MemoryReduction) {
    size_t original_bytes = 1024 * 1024 * 4;  // 4MB
    
    float nf4_reduction = quantized_model_utils::calculate_memory_reduction(
        original_bytes, QuantizationType::NF4);
    
    float int8_reduction = quantized_model_utils::calculate_memory_reduction(
        original_bytes, QuantizationType::INT8);
    
    // NF4 should have higher reduction than INT8
    EXPECT_GT(nf4_reduction, int8_reduction);
    EXPECT_GT(nf4_reduction, 0.70f);
    EXPECT_GT(int8_reduction, 0.60f);
}

TEST_F(QLoRATest, Utils_ConvertToQuantized) {
    std::unordered_map<std::string, Tensor> weights;
    weights["layer1"] = tensor_utils::randn({64, 64});
    weights["layer2"] = tensor_utils::randn({64, 128});
    weights["layer3"] = tensor_utils::randn({128, 64});
    
    QuantizedModel model = quantized_model_utils::convert_to_quantized(weights, config_);
    
    EXPECT_EQ(model.num_layers(), 3);
    
    // Check memory reduction
    size_t original_bytes = (64*64 + 64*128 + 128*64) * sizeof(float);
    size_t quantized_bytes = model.memory_bytes();
    
    float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
    EXPECT_GT(reduction, 0.60f);
}

// ===== Integration Tests =====

TEST_F(QLoRATest, Integration_FullWorkflow) {
    // Test complete QLoRA workflow
    
    // 1. Create base model weights
    Tensor base_weights = tensor_utils::randn({64, 64}, 0.0f, 0.1f);
    
    // 2. Quantize
    auto q_weights = std::make_shared<QuantizedLayerWeights>(base_weights, config_);
    
    // 3. Create QLoRA layer
    QLoRALayer layer(64, 64, 4, q_weights);
    
    // 4. Setup optimizer
    SGDOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());
    
    // 5. Training step
    Tensor input({1, 64});
    input.fill(1.0f);
    
    optimizer.zero_grad();
    Tensor output = layer.forward(input);
    
    // Create dummy gradient
    Tensor grad({1, 64});
    grad.fill(0.1f);
    
    layer.backward(grad);
    optimizer.step();
    
    // If we got here, the workflow works
    EXPECT_TRUE(true);
}

TEST_F(QLoRATest, Integration_CompareNF4vsINT8) {
    // Compare NF4 and INT8 for the same layer
    Tensor base_weights = tensor_utils::randn({128, 128}, 0.0f, 1.0f);
    
    // NF4
    QuantizedModelConfig nf4_config = config_;
    nf4_config.quantization_type = QuantizationType::NF4;
    auto nf4_weights = std::make_shared<QuantizedLayerWeights>(base_weights, nf4_config);
    
    // INT8
    QuantizedModelConfig int8_config = config_;
    int8_config.quantization_type = QuantizationType::INT8;
    auto int8_weights = std::make_shared<QuantizedLayerWeights>(base_weights, int8_config);
    
    // NF4 should use less memory
    EXPECT_LT(nf4_weights->memory_bytes(), int8_weights->memory_bytes());
    
    // INT8 should be more accurate
    Tensor nf4_reconstructed = nf4_weights->dequantize();
    Tensor int8_reconstructed = int8_weights->dequantize();
    
    float nf4_mse = 0.0f, int8_mse = 0.0f;
    for (size_t i = 0; i < base_weights.size(); ++i) {
        float nf4_diff = base_weights[i] - nf4_reconstructed[i];
        float int8_diff = base_weights[i] - int8_reconstructed[i];
        nf4_mse += nf4_diff * nf4_diff;
        int8_mse += int8_diff * int8_diff;
    }
    nf4_mse /= base_weights.size();
    int8_mse /= base_weights.size();
    
    EXPECT_LT(int8_mse, nf4_mse);
}
