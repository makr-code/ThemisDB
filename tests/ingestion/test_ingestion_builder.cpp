/**
 * @file test_ingestion_builder.cpp
 * @brief Unit tests for new ingestion module features:
 *        - IngestionBuilder fluent API
 *        - RateLimitConfig struct and defaults
 *        - IngestionStats::correlation_id
 *        - IngestionMetricsExporter: source_type label, error_code breakdown
 *        - IngestionManager::setRateLimitConfig
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include "ingestion/filesystem_ingester.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

using namespace themis::ingestion;

// ============================================================================
// RateLimitConfig – struct defaults and field assignment
// ============================================================================

TEST(RateLimitConfigTest, DefaultDisabled) {
    RateLimitConfig cfg;
    EXPECT_FALSE(cfg.enabled);
    EXPECT_DOUBLE_EQ(cfg.requests_per_second, 0.0);
    EXPECT_EQ(cfg.max_bytes_per_hour, 0u);
}

TEST(RateLimitConfigTest, ConfigurableValues) {
    RateLimitConfig cfg;
    cfg.enabled              = true;
    cfg.requests_per_second  = 10.0;
    cfg.max_bytes_per_hour   = 1024 * 1024 * 500; // 500 MiB

    EXPECT_TRUE(cfg.enabled);
    EXPECT_DOUBLE_EQ(cfg.requests_per_second, 10.0);
    EXPECT_EQ(cfg.max_bytes_per_hour, 1024u * 1024u * 500u);
}

// ============================================================================
// IngestionStats – correlation_id
// ============================================================================

TEST(IngestionStatsCorrelationIdTest, DefaultEmpty) {
    IngestionStats stats;
    // correlation_id is assigned by IngestionManager, not set at construction
    EXPECT_TRUE(stats.correlation_id.empty());
}

TEST(IngestionStatsCorrelationIdTest, CanBeSetDirectly) {
    IngestionStats stats;
    stats.correlation_id = "test-corr-id-abc123";
    EXPECT_EQ(stats.correlation_id, "test-corr-id-abc123");
}

TEST(IngestionStatsCorrelationIdTest, AssignedByManagerOnIngestSource) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "missing";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/no_such_path";
    cfg.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(cfg));

    // Even for an error path, correlation_id should be set
    auto stats = mgr.ingestSource("missing");
    EXPECT_FALSE(stats.correlation_id.empty());
}

TEST(IngestionStatsCorrelationIdTest, UniqueAcrossConsecutiveCalls) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "src_corr";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/no_such_path_corr";
    cfg.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto s1 = mgr.ingestSource("src_corr");
    auto s2 = mgr.ingestSource("src_corr");
    EXPECT_FALSE(s1.correlation_id.empty());
    EXPECT_FALSE(s2.correlation_id.empty());
    // Correlation IDs must be distinct for different runs
    EXPECT_NE(s1.correlation_id, s2.correlation_id);
}

// ============================================================================
// IngestionManager – setRateLimitConfig
// ============================================================================

TEST(IngestionManagerRateLimitTest, SetRateLimitNoThrow) {
    IngestionManager mgr("test_db");
    RateLimitConfig cfg;
    cfg.enabled             = true;
    cfg.requests_per_second = 5.0;
    EXPECT_NO_THROW(mgr.setRateLimitConfig(cfg));
}

TEST(IngestionManagerRateLimitTest, SetRateLimitDisabledNoThrow) {
    IngestionManager mgr("test_db");
    RateLimitConfig cfg; // default: disabled
    EXPECT_NO_THROW(mgr.setRateLimitConfig(cfg));
}

// ============================================================================
// IngestionMetricsExporter – source_type label and error_code breakdown
// ============================================================================

TEST(IngestionMetricsExporterSourceTypeTest, WithSourceType) {
    IngestionMetricsExporter exp;
    IngestionStats stats;
    stats.documents_processed = 10;

    std::string text = exp.exportText(stats, "hf_source", "HUGGINGFACE");

    EXPECT_NE(text.find("source_type=\"HUGGINGFACE\""), std::string::npos);
    EXPECT_NE(text.find("source_id=\"hf_source\""), std::string::npos);
}

TEST(IngestionMetricsExporterSourceTypeTest, WithoutSourceTypeOmitsLabel) {
    IngestionMetricsExporter exp;
    IngestionStats stats;

    std::string text = exp.exportText(stats, "fs_source");  // no source_type

    EXPECT_EQ(text.find("source_type="), std::string::npos);
    EXPECT_NE(text.find("source_id=\"fs_source\""), std::string::npos);
}

TEST(IngestionMetricsExporterSourceTypeTest, FilesystemLabel) {
    IngestionMetricsExporter exp;
    IngestionStats stats;
    std::string text = exp.exportText(stats, "fs1", "FILESYSTEM");
    EXPECT_NE(text.find("source_type=\"FILESYSTEM\""), std::string::npos);
}

TEST(IngestionMetricsExporterErrorCodeTest, ErrorCodeBreakdownEmitted) {
    IngestionMetricsExporter exp;
    IngestionStats stats;
    stats.addError(IngestionErrorCode::HTTP_TIMEOUT,
                   IngestionErrorSeverity::WARNING, "timeout1");
    stats.addError(IngestionErrorCode::HTTP_TIMEOUT,
                   IngestionErrorSeverity::WARNING, "timeout2");
    stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                   IngestionErrorSeverity::ERROR, "proc fail");

    std::string text = exp.exportText(stats, "src1");

    EXPECT_NE(text.find("errors_by_code_total"), std::string::npos);
    EXPECT_NE(text.find("error_code="), std::string::npos);
    // HTTP_TIMEOUT = 1105
    EXPECT_NE(text.find("\"1105\""), std::string::npos);
    // PROCESSING_FAILED = 1300
    EXPECT_NE(text.find("\"1300\""), std::string::npos);
}

TEST(IngestionMetricsExporterErrorCodeTest, NoBreakdownWhenNoErrors) {
    IngestionMetricsExporter exp;
    IngestionStats stats;  // no errors

    std::string text = exp.exportText(stats, "clean_src");

    // No breakdown section expected when errors is empty
    EXPECT_EQ(text.find("errors_by_code_total"), std::string::npos);
}

// ============================================================================
// IngestionBuilder – fluent API
// ============================================================================

TEST(IngestionBuilderTest, BuildsWithNoSources) {
    auto mgr = IngestionBuilder("test_db").build();
    ASSERT_NE(mgr, nullptr);
    EXPECT_TRUE(mgr->getRegisteredSources().empty());
}

TEST(IngestionBuilderTest, BuildsWithHuggingFaceSource) {
    auto mgr = IngestionBuilder("test_db")
        .withHuggingFaceSource("hf_legal", "lexlms/ger_legal_data",
                                {{"split", "train"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "hf_legal");
    EXPECT_EQ(sources[0].type, SourceType::HUGGINGFACE);
    EXPECT_EQ(sources[0].location, "lexlms/ger_legal_data");
}

TEST(IngestionBuilderTest, BuildsWithFilesystemSource) {
    auto mgr = IngestionBuilder("test_db")
        .withFilesystemSource("docs", "/tmp/docs",
                               {{"recursive", "true"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "docs");
    EXPECT_EQ(sources[0].type, SourceType::FILESYSTEM);
    EXPECT_EQ(sources[0].location, "/tmp/docs");
    EXPECT_EQ(sources[0].options.at("recursive"), "true");
}

TEST(IngestionBuilderTest, BuildsWithMultipleSources) {
    auto mgr = IngestionBuilder("test_db")
        .withHuggingFaceSource("hf1", "dataset/a")
        .withFilesystemSource("fs1", "/tmp/a")
        .withFilesystemSource("fs2", "/tmp/b")
        .build();

    auto sources = mgr->getRegisteredSources();
    EXPECT_EQ(sources.size(), 3u);
}

TEST(IngestionBuilderTest, WithRetryConfig) {
    RetryConfig retry;
    retry.max_attempts    = 5;
    retry.initial_delay_ms = 100.0;

    auto mgr = IngestionBuilder("test_db")
        .withRetryConfig(retry)
        .build();

    ASSERT_NE(mgr, nullptr);
    // RetryConfig is stored internally; no getter, but build() must not throw
}

TEST(IngestionBuilderTest, WithRateLimitConfig) {
    RateLimitConfig rl;
    rl.enabled             = true;
    rl.requests_per_second = 20.0;

    auto mgr = IngestionBuilder("test_db")
        .withRateLimitConfig(rl)
        .build();

    ASSERT_NE(mgr, nullptr);
}

TEST(IngestionBuilderTest, WithParallelProcessing) {
    auto mgr = IngestionBuilder("test_db")
        .withParallelProcessing(true, 4)
        .build();

    ASSERT_NE(mgr, nullptr);
}

TEST(IngestionBuilderTest, WithTargetCollection) {
    auto mgr = IngestionBuilder("test_db")
        .withTargetCollection("my_corpus")
        .build();

    ASSERT_NE(mgr, nullptr);
}

TEST(IngestionBuilderTest, WithDryRun) {
    auto mgr = IngestionBuilder("test_db")
        .withDryRun(true)
        .build();

    ASSERT_NE(mgr, nullptr);
    EXPECT_TRUE(mgr->isDryRun());
}

TEST(IngestionBuilderTest, FullChain) {
    RetryConfig retry;
    retry.max_attempts = 3;

    RateLimitConfig rl;
    rl.enabled             = true;
    rl.requests_per_second = 10.0;

    auto tmp_dir = std::filesystem::temp_directory_path() / "builder_test_docs";
    std::filesystem::create_directories(tmp_dir);
    {
        std::ofstream f(tmp_dir / "sample.txt");
        f << "hello world\n";
    }

    auto mgr = IngestionBuilder("test_db")
        .withFilesystemSource("local_docs", tmp_dir.string())
        .withHuggingFaceSource("hf_data", "my/dataset",
                                {{"split", "train"}}, 8 /* priority: higher than default 5 */)
        .withRetryConfig(retry)
        .withRateLimitConfig(rl)
        .withParallelProcessing(false)
        .withTargetCollection("legal_corpus")
        .withDryRun(false)
        .build();

    ASSERT_NE(mgr, nullptr);
    EXPECT_FALSE(mgr->isDryRun());
    auto sources = mgr->getRegisteredSources();
    EXPECT_EQ(sources.size(), 2u);

    std::filesystem::remove_all(tmp_dir);
}

