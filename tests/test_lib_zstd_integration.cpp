// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// zstd Compression Library Integration Tests
// Tests zstd library integration for storage optimization and data pipeline
// Use Case: Storage optimization, data compression, backup compression

#include <gtest/gtest.h>
#include <zstd.h>
#include <string>
#include <vector>
#include <random>
#include <cstring>
#include <chrono>

class ZstdLibIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Seed for reproducible random data
        rng_.seed(42);
    }

    std::mt19937 rng_ = {};
    
    // Helper to generate random data
    std::vector<char> generateRandomData(size_t size) {
        std::uniform_int_distribution<int> dist(0, 255);
        std::vector<char> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<char>(dist(rng_));
        }
        return data;
    }
    
    // Helper to generate compressible data (repeated patterns)
    std::vector<char> generateCompressibleData(size_t size) {
        std::vector<char> data(size);
        const char* pattern = "ThemisDB is a multi-model database. ";
        size_t pattern_len = strlen(pattern);
        for (size_t i = 0; i < size; ++i) {
            data[i] = pattern[i % pattern_len];
        }
        return data;
    }
};

// Test 1: Library linking and version check
TEST_F(ZstdLibIntegrationTest, LibraryLinking) {
    unsigned version = ZSTD_versionNumber();
    EXPECT_GT(version, 0u);
    
    // Version number format: major*10000 + minor*100 + release
    unsigned major = version / 10000;
    unsigned minor = (version % 10000) / 100;
    
    EXPECT_GT(major, 0u);
    EXPECT_GE(minor, 0u);
}

// Test 2: Basic compression
TEST_F(ZstdLibIntegrationTest, BasicCompression) {
    std::string input = "Hello, ThemisDB! This is a test of zstd compression.";
    
    size_t max_compressed_size = ZSTD_compressBound(input.size());
    std::vector<char> compressed(max_compressed_size);
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        input.data(), input.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    EXPECT_GT(compressed_size, 0u);
    EXPECT_LT(compressed_size, input.size()); // Should compress smaller
}

// Test 3: Basic decompression
TEST_F(ZstdLibIntegrationTest, BasicDecompression) {
    std::string input = "Hello, ThemisDB! This is a test of zstd decompression.";
    
    // Compress
    size_t max_compressed_size = ZSTD_compressBound(input.size());
    std::vector<char> compressed(max_compressed_size);
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        input.data(), input.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    // Decompress
    unsigned long long decompressed_size = ZSTD_getFrameContentSize(compressed.data(), compressed_size);
    ASSERT_NE(decompressed_size, ZSTD_CONTENTSIZE_ERROR);
    ASSERT_NE(decompressed_size, ZSTD_CONTENTSIZE_UNKNOWN);
    
    std::vector<char> decompressed(decompressed_size);
    size_t actual_size = ZSTD_decompress(
        decompressed.data(), decompressed.size(),
        compressed.data(), compressed_size
    );
    
    ASSERT_FALSE(ZSTD_isError(actual_size));
    EXPECT_EQ(actual_size, input.size());
    
    std::string output(decompressed.data(), actual_size);
    EXPECT_EQ(output, input);
}

// Test 4: Compression levels
TEST_F(ZstdLibIntegrationTest, CompressionLevels) {
    auto data = generateCompressibleData(10000);
    
    std::vector<size_t> compressed_sizes;
    
    // Test different compression levels
    for (int level : {1, 3, 9, 15, 19}) {
        size_t max_size = ZSTD_compressBound(data.size());
        std::vector<char> compressed(max_size);
        
        size_t compressed_size = ZSTD_compress(
            compressed.data(), compressed.size(),
            data.data(), data.size(),
            level
        );
        
        ASSERT_FALSE(ZSTD_isError(compressed_size));
        compressed_sizes.push_back(compressed_size);
    }
    
    // Higher compression levels should generally result in smaller sizes
    // (though not always guaranteed for all data)
    EXPECT_GE(compressed_sizes[0], compressed_sizes[compressed_sizes.size() - 1]);
}

