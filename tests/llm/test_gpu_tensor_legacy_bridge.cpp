/*
 * Tests for GPUTensor legacy Tensor bridge (STUB #4)
 *
 * Covers: GT-LB-01..GT-LB-03
 *   GT-LB-01 — from_legacy_tensor() preserves shape/data on CPU
 *   GT-LB-02 — to_legacy_tensor() preserves shape/data on CPU
 *   GT-LB-03 — round-trip Tensor -> GPUTensor -> Tensor preserves payload
 */

#include <gtest/gtest.h>

#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/lora_layers.h"

using namespace themis::llm::lora;

TEST(GPUTensorLegacyBridgeTest, FromLegacyTensorPreservesShapeAndDataOnCpu) {
    Tensor legacy({2, 3});
    for (size_t i = 0; i < legacy.size(); ++i) {
        legacy[i] = static_cast<float>(i + 1);
    }

    auto gpu_tensor = gpu_tensor_utils::from_legacy_tensor(legacy, Device::cpu(), DType::FLOAT32);

    EXPECT_EQ(gpu_tensor.shape(), legacy.shape());
    EXPECT_EQ(gpu_tensor.dtype(), DType::FLOAT32);
    EXPECT_TRUE(gpu_tensor.is_cpu());
    EXPECT_EQ(gpu_tensor.cpu_data(), legacy.data());
}

TEST(GPUTensorLegacyBridgeTest, ToLegacyTensorPreservesShapeAndDataOnCpu) {
    GPUTensor gpu_tensor({2, 2}, Device::cpu(), DType::FLOAT32);
    gpu_tensor.upload(std::vector<float>{1.5f, 2.5f, 3.5f, 4.5f});

    auto legacy = gpu_tensor_utils::to_legacy_tensor(gpu_tensor);

    EXPECT_EQ(legacy.shape(), std::vector<size_t>({2, 2}));
    EXPECT_EQ(legacy.data(), std::vector<float>({1.5f, 2.5f, 3.5f, 4.5f}));
}

TEST(GPUTensorLegacyBridgeTest, RoundTripPreservesPayload) {
    Tensor original({3});
    original[0] = -1.0f;
    original[1] = 0.0f;
    original[2] = 42.0f;

    auto gpu_tensor = gpu_tensor_utils::from_legacy_tensor(original, Device::cpu(), DType::FLOAT32);
    auto restored = gpu_tensor_utils::to_legacy_tensor(gpu_tensor);

    EXPECT_EQ(restored.shape(), original.shape());
    EXPECT_EQ(restored.data(), original.data());
}
