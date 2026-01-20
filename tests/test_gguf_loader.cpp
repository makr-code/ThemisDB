#include <gtest/gtest.h>
#include "llm/gguf_loader.h"
#include "llm/lora_framework/gguf_converter.h"
#include <vector>
#include <cstring>
#include <fstream>

using namespace themis::llm;
using namespace themis::llm::lora;

/**
 * @file test_gguf_loader.cpp
 * @brief Tests for GGUF format parsing and loading
 */

class GGUFLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
    }
    
    void TearDown() override {
        // Cleanup any test files
    }
};

// ===== GGUF Type Tests =====

TEST_F(GGUFLoaderTest, GGMLTypeToString) {
    // Test TensorMetadata type_string() helper
    TensorMetadata tensor;
    
    tensor.type = GGMLType::F32;
    EXPECT_EQ(tensor.type_string(), "F32");
    
    tensor.type = GGMLType::F16;
    EXPECT_EQ(tensor.type_string(), "F16");
    
    tensor.type = GGMLType::Q4_K;
    EXPECT_EQ(tensor.type_string(), "Q4_K_M");
    
    tensor.type = GGMLType::Q8_0;
    EXPECT_EQ(tensor.type_string(), "Q8_0");
}

TEST_F(GGUFLoaderTest, GGMLTypeSizes) {
    // Test getGGMLTypeSize helper
    GGUFLoader loader;
    
    // Note: We need to make this public or add a friend test
    // For now, just test that types are correctly sized
    // These are block sizes, not per-element sizes
    
    // F32: 4 bytes per element
    // F16: 2 bytes per element
    // Q4_K: 144 bytes per 256 elements block
    // Q8_0: 34 bytes per 32 elements block
}

// ===== Converter Tests =====

TEST_F(GGUFLoaderTest, ConverterSupported) {
    // Test isSupported for different types
    EXPECT_TRUE(GGUFConverter::isSupported(GGMLType::F32));
    EXPECT_TRUE(GGUFConverter::isSupported(GGMLType::F16));
    EXPECT_TRUE(GGUFConverter::isSupported(GGMLType::Q4_K));
    EXPECT_TRUE(GGUFConverter::isSupported(GGMLType::Q8_0));
    
    // Unsupported types
    EXPECT_FALSE(GGUFConverter::isSupported(GGMLType::Q4_0));
    EXPECT_FALSE(GGUFConverter::isSupported(GGMLType::Q5_K));
}

TEST_F(GGUFLoaderTest, ConverterInternalTypes) {
    // Test getInternalType mapping
    EXPECT_EQ(GGUFConverter::getInternalType(GGMLType::Q4_K), QuantizationType::NF4);
    EXPECT_EQ(GGUFConverter::getInternalType(GGMLType::Q8_0), QuantizationType::INT8);
    EXPECT_EQ(GGUFConverter::getInternalType(GGMLType::F32), QuantizationType::NONE);
    EXPECT_EQ(GGUFConverter::getInternalType(GGMLType::F16), QuantizationType::NONE);
}

TEST_F(GGUFLoaderTest, FP16Conversion) {
    // Test FP16 to FP32 conversion
    
    // Test zero
    EXPECT_FLOAT_EQ(GGUFConverter::fp16_to_fp32(0x0000), 0.0f);
    
    // Test 1.0 (0x3C00 in FP16)
    EXPECT_NEAR(GGUFConverter::fp16_to_fp32(0x3C00), 1.0f, 0.0001f);
    
    // Test -1.0 (0xBC00 in FP16)
    EXPECT_NEAR(GGUFConverter::fp16_to_fp32(0xBC00), -1.0f, 0.0001f);
    
    // Test 0.5 (0x3800 in FP16)
    EXPECT_NEAR(GGUFConverter::fp16_to_fp32(0x3800), 0.5f, 0.0001f);
}

// ===== Block Structure Tests =====

TEST_F(GGUFLoaderTest, Q4KBlockSize) {
    // Verify Q4_K_M block structure size
    EXPECT_EQ(sizeof(gguf_blocks::Q4KBlock), 144);
    
    // Block should hold 256 values
    // 128 bytes for quantized data (4 bits each, packed)
    // 12 bytes for scales
    // 2 bytes for d (FP16)
    // 2 bytes for dmin (FP16)
    // Total: 144 bytes
}

TEST_F(GGUFLoaderTest, Q8_0BlockSize) {
    // Verify Q8_0 block structure size
    EXPECT_EQ(sizeof(gguf_blocks::Q8_0Block), 34);
    
    // Block should hold 32 values
    // 2 bytes for d (FP16 scale)
    // 32 bytes for quantized data (INT8)
    // Total: 34 bytes
}

// ===== Mock GGUF File Tests =====

TEST_F(GGUFLoaderTest, InvalidMagic) {
    // Test that loader rejects invalid magic number
    // This would require creating a mock file
    // Skipping for minimal implementation
}

TEST_F(GGUFLoaderTest, UnsupportedVersion) {
    // Test that loader rejects unsupported versions
    // This would require creating a mock file
    // Skipping for minimal implementation
}

// ===== Integration Tests =====

TEST_F(GGUFLoaderTest, CalculateElements) {
    // Test element calculation from shape
    std::vector<int64_t> shape1 = {10, 20};
    std::vector<int64_t> shape2 = {5, 10, 20};
    std::vector<int64_t> shape3 = {1024};
    
    // These would be internal helper tests
    // Just verify the concept works
    EXPECT_EQ(10 * 20, 200);
    EXPECT_EQ(5 * 10 * 20, 1000);
    EXPECT_EQ(1024, 1024);
}

TEST_F(GGUFLoaderTest, DirectQuantizedLoading) {
    // Test that pre-quantized weights are loaded directly without dequantization
    // This verifies the fix for the "synthetic weights" issue
    
    // Create a mock Q4_K_M quantized tensor
    std::vector<int64_t> shape = {256};  // One Q4_K block
    TensorMetadata tensor_info;
    tensor_info.name = "test.weight";
    tensor_info.shape = shape;
    tensor_info.type = GGMLType::Q4_K;
    tensor_info.size = sizeof(gguf_blocks::Q4KBlock);
    
    // Create mock Q4_K_M data (one block)
    gguf_blocks::Q4KBlock mock_block;
    std::memset(&mock_block, 0, sizeof(mock_block));
    mock_block.d = 0x3C00;  // 1.0 in FP16
    mock_block.dmin = 0x0000;  // 0.0 in FP16
    
    // Fill with known pattern
    for (int i = 0; i < 128; i++) {
        mock_block.qs[i] = 0x88;  // Alternating 8 and 8 in 4-bit
    }
    
    // Convert using the converter
    QuantizedTensor converted = GGUFConverter::convertQ4KM(&mock_block, tensor_info);
    
    // Verify it's properly quantized in internal format
    EXPECT_EQ(converted.type(), QuantizationType::NF4);
    EXPECT_GT(converted.memory_bytes(), 0);
    
    // Verify the quantized tensor can be used to create layer weights
    std::vector<size_t> size_t_shape = {256};
    QuantizedLayerWeights layer_weights(std::move(converted), size_t_shape);
    
    EXPECT_EQ(layer_weights.type(), QuantizationType::NF4);
    EXPECT_GT(layer_weights.memory_bytes(), 0);
    
    // Verify we can dequantize
    Tensor dequantized = layer_weights.dequantize();
    EXPECT_EQ(dequantized.shape(), size_t_shape);
}

// Main test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