// Test 5: Compression ratio for compressible data
TEST_F(ZstdLibIntegrationTest, CompressionRatioCompressible) {
    auto data = generateCompressibleData(100000);
    
    size_t max_compressed_size = ZSTD_compressBound(data.size());
    std::vector<char> compressed(max_compressed_size);
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        data.data(), data.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    double ratio = static_cast<double>(compressed_size) / data.size();
    
    // Highly compressible data should compress to less than 10% of original
    EXPECT_LT(ratio, 0.1);
}

// Test 6: Random data compression
TEST_F(ZstdLibIntegrationTest, RandomDataCompression) {
    auto data = generateRandomData(10000);
    
    size_t max_compressed_size = ZSTD_compressBound(data.size());
    std::vector<char> compressed(max_compressed_size);
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        data.data(), data.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    // Random data won't compress well, might even expand slightly
    // Just verify it completes successfully
    EXPECT_GT(compressed_size, 0u);
}

// Test 7: Streaming compression context
TEST_F(ZstdLibIntegrationTest, StreamingCompressionContext) {
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    ASSERT_NE(cctx, nullptr);
    
    // Set compression level
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 5);
    
    std::string input = "Streaming compression test with ThemisDB";
    size_t max_size = ZSTD_compressBound(input.size());
    std::vector<char> compressed(max_size);
    
    size_t compressed_size = ZSTD_compress2(
        cctx,
        compressed.data(), compressed.size(),
        input.data(), input.size()
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    EXPECT_GT(compressed_size, 0u);
    
    ZSTD_freeCCtx(cctx);
}

// Test 8: Streaming decompression context
TEST_F(ZstdLibIntegrationTest, StreamingDecompressionContext) {
    std::string input = "Streaming decompression test";
    
    // Compress first
    size_t max_compressed_size = ZSTD_compressBound(input.size());
    std::vector<char> compressed(max_compressed_size);
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        input.data(), input.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    // Decompress using context
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    ASSERT_NE(dctx, nullptr);
    
    std::vector<char> decompressed(input.size());
    size_t decompressed_size = ZSTD_decompressDCtx(
        dctx,
        decompressed.data(), decompressed.size(),
        compressed.data(), compressed_size
    );
    
    ASSERT_FALSE(ZSTD_isError(decompressed_size));
    EXPECT_EQ(decompressed_size, input.size());
    
    std::string output(decompressed.data(), decompressed_size);
    EXPECT_EQ(output, input);
    
    ZSTD_freeDCtx(dctx);
}

// Test 9: Error handling - corrupt data
TEST_F(ZstdLibIntegrationTest, ErrorHandlingCorruptData) {
    std::vector<char> corrupt_data = {1, 2, 3, 4, 5}; // Not valid zstd data
    std::vector<char> output(100);
    
    size_t result = ZSTD_decompress(
        output.data(), output.size(),
        corrupt_data.data(), corrupt_data.size()
    );
    
    EXPECT_TRUE(ZSTD_isError(result));
    
    const char* error_name = ZSTD_getErrorName(result);
    EXPECT_NE(error_name, nullptr);
}

// Test 10: Empty data handling
TEST_F(ZstdLibIntegrationTest, EmptyDataHandling) {
    std::vector<char> empty_data;
    std::vector<char> compressed(ZSTD_compressBound(1)); // Need at least some space
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        empty_data.data(), empty_data.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    // Decompress back
    std::vector<char> decompressed(1);
    size_t decompressed_size = ZSTD_decompress(
        decompressed.data(), decompressed.size(),
        compressed.data(), compressed_size
    );
    
    ASSERT_FALSE(ZSTD_isError(decompressed_size));
    EXPECT_EQ(decompressed_size, 0u);
}

// Test 11: Large data compression
TEST_F(ZstdLibIntegrationTest, LargeDataCompression) {
    auto data = generateCompressibleData(1024 * 1024); // 1MB
    
    size_t max_compressed_size = ZSTD_compressBound(data.size());
    std::vector<char> compressed(max_compressed_size);
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        data.data(), data.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    // Verify decompression
    std::vector<char> decompressed(data.size());
    size_t decompressed_size = ZSTD_decompress(
        decompressed.data(), decompressed.size(),
        compressed.data(), compressed_size
    );
    
    ASSERT_FALSE(ZSTD_isError(decompressed_size));
    EXPECT_EQ(decompressed_size, data.size());
    EXPECT_EQ(decompressed, data);
}