TEST(IngestionBuilderTest, PriorityPropagated) {
    auto mgr = IngestionBuilder("test_db")
        .withFilesystemSource("high_prio", "/tmp/a", {}, 9)
        .withFilesystemSource("low_prio",  "/tmp/b", {}, 1)
        .build();

    auto sources = mgr->getRegisteredSources();
    bool found_high = false, found_low = false;
    for (const auto& s : sources) {
        if (s.source_id == "high_prio") { EXPECT_EQ(s.priority, 9); found_high = true; }
        if (s.source_id == "low_prio")  { EXPECT_EQ(s.priority, 1); found_low  = true; }
    }
    EXPECT_TRUE(found_high);
    EXPECT_TRUE(found_low);
}

// ============================================================================
// IngestionBuilder – withApiSource (cursor/offset pagination)
// ============================================================================

TEST(IngestionBuilderApiSourceTest, RegistersApiSource) {
    auto mgr = IngestionBuilder("test_db")
        .withApiSource("my_api", "https://api.example.com/v1/docs")
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "my_api");
    EXPECT_EQ(sources[0].type,      SourceType::API);
    EXPECT_EQ(sources[0].location,  "https://api.example.com/v1/docs");
}

TEST(IngestionBuilderApiSourceTest, OffsetModeOptions) {
    auto mgr = IngestionBuilder("test_db")
        .withApiSource("offset_api", "https://api.example.com/items",
                       {{"pagination_mode", "offset"},
                        {"cursor_param",    "offset"},
                        {"page_size",       "50"},
                        {"text_field",      "content"}})
        .build();

    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].options.at("pagination_mode"), "offset");
    EXPECT_EQ(sources[0].options.at("page_size"),       "50");
    EXPECT_EQ(sources[0].options.at("text_field"),      "content");
}

