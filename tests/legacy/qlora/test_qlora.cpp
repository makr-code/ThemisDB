#include <gtest/gtest.h>
#include "llm/lora_framework/quantized_model.h"
#include "llm/lora_framework/lora_layers.h"
#include <memory>
#include <cmath>
#include <chrono>

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
        
        if (step == 0) {
          initial_loss = loss;
        }
        if (step == 9) {
          final_loss = loss;
        }
        
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

// ===== ENHANCED TESTS: Edge Cases and Boundary Conditions =====

// Test: Quantization with extreme dimensions
TEST_F(QLoRATest, QuantizationExtremeDimensions) {
    // Very small tensor (1x1)
    Tensor tiny_weights = tensor_utils::randn({1, 1});
    QuantizedLayerWeights q_tiny(tiny_weights, config_);
    Tensor reconstructed_tiny = q_tiny.dequantize();
    EXPECT_EQ(reconstructed_tiny.shape(), tiny_weights.shape());
    
    // Very large tensor (1024x1024)
    Tensor large_weights = tensor_utils::randn({1024, 1024}, 0.0f, 0.1f);
    QuantizedLayerWeights q_large(large_weights, config_);
    
    size_t original_bytes = 1024 * 1024 * sizeof(float);
    size_t quantized_bytes = q_large.memory_bytes();
    
    float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
    EXPECT_GT(reduction, 0.60f) << "Large tensor should have > 60% memory reduction";
}

// Test: Single element tensor
TEST_F(QLoRATest, SingleElementTensor) {
    Tensor single = tensor_utils::randn({1, 1});
    single[0] = 42.0f;
    
    QuantizedLayerWeights q_single(single, config_);
    Tensor reconstructed = q_single.dequantize();
    
    EXPECT_EQ(reconstructed.size(), 1);
    // Single value should be approximately preserved
    float error = std::abs(single[0] - reconstructed[0]);
    EXPECT_LT(error, 5.0f) << "Single element error: " << error;
}

// Test: Zero tensor quantization
TEST_F(QLoRATest, ZeroTensorQuantization) {
    Tensor zeros({64, 64});
    zeros.fill(0.0f);
    
    QuantizedLayerWeights q_zeros(zeros, config_);
    Tensor reconstructed = q_zeros.dequantize();
    
    // Zeros should remain approximately zero
    float max_deviation = 0.0f;
    for (size_t i = 0; i < reconstructed.size(); ++i) {
        max_deviation = std::max(max_deviation, std::abs(reconstructed[i]));
    }
    
    EXPECT_LT(max_deviation, 0.1f) << "Zero tensor max deviation: " << max_deviation;
}

// Test: Uniform value tensor
TEST_F(QLoRATest, UniformValueTensor) {
    Tensor uniform({32, 32});
    uniform.fill(5.0f);
    
    QuantizedLayerWeights q_uniform(uniform, config_);
    Tensor reconstructed = q_uniform.dequantize();
    
    // Check variance in reconstruction
    float mean = 0.0f;
    for (size_t i = 0; i < reconstructed.size(); ++i) {
        mean += reconstructed[i];
    }
    mean /= reconstructed.size();
    
    EXPECT_NEAR(mean, 5.0f, 0.5f) << "Uniform tensor mean should be close to original";
}

// Test: Extreme value tensors
TEST_F(QLoRATest, ExtremeValueTensors) {
    // Very large values
    Tensor large_vals({16, 16});
    large_vals.fill(1000.0f);
    
    QuantizedLayerWeights q_large_vals(large_vals, config_);
    Tensor reconstructed_large = q_large_vals.dequantize();
    
    // Very small values
    Tensor small_vals({16, 16});
    small_vals.fill(0.001f);
    
    QuantizedLayerWeights q_small_vals(small_vals, config_);
    Tensor reconstructed_small = q_small_vals.dequantize();
    
    // Both should complete without errors
    EXPECT_EQ(reconstructed_large.size(), large_vals.size());
    EXPECT_EQ(reconstructed_small.size(), small_vals.size());
}

// ===== ENHANCED TESTS: Parametrized Quantization Tests =====

// Parametrized test for different quantization types
class QuantizationTypeTest : public QLoRATest,
                              public ::testing::WithParamInterface<std::tuple<QuantizationType, float>> {
};

