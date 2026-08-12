#include <gtest/gtest.h>
#include "content/content_errors.h"
#include <nlohmann/json.hpp>

using namespace themis::content;
using json = nlohmann::json;

// ============================================================================
// ContentError Tests
// ============================================================================

TEST(ContentErrorTest, DefaultConstruction) {
    ContentError err;
    EXPECT_EQ(err.code, ContentErrorCode::OK);
    EXPECT_TRUE(err.isOk());
    EXPECT_FALSE(err.failed());
}

TEST(ContentErrorTest, OkFactory) {
    auto err = ContentError::ok();
    EXPECT_EQ(err.code, ContentErrorCode::OK);
    EXPECT_TRUE(err.isOk());
    EXPECT_FALSE(err.failed());
    EXPECT_EQ(err.getHttpStatus(), 200);
}

TEST(ContentErrorTest, ErrorFactory) {
    auto err = ContentError::error(
        ContentErrorCode::CONTENT_SIZE_EXCEEDED,
        "File too large"
    );
    
    EXPECT_EQ(err.code, ContentErrorCode::CONTENT_SIZE_EXCEEDED);
    EXPECT_FALSE(err.isOk());
    EXPECT_TRUE(err.failed());
    EXPECT_EQ(err.message, "File too large");
}

TEST(ContentErrorTest, ErrorFactoryWithDefaultMessage) {
    auto err = ContentError::error(
        ContentErrorCode::CONTENT_MALWARE_DETECTED,
        ""  // Empty message, should use default
    );
    
    EXPECT_EQ(err.code, ContentErrorCode::CONTENT_MALWARE_DETECTED);
    EXPECT_FALSE(err.message.empty());
    EXPECT_EQ(err.message, "Malware detected in content");
}

TEST(ContentErrorTest, ErrorFactoryWithDetails) {
    auto err = ContentError::error(
        ContentErrorCode::CONTENT_PROCESSING_FAILED,
        "Processing error",
        "Stack trace: ..."
    );
    
    EXPECT_EQ(err.details, "Stack trace: ...");
}

// ============================================================================
// Error Classification Tests
// ============================================================================

TEST(ContentErrorTest, ClientErrors) {
    // Validation errors
    auto err1 = ContentError::error(ContentErrorCode::CONTENT_INVALID_INPUT, "");
    EXPECT_TRUE(err1.isClientError());
    EXPECT_FALSE(err1.isServerError());
    EXPECT_EQ(err1.getHttpStatus(), 400);
    
    // Size exceeded
    auto err2 = ContentError::error(ContentErrorCode::CONTENT_SIZE_EXCEEDED, "");
    EXPECT_TRUE(err2.isClientError());
    EXPECT_EQ(err2.getHttpStatus(), 413);
    
    // Unauthorized
    auto err3 = ContentError::error(ContentErrorCode::CONTENT_UNAUTHORIZED, "");
    EXPECT_EQ(err3.getHttpStatus(), 401);
    
    // Format unsupported
    auto err4 = ContentError::error(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED, "");
    EXPECT_TRUE(err4.isClientError());
    EXPECT_EQ(err4.getHttpStatus(), 415);
    
    // Rate limit
    auto err5 = ContentError::error(ContentErrorCode::CONTENT_RATE_LIMIT_EXCEEDED, "");
    EXPECT_TRUE(err5.isClientError());
    EXPECT_EQ(err5.getHttpStatus(), 429);
}

TEST(ContentErrorTest, ServerErrors) {
    // Processing errors
    auto err1 = ContentError::error(ContentErrorCode::CONTENT_PROCESSING_FAILED, "");
    EXPECT_TRUE(err1.isServerError());
    EXPECT_EQ(err1.getHttpStatus(), 500);
    
    // Storage errors
    auto err2 = ContentError::error(ContentErrorCode::CONTENT_STORAGE_FAILED, "");
    EXPECT_TRUE(err2.isServerError());
    EXPECT_EQ(err2.getHttpStatus(), 500);
    
    // Service unavailable
    auto err3 = ContentError::error(ContentErrorCode::CONTENT_PROCESSOR_UNAVAILABLE, "");
    EXPECT_TRUE(err3.isServerError());
    EXPECT_EQ(err3.getHttpStatus(), 503);
}

TEST(ContentErrorTest, SecurityErrors) {
    auto err1 = ContentError::error(ContentErrorCode::CONTENT_MALWARE_DETECTED, "");
    EXPECT_EQ(err1.getHttpStatus(), 403);
    
    auto err2 = ContentError::error(ContentErrorCode::CONTENT_PII_DETECTED, "");
    EXPECT_EQ(err2.getHttpStatus(), 403);
    
    auto err3 = ContentError::error(ContentErrorCode::CONTENT_ABUSE_DETECTED, "");
    EXPECT_EQ(err3.getHttpStatus(), 403);
}

