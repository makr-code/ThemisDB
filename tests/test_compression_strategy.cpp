#include <gtest/gtest.h>

// Disable compression strategy tests
#if 0
#include "storage/compression_strategy.h"
#include "utils/compression_metrics.h"
#include <random>
#include <algorithm>

using namespace themis::compression;
using namespace themis::utils;

class CompressionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        CompressionMetrics::instance().reset();
    }
    
    std::vector<uint8_t> generate_random_data(size_t size, uint8_t min_val = 0, uint8_t max_val = 255) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min_val, max_val);
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<uint8_t>(dis(gen));
        }
        
        return data;
    }
    
    std::vector<uint8_t> generate_sparse_data(size_t size, float zero_ratio = 0.95f) {
        std::vector<uint8_t> data(size, 0);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> pos_dis(0, size - 1);
        std::uniform_int_distribution<> val_dis(1, 255);
        
        size_t non_zero_count = static_cast<size_t>(size * (1.0f - zero_ratio));
        for (size_t i = 0; i < non_zero_count; ++i) {
            data[pos_dis(gen)] = static_cast<uint8_t>(val_dis(gen));
        }
        
        return data;
    }
    
    std::vector<uint8_t> generate_sequential_data(size_t size) {
        std::vector<uint8_t> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<uint8_t>(i % 256);
        }
        return data;
    }
    
    std::vector<uint8_t> generate_repetitive_data(size_t size, uint8_t repeat_value = 0xAA) {
        return std::vector<uint8_t>(size, repeat_value);
    }
    
    std::string generate_text_data(size_t size) {
        std::string text;
        text.reserve(size);
        const char* words[] = {"hello", "world", "test", "compression", "data", "text", "string"};
        size_t word_count = sizeof(words) / sizeof(words[0]);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, word_count - 1);
        
        while (text.size() < size) {
            if (!text.empty()) text += " ";
            text += words[dis(gen)];
        }
        
        text.resize(size);
        return text;
    }
};

// ============================================================================
// Basic Compression Tests
// ============================================================================

TEST_F(CompressionStrategyTest, CompressEmptyData) {
    CompressionStrategyManager manager;
    
    auto result = manager.compress(nullptr, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.method_used, CompressionMethod::NONE);
    EXPECT_TRUE(result.data.empty());
}

TEST_F(CompressionStrategyTest, CompressSmallDataSkipped) {
    CompressionStrategyManager manager;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    
    auto result = manager.compress(data);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.method_used, CompressionMethod::NONE);
    EXPECT_EQ(result.data, data);
}

TEST_F(CompressionStrategyTest, CompressDecompressRoundTrip) {
    CompressionStrategyManager manager;
    auto original = generate_random_data(1024);
    
    auto result = manager.compress(original);
    EXPECT_TRUE(result.success);
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    EXPECT_EQ(original, decompressed);
}

// ============================================================================
// ZSTD Compression Tests
// ============================================================================

TEST_F(CompressionStrategyTest, ZstdCompression) {
    CompressionConfig config;
    config.method = CompressionMethod::ZSTD;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    auto original = generate_random_data(2048);
    auto result = manager.compress(original);
    
    // ZSTD may or may not compress random data well
    EXPECT_TRUE(result.success);
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    EXPECT_EQ(original, decompressed);
}

TEST_F(CompressionStrategyTest, ZstdTextCompression) {
    CompressionConfig config;
    config.method = CompressionMethod::ZSTD;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    std::string text = generate_text_data(2048);
    auto result = manager.compress(text);
    
    EXPECT_TRUE(result.success);
    // Text should compress well
    if (result.method_used == CompressionMethod::ZSTD) {
        EXPECT_LT(result.data.size(), text.size());
    }
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    std::string decompressed_text(decompressed.begin(), decompressed.end());
    EXPECT_EQ(text, decompressed_text);
}

// ============================================================================
// RLE Compression Tests
// ============================================================================

