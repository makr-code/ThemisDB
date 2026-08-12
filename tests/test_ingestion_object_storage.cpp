/**
 * @file test_ingestion_object_storage.cpp
 * @brief Unit tests for the ObjectStorageConnector ingestion source connector.
 *
 * All tests use the mock-injection path (setObjectListForTesting /
 * setObjectFetchForTesting) so that no real cloud credentials or network
 * access is required.  The tests verify:
 *   - Initialization from SourceConfig (S3, GCS, Azure providers)
 *   - Plain-text object ingestion
 *   - JSON text-field extraction from .json objects
 *   - max_keys limit enforcement
 *   - Empty listing termination
 *   - Progress callback invocation
 *   - Empty / fetch-failed objects counted as failures
 *   - Path-traversal key rejection
 *   - IngestionBuilder::withObjectStorageSource() fluent API
 *   - Error handling: missing bucket configuration
 *   - SourceType::OBJECT_STORAGE in sourceTypeLabel (via IngestionMetricsExporter)
 *   - RetryConfig passthrough
 */

#include <gtest/gtest.h>
#include "ingestion/object_storage_connector.h"
#include "ingestion/ingestion_manager.h"
#include <string>
#include <vector>
#include <atomic>
#include <unordered_map>

using namespace themis::ingestion;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SourceConfig makeS3Config(const std::string& bucket = "test-bucket",
                                 const std::string& prefix = "") {
    SourceConfig cfg;
    cfg.source_id    = "test_s3";
    cfg.type         = SourceType::OBJECT_STORAGE;
    cfg.location     = bucket;
    cfg.options["provider"] = "s3";
    if (!prefix.empty()) cfg.options["prefix"] = prefix;
    return cfg;
}

static SourceConfig makeGCSConfig(const std::string& bucket = "gcs-bucket") {
    SourceConfig cfg;
    cfg.source_id    = "test_gcs";
    cfg.type         = SourceType::OBJECT_STORAGE;
    cfg.location     = bucket;
    cfg.options["provider"] = "gcs";
    return cfg;
}

static SourceConfig makeAzureConfig(const std::string& container = "azure-container") {
    SourceConfig cfg;
    cfg.source_id    = "test_azure";
    cfg.type         = SourceType::OBJECT_STORAGE;
    cfg.location     = container;
    cfg.options["provider"] = "azure";
    return cfg;
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, InitializeS3ValidConfig) {
    ObjectStorageConnector conn;
    EXPECT_TRUE(conn.initialize(makeS3Config()));
}

TEST(ObjectStorageConnectorTest, InitializeGCSValidConfig) {
    ObjectStorageConnector conn;
    EXPECT_TRUE(conn.initialize(makeGCSConfig()));
}

TEST(ObjectStorageConnectorTest, InitializeAzureValidConfig) {
    ObjectStorageConnector conn;
    EXPECT_TRUE(conn.initialize(makeAzureConfig()));
}

TEST(ObjectStorageConnectorTest, InitializeWrongType) {
    ObjectStorageConnector conn;
    SourceConfig cfg;
    cfg.source_id = "wrong";
    cfg.type      = SourceType::API;  // wrong type
    cfg.location  = "my-bucket";
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(ObjectStorageConnectorTest, InitializeEmptyBucket) {
    ObjectStorageConnector conn;
    SourceConfig cfg;
    cfg.source_id = "empty_bucket";
    cfg.type      = SourceType::OBJECT_STORAGE;
    cfg.location  = "";  // empty bucket
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(ObjectStorageConnectorTest, GetDocumentCountAlwaysZero) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());
    EXPECT_EQ(conn.getDocumentCount(), 0u);
}

TEST(ObjectStorageConnectorTest, IsAvailableWithMocksReturnsTrue) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());
    conn.setObjectListForTesting([]() { return std::vector<std::string>{}; });
    conn.setObjectFetchForTesting([](const std::string&) { return std::string{}; });
    EXPECT_TRUE(conn.isAvailable());
}

// ---------------------------------------------------------------------------
// Plain-text object ingestion
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, IngestPlainTextObjects) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());

    std::vector<std::string> keys = {"doc1.txt", "doc2.txt", "doc3.txt"};
    int list_call = 0;
    conn.setObjectListForTesting([&]() -> std::vector<std::string> {
        if (list_call++ == 0) return keys;
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& key) -> std::string {
        return "content of " + key;
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.bytes_processed, 0u);
}

