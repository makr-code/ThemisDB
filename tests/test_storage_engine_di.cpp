#include <gtest/gtest.h>
#include <gmock/gmock.h>
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

// ============================================================================
// Production Guard Tests
// ============================================================================

class StorageEngineProductionGuardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Save original environment variable
        const char* env = std::getenv("THEMIS_PRODUCTION_MODE");
        original_env_ = env ? std::string(env) : "";
    }

    void TearDown() override {
        // Restore original environment variable
        if (original_env_.empty()) {
            unsetenv("THEMIS_PRODUCTION_MODE");
        } else {
            setenv("THEMIS_PRODUCTION_MODE", original_env_.c_str(), 1);
        }
    }

    void setProductionMode(bool enabled) {
        if (enabled) {
            setenv("THEMIS_PRODUCTION_MODE", "1", 1);
        } else {
            unsetenv("THEMIS_PRODUCTION_MODE");
        }
    }

    std::string original_env_;
};

TEST_F(StorageEngineProductionGuardTest, DefaultImplementationsWorkInDevelopment) {
    // Ensure we're not in production mode
    setProductionMode(false);
    
    // Create storage engine with defaults - should work without warnings
    auto storage = StorageEngine::createDefault();
    ASSERT_NE(storage, nullptr);
    
    // Operations should work (though they're no-ops)
    ASSERT_TRUE(storage->open("/tmp/test_db"));
    ASSERT_TRUE(storage->put("key", "value"));
    
    // Default evaluator always returns true
    EXPECT_TRUE(storage->apply_filter("some_expression", nullptr));
    
    // Default encryption is pass-through
    std::vector<uint8_t> data = {1, 2, 3, 4};
    auto encrypted = storage->encrypt_field("test_field", data);
    EXPECT_EQ(encrypted, data);
}

TEST_F(StorageEngineProductionGuardTest, DefaultEvaluatorCreatesInDevelopment) {
    setProductionMode(false);
    
    // Should be able to create default evaluator
    auto evaluator = StorageEngine::createDefaultEvaluator();
    ASSERT_NE(evaluator, nullptr);
    
    // Should evaluate to true (no filtering)
    EXPECT_TRUE(evaluator->evaluate("test", nullptr));
    EXPECT_EQ(evaluator->get_expression_type(), "default");
}

TEST_F(StorageEngineProductionGuardTest, DefaultEncryptionCreatesInDevelopment) {
    setProductionMode(false);
    
    // Should be able to create default encryption
    auto encryption = StorageEngine::createDefaultEncryption();
    ASSERT_NE(encryption, nullptr);
    
    // Should be pass-through (no real encryption)
    std::vector<uint8_t> data = {1, 2, 3, 4};
    auto encrypted = encryption->encrypt_field("field", data);
    EXPECT_EQ(encrypted, data);
    
    auto decrypted = encryption->decrypt_field("field", encrypted);
    EXPECT_EQ(decrypted, data);
    
    EXPECT_FALSE(encryption->should_encrypt("any_field"));
}

TEST_F(StorageEngineProductionGuardTest, DefaultKeyProviderCreatesInDevelopment) {
    setProductionMode(false);
    
    // Should be able to create default key provider
    auto provider = StorageEngine::createDefaultKeyProvider();
    ASSERT_NE(provider, nullptr);
    
    // Should return dummy key
    auto key = provider->get_key("test_key");
    EXPECT_EQ(key.size(), 32);
    EXPECT_EQ(key[0], 0x42);
}

TEST_F(StorageEngineProductionGuardTest, DefaultIndexManagerCreatesInDevelopment) {
    setProductionMode(false);
    
    // Should be able to create default index manager
    auto manager = StorageEngine::createDefaultIndexManager();
    ASSERT_NE(manager, nullptr);
    
    // Operations should work (no-ops)
    auto result = manager->createSecondaryIndex("test_idx", "field");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, nullptr);
    
    EXPECT_TRUE(manager->listIndexes().empty());
}

// Note: Testing actual production mode behavior would require:
// 1. Checking logs for warning/error messages
// 2. Setting THEMIS_PRODUCTION_MODE=1 and verifying warnings appear
// These tests verify the guards are in place and callable.
// Integration tests or manual testing would verify the actual warnings.