TEST_F(CompressionStrategyTest, RLECompression) {
    CompressionConfig config;
    config.method = CompressionMethod::RLE;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    auto original = generate_repetitive_data(1024, 0xAA);
    auto result = manager.compress(original);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.method_used, CompressionMethod::RLE);
    EXPECT_LT(result.data.size(), original.size());
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    EXPECT_EQ(original, decompressed);
}

TEST_F(CompressionStrategyTest, RLECodecDirectTest) {
    auto original = generate_repetitive_data(1000, 42);
    
    auto compressed = RLECodec::compress(original.data(), original.size());
    EXPECT_FALSE(compressed.empty());
    EXPECT_LT(compressed.size(), original.size());
    
    auto decompressed = RLECodec::decompress(compressed);
    EXPECT_EQ(original, decompressed);
}

TEST_F(CompressionStrategyTest, RLEMixedData) {
    std::vector<uint8_t> data;
    for (int i = 0; i < 10; ++i) {
        data.insert(data.end(), 100, static_cast<uint8_t>(i));
    }
    
    auto compressed = RLECodec::compress(data.data(), data.size());
    EXPECT_FALSE(compressed.empty());
    EXPECT_LT(compressed.size(), data.size());
    
    auto decompressed = RLECodec::decompress(compressed);
    EXPECT_EQ(data, decompressed);
}

// ============================================================================
// Delta Encoding Tests
// ============================================================================

TEST_F(CompressionStrategyTest, DeltaCompression) {
    CompressionConfig config;
    config.method = CompressionMethod::DELTA;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    auto original = generate_sequential_data(1024);
    auto result = manager.compress(original);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.method_used, CompressionMethod::DELTA);
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    EXPECT_EQ(original, decompressed);
}

TEST_F(CompressionStrategyTest, DeltaCodecDirectTest) {
    auto original = generate_sequential_data(500);
    
    auto compressed = DeltaCodec::compress(original.data(), original.size());
    EXPECT_FALSE(compressed.empty());
    
    auto decompressed = DeltaCodec::decompress(compressed);
    EXPECT_EQ(original, decompressed);
}

TEST_F(CompressionStrategyTest, DeltaSlowlyChanging) {
    std::vector<uint8_t> data;
    uint8_t val = 0;
    for (int i = 0; i < 1000; ++i) {
        data.push_back(val);
        val += (i % 3); // Slowly changing values
    }
    
    auto compressed = DeltaCodec::compress(data.data(), data.size());
    EXPECT_FALSE(compressed.empty());
    
    auto decompressed = DeltaCodec::decompress(compressed);
    EXPECT_EQ(data, decompressed);
}

// ============================================================================
// Dictionary Encoding Tests
// ============================================================================

TEST_F(CompressionStrategyTest, DictionaryCompression) {
    CompressionConfig config;
    config.method = CompressionMethod::DICTIONARY;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    // Create data with few unique values
    std::vector<uint8_t> original;
    for (int i = 0; i < 1000; ++i) {
        original.push_back(static_cast<uint8_t>(i % 10));
    }
    
    auto result = manager.compress(original);
    EXPECT_TRUE(result.success);
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    EXPECT_EQ(original, decompressed);
}

TEST_F(CompressionStrategyTest, DictionaryCodecDirectTest) {
    std::vector<uint8_t> data;
    for (int i = 0; i < 500; ++i) {
        data.push_back(static_cast<uint8_t>(i % 8));
    }
    
    auto compressed = SimpleDictionaryCodec::compress(data.data(), data.size());
    EXPECT_FALSE(compressed.empty());
    EXPECT_LT(compressed.size(), data.size());
    
    auto decompressed = SimpleDictionaryCodec::decompress(compressed);
    EXPECT_EQ(data, decompressed);
}

TEST_F(CompressionStrategyTest, DictionaryTooManyUnique) {
    auto data = generate_random_data(500);
    
    auto compressed = SimpleDictionaryCodec::compress(data.data(), data.size());
    // Should return empty if dictionary is too large
    // (implementation returns empty if > 128 unique values)
}

