// Test: CDC Structured Error Codes
// Tests for structured error handling in CDC module

#include <gtest/gtest.h>
#include "cdc/cdc_error.h"

using namespace themis;
using namespace themis::cdc;

// ===== Error Code Tests =====

TEST(CDCErrorTest, ErrorCodeValues) {
    // Verify error code ranges
    EXPECT_EQ(static_cast<int>(ErrorCode::SUCCESS), 0);
    EXPECT_GE(static_cast<int>(ErrorCode::SEQUENCE_GENERATION_FAILED), 100);
    EXPECT_LT(static_cast<int>(ErrorCode::SEQUENCE_GENERATION_FAILED), 200);
    EXPECT_GE(static_cast<int>(ErrorCode::EVENT_RECORD_FAILED), 200);
    EXPECT_LT(static_cast<int>(ErrorCode::EVENT_RECORD_FAILED), 300);
    EXPECT_GE(static_cast<int>(ErrorCode::BUFFER_OVERFLOW), 300);
    EXPECT_LT(static_cast<int>(ErrorCode::BUFFER_OVERFLOW), 400);
}

TEST(CDCErrorTest, ErrorSeverityLevels) {
    // Verify severity levels exist
    ErrorSeverity info = ErrorSeverity::INFO;
    ErrorSeverity warning = ErrorSeverity::WARNING;
    ErrorSeverity error = ErrorSeverity::ERROR;
    ErrorSeverity critical = ErrorSeverity::CRITICAL;
    
    EXPECT_NE(info, warning);
    EXPECT_NE(warning, error);
    EXPECT_NE(error, critical);
}

// ===== CDCException Tests =====

TEST(CDCExceptionTest, BasicConstruction) {
    CDCException ex(
        ErrorCode::EVENT_RECORD_FAILED,
        ErrorSeverity::ERROR,
        "Test error message",
        "Test context"
    );
    
    EXPECT_EQ(ex.code(), ErrorCode::EVENT_RECORD_FAILED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::ERROR);
    EXPECT_EQ(ex.message(), "Test error message");
    EXPECT_EQ(ex.context(), "Test context");
}

TEST(CDCExceptionTest, CodeValue) {
    CDCException ex(
        ErrorCode::SEQUENCE_GENERATION_FAILED,
        ErrorSeverity::CRITICAL,
        "Sequence failed"
    );
    
    EXPECT_EQ(ex.codeValue(), 100);
}

TEST(CDCExceptionTest, WhatMessage) {
    CDCException ex(
        ErrorCode::BUFFER_OVERFLOW,
        ErrorSeverity::WARNING,
        "Buffer full",
        "size=1000"
    );
    
    std::string what = ex.what();
    EXPECT_NE(what.find("BUFFER_OVERFLOW"), std::string::npos);
    EXPECT_NE(what.find("WARNING"), std::string::npos);
    EXPECT_NE(what.find("Buffer full"), std::string::npos);
    EXPECT_NE(what.find("size=1000"), std::string::npos);
}

TEST(CDCExceptionTest, JsonSerialization) {
    CDCException ex(
        ErrorCode::RETRY_EXHAUSTED,
        ErrorSeverity::ERROR,
        "All retries failed",
        "attempts=3"
    );
    
    auto json = ex.toJson();
    
    EXPECT_EQ(json["code"], 500);
    EXPECT_EQ(json["code_name"], "RETRY_EXHAUSTED");
    EXPECT_EQ(json["severity"], "ERROR");
    EXPECT_EQ(json["message"], "All retries failed");
    EXPECT_EQ(json["context"], "attempts=3");
}

