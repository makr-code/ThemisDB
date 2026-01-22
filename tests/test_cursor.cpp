#include <gtest/gtest.h>
#include "utils/cursor.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using namespace themis::utils;

class CursorTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(CursorTest, EncodeDecodeRoundtrip) {
    std::string pk = "users:alice123";
    std::string collection = "users";
    
    std::string token = Cursor::encode(pk, collection);
    
    EXPECT_FALSE(token.empty());
    
    auto decoded = Cursor::decode(token);
    ASSERT_TRUE(decoded.has_value());
    
    auto [decoded_pk, decoded_collection] = *decoded;
    EXPECT_EQ(decoded_pk, pk);
    EXPECT_EQ(decoded_collection, collection);
}

TEST_F(CursorTest, EncodeSpecialCharacters) {
    std::string pk = "products:item-123/special#chars";
    std::string collection = "products";
    
    std::string token = Cursor::encode(pk, collection);
    auto decoded = Cursor::decode(token);
    
    ASSERT_TRUE(decoded.has_value());
    auto [decoded_pk, decoded_collection] = *decoded;
    EXPECT_EQ(decoded_pk, pk);
    EXPECT_EQ(decoded_collection, collection);
}

TEST_F(CursorTest, DecodeInvalidToken) {
    std::string invalid_token = "this-is-not-valid-base64!!!";
    
    auto decoded = Cursor::decode(invalid_token);
    EXPECT_FALSE(decoded.has_value());
}

TEST_F(CursorTest, DecodeEmptyToken) {
    auto decoded = Cursor::decode("");
    EXPECT_FALSE(decoded.has_value());
}

TEST_F(CursorTest, DecodeMalformedJSON) {
    // Valid base64 but invalid JSON content
    std::string malformed = Cursor::encode("pk", "coll");
    // Corrupt the base64
    if (!malformed.empty()) {
        malformed[0] = 'X';
    }
    
    auto decoded = Cursor::decode(malformed);
    // May or may not decode depending on corruption - just verify it doesn't crash
    // In most cases, this will fail gracefully
}

TEST_F(CursorTest, PaginatedResponseJSON) {
    PaginatedResponse response;
    response.items = nlohmann::json::array({
        {{"name", "Alice"}, {"age", 30}},
        {{"name", "Bob"}, {"age", 25}}
    });
    response.has_more = true;
    response.next_cursor = "abc123";
    response.batch_size = 2;
    
    auto json = response.toJSON();
    
    EXPECT_EQ(json["items"].size(), 2);
    EXPECT_TRUE(json["has_more"].get<bool>());
    EXPECT_EQ(json["next_cursor"].get<std::string>(), "abc123");
    EXPECT_EQ(json["batch_size"].get<size_t>(), 2);
}

TEST_F(CursorTest, PaginatedResponseNoMoreResults) {
    PaginatedResponse response;
    response.items = nlohmann::json::array({
        {{"name", "Charlie"}, {"age", 35}}
    });
    response.has_more = false;
    response.next_cursor = "";
    response.batch_size = 1;
    
    auto json = response.toJSON();
    
    EXPECT_EQ(json["items"].size(), 1);
    EXPECT_FALSE(json["has_more"].get<bool>());
    EXPECT_FALSE(json.contains("next_cursor")); // Should not include empty cursor
    EXPECT_EQ(json["batch_size"].get<size_t>(), 1);
}

TEST_F(CursorTest, EncodeDifferentCollections) {
    std::string pk1 = "item:1";
    std::string pk2 = "item:2";
    
    std::string token1 = Cursor::encode(pk1, "collection_a");
    std::string token2 = Cursor::encode(pk2, "collection_b");
    
    EXPECT_NE(token1, token2); // Different collections should produce different tokens
    
    auto decoded1 = Cursor::decode(token1);
    auto decoded2 = Cursor::decode(token2);
    
    ASSERT_TRUE(decoded1.has_value());
    ASSERT_TRUE(decoded2.has_value());
    
    EXPECT_EQ(decoded1->second, "collection_a");
    EXPECT_EQ(decoded2->second, "collection_b");
}

// New tests for enhanced cursor features

TEST_F(CursorTest, EncodeWithOrderValue) {
    std::string pk = "users:alice123";
    std::string collection = "users";
    std::string order_value = "Smith";
    
    std::string token = Cursor::encode(pk, collection, order_value);
    
    EXPECT_FALSE(token.empty());
    
    auto info = Cursor::decodeDetailed(token);
    ASSERT_TRUE(info.has_value());
    
    EXPECT_EQ(info->pk, pk);
    EXPECT_EQ(info->collection, collection);
    ASSERT_TRUE(info->order_value.has_value());
    EXPECT_EQ(*info->order_value, order_value);
}

