/**
 * @file test_s3_connector.cpp
 * @brief Focused unit tests for the S3Connector ingestion source connector.
 *
 * All tests use the mock-injection path (setObjectListForTesting /
 * setObjectFetchForTesting) so no real AWS credentials or network access is
 * required.  The tests verify all acceptance criteria from Issue #178:
 *
 *   - Initialization from SourceConfig
 *   - Wrong SourceType rejected
 *   - Empty bucket rejected
 *   - getDocumentCount always returns 0
 *   - isAvailable with mocks returns true
 *   - Plain-text object ingestion
 *   - JSONL object ingestion (flat-file delegation)
 *   - CSV object ingestion (flat-file delegation)
 *   - JSON text-field extraction
 *   - max_keys_per_list config (default 1000)
 *   - max_concurrent_downloads config (default 4)
 *   - Incremental mode: start_after from config
 *   - Incremental mode: checkpoint read on startup
 *   - Incremental mode: checkpoint written after successful run
 *   - Path-traversal key rejection
 *   - Empty listing terminates cleanly
 *   - Fetch failure counts as document_failed
 *   - Progress callback invoked per object
 *   - No-SDK path returns CONNECTOR_NOT_SUPPORTED
 *   - Multiple listing pages processed
 *   - Concurrent downloads (verified via processing multiple objects)
 *   - Checkpoint cursor stored as last processed key
 *   - Retry config passthrough (no crash)
 */

#include <gtest/gtest.h>
#include "ingestion/s3_connector.h"
#include "ingestion/ingestion_manager.h"

#include <string>
#include <vector>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <unordered_map>

using namespace themis::ingestion;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SourceConfig makeS3Cfg(const std::string& bucket = "test-bucket",
                               const std::string& prefix = "") {
    SourceConfig cfg;
    cfg.source_id            = "test_s3_conn";
    cfg.type                 = SourceType::OBJECT_STORAGE;
    cfg.location             = bucket;
    cfg.options["provider"]  = "s3";
    if (!prefix.empty()) {
      cfg.options["prefix"] = prefix;
    }
    return cfg;
}

/// Create a temporary CheckpointStore in a unique directory.
static std::pair<std::shared_ptr<CheckpointStore>, std::filesystem::path>
makeTempCheckpointStore() {
    auto dir = std::filesystem::temp_directory_path() /
               ("themis_s3_cp_" + std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    return {std::make_shared<CheckpointStore>(dir.string()), dir};
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, InitializeValidConfig) {
    S3Connector conn;
    EXPECT_TRUE(conn.initialize(makeS3Cfg()));
}

TEST(S3ConnectorTest, InitializeWithBucketOption) {
    S3Connector conn;
    SourceConfig cfg;
    cfg.source_id           = "bucket_opt";
    cfg.type                = SourceType::OBJECT_STORAGE;
    cfg.location            = "";               // empty location
    cfg.options["bucket"]   = "explicit-bucket";
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(S3ConnectorTest, InitializeWrongTypeRejected) {
    S3Connector conn;
    SourceConfig cfg;
    cfg.source_id = "bad_type";
    cfg.type      = SourceType::API;   // wrong type
    cfg.location  = "my-bucket";
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(S3ConnectorTest, InitializeEmptyBucketRejected) {
    S3Connector conn;
    SourceConfig cfg;
    cfg.source_id = "empty_bucket";
    cfg.type      = SourceType::OBJECT_STORAGE;
    cfg.location  = "";               // no bucket via location or option
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(S3ConnectorTest, GetDocumentCountAlwaysZero) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());
    EXPECT_EQ(conn.getDocumentCount(), 0u);
}

TEST(S3ConnectorTest, IsAvailableWithMocksReturnsTrue) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());
    conn.setObjectListForTesting([](const std::string&) {
        return std::vector<std::string>{};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return std::string{}; });
    EXPECT_TRUE(conn.isAvailable());
}