TEST(CDCExceptionTest, IsRetryable) {
    // Retryable errors
    CDCException retryable1(ErrorCode::DB_OPERATION_FAILED, ErrorSeverity::ERROR, "DB error");
    EXPECT_TRUE(retryable1.isRetryable());
    
    CDCException retryable2(ErrorCode::EVENT_RECORD_FAILED, ErrorSeverity::ERROR, "Record failed");
    EXPECT_TRUE(retryable2.isRetryable());
    
    // Non-retryable errors
    CDCException nonRetryable1(ErrorCode::EVENT_KEY_EMPTY, ErrorSeverity::ERROR, "Empty key");
    EXPECT_FALSE(nonRetryable1.isRetryable());
    
    CDCException nonRetryable2(ErrorCode::INVALID_ARGUMENT, ErrorSeverity::ERROR, "Bad arg");
    EXPECT_FALSE(nonRetryable2.isRetryable());
}

TEST(CDCExceptionTest, IsDataLossRisk) {
    // Data loss risks
    CDCException risk1(ErrorCode::SEQUENCE_GENERATION_FAILED, ErrorSeverity::CRITICAL, "Sequence failed");
    EXPECT_TRUE(risk1.isDataLossRisk());
    
    CDCException risk2(ErrorCode::RETRY_EXHAUSTED, ErrorSeverity::ERROR, "No more retries");
    EXPECT_TRUE(risk2.isDataLossRisk());
    
    CDCException risk3(ErrorCode::BUFFER_OVERFLOW, ErrorSeverity::CRITICAL, "Buffer full");
    EXPECT_TRUE(risk3.isDataLossRisk());
    
    // Not data loss risk
    CDCException noRisk(ErrorCode::COMPRESSION_FAILED, ErrorSeverity::WARNING, "Compression failed");
    EXPECT_FALSE(noRisk.isDataLossRisk());
}

// ===== Helper Functions Tests =====

TEST(CDCErrorHelpersTest, SequenceGenerationFailed) {
    auto ex = error::sequenceGenerationFailed("RocksDB write error");
    
    EXPECT_EQ(ex.code(), ErrorCode::SEQUENCE_GENERATION_FAILED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::CRITICAL);
    EXPECT_NE(ex.message().find("Sequence generation failed"), std::string::npos);
    EXPECT_EQ(ex.context(), "RocksDB write error");
}

TEST(CDCErrorHelpersTest, EventRecordFailed) {
    auto ex = error::eventRecordFailed("Serialization error");
    
    EXPECT_EQ(ex.code(), ErrorCode::EVENT_RECORD_FAILED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::ERROR);
    EXPECT_TRUE(ex.isRetryable());
}

TEST(CDCErrorHelpersTest, BufferOverflow) {
    auto ex = error::bufferOverflow(1000, 500);
    
    EXPECT_EQ(ex.code(), ErrorCode::BUFFER_OVERFLOW);
    EXPECT_EQ(ex.severity(), ErrorSeverity::WARNING);
    EXPECT_NE(ex.context().find("current=1000"), std::string::npos);
    EXPECT_NE(ex.context().find("max=500"), std::string::npos);
}

TEST(CDCErrorHelpersTest, CompressionFailed) {
    auto ex = error::compressionFailed("Zstd error");
    
    EXPECT_EQ(ex.code(), ErrorCode::COMPRESSION_FAILED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::WARNING);
    EXPECT_FALSE(ex.isDataLossRisk());
}

TEST(CDCErrorHelpersTest, DecompressionFailed) {
    auto ex = error::decompressionFailed("Corrupt data");
    
    EXPECT_EQ(ex.code(), ErrorCode::DECOMPRESSION_FAILED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::ERROR);
}

TEST(CDCErrorHelpersTest, RetryExhausted) {
    auto ex = error::retryExhausted(3, "DB connection timeout");
    
    EXPECT_EQ(ex.code(), ErrorCode::RETRY_EXHAUSTED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::ERROR);
    EXPECT_TRUE(ex.isDataLossRisk());
    EXPECT_NE(ex.context().find("attempts=3"), std::string::npos);
}

TEST(CDCErrorHelpersTest, RateLimitExceeded) {
    auto ex = error::rateLimitExceeded(15000, 10000);
    
    EXPECT_EQ(ex.code(), ErrorCode::RATE_LIMIT_EXCEEDED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::WARNING);
    EXPECT_NE(ex.context().find("current=15000"), std::string::npos);
    EXPECT_NE(ex.context().find("limit=10000"), std::string::npos);
}

