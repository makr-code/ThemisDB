// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include "content/pipeline/zstd_compression.h"
#include "content/pipeline/content_chunker.h"
#include "content/pipeline/bulk_upload_interface.h"
#include <vector>
#include <string>

using namespace themis::content::pipeline;

// ============================================================================
// ZstdCompression Tests
// ============================================================================

TEST(ContentPipelineTest, ZstdCompression_DefaultConstructor) {
    ZstdCompression compressor;
    EXPECT_EQ(compressor.get_compression_level(), 3);
}

TEST(ContentPipelineTest, ZstdCompression_SetCompressionLevel) {
    ZstdCompression compressor;
    compressor.set_compression_level(5);
    EXPECT_EQ(compressor.get_compression_level(), 5);
    
    compressor.set_compression_level(10);
    EXPECT_EQ(compressor.get_compression_level(), 10);
}

TEST(ContentPipelineTest, ZstdCompression_CompressEmptyData) {
    ZstdCompression compressor;
    std::vector<uint8_t> empty_data;
    auto compressed = compressor.compress(empty_data);
    EXPECT_TRUE(compressed.empty());
}

TEST(ContentPipelineTest, ZstdCompression_CompressDecompress) {
    ZstdCompression compressor;
    
    // Create test data
    std::string test_str = "Hello, ThemisDB Content Pipeline!";
    std::vector<uint8_t> data(test_str.begin(), test_str.end());
    
    // Compress and decompress (placeholder returns data as-is)
    auto compressed = compressor.compress(data);
    auto decompressed = compressor.decompress(compressed);
    
    EXPECT_EQ(data.size(), compressed.size());
    EXPECT_EQ(data, decompressed);
}

TEST(ContentPipelineTest, ZstdCompression_LargeData) {
    ZstdCompression compressor;
    
    // Create larger test data (1MB)
    std::vector<uint8_t> large_data(1024 * 1024, 0xAB);
    
    auto compressed = compressor.compress(large_data);
    auto decompressed = compressor.decompress(compressed);
    
    EXPECT_EQ(large_data, decompressed);
}

// ============================================================================
// ContentChunker Tests
// ============================================================================

TEST(ContentPipelineTest, ContentChunker_DefaultConfig) {
    ContentChunker chunker;
    auto config = chunker.get_config();
    
    EXPECT_EQ(config.chunk_size, 1024 * 1024);  // 1MB default
    EXPECT_EQ(config.overlap, 0);
    EXPECT_FALSE(config.content_aware);
}

TEST(ContentPipelineTest, ContentChunker_CustomConfig) {
    ContentChunker::ChunkConfig config;
    config.chunk_size = 512;
    config.overlap = 64;
    config.content_aware = true;
    
    ContentChunker chunker(config);
    auto retrieved_config = chunker.get_config();
    
    EXPECT_EQ(retrieved_config.chunk_size, 512);
    EXPECT_EQ(retrieved_config.overlap, 64);
    EXPECT_TRUE(retrieved_config.content_aware);
}

TEST(ContentPipelineTest, ContentChunker_EmptyData) {
    ContentChunker chunker;
    std::vector<uint8_t> empty_data;
    
    auto chunks = chunker.chunk(empty_data);
    EXPECT_TRUE(chunks.empty());
}

TEST(ContentPipelineTest, ContentChunker_SingleChunk) {
    ContentChunker::ChunkConfig config;
    config.chunk_size = 1024;
    ContentChunker chunker(config);
    
    std::vector<uint8_t> small_data(512, 0x42);
    auto chunks = chunker.chunk(small_data);
    
    ASSERT_EQ(chunks.size(), 1);
    EXPECT_EQ(chunks[0].index, 0);
    EXPECT_EQ(chunks[0].total_chunks, 1);
    EXPECT_EQ(chunks[0].original_offset, 0);
    EXPECT_EQ(chunks[0].data.size(), 512);
}

TEST(ContentPipelineTest, ContentChunker_MultipleChunks) {
    ContentChunker::ChunkConfig config;
    config.chunk_size = 100;
    ContentChunker chunker(config);
    
    std::vector<uint8_t> data(350, 0xCD);
    auto chunks = chunker.chunk(data);
    
    ASSERT_EQ(chunks.size(), 4);  // 350 / 100 = 3.5, so 4 chunks
    
    // Verify first chunk
    EXPECT_EQ(chunks[0].index, 0);
    EXPECT_EQ(chunks[0].total_chunks, 4);
    EXPECT_EQ(chunks[0].original_offset, 0);
    EXPECT_EQ(chunks[0].data.size(), 100);
    
    // Verify last chunk
    EXPECT_EQ(chunks[3].index, 3);
    EXPECT_EQ(chunks[3].total_chunks, 4);
    EXPECT_EQ(chunks[3].original_offset, 300);
    EXPECT_EQ(chunks[3].data.size(), 50);  // Remainder
}

TEST(ContentPipelineTest, ContentChunker_ReassembleChunks) {
    ContentChunker::ChunkConfig config;
    config.chunk_size = 100;
    ContentChunker chunker(config);
    
    // Create test data with pattern
    std::vector<uint8_t> original_data;
    for (size_t i = 0; i < 250; ++i) {
        original_data.push_back(static_cast<uint8_t>(i % 256));
    }
    
    auto chunks = chunker.chunk(original_data);
    auto reassembled = chunker.reassemble(chunks);
    
    EXPECT_EQ(original_data, reassembled);
}