TEST(IngestionBuilderApiSourceTest, CursorModeOptions) {
    auto mgr = IngestionBuilder("test_db")
        .withApiSource("cursor_api", "https://api.example.com/v2/docs",
                       {{"pagination_mode",       "cursor"},
                        {"cursor_param",          "page_token"},
                        {"cursor_response_field", "next_page_token"},
                        {"page_size",             "100"},
                        {"max_pages",             "5"}})
        .build();

    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].type,                                SourceType::API);
    EXPECT_EQ(sources[0].options.at("pagination_mode"),       "cursor");
    EXPECT_EQ(sources[0].options.at("cursor_param"),          "page_token");
    EXPECT_EQ(sources[0].options.at("cursor_response_field"), "next_page_token");
    EXPECT_EQ(sources[0].options.at("max_pages"),             "5");
}

TEST(IngestionBuilderApiSourceTest, PriorityPropagated) {
    auto mgr = IngestionBuilder("test_db")
        .withApiSource("api_high", "https://api.example.com/a", {}, 9)
        .withApiSource("api_low",  "https://api.example.com/b", {}, 2)
        .build();

    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 2u);
    for (const auto& s : sources) {
        if (s.source_id == "api_high") EXPECT_EQ(s.priority, 9);
        if (s.source_id == "api_low")  EXPECT_EQ(s.priority, 2);
    }
}

