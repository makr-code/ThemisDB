#include <gtest/gtest.h>
#include "content/content_logger.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using namespace themis::content;
using json = nlohmann::json;

// ============================================================================
// ContentLogger Basic Tests
// ============================================================================

TEST(ContentLoggerTest, Construction) {
    ContentLogger logger;
    EXPECT_TRUE(logger.getCorrelationId().empty());
    EXPECT_TRUE(logger.isJsonFormatting());
    EXPECT_TRUE(logger.isPiiSanitization());
}

TEST(ContentLoggerTest, ConstructionWithCorrelationId) {
    ContentLogger logger("test-correlation-123");
    EXPECT_EQ(logger.getCorrelationId(), "test-correlation-123");
}

TEST(ContentLoggerTest, SetCorrelationId) {
    ContentLogger logger;
    logger.setCorrelationId("new-id-456");
    EXPECT_EQ(logger.getCorrelationId(), "new-id-456");
}

TEST(ContentLoggerTest, SetJsonFormatting) {
    ContentLogger logger;
    
    logger.setJsonFormatting(false);
    EXPECT_FALSE(logger.isJsonFormatting());
    
    logger.setJsonFormatting(true);
    EXPECT_TRUE(logger.isJsonFormatting());
}

TEST(ContentLoggerTest, SetPiiSanitization) {
    ContentLogger logger;
    
    logger.setPiiSanitization(false);
    EXPECT_FALSE(logger.isPiiSanitization());
    
    logger.setPiiSanitization(true);
    EXPECT_TRUE(logger.isPiiSanitization());
}

// ============================================================================
// Content Operation Logging Tests
// ============================================================================

TEST(ContentLoggerTest, LogIngestion) {
    ContentLogger logger("corr-001");
    
    // Should not throw
    EXPECT_NO_THROW({
        logger.logIngestion("content-123", "application/pdf", 10000, "document.pdf");
    });
}

TEST(ContentLoggerTest, LogIngestionWithoutFilename) {
    ContentLogger logger("corr-002");
    
    EXPECT_NO_THROW({
        logger.logIngestion("content-456", "image/png", 5000);
    });
}

TEST(ContentLoggerTest, LogValidationSuccess) {
    ContentLogger logger("corr-003");
    
    EXPECT_NO_THROW({
        logger.logValidation("content-789", "text/plain", 1000, true, 0, 15.5);
    });
}

TEST(ContentLoggerTest, LogValidationFailure) {
    ContentLogger logger("corr-004");
    
    EXPECT_NO_THROW({
        logger.logValidation("content-999", "application/pdf", 100000000, false, 1001, 5.2);
    });
}

TEST(ContentLoggerTest, LogProcessingSuccess) {
    ContentLogger logger("corr-005");
    
    EXPECT_NO_THROW({
        logger.logProcessing("content-111", "extraction", 250.5, true);
    });
}

TEST(ContentLoggerTest, LogProcessingFailure) {
    ContentLogger logger("corr-006");
    
    EXPECT_NO_THROW({
        logger.logProcessing("content-222", "chunking", 100.0, false, 1100);
    });
}

TEST(ContentLoggerTest, LogError) {
    ContentLogger logger("corr-007");
    
    EXPECT_NO_THROW({
        logger.logError("content-333", "embedding", 1105, "Failed to generate embeddings", "processing");
    });
}

TEST(ContentLoggerTest, LogTimeout) {
    ContentLogger logger("corr-008");
    
    EXPECT_NO_THROW({
        logger.logTimeout("content-444", "extraction", 60.0, 65.5);
    });
}

TEST(ContentLoggerTest, LogCacheHit) {
    ContentLogger logger("corr-009");
    
    EXPECT_NO_THROW({
        logger.logCache("content-555", true);
    });
}

TEST(ContentLoggerTest, LogCacheMiss) {
    ContentLogger logger("corr-010");
    
    EXPECT_NO_THROW({
        logger.logCache("content-666", false);
    });
}

// ============================================================================
// Structured Logging Tests
// ============================================================================

TEST(ContentLoggerTest, LogInfo) {
    ContentLogger logger("corr-011");
    
    json metadata;
    metadata["key1"] = "value1";
    metadata["key2"] = 42;
    
    EXPECT_NO_THROW({
        logger.info("test.event", "Test message", metadata);
    });
}

TEST(ContentLoggerTest, LogWarn) {
    ContentLogger logger("corr-012");
    
    EXPECT_NO_THROW({
        logger.warn("test.warning", "Warning message");
    });
}

TEST(ContentLoggerTest, LogStructuredError) {
    ContentLogger logger("corr-013");
    
    EXPECT_NO_THROW({
        logger.error("test.error", "Error message");
    });
}

TEST(ContentLoggerTest, LogDebug) {
    ContentLogger logger("corr-014");
    
    EXPECT_NO_THROW({
        logger.debug("test.debug", "Debug message");
    });
}

// ============================================================================
// ContentOperationTimer Tests
// ============================================================================

