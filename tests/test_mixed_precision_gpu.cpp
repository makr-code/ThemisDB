#include <gtest/gtest.h>
#include "llm/lora_framework/tensor_dtype.h"
#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/mixed_precision.h"
#include <cmath>

using namespace themis::llm::lora;

namespace {
    constexpr float EPSILON = 1e-3f;  // Relaxed for FP16/BF16
    constexpr float FP16_EPSILON = 1e-2f;  // Even more relaxed for FP16
}

// ============================================================================
// DType Tests
// ============================================================================

TEST(TensorDTypeTest, DTypeSizes) {
    EXPECT_EQ(dtype_size(DType::FLOAT32), 4);
    EXPECT_EQ(dtype_size(DType::FLOAT16), 2);
    EXPECT_EQ(dtype_size(DType::BFLOAT16), 2);
}

TEST(TensorDTypeTest, DTypeNames) {
    EXPECT_EQ(dtype_name(DType::FLOAT32), "float32");
    EXPECT_EQ(dtype_name(DType::FLOAT16), "float16");
    EXPECT_EQ(dtype_name(DType::BFLOAT16), "bfloat16");
}

TEST(TensorDTypeTest, IsMixedPrecision) {
    EXPECT_FALSE(is_mixed_precision(DType::FLOAT32));
    EXPECT_TRUE(is_mixed_precision(DType::FLOAT16));
    EXPECT_TRUE(is_mixed_precision(DType::BFLOAT16));
}

TEST(TensorDTypeTest, FP16Conversion) {
    // Test simple values
    float test_values[] = {0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 2.0f, 100.0f};
    
    for (float val : test_values) {
        uint16_t fp16_bits = fp32_to_fp16_bits(val);
        float reconstructed = fp16_bits_to_fp32(fp16_bits);
        
        // FP16 has reduced precision, so we use a relaxed tolerance
        EXPECT_NEAR(val, reconstructed, std::abs(val) * 0.01f + 0.01f) 
            << "Failed for value: " << val;
    }
}

TEST(TensorDTypeTest, BF16Conversion) {
    // Test simple values
    float test_values[] = {0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 2.0f, 100.0f, 1000.0f};
    
    for (float val : test_values) {
        uint16_t bf16_bits = fp32_to_bf16_bits(val);
        float reconstructed = bf16_bits_to_fp32(bf16_bits);
        
        // BF16 has same exponent range as FP32, but reduced mantissa
        EXPECT_NEAR(val, reconstructed, std::abs(val) * 0.01f + 0.01f) 
            << "Failed for value: " << val;
    }
}

TEST(TensorDTypeTest, FP16Range) {
    // Test FP16 overflow/underflow handling
    float overflow_val = 70000.0f;  // Beyond FP16 max (~65504)
    uint16_t fp16_bits = fp32_to_fp16_bits(overflow_val);
    float reconstructed = fp16_bits_to_fp32(fp16_bits);
    
    // Should be clamped to FP16 max or infinity
    EXPECT_TRUE(std::isinf(reconstructed) || reconstructed >= 65000.0f);
}

// ============================================================================
// GPUTensor DType Tests
// ============================================================================

TEST(GPUTensorDTypeTest, ConstructorWithDType) {
    // FP32 (default)
    GPUTensor tensor_fp32({2, 3}, Device::cpu());
    EXPECT_EQ(tensor_fp32.dtype(), DType::FLOAT32);
    EXPECT_FALSE(tensor_fp32.is_mixed_precision());
    
    // FP16
    GPUTensor tensor_fp16({2, 3}, Device::cpu(), DType::FLOAT16);
    EXPECT_EQ(tensor_fp16.dtype(), DType::FLOAT16);
    EXPECT_TRUE(tensor_fp16.is_mixed_precision());
    
    // BF16
    GPUTensor tensor_bf16({2, 3}, Device::cpu(), DType::BFLOAT16);
    EXPECT_EQ(tensor_bf16.dtype(), DType::BFLOAT16);
    EXPECT_TRUE(tensor_bf16.is_mixed_precision());
}