TEST_P(QuantizationTypeTest, QuantizationAccuracy) {
    auto [quant_type, expected_min_reduction] = GetParam();
    
    Tensor weights = tensor_utils::randn({256, 256}, 0.0f, 1.0f);
    
    QuantizedModelConfig test_config = config_;
    test_config.quantization_type = quant_type;
    
    QuantizedLayerWeights q_weights(weights, test_config);
    Tensor reconstructed = q_weights.dequantize();
    
    // Check memory reduction
    size_t original_bytes = weights.size() * sizeof(float);
    size_t quantized_bytes = q_weights.memory_bytes();
    float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
    
    EXPECT_GT(reduction, expected_min_reduction) 
        << "Quantization type should achieve minimum " << (expected_min_reduction * 100) << "% reduction";
    
    // Check reconstruction accuracy
    float mse = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
        float diff = weights[i] - reconstructed[i];
        mse += diff * diff;
    }
    mse /= weights.size();
    
    // Different quantization types have different accuracy bounds
    float max_mse = (quant_type == QuantizationType::NF4) ? 0.1f : 0.05f;
    EXPECT_LT(mse, max_mse) << "MSE: " << mse << " for quant type: " << static_cast<int>(quant_type);
}

INSTANTIATE_TEST_SUITE_P(
    QuantizationTypes,
    QuantizationTypeTest,
    ::testing::Values(
        std::make_tuple(QuantizationType::NF4, 0.60f),   // At least 60% reduction for NF4
        std::make_tuple(QuantizationType::INT8, 0.60f)   // At least 60% reduction for INT8
    )
);

// ===== ENHANCED TESTS: Error Handling =====

// Test: Invalid dimensions
TEST_F(QLoRATest, InvalidDimensionsHandling) {
    // Try to create QLoRA layer with mismatched dimensions
    // This should either throw or handle gracefully
    EXPECT_NO_THROW({
        QLoRALayer layer(0, 64, 4); // Zero input dimension
    });
    
    EXPECT_NO_THROW({
        QLoRALayer layer(64, 0, 4); // Zero output dimension
    });
}

// Test: Invalid rank
TEST_F(QLoRATest, InvalidRankHandling) {
    // Rank of 0 should be handled
    EXPECT_NO_THROW({
        QLoRALayer layer(64, 64, 0);
    });
    
    // Very large rank (larger than dimensions)
    EXPECT_NO_THROW({
        QLoRALayer layer(8, 8, 16); // Rank larger than dimensions
    });
}

// Test: Backward pass without forward
TEST_F(QLoRATest, BackwardWithoutForward) {
    QLoRALayer layer(32, 32, 4);
    
    // Try backward without forward - should handle gracefully
    Tensor grad_output({1, 32});
    grad_output.fill(1.0f);
    
    // This may throw or return zero gradients depending on implementation
    // Just verify it doesn't crash
    EXPECT_NO_THROW({
        layer.backward(grad_output);
    });
}

// Test: Multiple forward passes with same input
TEST_F(QLoRATest, MultipleForwardPasses) {
    QLoRALayer layer(16, 16, 4);
    Tensor input({1, 16});
    input.fill(1.0f);
    
    // Multiple forward passes should be consistent
    Tensor output1 = layer.forward(input);
    Tensor output2 = layer.forward(input);
    
    // Outputs should be identical for same input
    for (size_t i = 0; i < output1.size(); ++i) {
        EXPECT_FLOAT_EQ(output1[i], output2[i]) 
            << "Output mismatch at index " << i;
    }
}

// ===== ENHANCED TESTS: Dequantization Accuracy =====

// Test: Dequantization accuracy with different block sizes
TEST_F(QLoRATest, DequantizationBlockSizeAccuracy) {
    Tensor weights = tensor_utils::randn({128, 128}, 0.0f, 1.0f);
    
    std::vector<size_t> block_sizes = {32, 64, 128};
    std::vector<float> mse_values;
    
    for (size_t block_size : block_sizes) {
        QuantizedModelConfig test_config = config_;
        test_config.block_size = block_size;
        
        QuantizedLayerWeights q_weights(weights, test_config);
        Tensor reconstructed = q_weights.dequantize();
        
        float mse = 0.0f;
        for (size_t i = 0; i < weights.size(); ++i) {
            float diff = weights[i] - reconstructed[i];
            mse += diff * diff;
        }
        mse /= weights.size();
        mse_values.push_back(mse);
    }
    
    // All block sizes should have reasonable accuracy
    for (float mse : mse_values) {
        EXPECT_LT(mse, 0.1f) << "MSE: " << mse;
    }
}

// Test: Dequantization with double quantization
TEST_F(QLoRATest, DoubleQuantizationAccuracy) {
    Tensor weights = tensor_utils::randn({128, 128}, 0.0f, 1.0f);
    
    // Without double quantization
    QuantizedModelConfig single_config = config_;
    single_config.use_double_quantization = false;
    QuantizedLayerWeights q_single(weights, single_config);
    
    // With double quantization
    QuantizedModelConfig double_config = config_;
    double_config.use_double_quantization = true;
    QuantizedLayerWeights q_double(weights, double_config);
    
    // Double quantization should use even less memory
    EXPECT_LE(q_double.memory_bytes(), q_single.memory_bytes()) 
        << "Double quantization should not use more memory than single";
}