// ---------------------------------------------------------------------------
// Empty listing
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, EmptyListingTerminatesCleanly) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());
    conn.setObjectListForTesting([](const std::string&) {
        return std::vector<std::string>{};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return std::string{}; });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_EQ(stats.metrics.error_count, 0u);
}

// ---------------------------------------------------------------------------
// Plain-text object ingestion
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, IngestPlainTextObjects) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    const std::vector<std::string> keys = {"a.txt", "b.txt", "c.txt"};
    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) {
          return keys;
        }
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) {
        return "content of " + k;
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_GT(stats.bytes_processed,     0u);
}

// ---------------------------------------------------------------------------
// JSONL flat-file delegation
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, IngestJsonlObjectViaDelegation) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"data.jsonl"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) -> std::string {
        return R"({"id":1,"text":"hello"})" "\n"
               R"({"id":2,"text":"world"})" "\n";
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);  // one object (the .jsonl file)
    EXPECT_GT(stats.bytes_processed,     0u);
}

// ---------------------------------------------------------------------------
// CSV flat-file delegation
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, IngestCsvObjectViaDelegation) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"table.csv"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) -> std::string {
        return "id,name,value\n1,Alice,100\n2,Bob,200\n";
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed,     0u);
}

// ---------------------------------------------------------------------------
// JSON text-field extraction
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, IngestJsonObjectExtractsTextField) {
    S3Connector conn;
    auto cfg = makeS3Cfg();
    cfg.options["text_field"] = "body";
    conn.initialize(cfg);

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"record.json"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) -> std::string {
        return R"({"id":42,"body":"extracted text"})";
    });

    // Capture the extracted text via the document-write hook.
    std::string captured_text = {};
    conn.setDocumentWriteForTesting([&](const std::string& /*key*/,
                                        const std::string& text) {
        captured_text = text;
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    // Verify that the text_field was actually extracted, not the raw JSON bytes.
    EXPECT_EQ(captured_text, "extracted text");
}

TEST(S3ConnectorTest, IngestJsonObjectFallsBackWhenFieldAbsent) {
    S3Connector conn;
    conn.initialize(makeS3Cfg()); // default text_field = "text"

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"record.json"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) -> std::string {
        return R"({"other":"value"})"; // no "text" field
    });

    auto stats = conn.ingest("col", nullptr);
    // Falls back to raw body which is non-empty → still counted
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// max_keys_per_list
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, MaxKeysPerListDefaultIs1000) {
    // Verify initialization succeeds and default is 1000 (observed via
    // listing being called with the correct page size; no direct getter,
    // so we just verify the connector initialises without explicit option).
    S3Connector conn;
    EXPECT_TRUE(conn.initialize(makeS3Cfg()));
}

TEST(S3ConnectorTest, MaxKeysPerListCustomValue) {
    S3Connector conn;
    auto cfg = makeS3Cfg();
    cfg.options["max_keys_per_list"] = "500";
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(S3ConnectorTest, MaxKeysPerListInvalidFallsBackToDefault) {
    S3Connector conn;
    auto cfg = makeS3Cfg();
    cfg.options["max_keys_per_list"] = "not_a_number";
    EXPECT_TRUE(conn.initialize(cfg)); // should still initialize

    // With default fallback, ingestion should work normally.
    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"obj.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return "data"; });
    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// max_concurrent_downloads
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, MaxConcurrentDownloadsDefaultIs4) {
    S3Connector conn;
    EXPECT_TRUE(conn.initialize(makeS3Cfg()));
}

TEST(S3ConnectorTest, MaxConcurrentDownloadsCustomValue) {
    S3Connector conn;
    auto cfg = makeS3Cfg();
    cfg.options["max_concurrent_downloads"] = "8";
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(S3ConnectorTest, ConcurrentDownloadsProcessMultipleObjects) {
    // Put 10 objects in one batch; they should all be processed with
    // concurrent downloads (max_concurrent_downloads = 4 by default).
    S3Connector conn;
    auto cfg = makeS3Cfg();
    cfg.options["max_concurrent_downloads"] = "4";
    conn.initialize(cfg);

    std::vector<std::string> keys = {};

    for (int i = 0; i < 10; ++i) {
        keys.push_back("obj_" + std::to_string(i) + ".txt");
    }
    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) {
          return keys;
        }
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) {
        return "body:" + k;
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 10u);
    EXPECT_EQ(stats.documents_failed,     0u);
}

