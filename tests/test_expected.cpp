/**
 * @file test_expected.cpp
 * @brief Unit tests for tl::expected-based error handling wrapper
 */

#include <gtest/gtest.h>
#include "utils/expected.h"
#include "utils/error_registry.h"
#include <string>
#include <memory>

namespace themis {
namespace test {

class ExpectedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // ErrorRegistry is initialized automatically as singleton
    }
};

// Test basic Error class functionality
TEST_F(ExpectedTest, ErrorConstruction) {
    Error err1(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    EXPECT_EQ(err1.code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    EXPECT_TRUE(err1.context().empty());
    
    Error err2(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, "/tmp/test.db");
    EXPECT_EQ(err2.code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    EXPECT_EQ(err2.context(), "/tmp/test.db");
}

TEST_F(ExpectedTest, ErrorMessage) {
    Error err(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, "/tmp/test.db");
    std::string msg = err.message();
    
    // Should contain formatted message with context
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("test.db"), std::string::npos);
}

TEST_F(ExpectedTest, ErrorMetadata) {
    Error err(errors::ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
    auto metadata = err.metadata();
    
    EXPECT_EQ(metadata.category, "LLM");
    EXPECT_EQ(metadata.severity, "Error");
    EXPECT_FALSE(metadata.message_template.empty());
    EXPECT_FALSE(metadata.solution.empty());
}

// Test Result<T> success cases
TEST_F(ExpectedTest, ResultSuccess) {
    Result<int> result = Ok(42);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
    EXPECT_EQ(result.value(), 42);
}

TEST_F(ExpectedTest, ResultSuccessString) {
    Result<std::string> result = Ok(std::string("success"));
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, "success");
}

// Test Result<T> error cases
TEST_F(ExpectedTest, ResultError) {
    Result<int> result = Err<int>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, "/tmp/test.db");
    
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    EXPECT_EQ(result.error().context(), "/tmp/test.db");
}

// Test Result<void> for void operations
TEST_F(ExpectedTest, ResultVoid) {
    Result<void> success = OkVoid();
    EXPECT_TRUE(success.has_value());
    
    Result<void> failure = ErrVoid(errors::ErrorCode::ERR_STORAGE_PERMISSION_DENIED, "/etc/config");
    EXPECT_FALSE(failure.has_value());
    EXPECT_EQ(failure.error().code(), errors::ErrorCode::ERR_STORAGE_PERMISSION_DENIED);
}

// Test fromNullable conversion
TEST_F(ExpectedTest, FromNullableSuccess) {
    int value = 42;
    int* ptr = &value;
    
    auto result = fromNullable(ptr, errors::ErrorCode::ERR_INDEX_NOT_FOUND);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, ptr);
}

TEST_F(ExpectedTest, FromNullableFailure) {
    int* ptr = nullptr;
    
    auto result = fromNullable(ptr, errors::ErrorCode::ERR_INDEX_NOT_FOUND, "myindex");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_INDEX_NOT_FOUND);
    EXPECT_EQ(result.error().context(), "myindex");
}

// Test fromBoolStatus conversion
TEST_F(ExpectedTest, FromBoolStatusSuccess) {
    auto result = fromBoolStatus(true, "", errors::ErrorCode::ERR_STORAGE_CORRUPTION);
    EXPECT_TRUE(result.has_value());
}

TEST_F(ExpectedTest, FromBoolStatusFailure) {
    auto result = fromBoolStatus(false, "write failed", errors::ErrorCode::ERR_STORAGE_DISK_FULL);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_STORAGE_DISK_FULL);
    EXPECT_EQ(result.error().context(), "write failed");
}

// Test fromOptional conversion
TEST_F(ExpectedTest, FromOptionalSuccess) {
    std::optional<int> opt = 42;
    
    auto result = fromOptional(std::move(opt), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST_F(ExpectedTest, FromOptionalFailure) {
    std::optional<int> opt = std::nullopt;
    
    auto result = fromOptional(std::move(opt), errors::ErrorCode::ERR_QUERY_PARSE_FAILED, "invalid syntax");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
    EXPECT_EQ(result.error().context(), "invalid syntax");
}

// Test new error codes
TEST_F(ExpectedTest, NewIndexErrorCodes) {
    Error err1(errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED);
    auto meta1 = err1.metadata();
    EXPECT_EQ(meta1.category, "Index");
    
    Error err2(errors::ErrorCode::ERR_INDEX_CREATION_FAILED);
    auto meta2 = err2.metadata();
    EXPECT_EQ(meta2.category, "Index");
}

TEST_F(ExpectedTest, NewQueryErrorCodes) {
    Error err1(errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
    auto meta1 = err1.metadata();
    EXPECT_EQ(meta1.category, "Query");
    
    Error err2(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED);
    auto meta2 = err2.metadata();
    EXPECT_EQ(meta2.category, "Query");
}

TEST_F(ExpectedTest, NewAPIErrorCodes) {
    Error err1(errors::ErrorCode::ERR_API_INVALID_REQUEST);
    auto meta1 = err1.metadata();
    EXPECT_EQ(meta1.category, "API");
    
    Error err2(errors::ErrorCode::ERR_API_UNAUTHORIZED);
    auto meta2 = err2.metadata();
    EXPECT_EQ(meta2.category, "API");
}

TEST_F(ExpectedTest, NewPluginErrorCodes) {
    Error err1(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
    auto meta1 = err1.metadata();
    EXPECT_EQ(meta1.category, "Plugin");
    
    Error err2(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED);
    auto meta2 = err2.metadata();
    EXPECT_EQ(meta2.category, "Plugin");
}

// Test monadic operations (and_then)
TEST_F(ExpectedTest, MonadicAndThen) {
    auto divide = [](int x, int y) -> Result<int> {
        if (y == 0) {
            return Err<int>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED, "division by zero");
        }
        return Ok(x / y);
    };
    
    auto result1 = divide(10, 2).and_then([](int x) -> Result<int> {
        return Ok(x * 2);
    });
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(*result1, 10);
    
    auto result2 = divide(10, 0).and_then([](int x) -> Result<int> {
        return Ok(x * 2);  // Should not be called
    });
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error().code(), errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED);
}

// Test value_or
TEST_F(ExpectedTest, ValueOr) {
    Result<int> success = Ok(42);
    EXPECT_EQ(success.value_or(0), 42);
    
    Result<int> failure = Err<int>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    EXPECT_EQ(failure.value_or(99), 99);
}

// Test practical usage pattern
Result<std::string> readConfig(const std::string& path) {
    if (path.empty()) {
        return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, "empty path");
    }
    if (path == "/invalid") {
        return Err<std::string>(errors::ErrorCode::ERR_STORAGE_PERMISSION_DENIED, path);
    }
    return Ok(std::string("config_data"));
}

TEST_F(ExpectedTest, PracticalUsagePattern) {
    // Success case
    auto result1 = readConfig("/etc/config.yaml");
    EXPECT_TRUE(result1.has_value());
    EXPECT_EQ(*result1, "config_data");
    
    // Error case 1
    auto result2 = readConfig("");
    EXPECT_FALSE(result2.has_value());
    EXPECT_EQ(result2.error().code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
    
    // Error case 2
    auto result3 = readConfig("/invalid");
    EXPECT_FALSE(result3.has_value());
    EXPECT_EQ(result3.error().code(), errors::ErrorCode::ERR_STORAGE_PERMISSION_DENIED);
    EXPECT_EQ(result3.error().context(), "/invalid");
}

} // namespace test
} // namespace themis