TEST(ContentOperationTimerTest, BasicTiming) {
    ContentLogger logger("corr-timer-001");
    
    {
        ContentOperationTimer timer(logger, "content-timer-1", "test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timer.setSuccess(true);
    }
    // Timer logs on destruction
}

TEST(ContentOperationTimerTest, FailureWithErrorCode) {
    ContentLogger logger("corr-timer-002");
    
    {
        ContentOperationTimer timer(logger, "content-timer-2", "test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        timer.setSuccess(false);
        timer.setErrorCode(1234);
    }
}

TEST(ContentOperationTimerTest, GetElapsedMs) {
    ContentLogger logger("corr-timer-003");
    
    ContentOperationTimer timer(logger, "content-timer-3", "test_operation");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    
    double elapsed = timer.getElapsedMs();
    EXPECT_GE(elapsed, 15.0);  // Should be at least 15ms
    EXPECT_LE(elapsed, 50.0);  // But not more than 50ms (with tolerance)
}

TEST(ContentOperationTimerTest, DefaultSuccess) {
    ContentLogger logger("corr-timer-004");
    
    {
        ContentOperationTimer timer(logger, "content-timer-4", "test_operation");
        // Default success is true, so should log success
    }
}

// ============================================================================
// PII Sanitization Tests (Manual Testing)
// ============================================================================

// Note: These tests verify that the methods don't throw, but don't validate
// sanitization output since that would require accessing internal methods
// or capturing log output

TEST(ContentLoggerTest, SanitizeFilenameWithEmail) {
    ContentLogger logger;
    logger.setPiiSanitization(true);
    
    EXPECT_NO_THROW({
        logger.logIngestion("content-email", "text/plain", 1000, "user@example.com_document.txt");
    });
}

TEST(ContentLoggerTest, SanitizeFilenameWithPhone) {
    ContentLogger logger;
    logger.setPiiSanitization(true);
    
    EXPECT_NO_THROW({
        logger.logIngestion("content-phone", "text/plain", 1000, "call_123-456-7890.txt");
    });
}

TEST(ContentLoggerTest, SanitizeFilenameWithSSN) {
    ContentLogger logger;
    logger.setPiiSanitization(true);
    
    EXPECT_NO_THROW({
        logger.logIngestion("content-ssn", "text/plain", 1000, "file_123-45-6789.txt");
    });
}

TEST(ContentLoggerTest, SanitizeErrorMessageWithPII) {
    ContentLogger logger;
    logger.setPiiSanitization(true);
    
    EXPECT_NO_THROW({
        logger.logError(
            "content-pii",
            "validation",
            1000,
            "Failed to process file from user@example.com with phone 555-1234",
            "validation"
        );
    });
}

TEST(ContentLoggerTest, DisablePiiSanitization) {
    ContentLogger logger;
    logger.setPiiSanitization(false);
    
    // Even with sanitization disabled, should not throw
    EXPECT_NO_THROW({
        logger.logIngestion("content-no-san", "text/plain", 1000, "user@example.com_document.txt");
    });
}

// ============================================================================
// Formatting Tests
// ============================================================================

TEST(ContentLoggerTest, JsonFormattingEnabled) {
    ContentLogger logger;
    logger.setJsonFormatting(true);
    
    EXPECT_NO_THROW({
        logger.info("test.json", "Test message");
    });
}

TEST(ContentLoggerTest, JsonFormattingDisabled) {
    ContentLogger logger;
    logger.setJsonFormatting(false);
    
    EXPECT_NO_THROW({
        logger.info("test.text", "Test message");
    });
}

// ============================================================================
// Concurrent Logging Tests
// ============================================================================

TEST(ContentLoggerTest, ConcurrentLogging) {
    ContentLogger logger("concurrent-test");
    
    auto worker = [&logger](int worker_id) {
        for (int i = 0; i < 100; i++) {
            std::string content_id = "content-worker-" + std::to_string(worker_id) + "-" + std::to_string(i);
            logger.logIngestion(content_id, "application/pdf", 1000);
            logger.logValidation(content_id, "application/pdf", 1000, true, 0, 10.0);
        }
    };
    
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    // If we get here without crashing, concurrent logging works
    SUCCEED();
}

TEST(ContentLoggerTest, ConcurrentTimers) {
    ContentLogger logger("concurrent-timers");
    
    auto worker = [&logger](int worker_id) {
        for (int i = 0; i < 50; i++) {
            std::string content_id = "content-timer-" + std::to_string(worker_id) + "-" + std::to_string(i);
            ContentOperationTimer timer(logger, content_id, "test_op");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            timer.setSuccess(true);
        }
    };
    
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    SUCCEED();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(ContentLoggerTest, EmptyStrings) {
    ContentLogger logger;
    
    EXPECT_NO_THROW({
        logger.logIngestion("", "", 0, "");
        logger.logValidation("", "", 0, true);
        logger.logProcessing("", "", 0.0, true);
        logger.logError("", "", 0, "", "");
    });
}

TEST(ContentLoggerTest, VeryLongStrings) {
    ContentLogger logger;
    
    std::string long_id(10000, 'x');
    std::string long_message(10000, 'y');
    
    EXPECT_NO_THROW({
        logger.logIngestion(long_id, "text/plain", 0);
        logger.logError(long_id, "test", 0, long_message);
    });
}

TEST(ContentLoggerTest, SpecialCharacters) {
    ContentLogger logger;
    
    EXPECT_NO_THROW({
        logger.logIngestion("content-<>&\"'", "text/plain", 0, "file\n\t\r.txt");
        logger.logError("content-special", "op", 0, "Error: <tag> & \"quote\"");
    });
}