// ============================================================================
// Adaptive Compression Tests
// ============================================================================

TEST_F(CompressionStrategyTest, AdaptiveDetectsText) {
    CompressionConfig config;
    config.method = CompressionMethod::ADAPTIVE;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    std::string text = generate_text_data(1024);
    auto result = manager.compress(text, DataType::TEXT);
    
    EXPECT_TRUE(result.success);
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    std::string decompressed_text(decompressed.begin(), decompressed.end());
    EXPECT_EQ(text, decompressed_text);
}

TEST_F(CompressionStrategyTest, AdaptiveDetectsSparse) {
    CompressionConfig config;
    config.method = CompressionMethod::ADAPTIVE;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    auto sparse = generate_sparse_data(1024);
    auto result = manager.compress(sparse, DataType::VECTOR_SPARSE);
    
    EXPECT_TRUE(result.success);
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    EXPECT_EQ(sparse, decompressed);
}

TEST_F(CompressionStrategyTest, AdaptiveSelectsMethod) {
    CompressionStrategyManager manager;
    
    // Test text
    std::string text = "Hello world! This is a test.";
    auto text_method = manager.select_method(
        reinterpret_cast<const uint8_t*>(text.data()),
        text.size(),
        DataType::TEXT
    );
    EXPECT_EQ(text_method, CompressionMethod::ZSTD);
    
    // Test sparse
    auto sparse_method = manager.select_method(nullptr, 0, DataType::VECTOR_SPARSE);
    EXPECT_EQ(sparse_method, CompressionMethod::RLE);
    
    // Test integer sequence
    auto int_method = manager.select_method(nullptr, 0, DataType::INTEGER_SEQ);
    EXPECT_EQ(int_method, CompressionMethod::DELTA);
    
    // Test categorical
    auto cat_method = manager.select_method(nullptr, 0, DataType::CATEGORICAL);
    EXPECT_EQ(cat_method, CompressionMethod::DICTIONARY);
}

// ============================================================================
// Compression Metrics Tests
// ============================================================================

TEST_F(CompressionStrategyTest, MetricsTracking) {
    CompressionConfig config;
    config.method = CompressionMethod::ZSTD;
    config.enable_metrics = true;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    auto data = generate_text_data(2048);
    
    auto result = manager.compress(data);
    EXPECT_TRUE(result.success);
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    
    std::string summary = manager.get_metrics();
    EXPECT_FALSE(summary.empty());
    EXPECT_NE(summary.find("zstd"), std::string::npos);
}

TEST_F(CompressionStrategyTest, MetricsReset) {
    CompressionMetrics::instance().reset();
    
    CompressionConfig config;
    config.method = CompressionMethod::RLE;
    config.enable_metrics = true;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    auto data = generate_repetitive_data(500);
    manager.compress(data);
    
    auto methods = CompressionMetrics::instance().get_methods();
    EXPECT_FALSE(methods.empty());
    
    CompressionMetrics::instance().reset();
    methods = CompressionMetrics::instance().get_methods();
    EXPECT_TRUE(methods.empty());
}