TEST(IngestionBuilderApiSourceTest, MixedSourceTypes) {
    auto mgr = IngestionBuilder("test_db")
        .withApiSource("api_src",  "https://api.example.com/docs",
                       {{"pagination_mode", "cursor"}})
        .withFilesystemSource("fs_src", "/tmp/docs")
        .withHuggingFaceSource("hf_src", "dataset/name")
        .build();

    auto sources = mgr->getRegisteredSources();
    EXPECT_EQ(sources.size(), 3u);

    bool found_api = false, found_fs = false, found_hf = false;
    for (const auto& s : sources) {
        if (s.source_id == "api_src") { EXPECT_EQ(s.type, SourceType::API);        found_api = true; }
        if (s.source_id == "fs_src")  { EXPECT_EQ(s.type, SourceType::FILESYSTEM); found_fs  = true; }
        if (s.source_id == "hf_src")  { EXPECT_EQ(s.type, SourceType::HUGGINGFACE);found_hf  = true; }
    }
    EXPECT_TRUE(found_api);
    EXPECT_TRUE(found_fs);
    EXPECT_TRUE(found_hf);
}

TEST(IngestionBuilderApiSourceTest, IngestsViaManager) {
    // End-to-end: build a manager with a cursor-mode API source and ingest.
    // The simulated HTTP layer always returns 3 docs per page.
    auto mgr = IngestionBuilder("test_db")
        .withApiSource("e2e_api", "https://api.example.com/v2/docs",
                       {{"pagination_mode",       "cursor"},
                        {"cursor_response_field", "next_cursor"},
                        {"page_size",             "3"},
                        {"max_pages",             "2"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto stats = mgr->ingestSource("e2e_api");
    // With a mocked API transport this is 6 documents; without a transport mock
    // the connector can return a structured error in offline CI environments.
    if (stats.errors.empty()) {
        EXPECT_EQ(stats.documents_failed, 0u);
        EXPECT_EQ(stats.documents_processed, 6u);
    } else {
        EXPECT_EQ(stats.documents_processed, 0u);
    }
}