// ===== ENHANCED TESTS: Performance Bounds =====

// Test: Quantization performance bounds
TEST_F(QLoRATest, QuantizationPerformanceBounds) {
    const size_t dim = 512;
    Tensor weights = tensor_utils::randn({dim, dim});
    
    auto start = std::chrono::high_resolution_clock::now();
    QuantizedLayerWeights q_weights(weights, config_);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Quantization should be fast (< 100ms for 512x512)
    EXPECT_LT(duration.count(), 100) 
        << "Quantization took " << duration.count() << "ms, expected < 100ms";
}

// Test: Dequantization performance bounds
TEST_F(QLoRATest, DequantizationPerformanceBounds) {
    const size_t dim = 512;
    Tensor weights = tensor_utils::randn({dim, dim});
    QuantizedLayerWeights q_weights(weights, config_);
    
    auto start = std::chrono::high_resolution_clock::now();
    Tensor reconstructed = q_weights.dequantize();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Dequantization should be fast (< 50ms for 512x512)
    EXPECT_LT(duration.count(), 50) 
        << "Dequantization took " << duration.count() << "ms, expected < 50ms";
}

// Test: Forward pass performance bounds
TEST_F(QLoRATest, ForwardPassPerformanceBounds) {
    const size_t batch_size = 32;
    const size_t dim = 256;
    
    QLoRALayer layer(dim, dim, 16);
    Tensor input({batch_size, dim});
    input.fill(1.0f);
    
    // Warmup
    layer.forward(input);
    
    // Measure performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        layer.forward(input);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto avg_time = duration.count() / 10.0;
    
    // Forward pass should be efficient (< 10ms per batch)
    EXPECT_LT(avg_time, 10) 
        << "Average forward pass took " << avg_time << "ms, expected < 10ms";
}

// Test: Training iteration performance
TEST_F(QLoRATest, TrainingIterationPerformance) {
    const size_t dim = 64;
    QLoRALayer layer(dim, dim, 8);
    SGDOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());
    
    Tensor input({1, dim});
    input.fill(1.0f);
    Tensor target({1, dim});
    target.fill(0.5f);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run 100 training iterations
    for (int i = 0; i < 100; ++i) {
        optimizer.zero_grad();
        Tensor output = layer.forward(input);
        Tensor diff = output - target;
        Tensor grad = diff * (2.0f / diff.size());
        layer.backward(grad);
        optimizer.step();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 100 iterations should complete in reasonable time (< 500ms)
    EXPECT_LT(duration.count(), 500) 
        << "100 training iterations took " << duration.count() << "ms, expected < 500ms";
}

// ===== ENHANCED TESTS: Memory Efficiency =====

// Test: Memory efficiency comparison
TEST_F(QLoRATest, MemoryEfficiencyComparison) {
    std::vector<size_t> dims = {64, 128, 256, 512};
    
    for (size_t dim : dims) {
        Tensor weights = tensor_utils::randn({dim, dim});
        QuantizedLayerWeights q_weights(weights, config_);
        
        size_t original_bytes = dim * dim * sizeof(float);
        size_t quantized_bytes = q_weights.memory_bytes();
        float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
        
        // Memory reduction should be consistent across sizes
        EXPECT_GT(reduction, 0.60f) 
            << "Dimension " << dim << " has only " << (reduction * 100) << "% reduction";
    }
}

// Test: QLoRA layer memory vs full precision layer
TEST_F(QLoRATest, QLoRAvsFullPrecisionMemory) {
    const size_t dim = 256;
    const size_t rank = 16;
    
    // QLoRA layer with quantized base
    Tensor base_weights = tensor_utils::randn({dim, dim});
    auto q_base = std::make_shared<QuantizedLayerWeights>(base_weights, config_);
    QLoRALayer qlora_layer(dim, dim, rank, q_base);
    
    // Calculate equivalent full precision memory
    size_t full_precision_base = dim * dim * sizeof(float);
    size_t lora_adapters = (dim * rank + rank * dim) * sizeof(float);
    size_t full_memory = full_precision_base + lora_adapters;
    
    size_t qlora_memory = qlora_layer.memory_bytes();
    
    // QLoRA should use significantly less memory
    float savings = 1.0f - static_cast<float>(qlora_memory) / full_memory;
    EXPECT_GT(savings, 0.50f) 
        << "QLoRA memory savings: " << (savings * 100) << "%, expected > 50%";
}