TEST(ContentErrorTest, NotFoundError) {
    auto err = ContentError::error(ContentErrorCode::CONTENT_NOT_FOUND, "");
    EXPECT_EQ(err.getHttpStatus(), 404);
}

// ============================================================================
// Retryable Errors Tests
// ============================================================================

TEST(ContentErrorTest, RetryableErrors) {
    // Timeout is retryable
    auto err1 = ContentError::error(ContentErrorCode::CONTENT_TIMEOUT, "");
    EXPECT_TRUE(err1.isRetryable());
    
    // Processor unavailable is retryable
    auto err2 = ContentError::error(ContentErrorCode::CONTENT_PROCESSOR_UNAVAILABLE, "");
    EXPECT_TRUE(err2.isRetryable());
    
    // Queue full is retryable
    auto err3 = ContentError::error(ContentErrorCode::CONTENT_QUEUE_FULL, "");
    EXPECT_TRUE(err3.isRetryable());
    
    // Backpressure is retryable
    auto err4 = ContentError::error(ContentErrorCode::CONTENT_BACKPRESSURE, "");
    EXPECT_TRUE(err4.isRetryable());
}

TEST(ContentErrorTest, NonRetryableErrors) {
    // Validation errors are not retryable
    auto err1 = ContentError::error(ContentErrorCode::CONTENT_INVALID_INPUT, "");
    EXPECT_FALSE(err1.isRetryable());
    
    // Security errors are not retryable
    auto err2 = ContentError::error(ContentErrorCode::CONTENT_MALWARE_DETECTED, "");
    EXPECT_FALSE(err2.isRetryable());
    
    // Size exceeded is not retryable
    auto err3 = ContentError::error(ContentErrorCode::CONTENT_SIZE_EXCEEDED, "");
    EXPECT_FALSE(err3.isRetryable());
}

// ============================================================================
// JSON Serialization Tests
// ============================================================================

TEST(ContentErrorTest, ToJsonBasic) {
    auto err = ContentError::error(
        ContentErrorCode::CONTENT_SIZE_EXCEEDED,
        "File size exceeds 100MB limit"
    );
    err.correlation_id = "req-12345";
    
    json j = err.toJson();
    
    EXPECT_EQ(j["code"], static_cast<int>(ContentErrorCode::CONTENT_SIZE_EXCEEDED));
    EXPECT_EQ(j["error"], "CONTENT_SIZE_EXCEEDED");
    EXPECT_EQ(j["message"], "File size exceeds 100MB limit");
    EXPECT_EQ(j["correlation_id"], "req-12345");
    
    // Should not include details in basic JSON
    EXPECT_FALSE(j.contains("details"));
}

TEST(ContentErrorTest, ToJsonVerbose) {
    auto err = ContentError::error(
        ContentErrorCode::CONTENT_PROCESSING_FAILED,
        "Processing failed",
        "Internal error: division by zero at line 42"
    );
    err.correlation_id = "req-67890";
    err.content_id = "content-abc123";
    
    json j = err.toJsonVerbose();
    
    EXPECT_EQ(j["code"], static_cast<int>(ContentErrorCode::CONTENT_PROCESSING_FAILED));
    EXPECT_EQ(j["error"], "CONTENT_PROCESSING_FAILED");
    EXPECT_EQ(j["message"], "Processing failed");
    EXPECT_EQ(j["details"], "Internal error: division by zero at line 42");
    EXPECT_EQ(j["correlation_id"], "req-67890");
    EXPECT_EQ(j["content_id"], "content-abc123");
    EXPECT_EQ(j["category"], "processing");
    EXPECT_EQ(j["http_status"], 500);
    EXPECT_FALSE(j["retryable"].get<bool>());
    EXPECT_TRUE(j["server_error"].get<bool>());
}

TEST(ContentErrorTest, FromJson) {
    json j;
    j["code"] = 1001;
    j["message"] = "Size exceeded";
    j["details"] = "File was 200MB";
    j["correlation_id"] = "test-123";
    j["content_id"] = "content-xyz";
    j["metadata"] = {{"max_size", "100MB"}};
    
    auto err = ContentError::fromJson(j);
    
    EXPECT_EQ(err.code, ContentErrorCode::CONTENT_SIZE_EXCEEDED);
    EXPECT_EQ(err.message, "Size exceeded");
    EXPECT_EQ(err.details, "File was 200MB");
    EXPECT_EQ(err.correlation_id, "test-123");
    EXPECT_EQ(err.content_id, "content-xyz");
    EXPECT_EQ(err.metadata["max_size"], "100MB");
}

