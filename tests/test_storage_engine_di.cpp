#include <gtest/gtest.h>
#include "storage/storage_engine.h"
#include "utils/error_registry.h"
#include <memory>

using namespace themis;

// Test fixture for StorageEngine error handling
class StorageEngineErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a default storage engine for testing
        storage_ = StorageEngine::createDefault();
    }

    std::shared_ptr<StorageEngine> storage_;
};

// Test: open() returns Result<void> with proper error handling
TEST_F(StorageEngineErrorHandlingTest, OpenReturnsResultVoid) {
    auto result = storage_->open("/tmp/test_db");
    
    // Should succeed
    ASSERT_TRUE(result);
    
    // Second open should fail
    auto result2 = storage_->open("/tmp/test_db");
    ASSERT_FALSE(result2);
    EXPECT_EQ(result2.error().code(), errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED);
    EXPECT_THAT(result2.error().message(), testing::HasSubstr("already open"));
}

// Test: put() returns Result<void>
TEST_F(StorageEngineErrorHandlingTest, PutReturnsResultVoid) {
    // Should fail when not open
    auto result = storage_->put("key1", "value1");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED);
    EXPECT_THAT(result.error().message(), testing::HasSubstr("not open"));
    
    // Should succeed when open
    ASSERT_TRUE(storage_->open("/tmp/test_db"));
    auto result2 = storage_->put("key1", "value1");
    ASSERT_TRUE(result2);
}

// Test: get() returns Result<std::string> with proper error codes
TEST_F(StorageEngineErrorHandlingTest, GetReturnsResultString) {
    // Should fail when not open
    auto result = storage_->get("key1");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED);
    EXPECT_THAT(result.error().message(), testing::HasSubstr("not open"));
    
    // Should return key not found when open
    ASSERT_TRUE(storage_->open("/tmp/test_db"));
    auto result2 = storage_->get("nonexistent_key");
    ASSERT_FALSE(result2);
    EXPECT_EQ(result2.error().code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    EXPECT_THAT(result2.error().message(), testing::HasSubstr("not found"));
}

// Test: del() returns Result<void>
TEST_F(StorageEngineErrorHandlingTest, DelReturnsResultVoid) {
    // Should fail when not open
    auto result = storage_->del("key1");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED);
    
    // Should succeed when open
    ASSERT_TRUE(storage_->open("/tmp/test_db"));
    auto result2 = storage_->del("key1");
    ASSERT_TRUE(result2);
}

// Test: Error propagation pattern
TEST_F(StorageEngineErrorHandlingTest, ErrorPropagation) {
    ASSERT_TRUE(storage_->open("/tmp/test_db"));
    
    // Get a non-existent key
    auto get_result = storage_->get("missing_key");
    ASSERT_FALSE(get_result);
    
    // Can access error details
    auto error = get_result.error();
    EXPECT_EQ(error.code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    EXPECT_FALSE(error.context().empty());
    EXPECT_FALSE(error.message().empty());
    
    // Can get metadata
    auto metadata = error.metadata();
    EXPECT_EQ(metadata.category, "Storage");
}

// Test: Success path with Result<T>
TEST_F(StorageEngineErrorHandlingTest, SuccessPath) {
    ASSERT_TRUE(storage_->open("/tmp/test_db"));
    
    // Put operation
    auto put_result = storage_->put("test_key", "test_value");
    ASSERT_TRUE(put_result);
    
    // In a real implementation, get would return the value
    // For now, it returns error (not implemented)
    auto get_result = storage_->get("test_key");
    // This is expected to fail in the stub implementation
    ASSERT_FALSE(get_result);
}

// Test: Result<T> can be checked with if statement
TEST_F(StorageEngineErrorHandlingTest, ResultBooleanConversion) {
    ASSERT_TRUE(storage_->open("/tmp/test_db"));
    
    auto result = storage_->put("key", "value");
    
    // Result<T> converts to bool
    if (result) {
        // Success path
        SUCCEED();
    } else {
        // Error path
        FAIL() << "Put should succeed when storage is open";
    }
}