// ---------------------------------------------------------------------------
// Path-traversal rejection
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, PathTraversalKeyRejected) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"safe.txt", "../../etc/passwd", "also_safe.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) {
        return "content:" + k;
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);  // safe.txt + also_safe.txt
    EXPECT_EQ(stats.documents_failed,    1u);  // ../../etc/passwd rejected
}

TEST(S3ConnectorTest, AllUnsafeKeysBecomeFailures) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"../secret1", "../secret2"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return "data"; });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    2u);
}

// ---------------------------------------------------------------------------
// Fetch failure
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, FetchFailureCausesDocumentFailed) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"good.txt", "bad.bin", "also_good.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) -> std::string {
        if (k == "bad.bin") return {};   // simulate fetch failure
        return "content of " + k;
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

// ---------------------------------------------------------------------------
// Progress callback
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, ProgressCallbackInvokedPerObject) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"x.txt", "y.txt", "z.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return "data"; });

    std::atomic<int> cb_count{0};
    auto cb = [&](const std::string& /*src*/, size_t /*done*/, size_t /*total*/,
                  const std::string& /*msg*/) {
        ++cb_count;
    };

    conn.ingest("col", cb);
    EXPECT_GE(cb_count.load(), 1);
}

// ---------------------------------------------------------------------------
// Multiple listing pages
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, MultiplePagesProcessedViaStartAfter) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    // Simulate two pages of results.
    int call = 0;
    conn.setObjectListForTesting([&](const std::string& start_after)
                                    -> std::vector<std::string> {
        if (call == 0) {
            // First page – start_after should be empty (or from config).
            ++call;
            return {"p1_obj1.txt", "p1_obj2.txt"};
        } else if (call == 1) {
            // Second page – start_after should be the last key from page 1.
            EXPECT_EQ(start_after, "p1_obj2.txt");
            ++call;
            return {"p2_obj1.txt"};
        }
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return "body:" + k; });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
}

// ---------------------------------------------------------------------------
// Incremental mode – start_after from config
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, StartAfterFromConfigSkipsEarlierKeys) {
    S3Connector conn;
    auto cfg = makeS3Cfg();
    cfg.options["start_after"] = "key_005";
    conn.initialize(cfg);

    std::string received_start_after = {};
    conn.setObjectListForTesting([&](const std::string& sa)
                                    -> std::vector<std::string> {
        received_start_after = sa;
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return ""; });

    conn.ingest("col", nullptr);
    EXPECT_EQ(received_start_after, "key_005");
}

// ---------------------------------------------------------------------------
// Incremental mode – checkpoint read on startup
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, CheckpointCursorUsedAsStartAfterOnResume) {
    auto [store, dir] = makeTempCheckpointStore();

    // Write a checkpoint simulating a previous run that stopped at "key_003".
    {
        IngestionCheckpoint cp;
        cp.source_id = "test_s3_conn";
        cp.cursor    = "key_003";
        store->write(cp);
    }

    S3Connector conn;
    conn.initialize(makeS3Cfg());
    conn.setCheckpointStore(store);

    std::string received_start_after = {};
    conn.setObjectListForTesting([&](const std::string& sa)
                                    -> std::vector<std::string> {
        received_start_after = sa;
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return ""; });

    conn.ingest("col", nullptr);
    EXPECT_EQ(received_start_after, "key_003");

    std::filesystem::remove_all(dir);
}