TEST(ContentPipelineTest, ContentChunker_UpdateConfig) {
    ContentChunker chunker;
    
    ContentChunker::ChunkConfig new_config;
    new_config.chunk_size = 2048;
    new_config.overlap = 128;
    
    chunker.set_config(new_config);
    auto config = chunker.get_config();
    
    EXPECT_EQ(config.chunk_size, 2048);
    EXPECT_EQ(config.overlap, 128);
}

// ============================================================================
// BulkUploadInterface Tests
// ============================================================================

TEST(ContentPipelineTest, BulkUploadInterface_SingleUpload) {
    BulkUploadInterface uploader;
    
    std::vector<uint8_t> content = {1, 2, 3, 4, 5};
    BulkUploadInterface::ContentMetadata metadata;
    metadata.content_id = "test-123";
    metadata.content_type = "application/octet-stream";
    metadata.content_size = content.size();
    
    auto result = uploader.upload(content, metadata);
    
    EXPECT_EQ(result.status, BulkUploadInterface::UploadStatus::COMPLETED);
    EXPECT_EQ(result.content_id, "test-123");
    EXPECT_EQ(result.bytes_uploaded, 5);
    EXPECT_TRUE(result.error_message.empty());
}

TEST(ContentPipelineTest, BulkUploadInterface_EmptyContent) {
    BulkUploadInterface uploader;
    
    std::vector<uint8_t> empty_content;
    BulkUploadInterface::ContentMetadata metadata;
    metadata.content_id = "empty-456";
    
    auto result = uploader.upload(empty_content, metadata);
    
    EXPECT_EQ(result.status, BulkUploadInterface::UploadStatus::COMPLETED);
    EXPECT_EQ(result.bytes_uploaded, 0);
}

TEST(ContentPipelineTest, BulkUploadInterface_BulkUpload) {
    BulkUploadInterface uploader;
    
    // Prepare multiple content items
    std::vector<std::vector<uint8_t>> contents = {
        {1, 2, 3},
        {4, 5, 6, 7},
        {8, 9}
    };
    
    std::vector<BulkUploadInterface::ContentMetadata> metadata_list;
    for (size_t i = 0; i < contents.size(); ++i) {
        BulkUploadInterface::ContentMetadata meta;
        meta.content_id = "bulk-" + std::to_string(i);
        meta.content_type = "test/data";
        metadata_list.push_back(meta);
    }
    
    auto results = uploader.bulk_upload(contents, metadata_list);
    
    ASSERT_EQ(results.size(), 3);
    
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i].status, BulkUploadInterface::UploadStatus::COMPLETED);
        EXPECT_EQ(results[i].content_id, "bulk-" + std::to_string(i));
        EXPECT_EQ(results[i].bytes_uploaded, contents[i].size());
    }
}

TEST(ContentPipelineTest, BulkUploadInterface_MismatchedSizes) {
    BulkUploadInterface uploader;
    
    std::vector<std::vector<uint8_t>> contents = {{1, 2}, {3, 4}};
    std::vector<BulkUploadInterface::ContentMetadata> metadata_list;
    metadata_list.push_back(BulkUploadInterface::ContentMetadata{});
    // Only one metadata for two contents - mismatch
    
    auto results = uploader.bulk_upload(contents, metadata_list);
    
    ASSERT_EQ(results.size(), 2);
    for (const auto& result : results) {
        EXPECT_EQ(result.status, BulkUploadInterface::UploadStatus::FAILED);
        EXPECT_FALSE(result.error_message.empty());
    }
}

TEST(ContentPipelineTest, BulkUploadInterface_ProgressCallback) {
    BulkUploadInterface uploader;
    
    bool callback_called = false;
    std::string callback_id;
    size_t callback_bytes = 0;
    
    uploader.set_progress_callback(
        [&](const std::string& id, size_t uploaded, size_t total) {
            callback_called = true;
            callback_id = id;
            callback_bytes = uploaded;
        }
    );
    
    std::vector<uint8_t> content = {1, 2, 3, 4, 5};
    BulkUploadInterface::ContentMetadata metadata;
    metadata.content_id = "callback-test";
    
    auto result = uploader.upload(content, metadata);
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(callback_id, "callback-test");
    EXPECT_EQ(callback_bytes, 5);
}

TEST(ContentPipelineTest, BulkUploadInterface_CancelUpload) {
    BulkUploadInterface uploader;
    
    // Placeholder implementation always returns false
    bool cancelled = uploader.cancel_upload("test-id");
    EXPECT_FALSE(cancelled);
}

TEST(ContentPipelineTest, BulkUploadInterface_GetUploadStatus) {
    BulkUploadInterface uploader;
    
    // Placeholder implementation always returns COMPLETED
    auto status = uploader.get_upload_status("any-id");
    EXPECT_EQ(status, BulkUploadInterface::UploadStatus::COMPLETED);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(ContentPipelineTest, Integration_CompressChunkUpload) {
    // Test integration of compression, chunking, and upload
    
    ZstdCompression compressor;
    ContentChunker::ChunkConfig config;
    config.chunk_size = 50;
    ContentChunker chunker(config);
    BulkUploadInterface uploader;
    
    // Create test data
    std::vector<uint8_t> data(150, 0xAB);
    
    // Compress
    auto compressed = compressor.compress(data);
    
    // Chunk
    auto chunks = chunker.chunk(compressed);
    EXPECT_GT(chunks.size(), 1);
    
    // Upload each chunk
    for (const auto& chunk : chunks) {
        BulkUploadInterface::ContentMetadata metadata;
        metadata.content_id = "chunk-" + std::to_string(chunk.index);
        metadata.chunk_count = chunk.total_chunks;
        
        auto result = uploader.upload(chunk.data, metadata);
        EXPECT_EQ(result.status, BulkUploadInterface::UploadStatus::COMPLETED);
    }
}