TEST(ContentErrorTest, RoundTripSerialization) {
    auto err1 = ContentError::error(
        ContentErrorCode::CONTENT_MALWARE_DETECTED,
        "Threat detected"
    );
    err1.correlation_id = "req-999";
    err1.metadata = {{"threat", "trojan.win32"}, {"scanner", "ClamAV"}};
    
    json j = err1.toJson();
    auto err2 = ContentError::fromJson(j);
    
    EXPECT_EQ(err1.code, err2.code);
    EXPECT_EQ(err1.message, err2.message);
    EXPECT_EQ(err1.correlation_id, err2.correlation_id);
    EXPECT_EQ(err1.metadata, err2.metadata);
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST(ContentErrorTest, ErrorCodeToString) {
    EXPECT_EQ(errorCodeToString(ContentErrorCode::OK), "OK");
    EXPECT_EQ(errorCodeToString(ContentErrorCode::CONTENT_SIZE_EXCEEDED), "CONTENT_SIZE_EXCEEDED");
    EXPECT_EQ(errorCodeToString(ContentErrorCode::CONTENT_MALWARE_DETECTED), "CONTENT_MALWARE_DETECTED");
    EXPECT_EQ(errorCodeToString(ContentErrorCode::CONTENT_TIMEOUT), "CONTENT_TIMEOUT");
}

TEST(ContentErrorTest, ErrorCodeCategory) {
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::OK), "success");
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::CONTENT_INVALID_INPUT), "validation");
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::CONTENT_PROCESSING_FAILED), "processing");
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::CONTENT_MALWARE_DETECTED), "security");
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::CONTENT_STORAGE_FAILED), "storage");
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::CONTENT_RATE_LIMIT_EXCEEDED), "rate_limiting");
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::CONTENT_MEMORY_LIMIT), "resources");
    EXPECT_EQ(errorCodeCategory(ContentErrorCode::CONTENT_INTERNAL_ERROR), "internal");
}

TEST(ContentErrorTest, IsSecurityError) {
    EXPECT_TRUE(isSecurityError(ContentErrorCode::CONTENT_MALWARE_DETECTED));
    EXPECT_TRUE(isSecurityError(ContentErrorCode::CONTENT_PII_DETECTED));
    EXPECT_TRUE(isSecurityError(ContentErrorCode::CONTENT_ABUSE_DETECTED));
    EXPECT_TRUE(isSecurityError(ContentErrorCode::CONTENT_UNAUTHORIZED));
    
    EXPECT_FALSE(isSecurityError(ContentErrorCode::CONTENT_SIZE_EXCEEDED));
    EXPECT_FALSE(isSecurityError(ContentErrorCode::CONTENT_PROCESSING_FAILED));
}

TEST(ContentErrorTest, IsValidationError) {
    EXPECT_TRUE(isValidationError(ContentErrorCode::CONTENT_INVALID_INPUT));
    EXPECT_TRUE(isValidationError(ContentErrorCode::CONTENT_SIZE_EXCEEDED));
    EXPECT_TRUE(isValidationError(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED));
    EXPECT_TRUE(isValidationError(ContentErrorCode::CONTENT_EMPTY));
    
    EXPECT_FALSE(isValidationError(ContentErrorCode::CONTENT_PROCESSING_FAILED));
    EXPECT_FALSE(isValidationError(ContentErrorCode::CONTENT_MALWARE_DETECTED));
}

TEST(ContentErrorTest, GetDefaultErrorMessage) {
    auto msg1 = getDefaultErrorMessage(ContentErrorCode::CONTENT_SIZE_EXCEEDED);
    EXPECT_FALSE(msg1.empty());
    EXPECT_EQ(msg1, "Content size exceeds maximum allowed limit");
    
    auto msg2 = getDefaultErrorMessage(ContentErrorCode::CONTENT_MALWARE_DETECTED);
    EXPECT_EQ(msg2, "Malware detected in content");
    
    auto msg3 = getDefaultErrorMessage(ContentErrorCode::OK);
    EXPECT_EQ(msg3, "Success");
}

// ============================================================================
// Metadata Tests
// ============================================================================

TEST(ContentErrorTest, ErrorWithMetadata) {
    auto err = ContentError::error(
        ContentErrorCode::CONTENT_SIZE_EXCEEDED,
        "File too large"
    );
    err.metadata = {
        {"file_size", 200000000},
        {"max_size", 100000000},
        {"filename", "large_file.pdf"}
    };
    
    json j = err.toJson();
    EXPECT_TRUE(j.contains("metadata"));
    EXPECT_EQ(j["metadata"]["file_size"], 200000000);
    EXPECT_EQ(j["metadata"]["max_size"], 100000000);
}

TEST(ContentErrorTest, ErrorWithCorrelationId) {
    auto err = ContentError::error(
        ContentErrorCode::CONTENT_TIMEOUT,
        "Request timed out"
    );
    err.correlation_id = "trace-abc-123";
    
    json j = err.toJson();
    EXPECT_EQ(j["correlation_id"], "trace-abc-123");
}
