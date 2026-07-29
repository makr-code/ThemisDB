/*
 * ThemisDB | Test: test_stream_protocol_focused.cpp | Version: 0.0.47
 * Focused Unit Tests for W2-S03: Stream Protocol Chunk Metadata Validation
 * 
 * Test Coverage:
 * - StreamChunk::deserialize() validation (invalid metadata bounds)
 * - WALApplier::applyBatch() validation (empty batch, invalid LSN bounds)
 * - LSN ordering enforcement (strict mode)
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "sharding/stream_protocol.h"
#include "sharding/wal_applier.h"
#include <vector>
#include <memory>
#include <limits>

namespace themisdb { namespace streaming { 

// ============================================================================
// StreamChunk Deserialization Tests
// ============================================================================

class StreamChunkValidationTest : public ::testing::Test {
protected:
    // Helper to build a valid serialized chunk
    std::vector<uint8_t> buildValidChunk(
        uint64_t file_offset = 0,
        uint32_t chunk_index = 0,
        uint32_t uncompressed_size = 1024,
        uint32_t compressed_size = 1024,
        uint32_t checksum = 0xDEADBEEF) {
        
        std::vector<uint8_t> result;
        result.reserve(24);
        
        // file_offset (8 bytes, big-endian)
        for (int i = 7; i >= 0; --i) {
            result.push_back((file_offset >> (i * 8)) & 0xFF);
        }
        
        // chunk_index (4 bytes)
        result.push_back((chunk_index >> 24) & 0xFF);
        result.push_back((chunk_index >> 16) & 0xFF);
        result.push_back((chunk_index >> 8) & 0xFF);
        result.push_back(chunk_index & 0xFF);
        
        // uncompressed_size (4 bytes)
        result.push_back((uncompressed_size >> 24) & 0xFF);
        result.push_back((uncompressed_size >> 16) & 0xFF);
        result.push_back((uncompressed_size >> 8) & 0xFF);
        result.push_back(uncompressed_size & 0xFF);
        
        // compressed_size (4 bytes)
        result.push_back((compressed_size >> 24) & 0xFF);
        result.push_back((compressed_size >> 16) & 0xFF);
        result.push_back((compressed_size >> 8) & 0xFF);
        result.push_back(compressed_size & 0xFF);
        
        // checksum (4 bytes)
        result.push_back((checksum >> 24) & 0xFF);
        result.push_back((checksum >> 16) & 0xFF);
        result.push_back((checksum >> 8) & 0xFF);
        result.push_back(checksum & 0xFF);
        
        // Add dummy data matching compressed_size
        for (uint32_t i = 0; i < compressed_size; ++i) {
            result.push_back(0xAB);
        }
        
        return result;
    }
};

// W2-S03: Fail-closed on uncompressed_size == 0
TEST_F(StreamChunkValidationTest, DeserializeRejectsZeroUncompressedSize) {
    auto data = buildValidChunk(
        0, 0, 0,  // uncompressed_size = 0
        100, 0xDEADBEEF);
    
    auto result = StreamChunk::deserialize(data);
    EXPECT_FALSE(result.has_value()) << "Should reject uncompressed_size == 0";
}

// W2-S03: Fail-closed on uncompressed_size > 1GB
TEST_F(StreamChunkValidationTest, DeserializeRejectsExcessiveUncompressedSize) {
    auto data = buildValidChunk(
        0, 0, 
        2u * 1024u * 1024u * 1024u,  // 2GB - exceeds 1GB max
        100, 0xDEADBEEF);
    
    auto result = StreamChunk::deserialize(data);
    EXPECT_FALSE(result.has_value()) << "Should reject uncompressed_size > 1GB";
}

// W2-S03: Fail-closed on compressed_size mismatch with actual payload
TEST_F(StreamChunkValidationTest, DeserializeRejectsCompressedSizeMismatch) {
    auto data = buildValidChunk(
        0, 0, 1024, 100, 0xDEADBEEF);
    
    // Manually adjust compressed_size in header to 200, but only provide 100 bytes of data
    uint32_t wrong_size = 200;
    data[16] = (wrong_size >> 24) & 0xFF;
    data[17] = (wrong_size >> 16) & 0xFF;
    data[18] = (wrong_size >> 8) & 0xFF;
    data[19] = wrong_size & 0xFF;
    
    auto result = StreamChunk::deserialize(data);
    EXPECT_FALSE(result.has_value()) << "Should reject compressed_size mismatch";
}

// W2-S03: Fail-closed when uncompressed_size < compressed_size (invalid compression)
TEST_F(StreamChunkValidationTest, DeserializeRejectsInvalidCompressionRatio) {
    auto data = buildValidChunk(
        0, 0, 100,  // uncompressed = 100
        200, 0xDEADBEEF);  // compressed = 200 (impossible!)
    
    // Update compressed_size in header to 200
    uint32_t wrong_size = 200;
    data[16] = (wrong_size >> 24) & 0xFF;
    data[17] = (wrong_size >> 16) & 0xFF;
    data[18] = (wrong_size >> 8) & 0xFF;
    data[19] = wrong_size & 0xFF;
    
    auto result = StreamChunk::deserialize(data);
    EXPECT_FALSE(result.has_value()) << "Should reject uncompressed_size < compressed_size";
}

// W2-S03: Accept valid chunk with realistic metadata
TEST_F(StreamChunkValidationTest, DeserializeAcceptsValidChunk) {
    auto data = buildValidChunk(0, 0, 4096, 4096, 0xDEADBEEF);
    
    auto result = StreamChunk::deserialize(data);
    EXPECT_TRUE(result.has_value()) << "Should accept valid chunk";
    EXPECT_EQ(result->file_offset, 0);
    EXPECT_EQ(result->chunk_index, 0);
    EXPECT_EQ(result->uncompressed_size, 4096);
    EXPECT_EQ(result->compressed_size, 4096);
}

// W2-S03: Accept chunk at maximum reasonable size
TEST_F(StreamChunkValidationTest, DeserializeAcceptsLargeValidChunk) {
    // 512MB is within limits
    constexpr uint32_t SIZE_512MB = 512u * 1024u * 1024u;
    
    auto data = buildValidChunk(0, 0, SIZE_512MB, SIZE_512MB, 0xABCDEF00);
    auto result = StreamChunk::deserialize(data);
    
    EXPECT_TRUE(result.has_value()) << "Should accept chunk up to 512MB";
    EXPECT_EQ(result->uncompressed_size, SIZE_512MB);
}

// W2-S03: Reject header-only data (no payload)
TEST_F(StreamChunkValidationTest, DeserializeRejectsHeaderOnlyWithNonzeroCompressedSize) {
    std::vector<uint8_t> data;
    data.reserve(24);
    
    // 24-byte header with compressed_size = 100
    for (int i = 0; i < 16; ++i) data.push_back(0);  // offset, index, uncompressed_size
    
    uint32_t compressed_size = 100;
    data.push_back((compressed_size >> 24) & 0xFF);
    data.push_back((compressed_size >> 16) & 0xFF);
    data.push_back((compressed_size >> 8) & 0xFF);
    data.push_back(compressed_size & 0xFF);
    
    uint32_t checksum = 0xDEADBEEF;
    data.push_back((checksum >> 24) & 0xFF);
    data.push_back((checksum >> 16) & 0xFF);
    data.push_back((checksum >> 8) & 0xFF);
    data.push_back(checksum & 0xFF);
    
    auto result = StreamChunk::deserialize(data);
    EXPECT_FALSE(result.has_value()) << "Should reject when payload missing";
}
} } // namespace themisdb::streaming
namespace themis { namespace sharding { 

// ============================================================================
// WAL Applier Validation Tests
// ============================================================================

class WALApplierValidationTest : public ::testing::Test {
protected:
    WALApplierConfig getDefaultConfig() {
        WALApplierConfig config;
        config.replica_id = "test-replica";
        config.strict_mode = true;
        return config;
    }
    
    WALEntry buildValidEntry(uint64_t segment = 0, uint64_t offset = 1) {
        WALEntry entry;
        entry.lsn = LSN{segment, offset};
        entry.type = WALEntryType::INSERT;
        entry.transaction_id = "txn-123";
        entry.timestamp = 123456789ull;
        entry.data = {
            {"key", "doc-123"},
            {"payload", "test"}
        };
        return entry;
    }
};

// W2-S03: Fail-closed on empty batch
TEST_F(WALApplierValidationTest, ApplyBatchRejectsEmptyBatch) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    // Set a dummy apply handler
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;  // Always succeed
    });
    
    std::vector<WALEntry> empty_batch;
    auto result = applier.applyBatch(empty_batch);
    
    EXPECT_FALSE(result.success) << "Should reject empty batch";
    EXPECT_FALSE(result.errors.empty()) << "Should have error message";
}

// W2-S03: Fail-closed on invalid LSN segment bounds
TEST_F(WALApplierValidationTest, ApplyBatchRejectsMaxSegmentLSN) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;
    });
    
    std::vector<WALEntry> batch;
    auto entry = buildValidEntry(0, 1);
    entry.lsn.segment = std::numeric_limits<uint64_t>::max();
    batch.push_back(entry);
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_FALSE(result.success) << "Should reject max segment LSN";
    EXPECT_FALSE(result.errors.empty()) << "Should have error message";
}

// W2-S03: Fail-closed on invalid LSN offset bounds
TEST_F(WALApplierValidationTest, ApplyBatchRejectsMaxOffsetLSN) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;
    });
    
    std::vector<WALEntry> batch;
    auto entry = buildValidEntry(0, 1);
    entry.lsn.offset = std::numeric_limits<uint64_t>::max();
    batch.push_back(entry);
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_FALSE(result.success) << "Should reject max offset LSN";
    EXPECT_FALSE(result.errors.empty()) << "Should have error message";
}

// W2-S03: Fail-closed on stale LSN in strict mode
TEST_F(WALApplierValidationTest, ApplyBatchRejectsStaleLSNInStrictMode) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;
    });
    
    // Initialize to (0, 5)
    applier.setCurrentLSN(LSN{0, 5});
    
    // Try to apply (0, 4) - stale!
    std::vector<WALEntry> batch;
    batch.push_back(buildValidEntry(0, 4));
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_FALSE(result.success) << "Should reject stale LSN in strict mode";
    EXPECT_FALSE(result.errors.empty()) << "Should have error message";
}

// W2-S03: Fail-closed on duplicate LSN in strict mode
TEST_F(WALApplierValidationTest, ApplyBatchRejectsDuplicateLSNInStrictMode) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;
    });
    
    // Initialize to (0, 5)
    applier.setCurrentLSN(LSN{0, 5});
    
    // Try to apply (0, 5) - duplicate!
    std::vector<WALEntry> batch;
    batch.push_back(buildValidEntry(0, 5));
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_FALSE(result.success) << "Should reject duplicate LSN in strict mode";
    EXPECT_FALSE(result.errors.empty()) << "Should have error message";
}

// W2-S03: Accept valid batch with proper LSN sequence
TEST_F(WALApplierValidationTest, ApplyBatchAcceptsValidSequence) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;  // Always succeed
    });
    
    // Initialize to (0, 5)
    applier.setCurrentLSN(LSN{0, 5});
    
    // Apply (0, 6), (0, 7), (0, 8)
    std::vector<WALEntry> batch;
    batch.push_back(buildValidEntry(0, 6));
    batch.push_back(buildValidEntry(0, 7));
    batch.push_back(buildValidEntry(0, 8));
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_TRUE(result.success) << "Should accept valid LSN sequence";
    EXPECT_EQ(result.entries_applied, 3) << "Should apply all 3 entries";
    EXPECT_EQ(result.last_applied_lsn, LSN(0, 8)) << "Final LSN should be (0, 8)";
}

// W2-S03: Accept segment boundary transition
TEST_F(WALApplierValidationTest, ApplyBatchAcceptsSegmentBoundary) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;
    });
    
    // Initialize to end of segment 0
    applier.setCurrentLSN(LSN{0, 1000});
    
    // Apply (1, 0) - next segment at offset 0
    std::vector<WALEntry> batch;
    batch.push_back(buildValidEntry(1, 0));
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_TRUE(result.success) << "Should accept segment boundary transition";
    EXPECT_EQ(result.last_applied_lsn, LSN(1, 0));
}

// W2-S03: Reject out-of-order within batch
TEST_F(WALApplierValidationTest, ApplyBatchRejectsOutOfOrderWithinBatch) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    applier.setApplyHandler([](const WALEntry& entry) {
        return true;
    });
    
    applier.setCurrentLSN(LSN{0, 5});
    
    // Try to apply (0, 6), then (0, 5) - second one is stale relative to first
    std::vector<WALEntry> batch;
    batch.push_back(buildValidEntry(0, 6));
    batch.push_back(buildValidEntry(0, 5));  // Out of order!
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_FALSE(result.success) << "Should reject out-of-order LSNs";
    EXPECT_LE(result.entries_applied, 1) << "Should stop after first LSN mismatch";
}

// W2-S03: Stop on apply handler failure
TEST_F(WALApplierValidationTest, ApplyBatchStopsOnApplyFailure) {
    auto config = getDefaultConfig();
    WALApplier applier(config);
    
    // Handler fails on third entry
    int apply_count = 0;
    applier.setApplyHandler([&apply_count](const WALEntry& entry) {
        apply_count++;
        return apply_count < 3;  // Fail on third call
    });
    
    applier.setCurrentLSN(LSN{0, 5});
    
    std::vector<WALEntry> batch;
    batch.push_back(buildValidEntry(0, 6));
    batch.push_back(buildValidEntry(0, 7));
    batch.push_back(buildValidEntry(0, 8));
    batch.push_back(buildValidEntry(0, 9));
    
    auto result = applier.applyBatch(batch);
    
    EXPECT_FALSE(result.success) << "Should fail when handler returns false";
    EXPECT_EQ(result.entries_applied, 2) << "Should stop after second successful apply";
}
} } // namespace themis::sharding