TEST(CDCErrorHelpersTest, DBOperationFailed) {
    auto ex = error::dbOperationFailed("Put", "Status: IOError");
    
    EXPECT_EQ(ex.code(), ErrorCode::DB_OPERATION_FAILED);
    EXPECT_EQ(ex.severity(), ErrorSeverity::ERROR);
    EXPECT_TRUE(ex.isRetryable());
    EXPECT_NE(ex.message().find("Put"), std::string::npos);
}

TEST(CDCErrorHelpersTest, InvalidArgument) {
    auto ex = error::invalidArgument("event.key", "Key cannot be empty");
    
    EXPECT_EQ(ex.code(), ErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(ex.severity(), ErrorSeverity::ERROR);
    EXPECT_FALSE(ex.isRetryable());
    EXPECT_NE(ex.message().find("event.key"), std::string::npos);
    EXPECT_NE(ex.context().find("Key cannot be empty"), std::string::npos);
}

// ===== Exception Throwing and Catching Tests =====

TEST(CDCExceptionTest, ThrowAndCatch) {
    try {
        throw error::sequenceGenerationFailed("Test error");
        FAIL() << "Exception should have been thrown";
    } catch (const CDCException& ex) {
        EXPECT_EQ(ex.code(), ErrorCode::SEQUENCE_GENERATION_FAILED);
        EXPECT_EQ(ex.severity(), ErrorSeverity::CRITICAL);
    }
}

TEST(CDCExceptionTest, CatchAsStdException) {
    try {
        throw error::eventRecordFailed("Test");
        FAIL() << "Exception should have been thrown";
    } catch (const std::runtime_error& ex) {
        // Should be catchable as std::runtime_error
        EXPECT_NE(std::string(ex.what()).find("EVENT_RECORD_FAILED"), std::string::npos);
    }
}

// ===== Error Code Grouping Tests =====

TEST(CDCErrorTest, ErrorCodeRanges) {
    // Sequence errors: 100-199
    EXPECT_GE(static_cast<int>(ErrorCode::SEQUENCE_GENERATION_FAILED), 100);
    EXPECT_LE(static_cast<int>(ErrorCode::SEQUENCE_READ_FAILED), 199);
    
    // Event errors: 200-299
    EXPECT_GE(static_cast<int>(ErrorCode::EVENT_RECORD_FAILED), 200);
    EXPECT_LE(static_cast<int>(ErrorCode::EVENT_PAYLOAD_TOO_LARGE), 299);
    
    // Buffer errors: 300-399
    EXPECT_GE(static_cast<int>(ErrorCode::BUFFER_OVERFLOW), 300);
    EXPECT_LE(static_cast<int>(ErrorCode::BUFFER_ALREADY_RUNNING), 399);
    
    // Compression errors: 400-499
    EXPECT_GE(static_cast<int>(ErrorCode::COMPRESSION_FAILED), 400);
    EXPECT_LE(static_cast<int>(ErrorCode::COMPRESSION_RATIO_TOO_LOW), 499);
}

// ===== Multiple Error Scenarios =====

TEST(CDCErrorTest, MultipleErrors) {
    std::vector<CDCException> errors;
    
    errors.push_back(error::compressionFailed("Test 1"));
    errors.push_back(error::bufferOverflow(100, 50));
    errors.push_back(error::retryExhausted(3, "timeout"));
    
    EXPECT_EQ(errors.size(), 3);
    EXPECT_EQ(errors[0].code(), ErrorCode::COMPRESSION_FAILED);
    EXPECT_EQ(errors[1].code(), ErrorCode::BUFFER_OVERFLOW);
    EXPECT_EQ(errors[2].code(), ErrorCode::RETRY_EXHAUSTED);
    
    // Count data loss risks
    int dataLossCount = 0;
    for (const auto& ex : errors) {
        if (ex.isDataLossRisk()) {
            dataLossCount++;
        }
    }
    EXPECT_EQ(dataLossCount, 1);  // Only RETRY_EXHAUSTED
}