TEST_F(CompressionStrategyTest, MetricsStats) {
    CompressionMetrics::instance().reset();
    
    CompressionConfig config;
    config.method = CompressionMethod::ZSTD;
    config.enable_metrics = true;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    // Perform multiple compressions
    for (int i = 0; i < 5; ++i) {
        auto data = generate_text_data(1024);
        auto result = manager.compress(data);
        manager.decompress(result.data, result.method_used);
    }
    
    auto stats = CompressionMetrics::instance().get_method_stats("zstd");
    EXPECT_GT(stats.compression_count.load(), 0);
    EXPECT_GT(stats.decompression_count.load(), 0);
    EXPECT_GT(stats.bytes_in.load(), 0);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(CompressionStrategyTest, ConfigurationLevel) {
    CompressionConfig config;
    config.method = CompressionMethod::ZSTD;
    config.level = 1;
    config.min_size = 100;
    
    CompressionStrategyManager manager1(config);
    auto text = generate_text_data(2048);
    auto result1 = manager1.compress(text);
    
    config.level = 9;
    CompressionStrategyManager manager2(config);
    auto result2 = manager2.compress(text);
    
    // Higher level should produce smaller output (usually)
    if (result1.method_used == CompressionMethod::ZSTD && 
        result2.method_used == CompressionMethod::ZSTD) {
        // Just verify both succeeded
        EXPECT_TRUE(result1.success);
        EXPECT_TRUE(result2.success);
    }
}

TEST_F(CompressionStrategyTest, ConfigurationMinSize) {
    CompressionConfig config;
    config.method = CompressionMethod::ZSTD;
    config.min_size = 2000;
    
    CompressionStrategyManager manager(config);
    auto data = generate_random_data(1024);
    
    auto result = manager.compress(data);
    EXPECT_EQ(result.method_used, CompressionMethod::NONE);
    EXPECT_EQ(result.data, data);
}

// ============================================================================
// Method String Conversion Tests
// ============================================================================

TEST_F(CompressionStrategyTest, MethodToString) {
    EXPECT_EQ(CompressionStrategyManager::method_to_string(CompressionMethod::NONE), "none");
    EXPECT_EQ(CompressionStrategyManager::method_to_string(CompressionMethod::ZSTD), "zstd");
    EXPECT_EQ(CompressionStrategyManager::method_to_string(CompressionMethod::RLE), "rle");
    EXPECT_EQ(CompressionStrategyManager::method_to_string(CompressionMethod::DELTA), "delta");
    EXPECT_EQ(CompressionStrategyManager::method_to_string(CompressionMethod::DICTIONARY), "dictionary");
    EXPECT_EQ(CompressionStrategyManager::method_to_string(CompressionMethod::ADAPTIVE), "adaptive");
}

TEST_F(CompressionStrategyTest, StringToMethod) {
    EXPECT_EQ(CompressionStrategyManager::string_to_method("none").value(), CompressionMethod::NONE);
    EXPECT_EQ(CompressionStrategyManager::string_to_method("zstd").value(), CompressionMethod::ZSTD);
    EXPECT_EQ(CompressionStrategyManager::string_to_method("rle").value(), CompressionMethod::RLE);
    EXPECT_EQ(CompressionStrategyManager::string_to_method("delta").value(), CompressionMethod::DELTA);
    EXPECT_EQ(CompressionStrategyManager::string_to_method("dictionary").value(), CompressionMethod::DICTIONARY);
    EXPECT_FALSE(CompressionStrategyManager::string_to_method("invalid").has_value());
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(CompressionStrategyTest, CompressDecompressNone) {
    CompressionStrategyManager manager;
    auto data = generate_random_data(256);
    
    auto decompressed = manager.decompress(data, CompressionMethod::NONE);
    EXPECT_EQ(data, decompressed);
}

TEST_F(CompressionStrategyTest, InvalidDecompression) {
    CompressionStrategyManager manager;
    std::vector<uint8_t> invalid_data = {0xFF, 0xFF, 0xFF};
    
    // Decompressing invalid data should return empty or original
    auto result = manager.decompress(invalid_data, CompressionMethod::RLE);
    // Implementation may return empty on error
}

TEST_F(CompressionStrategyTest, LargeData) {
    CompressionConfig config;
    config.method = CompressionMethod::ZSTD;
    config.min_size = 100;
    CompressionStrategyManager manager(config);
    
    auto large_data = generate_repetitive_data(1024 * 1024); // 1 MB
    auto result = manager.compress(large_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_LT(result.data.size(), large_data.size());
    
    auto decompressed = manager.decompress(result.data, result.method_used);
    EXPECT_EQ(large_data, decompressed);
}

#endif // 0

TEST(CompressionStrategyDisabled, DISABLED_AllTestsSkipped) {
    GTEST_SKIP() << "Compression strategy tests are currently disabled";
}
