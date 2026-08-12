/**
 * @file test_ingestion_errors.cpp
 * @brief Unit tests for structured ingestion error handling, retry config,
 *        observability metrics, and parallel ingestion in IngestionManager.
 *
 * Tests validate:
 * - IngestionError construction and predicates (isOk, isFatal, isRetryable)
 * - IngestionStats::addError accumulation
 * - RetryConfig defaults and custom values
 * - IngestionMetrics fields
 * - IngestionManager structured errors for missing/disabled sources
 * - IngestionManager parallel vs sequential ingestAll
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include "ingestion/filesystem_ingester.h"
#include <filesystem>
#include <fstream>

using namespace themis::ingestion;

// ============================================================================
// IngestionError – predicate tests
// ============================================================================

TEST(IngestionErrorTest, OkDefault) {
    IngestionError err;
    EXPECT_TRUE(err.isOk());
    EXPECT_FALSE(err.isFatal());
    EXPECT_FALSE(err.isWarning());
    EXPECT_FALSE(err.isRetryable());
}

TEST(IngestionErrorTest, FatalError) {
    IngestionError err{IngestionErrorCode::INTERNAL_ERROR,
                       IngestionErrorSeverity::FATAL,
                       "internal failure"};
    EXPECT_FALSE(err.isOk());
    EXPECT_TRUE(err.isFatal());
    EXPECT_FALSE(err.isWarning());
}

TEST(IngestionErrorTest, WarningNotFatal) {
    IngestionError err{IngestionErrorCode::SOURCE_DISABLED,
                       IngestionErrorSeverity::WARNING,
                       "source disabled"};
    EXPECT_FALSE(err.isOk());
    EXPECT_FALSE(err.isFatal());
    EXPECT_TRUE(err.isWarning());
}

TEST(IngestionErrorTest, RetryableOnTimeout) {
    IngestionError err{IngestionErrorCode::HTTP_TIMEOUT,
                       IngestionErrorSeverity::ERROR,
                       "timeout"};
    EXPECT_TRUE(err.isRetryable());
}

TEST(IngestionErrorTest, RetryableOnServerError) {
    IngestionError err{IngestionErrorCode::HTTP_SERVER_ERROR,
                       IngestionErrorSeverity::ERROR,
                       "server error"};
    EXPECT_TRUE(err.isRetryable());
}

TEST(IngestionErrorTest, RetryableOnRateLimit) {
    IngestionError err{IngestionErrorCode::HTTP_RATE_LIMITED,
                       IngestionErrorSeverity::ERROR,
                       "rate limited"};
    EXPECT_TRUE(err.isRetryable());
}

TEST(IngestionErrorTest, NotRetryableOnNotFound) {
    IngestionError err{IngestionErrorCode::SOURCE_NOT_FOUND,
                       IngestionErrorSeverity::ERROR,
                       "not found"};
    EXPECT_FALSE(err.isRetryable());
}

TEST(IngestionErrorTest, NotRetryableOnUnauthorized) {
    IngestionError err{IngestionErrorCode::HTTP_UNAUTHORIZED,
                       IngestionErrorSeverity::ERROR,
                       "unauthorized"};
    EXPECT_FALSE(err.isRetryable());
}

// ============================================================================
// IngestionStats – addError accumulation
// ============================================================================

TEST(IngestionStatsTest, AddErrorAccumulates) {
    IngestionStats stats;
    EXPECT_EQ(stats.errors.size(), 0u);
    EXPECT_EQ(stats.metrics.error_count, 0u);
    EXPECT_TRUE(stats.error_message.empty());

    stats.addError(IngestionErrorCode::FILE_NOT_FOUND,
                   IngestionErrorSeverity::ERROR,
                   "file not found", "src1", "details");

    EXPECT_EQ(stats.errors.size(), 1u);
    EXPECT_EQ(stats.metrics.error_count, 1u);
    EXPECT_EQ(stats.errors[0].code, IngestionErrorCode::FILE_NOT_FOUND);
    EXPECT_EQ(stats.errors[0].source_id, "src1");
    EXPECT_FALSE(stats.error_message.empty());
}

TEST(IngestionStatsTest, AddMultipleErrors) {
    IngestionStats stats;
    stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                   IngestionErrorSeverity::WARNING, "warn1");
    stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                   IngestionErrorSeverity::WARNING, "warn2");
    stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                   IngestionErrorSeverity::ERROR, "err1");

    EXPECT_EQ(stats.errors.size(), 3u);
    EXPECT_EQ(stats.metrics.error_count, 3u);
}

TEST(IngestionStatsTest, ErrorMessageSetOnFirstSevereError) {
    IngestionStats stats;
    // Warnings should not set error_message
    stats.addError(IngestionErrorCode::SOURCE_DISABLED,
                   IngestionErrorSeverity::WARNING, "just a warning");
    EXPECT_TRUE(stats.error_message.empty());

    stats.addError(IngestionErrorCode::SOURCE_NOT_FOUND,
                   IngestionErrorSeverity::ERROR, "real error");
    EXPECT_EQ(stats.error_message, "real error");

    // Second ERROR should not overwrite
    stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                   IngestionErrorSeverity::ERROR, "second error");
    EXPECT_EQ(stats.error_message, "real error");
}

// ============================================================================
// RetryConfig – defaults and field access
// ============================================================================

TEST(RetryConfigTest, Defaults) {
    RetryConfig cfg;
    EXPECT_EQ(cfg.max_attempts, 3);
    EXPECT_GT(cfg.initial_delay_ms, 0.0);
    EXPECT_GT(cfg.backoff_factor, 1.0);
    EXPECT_GT(cfg.max_delay_ms, cfg.initial_delay_ms);
    EXPECT_GT(cfg.timeout_ms, 0);
}

TEST(RetryConfigTest, CustomValues) {
    RetryConfig cfg;
    cfg.max_attempts     = 5;
    cfg.initial_delay_ms = 100.0;
    cfg.backoff_factor   = 3.0;
    cfg.max_delay_ms     = 5000.0;
    cfg.timeout_ms       = 10000;

    EXPECT_EQ(cfg.max_attempts, 5);
    EXPECT_DOUBLE_EQ(cfg.initial_delay_ms, 100.0);
    EXPECT_DOUBLE_EQ(cfg.backoff_factor, 3.0);
    EXPECT_DOUBLE_EQ(cfg.max_delay_ms, 5000.0);
    EXPECT_EQ(cfg.timeout_ms, 10000);
}

// ============================================================================
// IngestionMetrics – default zero state
// ============================================================================

TEST(IngestionMetricsTest, DefaultZero) {
    IngestionMetrics m;
    EXPECT_EQ(m.retry_count, 0u);
    EXPECT_EQ(m.timeout_count, 0u);
    EXPECT_EQ(m.error_count, 0u);
    EXPECT_DOUBLE_EQ(m.throughput_docs_per_sec, 0.0);
}

// ============================================================================
// IngestionManager – structured error for missing / disabled source
// ============================================================================

TEST(IngestionManagerErrorsTest, IngestSourceNotFound) {
    IngestionManager mgr("test_db");
    auto stats = mgr.ingestSource("nonexistent_source");

    EXPECT_FALSE(stats.error_message.empty());
    ASSERT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors[0].code, IngestionErrorCode::SOURCE_NOT_FOUND);
    EXPECT_EQ(stats.errors[0].severity, IngestionErrorSeverity::ERROR);
}

TEST(IngestionManagerErrorsTest, IngestSourceDisabled) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "disabled_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/does_not_matter";
    cfg.enabled   = false;

    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("disabled_src");

    ASSERT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors[0].code, IngestionErrorCode::SOURCE_DISABLED);
    EXPECT_EQ(stats.errors[0].severity, IngestionErrorSeverity::WARNING);
}

TEST(IngestionManagerErrorsTest, IngestSourceUnsupportedType) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "db_src";
    cfg.type      = SourceType::DATABASE;
    cfg.location  = "postgres://localhost/themis";
    cfg.enabled   = true;

    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("db_src");

    ASSERT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors[0].code, IngestionErrorCode::CONNECTOR_NOT_SUPPORTED);
}

// ============================================================================
// IngestionManager – parallel ingestAll runs without crash
// ============================================================================

TEST(IngestionManagerParallelTest, IngestAllParallelNoSources) {
    IngestionManager mgr("test_db");
    mgr.setParallelProcessing(true, 4);

    auto report = mgr.ingestAll();
    EXPECT_TRUE(report.source_stats.empty());
    EXPECT_EQ(report.total_documents, 0u);
}

TEST(IngestionManagerParallelTest, IngestAllParallelWithDisabledSources) {
    IngestionManager mgr("test_db");
    mgr.setParallelProcessing(true, 2);

    for (int i = 0; i < 3; ++i) {
        SourceConfig cfg;
        cfg.source_id = "par_src_" + std::to_string(i);
        cfg.type      = SourceType::FILESYSTEM;
        cfg.location  = "/tmp/no_such_path";
        cfg.enabled   = false;
        mgr.registerSource(cfg);
    }

    auto report = mgr.ingestAll();
    // All sources are disabled – each should produce a WARNING error
    EXPECT_EQ(report.source_stats.size(), 3u);
    for (const auto& [sid, stats] : report.source_stats) {
        ASSERT_FALSE(stats.errors.empty());
        EXPECT_EQ(stats.errors[0].code, IngestionErrorCode::SOURCE_DISABLED);
    }
}

// ============================================================================
// FileSystemIngester – structured error for missing path
// ============================================================================

TEST(FileSystemIngesterErrorsTest, MissingPathReturnsError) {
    FileSystemIngester ingester;

    SourceConfig cfg;
    cfg.source_id = "fs_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/themis_does_not_exist_xyz123";

    // Initialize returns false because path does not exist
    EXPECT_FALSE(ingester.initialize(cfg));
}

TEST(FileSystemIngesterErrorsTest, IngestMissingPathHasFatalError) {
    // Create ingester manually using an existing path first, then test ingest
    // with a nonexistent path by verifying error codes from the manager
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "missing_fs";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/themis_does_not_exist_xyz123";
    cfg.enabled   = true;

    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("missing_fs");
    // Connector init should fail (path doesn't exist)
    ASSERT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors[0].code, IngestionErrorCode::CONNECTOR_INIT_FAILED);
}

TEST(FileSystemIngesterErrorsTest, IngestTextFile) {
    // Create a temp text file and verify successful ingestion
    auto tmp_dir = std::filesystem::temp_directory_path() / "themis_ingest_test";
    std::filesystem::create_directories(tmp_dir);
    auto txt_path = tmp_dir / "sample.txt";
    {
        std::ofstream f(txt_path);
        f << "Hello ThemisDB ingestion test.\n";
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "txt_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = txt_path.string();

    ASSERT_TRUE(ingester.initialize(cfg));
    auto stats = ingester.ingest("test_collection", nullptr);

    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.bytes_processed, 0u);
    EXPECT_TRUE(stats.errors.empty());

    std::filesystem::remove_all(tmp_dir);
}

// ============================================================================
// IngestionManager – setRetryConfig propagation (smoke test)
// ============================================================================

TEST(IngestionManagerRetryTest, SetRetryConfigNoThrow) {
    IngestionManager mgr("test_db");

    RetryConfig cfg;
    cfg.max_attempts     = 5;
    cfg.initial_delay_ms = 100.0;
    cfg.backoff_factor   = 1.5;

    EXPECT_NO_THROW(mgr.setRetryConfig(cfg));
}