TEST(GPUTensorDTypeTest, DTypeConversion_FP32toFP16) {
    GPUTensor tensor_fp32({2, 2}, Device::cpu(), DType::FLOAT32);
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    tensor_fp32.upload(data);
    
    auto tensor_fp16 = tensor_fp32.to_fp16();
    
    EXPECT_EQ(tensor_fp16.dtype(), DType::FLOAT16);
    auto fp16_data = tensor_fp16.cpu_data();
    
    // Check values (with FP16 precision tolerance)
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(data[i], fp16_data[i], FP16_EPSILON);
    }
}

TEST(GPUTensorDTypeTest, DTypeConversion_FP32toBF16) {
    GPUTensor tensor_fp32({2, 2}, Device::cpu(), DType::FLOAT32);
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    tensor_fp32.upload(data);
    
    auto tensor_bf16 = tensor_fp32.to_bf16();
    
    EXPECT_EQ(tensor_bf16.dtype(), DType::BFLOAT16);
    auto bf16_data = tensor_bf16.cpu_data();
    
    // Check values (with BF16 precision tolerance)
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(data[i], bf16_data[i], EPSILON);
    }
}

TEST(GPUTensorDTypeTest, DTypeConversion_RoundTrip) {
    GPUTensor tensor_fp32({2, 2}, Device::cpu(), DType::FLOAT32);
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    tensor_fp32.upload(data);
    
    // FP32 -> FP16 -> FP32
    auto tensor_fp16 = tensor_fp32.to_fp16();
    auto tensor_back = tensor_fp16.to_fp32();
    
    EXPECT_EQ(tensor_back.dtype(), DType::FLOAT32);
    auto back_data = tensor_back.cpu_data();
    
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(data[i], back_data[i], FP16_EPSILON);
    }
}

TEST(GPUTensorDTypeTest, ClonePreservesDType) {
    GPUTensor tensor_fp16({2, 2}, Device::cpu(), DType::FLOAT16);
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    tensor_fp16.upload(data);
    
    auto cloned = tensor_fp16.clone();
    
    EXPECT_EQ(cloned.dtype(), DType::FLOAT16);
    auto cloned_data = cloned.cpu_data();
    
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(data[i], cloned_data[i], FP16_EPSILON);
    }
}

TEST(GPUTensorDTypeTest, UtilityFunctions_WithDType) {
    // Test zeros
    auto zeros_fp16 = gpu_tensor_utils::zeros({2, 2}, Device::cpu(), DType::FLOAT16);
    EXPECT_EQ(zeros_fp16.dtype(), DType::FLOAT16);
    auto zeros_data = zeros_fp16.cpu_data();
    for (auto val : zeros_data) {
        EXPECT_FLOAT_EQ(val, 0.0f);
    }
    
    // Test ones
    auto ones_bf16 = gpu_tensor_utils::ones({2, 2}, Device::cpu(), DType::BFLOAT16);
    EXPECT_EQ(ones_bf16.dtype(), DType::BFLOAT16);
    auto ones_data = ones_bf16.cpu_data();
    for (auto val : ones_data) {
        EXPECT_NEAR(val, 1.0f, EPSILON);
    }
}

// ============================================================================
// Mixed Precision Trainer Tests (existing tests should still pass)
// ============================================================================

TEST(MixedPrecisionTest, BasicConfiguration) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 2048.0f;
    
    MixedPrecisionTrainer trainer(config);
    
    EXPECT_EQ(trainer.get_precision_mode(), PrecisionMode::FP16);
    EXPECT_TRUE(trainer.is_enabled());
    EXPECT_FLOAT_EQ(trainer.get_loss_scale(), 2048.0f);
}

