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
    
    auto get_result = storage_->get("test_key");
    ASSERT_TRUE(get_result);
    EXPECT_EQ(*get_result, "test_value");
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
        const char* mode_env = std::getenv("THEMIS_PRODUCTION_MODE");
        const char* env_env = std::getenv("THEMIS_ENVIRONMENT");
        
        original_mode_env_ = mode_env ? std::string(mode_env) : "";
        original_env_env_ = env_env ? std::string(env_env) : "";
        
        // Ensure we're not in production mode for tests
        #ifdef _WIN32
        _putenv("THEMIS_PRODUCTION_MODE=");
        _putenv("THEMIS_ENVIRONMENT=");
        #else
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
        #endif
    }

    void TearDown() override {
        // Restore original environment variables
        #ifdef _WIN32
        if (!original_mode_env_.empty()) {
            _putenv_s("THEMIS_PRODUCTION_MODE", original_mode_env_.c_str());
        } else {
            _putenv("THEMIS_PRODUCTION_MODE=");
        }
        
        if (!original_env_env_.empty()) {
            _putenv_s("THEMIS_ENVIRONMENT", original_env_env_.c_str());
        } else {
            _putenv("THEMIS_ENVIRONMENT=");
        }
        #else
        if (!original_mode_env_.empty()) {
            setenv("THEMIS_PRODUCTION_MODE", original_mode_env_.c_str(), 1);
        } else {
            unsetenv("THEMIS_PRODUCTION_MODE");
        }
        
        if (!original_env_env_.empty()) {
            setenv("THEMIS_ENVIRONMENT", original_env_env_.c_str(), 1);
        } else {
            unsetenv("THEMIS_ENVIRONMENT");
        }
        #endif
    }

    std::string original_mode_env_;
    std::string original_env_env_;
};

TEST_F(StorageEngineProductionGuardTest, DefaultImplementationsWorkInDevelopment) {
    // Ensure we're not in production mode
    #ifdef _WIN32
    _putenv("THEMIS_PRODUCTION_MODE=");
    _putenv("THEMIS_ENVIRONMENT=");
    #else
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    #endif
    
    // Create storage engine with defaults - should work without exceptions
    auto storage = StorageEngine::createDefault();
    ASSERT_NE(storage, nullptr);
    
    // Operations should work (though they're no-ops)
    ASSERT_TRUE(storage->open("/tmp/test_db"));
    ASSERT_TRUE(storage->put("key", "value"));
    
    // Default evaluator only accepts empty expressions and throws for non-empty
    EXPECT_TRUE(storage->apply_filter("", nullptr));
    EXPECT_THROW(storage->apply_filter("some_expression", nullptr), std::logic_error);
    
    // Default encryption is pass-through
    std::vector<uint8_t> data = {1, 2, 3, 4};
    auto encrypted = storage->encrypt_field("test_field", data);
    EXPECT_EQ(encrypted, data);
}

TEST_F(StorageEngineProductionGuardTest, EncryptionFailsInProductionMode) {
    // Set production mode
    #ifdef _WIN32
    _putenv_s("THEMIS_PRODUCTION_MODE", "1");
    #else
    setenv("THEMIS_PRODUCTION_MODE", "1", 1);
    #endif
    
    // Attempting to create default encryption should throw
    EXPECT_THROW(StorageEngine::createDefaultEncryption(), std::runtime_error);
}

TEST_F(StorageEngineProductionGuardTest, KeyProviderFailsInProductionMode) {
    // Set production mode via THEMIS_ENVIRONMENT
    #ifdef _WIN32
    _putenv_s("THEMIS_ENVIRONMENT", "production");
    #else
    setenv("THEMIS_ENVIRONMENT", "production", 1);
    #endif
    
    // Attempting to create default key provider should throw
    EXPECT_THROW(StorageEngine::createDefaultKeyProvider(), std::runtime_error);
}

TEST_F(StorageEngineProductionGuardTest, DefaultEvaluatorAllowedInProduction) {
    // Set production mode
    #ifdef _WIN32
    _putenv_s("THEMIS_PRODUCTION_MODE", "true");
    #else
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    #endif
    
    // Evaluator warns but doesn't fail (less critical than encryption/keys)
    auto evaluator = StorageEngine::createDefaultEvaluator();
    ASSERT_NE(evaluator, nullptr);
    
    // Empty expressions still pass, non-empty expressions fail fast.
    EXPECT_TRUE(evaluator->evaluate("", nullptr));
    EXPECT_THROW(evaluator->evaluate("test", nullptr), std::logic_error);
}

TEST_F(StorageEngineProductionGuardTest, IndexManagerAllowedInProduction) {
    // Set production mode
    #ifdef _WIN32
    _putenv_s("THEMIS_PRODUCTION_MODE", "production");
    #else
    setenv("THEMIS_PRODUCTION_MODE", "production", 1);
    #endif
    
    // Index manager warns but doesn't fail
    auto manager = StorageEngine::createDefaultIndexManager();
    ASSERT_NE(manager, nullptr);
    
    // Operations should work (no-ops)
    auto result = manager->createSecondaryIndex("test_idx", "field");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, nullptr);
}