// ---------------------------------------------------------------------------
// JSON text-field extraction for .json objects
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, IngestJsonObjectsExtractTextField) {
    ObjectStorageConnector conn;
    auto cfg = makeS3Config();
    cfg.options["text_field"] = "content";
    conn.initialize(cfg);

    int list_call = 0;
    conn.setObjectListForTesting([&]() -> std::vector<std::string> {
        if (list_call++ == 0) return {"record.json"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) -> std::string {
        return R"({"content":"hello world","id":1})";
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

TEST(ObjectStorageConnectorTest, IngestJsonObjectsFallbackWhenFieldAbsent) {
    // When the text_field is absent in the JSON body, the whole body is used.
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());

    int list_call = 0;
    conn.setObjectListForTesting([&]() -> std::vector<std::string> {
        if (list_call++ == 0) return {"data.json"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) -> std::string {
        return R"({"other_field":"value"})";
    });

    auto stats = conn.ingest("docs", nullptr);
    // Falls back to full body; body is non-empty → counted as document
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// max_keys limit
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, MaxKeysLimitEnforced) {
    ObjectStorageConnector conn;
    auto cfg = makeS3Config();
    cfg.options["max_keys"] = "2";
    conn.initialize(cfg);

    conn.setObjectListForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ < 3) return {"k1.txt", "k2.txt", "k3.txt", "k4.txt", "k5.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return k; });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
}

TEST(ObjectStorageConnectorTest, MaxKeysZeroMeansUnlimited) {
    ObjectStorageConnector conn;
    auto cfg = makeS3Config();
    cfg.options["max_keys"] = "0";
    conn.initialize(cfg);

    int batch_count = 0;
    conn.setObjectListForTesting([&]() -> std::vector<std::string> {
        if (batch_count++ < 3) return {"obj.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return k; });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
}

// ---------------------------------------------------------------------------
// Empty listing terminates ingestion
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, EmptyListingTerminates) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());

    conn.setObjectListForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {"only.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return k; });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_TRUE(stats.errors.empty());
}

// ---------------------------------------------------------------------------
// Empty / fetch-failed objects counted as failures
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, EmptyFetchCountedAsFailure) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());

    int list_call = 0;
    conn.setObjectListForTesting([&]() -> std::vector<std::string> {
        if (list_call++ == 0) return {"good.txt", "bad.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& key) -> std::string {
        if (key == "bad.txt") return "";  // simulate fetch failure
        return "good content";
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 1u);
    EXPECT_FALSE(stats.errors.empty());
}

// ---------------------------------------------------------------------------
// Path-traversal key rejection
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, PathTraversalKeyRejected) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());

    int list_call = 0;
    conn.setObjectListForTesting([&]() -> std::vector<std::string> {
        if (list_call++ == 0) return {"../../etc/passwd", "safe.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return k; });

    auto stats = conn.ingest("docs", nullptr);
    // The traversal key is rejected; only safe.txt succeeds.
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 1u);
    bool has_file_error = false;
    for (const auto& e : stats.errors) {
        if (e.code == IngestionErrorCode::FILE_READ_ERROR) {
            has_file_error = true;
        }
    }
    EXPECT_TRUE(has_file_error);
}

// ---------------------------------------------------------------------------
// Progress callback
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, ProgressCallbackInvoked) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());

    conn.setObjectListForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {"a.txt", "b.txt", "c.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return k; });

    std::atomic<int> cb_count{0};
    auto cb = [&](const std::string& source_id, size_t processed,
                  size_t total, const std::string& status) {
        (void)source_id; (void)processed; (void)total; (void)status;
        ++cb_count;
    };

    conn.ingest("docs", cb);
    EXPECT_GT(cb_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Error: not configured (no bucket)
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, IngestWithoutInitializationReturnsError) {
    ObjectStorageConnector conn;
    // Inject mocks but do not call initialize() – bucket_ is empty
    conn.setObjectListForTesting([]() { return std::vector<std::string>{}; });
    conn.setObjectFetchForTesting([](const std::string&) { return std::string{}; });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_FALSE(stats.errors.empty());
    bool has_config_error = false;
    for (const auto& e : stats.errors) {
        if (e.code == IngestionErrorCode::SOURCE_NOT_CONFIGURED) {
            has_config_error = true;
        }
    }
    EXPECT_TRUE(has_config_error);
}

// ---------------------------------------------------------------------------
// Elapsed time
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, StatsHaveElapsedTime) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());
    conn.setObjectListForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {"obj1.txt", "obj2.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return k; });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

// ---------------------------------------------------------------------------
// No SDK compiled: CONNECTOR_NOT_SUPPORTED without mocks
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, NoMockNoSdkReturnsNotSupported) {
    // Without mock AND without a compiled SDK, ingest should return
    // CONNECTOR_NOT_SUPPORTED.  This test is meaningful only when neither
    // THEMIS_ENABLE_S3 nor any other provider flag is set.
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());
    // No mocks injected → will hit the no-SDK branch.
    auto stats = conn.ingest("docs", nullptr);