TEST(MixedPrecisionTest, LossScaling) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 1024.0f;
    
    MixedPrecisionTrainer trainer(config);
    
    float original_loss = 0.5f;
    float scaled_loss = trainer.scale_loss(original_loss);
    
    EXPECT_FLOAT_EQ(scaled_loss, 0.5f * 1024.0f);
}

TEST(MixedPrecisionTest, GradientOverflowDetection) {
    std::vector<float> data_normal = {1.0f, 2.0f, 3.0f};
    std::vector<float> data_overflow = {1.0f, std::numeric_limits<float>::infinity(), 3.0f};
    std::vector<float> data_nan = {1.0f, std::numeric_limits<float>::quiet_NaN(), 3.0f};
    
    Tensor tensor_normal({3});
    tensor_normal.data() = data_normal;
    
    Tensor tensor_overflow({3});
    tensor_overflow.data() = data_overflow;
    
    Tensor tensor_nan({3});
    tensor_nan.data() = data_nan;
    
    std::vector<Tensor*> grads_normal = {&tensor_normal};
    std::vector<Tensor*> grads_overflow = {&tensor_overflow};
    std::vector<Tensor*> grads_nan = {&tensor_nan};
    
    EXPECT_FALSE(MixedPrecisionTrainer::has_overflow(grads_normal));
    EXPECT_TRUE(MixedPrecisionTrainer::has_overflow(grads_overflow));
    EXPECT_TRUE(MixedPrecisionTrainer::has_overflow(grads_nan));
}

TEST(MixedPrecisionTest, DynamicLossScaling) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 1024.0f;
    config.loss_scale_factor = 2.0f;
    config.dynamic_loss_scaling = true;
    
    MixedPrecisionTrainer trainer(config);
    
    float initial_scale = trainer.get_loss_scale();
    
    // Simulate overflow - should reduce scale
    trainer.update_loss_scale(true);
    EXPECT_LT(trainer.get_loss_scale(), initial_scale);
    
    // Reset for next test
    trainer.reset_stats();
    
    // Simulate many successful steps - should eventually increase scale
    for (int i = 0; i < config.loss_scale_window + 1; ++i) {
        trainer.update_loss_scale(false);
    }
    EXPECT_GT(trainer.get_loss_scale(), initial_scale);
}

TEST(MixedPrecisionTest, Statistics) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::AMP;
    
    MixedPrecisionTrainer trainer(config);
    
    // Perform some operations
    trainer.scale_loss(0.5f);
    trainer.scale_loss(0.6f);
    
    auto stats = trainer.get_stats();
    
    EXPECT_EQ(stats["total_steps"], 2);
    EXPECT_EQ(stats["precision_mode"], static_cast<int>(PrecisionMode::AMP));
}

// ============================================================================
// Integration Test
// ============================================================================

TEST(MixedPrecisionIntegrationTest, GPUTensorWithMixedPrecision) {
    // Create FP32 tensor
    GPUTensor tensor_fp32({4, 4}, Device::cpu(), DType::FLOAT32);
    std::vector<float> data(16, 1.0f);
    tensor_fp32.upload(data);
    
    // Convert to FP16 for forward pass
    auto tensor_fp16 = tensor_fp32.to_fp16();
    EXPECT_EQ(tensor_fp16.dtype(), DType::FLOAT16);
    
    // Perform operations (still in FP16)
    auto result_fp16 = tensor_fp16 * 2.0f;
    EXPECT_EQ(result_fp16.dtype(), DType::FLOAT16);
    
    // Convert back to FP32 for loss computation
    auto result_fp32 = result_fp16.to_fp32();
    EXPECT_EQ(result_fp32.dtype(), DType::FLOAT32);
    
    // Check values (with FP16 precision loss)
    auto result_data = result_fp32.cpu_data();
    for (auto val : result_data) {
        EXPECT_NEAR(val, 2.0f, FP16_EPSILON);
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