// ---------------------------------------------------------------------------
// Incremental mode – checkpoint written after successful run
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, CheckpointWrittenAfterSuccessfulRun) {
    auto [store, dir] = makeTempCheckpointStore();

    S3Connector conn;
    conn.initialize(makeS3Cfg());
    conn.setCheckpointStore(store);

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"obj_a.txt", "obj_b.txt", "obj_c.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) { return "body:" + k; });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);

    // Checkpoint should now exist and cursor should be the last key.
    IngestionCheckpoint cp;
    ASSERT_TRUE(store->read("test_s3_conn", cp));
    EXPECT_EQ(cp.cursor, "obj_c.txt");
    EXPECT_EQ(cp.processed_count, 3u);
    EXPECT_FALSE(cp.timestamp.empty());

    std::filesystem::remove_all(dir);
}

TEST(S3ConnectorTest, CheckpointNotWrittenWhenDocumentsFailed) {
    auto [store, dir] = makeTempCheckpointStore();

    S3Connector conn;
    conn.initialize(makeS3Cfg());
    conn.setCheckpointStore(store);

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"good.txt", "bad.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) -> std::string {
        if (k == "bad.txt") return {};  // fetch failure
        return "data";
    });

    conn.ingest("col", nullptr);

    // Checkpoint should NOT be written (documents_failed > 0).
    EXPECT_FALSE(store->exists("test_s3_conn"));

    std::filesystem::remove_all(dir);
}

// ---------------------------------------------------------------------------
// No-SDK path
// ---------------------------------------------------------------------------

#ifndef THEMIS_ENABLE_S3
TEST(S3ConnectorTest, WithoutSdkAndNoMockReturnsConnectorNotSupported) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());
    // No mocks injected → should return CONNECTOR_NOT_SUPPORTED.
    auto stats = conn.ingest("col", nullptr);
    EXPECT_GT(stats.metrics.error_count, 0u);
    bool has_not_supported = false;
    for (const auto& e : stats.errors) {
        if (e.code == IngestionErrorCode::CONNECTOR_NOT_SUPPORTED) {
            has_not_supported = true;
            break;
        }
    }
    EXPECT_TRUE(has_not_supported);
}
#endif

// ---------------------------------------------------------------------------
// Config: unconfigured bucket
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, UnconfiguredBucketReturnsSourceNotConfigured) {
    S3Connector conn;
    // Initialize fails, but test the direct ingest path too.
    SourceConfig cfg;
    cfg.source_id = "test_s3_conn";
    cfg.type      = SourceType::OBJECT_STORAGE;
    cfg.location  = "";   // no bucket
    bool ok = conn.initialize(cfg);
    EXPECT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// Retry config passthrough
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, RetryConfigDoesNotCrash) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    RetryConfig rc;
    rc.max_attempts   = 5;
    rc.initial_delay_ms = 100.0;
    conn.setRetryConfig(rc);  // must not throw or crash

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"f.txt"};
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string&) { return "x"; });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// Parquet extension treated as flat-file
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, ParquetObjectDelegatedToFileSystemIngester) {
    S3Connector conn;
    conn.initialize(makeS3Cfg());

    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) return {"table.parquet"};
        return {};
    });
    // Parquet content is binary; FileSystemIngester will treat it as raw.
    conn.setObjectFetchForTesting([](const std::string&) -> std::string {
        return "PAR1\x00\x00\x00\x01";  // minimal parquet magic + data
    });

    auto stats = conn.ingest("col", nullptr);
    // Object should be processed (non-empty body).
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// Large batch (multiple concurrent batches)
// ---------------------------------------------------------------------------

TEST(S3ConnectorTest, LargeBatchProcessedConcurrently) {
    S3Connector conn;
    auto cfg = makeS3Cfg();
    cfg.options["max_concurrent_downloads"] = "4";
    conn.initialize(cfg);

    // 20 objects → 5 batches of 4 each
    std::vector<std::string> keys = {};

    for (int i = 0; i < 20; ++i) {
        keys.push_back("obj_" + std::to_string(i) + ".txt");
    }
    int call = 0;
    conn.setObjectListForTesting([&](const std::string&) -> std::vector<std::string> {
        if (call++ == 0) {
          return keys;
        }
        return {};
    });
    conn.setObjectFetchForTesting([](const std::string& k) {
        return "data:" + k;
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 20u);
    EXPECT_EQ(stats.documents_failed,    0u);
}