#if !defined(THEMIS_ENABLE_S3) && !defined(THEMIS_ENABLE_GCS) && !defined(THEMIS_ENABLE_AZURE)
    bool has_not_supported = false;
    for (const auto& e : stats.errors) {
        if (e.code == IngestionErrorCode::CONNECTOR_NOT_SUPPORTED) {
            has_not_supported = true;
        }
    }
    EXPECT_TRUE(has_not_supported);
#else
    // At least one SDK is compiled in; just verify no crash.
    (void)stats;
#endif
}

// ---------------------------------------------------------------------------
// GCS and Azure provider selection (via options["provider"])
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, GCSProviderInitializesCorrectly) {
    ObjectStorageConnector conn;
    EXPECT_TRUE(conn.initialize(makeGCSConfig()));
    conn.setObjectListForTesting([]() { return std::vector<std::string>{}; });
    conn.setObjectFetchForTesting([](const std::string&) { return std::string{}; });
    EXPECT_TRUE(conn.isAvailable());
}

TEST(ObjectStorageConnectorTest, AzureProviderInitializesCorrectly) {
    ObjectStorageConnector conn;
    EXPECT_TRUE(conn.initialize(makeAzureConfig()));
    conn.setObjectListForTesting([]() { return std::vector<std::string>{}; });
    conn.setObjectFetchForTesting([](const std::string&) { return std::string{}; });
    EXPECT_TRUE(conn.isAvailable());
}

// ---------------------------------------------------------------------------
// IngestionBuilder::withObjectStorageSource
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, BuilderWithObjectStorageSource) {
    auto mgr = IngestionBuilder("test_db")
        .withObjectStorageSource("s3_src", "my-bucket",
                                 {{"provider","s3"},
                                  {"prefix","2026/"},
                                  {"max_keys","100"},
                                  {"region","eu-west-1"}})
        .withDryRun(true)
        .build();

    ASSERT_NE(mgr, nullptr);

    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "s3_src");
    EXPECT_EQ(sources[0].type, SourceType::OBJECT_STORAGE);
    EXPECT_EQ(sources[0].location, "my-bucket");
    EXPECT_EQ(sources[0].options.at("provider"), "s3");
    EXPECT_EQ(sources[0].options.at("prefix"), "2026/");
    EXPECT_EQ(sources[0].options.at("max_keys"), "100");
    EXPECT_EQ(sources[0].options.at("region"), "eu-west-1");
}

TEST(ObjectStorageConnectorTest, BuilderWithObjectStorageSourceDefaultOptions) {
    auto mgr = IngestionBuilder("test_db")
        .withObjectStorageSource("obj_src", "data-bucket")
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].type, SourceType::OBJECT_STORAGE);
    EXPECT_EQ(sources[0].location, "data-bucket");
}

TEST(ObjectStorageConnectorTest, BuilderWithGCSSource) {
    auto mgr = IngestionBuilder("test_db")
        .withObjectStorageSource("gcs_src", "gcs-bucket",
                                 {{"provider","gcs"},{"prefix","data/"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].options.at("provider"), "gcs");
}

TEST(ObjectStorageConnectorTest, BuilderWithAzureSource) {
    auto mgr = IngestionBuilder("test_db")
        .withObjectStorageSource("azure_src", "my-container",
                                 {{"provider","azure"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].options.at("provider"), "azure");
}

// ---------------------------------------------------------------------------
// SourceType::OBJECT_STORAGE in Prometheus metrics exporter
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, MetricsExporterIncludesObjectStorageSourceType) {
    IngestionStats stats;
    stats.documents_processed = 10;
    stats.elapsed_seconds     = 0.3;

    IngestionMetricsExporter exporter;
    exporter.setPrefix("themis_ingestion");

    std::string text = exporter.exportText(stats, "s3_src", "OBJECT_STORAGE");
    EXPECT_FALSE(text.empty());
    EXPECT_NE(text.find("OBJECT_STORAGE"), std::string::npos)
        << "Expected 'OBJECT_STORAGE' source_type label in Prometheus output";
}

// ---------------------------------------------------------------------------
// RetryConfig passthrough
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, SetRetryConfigDoesNotCrash) {
    ObjectStorageConnector conn;
    conn.initialize(makeS3Config());
    RetryConfig rc;
    rc.max_attempts     = 5;
    rc.initial_delay_ms = 200.0;
    EXPECT_NO_THROW(conn.setRetryConfig(rc));
}

// ---------------------------------------------------------------------------
// Prefix option passed through SourceConfig
// ---------------------------------------------------------------------------

TEST(ObjectStorageConnectorTest, PrefixOptionStoredInConfig) {
    auto mgr = IngestionBuilder("test_db")
        .withObjectStorageSource("obj_src", "bucket",
                                 {{"provider","s3"},{"prefix","logs/"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].options.at("prefix"), "logs/");
}