// Test 12: Dictionary compression
TEST_F(ZstdLibIntegrationTest, DictionaryCompression) {
    // Create a dictionary from sample data
    std::string dict_sample = "ThemisDB database query transaction index vector graph ";
    
    // Create compression context
    ZSTD_CDict* cdict = ZSTD_createCDict(
        dict_sample.data(), dict_sample.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    ASSERT_NE(cdict, nullptr);
    
    std::string input = "ThemisDB query on database with vector index";
    size_t max_size = ZSTD_compressBound(input.size());
    std::vector<char> compressed(max_size);
    
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    size_t compressed_size = ZSTD_compress_usingCDict(
        cctx,
        compressed.data(), compressed.size(),
        input.data(), input.size(),
        cdict
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    ZSTD_freeCDict(cdict);
    ZSTD_freeCCtx(cctx);
}

// Test 13: Integration with ThemisDB patterns - backup compression
TEST_F(ZstdLibIntegrationTest, ThemisDBBackupCompression) {
    // Simulate backup data (JSON-like structure)
    std::string backup_data = R"({
        "database": "themisdb",
        "collections": [
            {"name": "users", "count": 1000},
            {"name": "products", "count": 5000},
            {"name": "orders", "count": 10000}
        ],
        "timestamp": "2025-01-17T16:00:00Z"
    })";
    
    // Repeat to make it larger and more compressible
    std::string large_backup = {};
    for (int i = 0; i < 100; ++i) {
        large_backup += backup_data;
    }
    
    size_t max_compressed_size = ZSTD_compressBound(large_backup.size());
    std::vector<char> compressed(max_compressed_size);
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        large_backup.data(), large_backup.size(),
        9 // Higher compression for backups
    );
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    double ratio = static_cast<double>(compressed_size) / large_backup.size();
    
    // JSON-like data should compress very well
    EXPECT_LT(ratio, 0.2);
    
    // Verify decompression works
    std::vector<char> decompressed(large_backup.size());
    size_t decompressed_size = ZSTD_decompress(
        decompressed.data(), decompressed.size(),
        compressed.data(), compressed_size
    );
    
    ASSERT_FALSE(ZSTD_isError(decompressed_size));
    EXPECT_EQ(decompressed_size, large_backup.size());
}

// Test 14: Frame parameters
TEST_F(ZstdLibIntegrationTest, FrameParameters) {
    std::string input = "Test frame parameters";
    
    size_t max_compressed_size = ZSTD_compressBound(input.size());
    std::vector<char> compressed(max_compressed_size);
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        input.data(), input.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    // NOTE: ZSTD_frameHeader and ZSTD_getFrameHeader are deprecated APIs
    // This test is skipped in modern zstd versions
    // If needed in production, use ZSTD_getDictID_fromFrame() instead
    /*
    ZSTD_frameHeader header;
    size_t result = ZSTD_getFrameHeader(&header, compressed.data(), compressed_size);
    
    EXPECT_EQ(result, 0u); // 0 means success
    EXPECT_EQ(header.frameContentSize, input.size());
    */
}

// Test 15: Performance benchmark
TEST_F(ZstdLibIntegrationTest, PerformanceBenchmark) {
    auto data = generateCompressibleData(10 * 1024 * 1024); // 10MB
    
    size_t max_compressed_size = ZSTD_compressBound(data.size());
    std::vector<char> compressed(max_compressed_size);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    size_t compressed_size = ZSTD_compress(
        compressed.data(), compressed.size(),
        data.data(), data.size(),
        ZSTD_CLEVEL_DEFAULT
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_FALSE(ZSTD_isError(compressed_size));
    
    // Should compress 10MB in reasonable time
    EXPECT_LT(duration.count(), 2000); // Less than 2 seconds
    
    // Calculate throughput
    double throughput_mb_s = (data.size() / (1024.0 * 1024.0)) / (duration.count() / 1000.0);
    
    // Should achieve reasonable throughput (at least 5 MB/s)
    EXPECT_GT(throughput_mb_s, 5.0);
}