TEST_F(CursorTest, EncodeWithoutOrderValue) {
    std::string pk = "users:bob456";
    std::string collection = "users";
    
    std::string token = Cursor::encode(pk, collection, std::nullopt);
    
    auto info = Cursor::decodeDetailed(token);
    ASSERT_TRUE(info.has_value());
    
    EXPECT_EQ(info->pk, pk);
    EXPECT_EQ(info->collection, collection);
    EXPECT_FALSE(info->order_value.has_value());
}

TEST_F(CursorTest, CursorContainsTimestamp) {
    std::string token = Cursor::encode("pk:123", "collection");
    
    auto info = Cursor::decodeDetailed(token);
    ASSERT_TRUE(info.has_value());
    
    EXPECT_GT(info->created_at, 0);
    EXPECT_EQ(info->version, 1);
}

TEST_F(CursorTest, CursorValidation) {
    std::string token = Cursor::encode("pk:123", "collection");
    
    // Should be valid with no TTL check
    EXPECT_TRUE(Cursor::isValid(token, 0));
    
    // Should be valid with large TTL
    EXPECT_TRUE(Cursor::isValid(token, 3600));
    
    // Invalid token should fail
    EXPECT_FALSE(Cursor::isValid("invalid", 3600));
}

TEST_F(CursorTest, CursorExpiration) {
    std::string token = Cursor::encode("pk:123", "collection");
    
    // Fresh cursor should be valid with 1 second TTL
    EXPECT_TRUE(Cursor::isValid(token, 1));
    
    // Wait 2 seconds and test again
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Now should be expired with 1 second TTL
    EXPECT_FALSE(Cursor::isValid(token, 1));
    
    // But still valid with 10 second TTL
    EXPECT_TRUE(Cursor::isValid(token, 10));
}

TEST_F(CursorTest, NormalizePageSize) {
    PaginationConfig config;
    config.min_page_size = 10;
    config.max_page_size = 1000;
    config.default_page_size = 100;
    
    // Test within bounds
    EXPECT_EQ(Cursor::normalizePageSize(50, config), 50);
    EXPECT_EQ(Cursor::normalizePageSize(500, config), 500);
    
    // Test below minimum
    EXPECT_EQ(Cursor::normalizePageSize(5, config), 10);
    EXPECT_EQ(Cursor::normalizePageSize(0, config), 10);
    
    // Test above maximum
    EXPECT_EQ(Cursor::normalizePageSize(2000, config), 1000);
    EXPECT_EQ(Cursor::normalizePageSize(10000, config), 1000);
}

TEST_F(CursorTest, PageInfoJSON) {
    PageInfo page_info;
    page_info.page_size = 10;
    page_info.has_next_page = true;
    page_info.has_prev_page = false;
    page_info.total_count = 100;
    page_info.current_page = 1;
    page_info.total_pages = 10;
    
    auto json = page_info.toJSON();
    
    EXPECT_EQ(json["page_size"].get<size_t>(), 10);
    EXPECT_TRUE(json["has_next_page"].get<bool>());
    EXPECT_FALSE(json["has_prev_page"].get<bool>());
    EXPECT_EQ(json["total_count"].get<size_t>(), 100);
    EXPECT_EQ(json["current_page"].get<size_t>(), 1);
    EXPECT_EQ(json["total_pages"].get<size_t>(), 10);
}

TEST_F(CursorTest, PaginatedResponseWithPageInfo) {
    PaginatedResponse response;
    response.items = nlohmann::json::array({
        {{"id", 1}},
        {{"id", 2}}
    });
    response.has_more = true;
    response.next_cursor = "cursor123";
    response.batch_size = 2;
    response.method = PaginationMethod::CURSOR;
    
    response.page_info.page_size = 2;
    response.page_info.has_next_page = true;
    response.page_info.has_prev_page = false;
    
    auto json = response.toJSON();
    
    EXPECT_EQ(json["items"].size(), 2);
    EXPECT_TRUE(json["has_more"].get<bool>());
    EXPECT_EQ(json["pagination_method"].get<std::string>(), "cursor");
    EXPECT_TRUE(json.contains("page_info"));
    EXPECT_EQ(json["page_info"]["page_size"].get<size_t>(), 2);
    EXPECT_TRUE(json["page_info"]["has_next_page"].get<bool>());
}

TEST_F(CursorTest, PaginationMethodSerialization) {
    PaginatedResponse response;
    response.items = nlohmann::json::array();
    
    // Test cursor method
    response.method = PaginationMethod::CURSOR;
    auto json1 = response.toJSON();
    EXPECT_EQ(json1["pagination_method"].get<std::string>(), "cursor");
    
    // Test offset method
    response.method = PaginationMethod::OFFSET;
    auto json2 = response.toJSON();
    EXPECT_EQ(json2["pagination_method"].get<std::string>(), "offset");
    
    // Test keyset method
    response.method = PaginationMethod::KEYSET;
    auto json3 = response.toJSON();
    EXPECT_EQ(json3["pagination_method"].get<std::string>(), "keyset");
}
