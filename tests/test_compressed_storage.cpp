/**
 * @file test_compressed_storage.cpp
 * @brief Tests for CompressedStorageWrapper (src/storage/compressed_storage.cpp)
 *
 * Uses an in-memory IStorageBackend stub so no RocksDB instance is needed.
 *
 * Covers:
 *   - put(string) + get_string: round-trip with various sizes
 *   - put(bytes) + get(bytes): round-trip
 *   - get on missing key returns nullopt
 *   - exists: true after put, false for missing key
 *   - del: removes key, exists returns false afterwards
 *   - get_compression_stats: returns non-empty string
 *   - reset_compression_stats: smoke test (no throw)
 *   - set_compression_config: changes method, still functional
 *   - CompressedValue::serialize / deserialize round-trip
 */

#include <gtest/gtest.h>
#include "storage/compressed_storage.h"
#include "storage/compression_strategy.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <optional>

using namespace themis::storage;
using namespace themis::compression;

// ============================================================================
// In-memory backend stub
// ============================================================================

class InMemoryBackend : public CompressedStorageWrapper::IStorageBackend {
public:
    bool put(const std::string& key, const std::vector<uint8_t>& value) override {
        store_[key] = value;
        return true;
    }

    std::optional<std::vector<uint8_t>> get(const std::string& key) override {
        auto it = store_.find(key);
        if (it == store_.end()) return std::nullopt;
        return it->second;
    }

    bool del(const std::string& key) override {
        return store_.erase(key) > 0;
    }

    bool exists(const std::string& key) override {
        return store_.count(key) > 0;
    }

private:
    std::unordered_map<std::string, std::vector<uint8_t>> store_;
};

// ============================================================================
// Fixture
// ============================================================================

class CompressedStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto backend = std::make_shared<InMemoryBackend>();
        CompressionConfig cfg;
        cfg.method   = CompressionMethod::ZSTD;
        cfg.min_size = 0; // compress everything
        wrapper_ = std::make_unique<CompressedStorageWrapper>(backend, cfg);
    }

    std::unique_ptr<CompressedStorageWrapper> wrapper_;
};

// ============================================================================
// put (string) + get_string
// ============================================================================

TEST_F(CompressedStorageTest, PutString_GetString_RoundTrip) {
    ASSERT_TRUE(wrapper_->put("k1", "hello world"));
    auto val = wrapper_->get_string("k1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "hello world");
}

TEST_F(CompressedStorageTest, PutString_LargeValue_RoundTrip) {
    // 4 KB of repetitive data (highly compressible)
    std::string large(4096, 'X');
    ASSERT_TRUE(wrapper_->put("large_key", large));
    auto val = wrapper_->get_string("large_key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, large);
}

TEST_F(CompressedStorageTest, PutString_EmptyValue_RoundTrip) {
    ASSERT_TRUE(wrapper_->put("empty", ""));
    auto val = wrapper_->get_string("empty");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "");
}

// ============================================================================
// put (bytes) + get (bytes)
// ============================================================================

TEST_F(CompressedStorageTest, PutBytes_GetBytes_RoundTrip) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0xFF, 0x00};
    ASSERT_TRUE(wrapper_->put("bin_key", data));
    auto result = wrapper_->get("bin_key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, data);
}

// ============================================================================
// get on missing key
// ============================================================================

TEST_F(CompressedStorageTest, Get_MissingKey_ReturnsNullopt) {
    auto val = wrapper_->get("does_not_exist");
    EXPECT_FALSE(val.has_value());
}

TEST_F(CompressedStorageTest, GetString_MissingKey_ReturnsNullopt) {
    auto val = wrapper_->get_string("also_missing");
    EXPECT_FALSE(val.has_value());
}

// ============================================================================
// exists
// ============================================================================

TEST_F(CompressedStorageTest, Exists_AfterPut_ReturnsTrue) {
    wrapper_->put("exist_key", "data");
    EXPECT_TRUE(wrapper_->exists("exist_key"));
}

TEST_F(CompressedStorageTest, Exists_MissingKey_ReturnsFalse) {
    EXPECT_FALSE(wrapper_->exists("never_stored"));
}

// ============================================================================
// del
// ============================================================================

TEST_F(CompressedStorageTest, Del_AfterPut_KeyIsGone) {
    wrapper_->put("del_key", "value");
    ASSERT_TRUE(wrapper_->exists("del_key"));

    EXPECT_TRUE(wrapper_->del("del_key"));
    EXPECT_FALSE(wrapper_->exists("del_key"));
    EXPECT_FALSE(wrapper_->get_string("del_key").has_value());
}

TEST_F(CompressedStorageTest, Del_MissingKey_ReturnsFalse) {
    EXPECT_FALSE(wrapper_->del("missing_key"));
}

// ============================================================================
// Multiple keys don't interfere
// ============================================================================

TEST_F(CompressedStorageTest, MultipleKeys_NoInterference) {
    wrapper_->put("a", "value_a");
    wrapper_->put("b", "value_b");
    wrapper_->put("c", "value_c");

    EXPECT_EQ(*wrapper_->get_string("a"), "value_a");
    EXPECT_EQ(*wrapper_->get_string("b"), "value_b");
    EXPECT_EQ(*wrapper_->get_string("c"), "value_c");
}

// ============================================================================
// get_compression_stats / reset_compression_stats
// ============================================================================

TEST_F(CompressedStorageTest, GetCompressionStats_ReturnsNonEmpty) {
    wrapper_->put("stat_key", std::string(1024, 'A'));
    auto stats = wrapper_->get_compression_stats();
    EXPECT_FALSE(stats.empty());
}

TEST_F(CompressedStorageTest, ResetCompressionStats_DoesNotThrow) {
    wrapper_->put("k", "v");
    EXPECT_NO_THROW(wrapper_->reset_compression_stats());
}

// ============================================================================
// set_compression_config: change method and verify functionality
// ============================================================================

TEST_F(CompressedStorageTest, SetCompressionConfig_NoneMethod_StillFunctional) {
    CompressionConfig cfg;
    cfg.method = CompressionMethod::NONE;
    wrapper_->set_compression_config(cfg);

    ASSERT_TRUE(wrapper_->put("nc_key", "no compression value"));
    auto val = wrapper_->get_string("nc_key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "no compression value");
}

// ============================================================================
// CompressedValue serialize / deserialize round-trip
// ============================================================================

TEST(CompressedValueTest, SerializeDeserialize_RoundTrip) {
    CompressedValue cv;
    cv.data   = {0xDE, 0xAD, 0xBE, 0xEF};
    cv.method = CompressionMethod::ZSTD;

    auto serialized   = cv.serialize();
    auto deserialized = CompressedValue::deserialize(serialized);

    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(deserialized->data,   cv.data);
    EXPECT_EQ(deserialized->method, cv.method);
}

TEST(CompressedValueTest, Deserialize_EmptyBytes_ReturnsNullopt) {
    auto result = CompressedValue::deserialize({});
    EXPECT_FALSE(result.has_value());
}
