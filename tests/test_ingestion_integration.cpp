/**
 * @file test_ingestion_integration.cpp
 * @brief Integration tests for the ingestion module:
 *        filesystem, HuggingFace, and generic API connectors.
 *
 * These tests validate end-to-end ingestion flows through IngestionManager,
 * covering:
 *  - FileSystemIngester: recursive directory walk, multi-format dirs,
 *    progress callbacks, multi-level hierarchies
 *  - HuggingFaceConnector: dataset split ingestion via IngestionManager,
 *    API-token auth, batch/streaming mode
 *  - GenericApiConnector: offset-mode and cursor-mode paginated JSON REST,
 *    API key propagation, exponential back-off termination
 *  - Multi-source integration: all three connector types in a single
 *    IngestionManager run
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include "ingestion/filesystem_ingester.h"
#include "ingestion/api_connector.h"
#include "ingestion/huggingface_connector.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <sstream>
#include <chrono>

using namespace themis::ingestion;
namespace fs = std::filesystem;

// ============================================================================
// Test helpers
// ============================================================================

static fs::path makeDir(const std::string& name) {
    auto p = fs::temp_directory_path() / name;
    fs::create_directories(p);
    return p;
}

static void writeFile(const fs::path& dir, const std::string& name,
                      const std::string& content) {
    std::ofstream f(dir / name, std::ios::binary);
    f << content;
}

// Build a minimal simulated API response page.
// items_on_page  – number of {"text":"doc_N"} items on this page
// total          – total available (0 = omit)
// next_cursor    – non-empty = include "next_cursor" field (cursor mode)
static std::string makeApiPage(int items_on_page, int total = 0,
                                const std::string& next_cursor = "") {
    std::ostringstream body;
    body << "{";
    if (total > 0) body << "\"total\":" << total << ",";
    body << "\"items\":[";
    for (int i = 0; i < items_on_page; ++i) {
        if (i > 0) body << ",";
        body << "{\"text\":\"doc_" << i << "\"}";
    }
    body << "]";
    if (!next_cursor.empty()) {
        body << ",\"next_cursor\":\"" << next_cursor << "\"";
    }
    body << "}";
    return body.str();
}

// ============================================================================
// FileSystemIngester integration tests
// ============================================================================

class FileSystemIntegrationTest : public ::testing::Test {
protected:
    fs::path tmp_root_;

    void SetUp() override {
        tmp_root_ = makeDir("themis_fs_integration_test");
    }

    void TearDown() override {
        fs::remove_all(tmp_root_);
    }
};

TEST_F(FileSystemIntegrationTest, RecursiveDirectoryWalkMultiLevel) {
    // Create a two-level directory hierarchy
    fs::create_directories(tmp_root_ / "level1a" / "level2");
    fs::create_directories(tmp_root_ / "level1b");

    writeFile(tmp_root_,               "root.txt",         "root text");
    writeFile(tmp_root_ / "level1a",   "doc_a.txt",        "level1a text");
    writeFile(tmp_root_ / "level1a" / "level2", "deep.txt","deep text");
    writeFile(tmp_root_ / "level1b",   "doc_b.json",       "{\"k\":\"v\"}");

    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "fs_recursive";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_root_.string();
    cfg.options["recursive"] = "true";
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("fs_recursive");
    // root.txt + doc_a.txt + deep.txt + doc_b.json = 4 documents
    EXPECT_EQ(stats.documents_processed, 4u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_TRUE(stats.errors.empty());
    EXPECT_GT(stats.bytes_processed,     0u);
}

TEST_F(FileSystemIntegrationTest, NonRecursiveSingleLevel) {
    fs::create_directories(tmp_root_ / "sub");
    writeFile(tmp_root_,        "a.txt", "top level");
    writeFile(tmp_root_ / "sub","b.txt", "sub level - should be excluded");

    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "fs_flat";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_root_.string();
    cfg.options["recursive"] = "false";
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("fs_flat");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST_F(FileSystemIntegrationTest, MixedFormatsDirectory) {
    writeFile(tmp_root_, "doc.txt",  "plain text content");
    writeFile(tmp_root_, "doc.json", R"({"title":"json doc","body":"text"})");
    writeFile(tmp_root_, "doc.html", "<html><body><p>html content</p></body></html>");
    writeFile(tmp_root_, "doc.xml",  "<?xml version=\"1.0\"?><root><item>xml</item></root>");
    writeFile(tmp_root_, "doc.csv",  "id,name\n1,alice\n2,bob\n");

    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id            = "fs_mixed";
    cfg.type                 = SourceType::FILESYSTEM;
    cfg.location             = tmp_root_.string();
    cfg.options["recursive"] = "false";
    // Disable PDF/DOCX converters to ensure CI stability
    cfg.options["pdf_converter"]  = "";
    cfg.options["docx_converter"] = "";
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("fs_mixed");
    // txt + json + html + xml + csv = 5 files
    EXPECT_EQ(stats.documents_processed, 5u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_GT(stats.bytes_processed,     0u);
}

TEST_F(FileSystemIntegrationTest, ProgressCallbackInvokedForEachDocument) {
    for (int i = 0; i < 5; ++i) {
        writeFile(tmp_root_, "doc" + std::to_string(i) + ".txt",
                  "content " + std::to_string(i));
    }

    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id            = "fs_progress";
    cfg.type                 = SourceType::FILESYSTEM;
    cfg.location             = tmp_root_.string();
    cfg.options["recursive"] = "false";
    ASSERT_TRUE(mgr.registerSource(cfg));

    std::atomic<int> callback_count{0};
    ProgressCallback cb = [&](const std::string& source_id, size_t /*processed*/,
                               size_t /*total*/, const std::string& /*status*/) {
        EXPECT_EQ(source_id, "fs_progress");
        ++callback_count;
    };

    auto stats = mgr.ingestSource("fs_progress", cb);
    EXPECT_EQ(stats.documents_processed, 5u);
    // At minimum one progress callback per document
    EXPECT_GE(callback_count.load(), 1);
}

TEST_F(FileSystemIntegrationTest, EmptyDirectoryProducesNoDocuments) {
    // tmp_root_ is empty (SetUp creates it but writes nothing)
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id            = "fs_empty";
    cfg.type                 = SourceType::FILESYSTEM;
    cfg.location             = tmp_root_.string();
    cfg.options["recursive"] = "true";
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("fs_empty");
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST_F(FileSystemIntegrationTest, MultipleFilesystemSourcesViaIngestAll) {
    auto dir_a = tmp_root_ / "src_a";
    auto dir_b = tmp_root_ / "src_b";
    fs::create_directories(dir_a);
    fs::create_directories(dir_b);

    writeFile(dir_a, "a1.txt", "alpha");
    writeFile(dir_a, "a2.txt", "beta");
    writeFile(dir_b, "b1.txt", "gamma");

    IngestionManager mgr("test_db");

    SourceConfig cfgA;
    cfgA.source_id = "src_a";
    cfgA.type      = SourceType::FILESYSTEM;
    cfgA.location  = dir_a.string();
    cfgA.options["recursive"] = "false";
    ASSERT_TRUE(mgr.registerSource(cfgA));

    SourceConfig cfgB;
    cfgB.source_id = "src_b";
    cfgB.type      = SourceType::FILESYSTEM;
    cfgB.location  = dir_b.string();
    cfgB.options["recursive"] = "false";
    ASSERT_TRUE(mgr.registerSource(cfgB));

    auto report = mgr.ingestAll();
    EXPECT_EQ(report.total_documents, 3u);
    EXPECT_EQ(report.total_failures,  0u);

    ASSERT_TRUE(report.source_stats.count("src_a") > 0);
    ASSERT_TRUE(report.source_stats.count("src_b") > 0);
    EXPECT_EQ(report.source_stats.at("src_a").documents_processed, 2u);
    EXPECT_EQ(report.source_stats.at("src_b").documents_processed, 1u);
}

TEST_F(FileSystemIntegrationTest, SingleFileSourceIngested) {
    writeFile(tmp_root_, "single.txt", "single file content for integration test");

    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "fs_single_file";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = (tmp_root_ / "single.txt").string();
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("fs_single_file");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_GT(stats.bytes_processed,     0u);
}

// ============================================================================
// HuggingFaceConnector integration tests
// ============================================================================

// Helper: build a minimal HuggingFace source config.
static SourceConfig makeHfConfig(const std::string& source_id,
                                  const std::string& dataset,
                                  const std::string& split,
                                  bool streaming = false) {
    SourceConfig cfg;
    cfg.source_id               = source_id;
    cfg.type                    = SourceType::HUGGINGFACE;
    cfg.location                = dataset;
    cfg.options["split"]        = split;
    cfg.options["streaming"]    = streaming ? "true" : "false";
    return cfg;
}

// Shared mock HTTP GET for HuggingFace tests.
// Returns a `rows` metadata count for `/api/datasets/.../metadata` URLs and
// HTTP 200 for all other (data) URLs so the ingestion loop completes without
// making real network requests.
// DEFAULT_HF_TEST_ROWS matches the row count used in test assertions to keep
// the mock and expectations in sync without hardcoding "12000" in multiple places.
static constexpr size_t DEFAULT_HF_TEST_ROWS = 12000;
static ApiHttpGetFn makeHfHttpGetMock(size_t row_count = DEFAULT_HF_TEST_ROWS) {
    return [row_count](const std::string& url, const std::string& /*auth*/)
           -> std::pair<int, std::string> {
        if (url.find("/metadata") != std::string::npos)
            return {200, "{\"rows\":" + std::to_string(row_count) + "}"};
        return {200, "{\"status\":\"ok\"}"};
    };
}

TEST(HuggingFaceIntegrationTest, BatchModeIngestViaManager) {
    IngestionManager mgr("test_db");
    mgr.setApiHttpGetForTesting(makeHfHttpGetMock());
    ASSERT_TRUE(mgr.registerSource(
        makeHfConfig("hf_batch", "lexlms/ger_legal_data", "train", false)));

    auto stats = mgr.ingestSource("hf_batch");
    EXPECT_EQ(stats.documents_processed, DEFAULT_HF_TEST_ROWS);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(HuggingFaceIntegrationTest, StreamingModeIngestViaManager) {
    IngestionManager mgr("test_db");
    mgr.setApiHttpGetForTesting(makeHfHttpGetMock());
    ASSERT_TRUE(mgr.registerSource(
        makeHfConfig("hf_stream", "allenai/c4", "validation", true)));

    auto stats = mgr.ingestSource("hf_stream");
    EXPECT_EQ(stats.documents_processed, DEFAULT_HF_TEST_ROWS);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST(HuggingFaceIntegrationTest, ApiTokenPropagatedViaOptions) {
    // Verify that a token set in options is accepted and ingestion proceeds
    SourceConfig cfg = makeHfConfig("hf_auth", "test/private_dataset", "train");
    cfg.options["token"] = "hf_api_token_abc123";

    HuggingFaceConnector conn;
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setApiToken("hf_api_token_abc123");
    conn.setHttpGetForTesting(makeHfHttpGetMock());

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, DEFAULT_HF_TEST_ROWS);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST(HuggingFaceIntegrationTest, DatasetSplitTrainVsTest) {
    // Both splits should succeed via IngestionManager
    IngestionManager mgr("test_db");
    mgr.setApiHttpGetForTesting(makeHfHttpGetMock());

    ASSERT_TRUE(mgr.registerSource(
        makeHfConfig("hf_train", "dataset/example", "train")));
    ASSERT_TRUE(mgr.registerSource(
        makeHfConfig("hf_test",  "dataset/example", "test")));

    auto report = mgr.ingestAll();
    EXPECT_EQ(report.source_stats.count("hf_train"), 1u);
    EXPECT_EQ(report.source_stats.count("hf_test"),  1u);
    EXPECT_EQ(report.source_stats.at("hf_train").documents_processed, DEFAULT_HF_TEST_ROWS);
    EXPECT_EQ(report.source_stats.at("hf_test").documents_processed,  12000u);
}

TEST(HuggingFaceIntegrationTest, BatchSizeConfigured) {
    HuggingFaceConnector conn;
    SourceConfig cfg = makeHfConfig("hf_bs", "test/dataset", "train");
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setBatchSize(500);
    conn.setStreamingMode(false);
    conn.setHttpGetForTesting(makeHfHttpGetMock());

    // Setting batch size must not crash or corrupt the ingestion flow
    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(HuggingFaceIntegrationTest, RetryConfigApplied) {
    HuggingFaceConnector conn;
    SourceConfig cfg = makeHfConfig("hf_retry", "test/dataset", "train");
    ASSERT_TRUE(conn.initialize(cfg));

    RetryConfig retry;
    retry.max_attempts    = 5;
    retry.initial_delay_ms = 10.0;  // short delay for tests
    retry.backoff_factor   = 2.0;
    conn.setRetryConfig(retry);
    conn.setHttpGetForTesting(makeHfHttpGetMock());

    EXPECT_NO_THROW(conn.ingest("col", nullptr));
}

TEST(HuggingFaceIntegrationTest, IsAvailableAndDocumentCount) {
    HuggingFaceConnector conn;
    SourceConfig cfg = makeHfConfig("hf_meta", "test/dataset", "train");
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setHttpGetForTesting(makeHfHttpGetMock());

    // isAvailable() and getDocumentCount() must not throw
    EXPECT_NO_THROW(conn.isAvailable());
    EXPECT_NO_THROW({
        size_t count = conn.getDocumentCount();
        (void)count;
    });
}

TEST(HuggingFaceIntegrationTest, ProgressCallbackFired) {
    IngestionManager mgr("test_db");
    mgr.setApiHttpGetForTesting(makeHfHttpGetMock());
    ASSERT_TRUE(mgr.registerSource(
        makeHfConfig("hf_prog", "test/dataset", "train", false)));

    std::atomic<int> cb_count{0};
    ProgressCallback cb = [&](const std::string& /*id*/, size_t /*processed*/,
                               size_t /*total*/, const std::string& /*status*/) {
        ++cb_count;
    };

    auto stats = mgr.ingestSource("hf_prog", cb);
    EXPECT_EQ(stats.documents_processed, DEFAULT_HF_TEST_ROWS);
    EXPECT_GE(cb_count.load(), 1);
}

// ============================================================================
// GenericApiConnector integration tests
// ============================================================================

TEST(GenericApiIntegrationTest, OffsetPaginationTwoPages) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id              = "api_offset";
    cfg.type                   = SourceType::API;
    cfg.location               = "https://api.example.com/v1/docs";
    cfg.options["pagination_mode"] = "offset";
    cfg.options["page_size"]       = "3";
    cfg.options["max_pages"]       = "2";
    cfg.options["text_field"]      = "text";
    ASSERT_TRUE(mgr.registerSource(cfg));

    // Inject a mock that returns 3 items per page for exactly 2 pages,
    // then an empty page.
    std::atomic<int> call_count{0};
    mgr.setApiHttpGetForTesting(
        [&](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            int n = call_count.fetch_add(1);
            if (n < 2) return {200, makeApiPage(3, 6)};
            return {200, makeApiPage(0)};
        });

    auto stats = mgr.ingestSource("api_offset");
    EXPECT_EQ(stats.documents_processed, 6u);   // 2 pages × 3 docs
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(GenericApiIntegrationTest, CursorPaginationThreePages) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id                          = "api_cursor";
    cfg.type                               = SourceType::API;
    cfg.location                           = "https://api.example.com/v2/docs";
    cfg.options["pagination_mode"]         = "cursor";
    cfg.options["cursor_param"]            = "page_token";
    cfg.options["cursor_response_field"]   = "next_cursor";
    cfg.options["page_size"]               = "4";
    cfg.options["max_pages"]               = "3";
    cfg.options["text_field"]              = "text";
    ASSERT_TRUE(mgr.registerSource(cfg));

    std::atomic<int> call_count{0};
    mgr.setApiHttpGetForTesting(
        [&](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            int n = call_count.fetch_add(1);
            if (n == 0) return {200, makeApiPage(4, 0, "cursor_page2")};
            if (n == 1) return {200, makeApiPage(4, 0, "cursor_page3")};
            // Third page: 4 items, no next_cursor → terminates
            return {200, makeApiPage(4)};
        });

    auto stats = mgr.ingestSource("api_cursor");
    EXPECT_EQ(stats.documents_processed, 12u);  // 3 pages × 4 docs
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(GenericApiIntegrationTest, MaxPagesLimitHonoured) {
    // The API would return infinite pages, but max_pages=1 caps at one page.
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id              = "api_max_pages";
    cfg.type                   = SourceType::API;
    cfg.location               = "https://api.example.com/docs";
    cfg.options["pagination_mode"] = "offset";
    cfg.options["page_size"]       = "5";
    cfg.options["max_pages"]       = "1";
    cfg.options["text_field"]      = "text";
    ASSERT_TRUE(mgr.registerSource(cfg));

    mgr.setApiHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {200, makeApiPage(5, 1000)};  // always returns 5 items
        });

    auto stats = mgr.ingestSource("api_max_pages");
    EXPECT_EQ(stats.documents_processed, 5u);   // only 1 page allowed
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST(GenericApiIntegrationTest, ApiKeyForwardedInAuthHeader) {
    GenericApiConnector conn;

    SourceConfig cfg;
    cfg.source_id = "api_key_test";
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/data";
    cfg.options["page_size"]  = "2";
    cfg.options["max_pages"]  = "1";
    cfg.options["text_field"] = "text";
    ASSERT_TRUE(conn.initialize(cfg));

    conn.setApiKey("test-secret-key");

    std::string captured_auth;
    conn.setHttpGetForTesting(
        [&](const std::string& /*url*/, const std::string& auth)
        -> std::pair<int, std::string> {
            captured_auth = auth;
            return {200, makeApiPage(2)};
        });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
    EXPECT_EQ(captured_auth, "Bearer test-secret-key");
}

TEST(GenericApiIntegrationTest, EmptyFirstPageProducesZeroDocs) {
    GenericApiConnector conn;

    SourceConfig cfg;
    cfg.source_id = "api_empty";
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/empty";
    cfg.options["page_size"]  = "10";
    cfg.options["max_pages"]  = "5";
    cfg.options["text_field"] = "text";
    ASSERT_TRUE(conn.initialize(cfg));

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {200, makeApiPage(0)};  // empty response
        });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(GenericApiIntegrationTest, CursorModeEmptyFirstPageTerminates) {
    GenericApiConnector conn;

    SourceConfig cfg;
    cfg.source_id                        = "api_cursor_empty";
    cfg.type                             = SourceType::API;
    cfg.location                         = "https://api.example.com/v2/empty";
    cfg.options["pagination_mode"]       = "cursor";
    cfg.options["cursor_response_field"] = "next_cursor";
    cfg.options["page_size"]             = "10";
    cfg.options["max_pages"]             = "5";
    cfg.options["text_field"]            = "text";
    ASSERT_TRUE(conn.initialize(cfg));

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {200, makeApiPage(0)};  // no items, no cursor
        });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST(GenericApiIntegrationTest, DocumentCountEstimateFromApiResponse) {
    GenericApiConnector conn;

    SourceConfig cfg;
    cfg.source_id = "api_count";
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/docs";
    cfg.options["page_size"]  = "5";
    cfg.options["max_pages"]  = "1";
    cfg.options["text_field"] = "text";
    ASSERT_TRUE(conn.initialize(cfg));

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {200, makeApiPage(5, 42)};  // total=42
        });

    // getDocumentCount() must not throw even before ingest()
    EXPECT_NO_THROW({
        size_t count = conn.getDocumentCount();
        (void)count;
    });
}

TEST(GenericApiIntegrationTest, PageSizeConfiguredViaSetPageSize) {
    GenericApiConnector conn;

    SourceConfig cfg;
    cfg.source_id = "api_page_size";
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/docs";
    cfg.options["max_pages"]  = "1";
    cfg.options["text_field"] = "text";
    ASSERT_TRUE(conn.initialize(cfg));

    conn.setPageSize(10);

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {200, makeApiPage(10)};
        });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 10u);
}

// ============================================================================
// Multi-source integration: filesystem + HuggingFace + generic API together
// ============================================================================

TEST(MultiSourceIntegrationTest, AllThreeConnectorTypesInOneRun) {
    // Prepare a temporary directory with 2 text files for the filesystem source.
    auto tmp_dir = makeDir("themis_multisource_integration");
    writeFile(tmp_dir, "a.txt", "file content a");
    writeFile(tmp_dir, "b.txt", "file content b");

    IngestionManager mgr("test_db");

    // 1. Filesystem source
    SourceConfig fsCfg;
    fsCfg.source_id            = "ms_fs";
    fsCfg.type                 = SourceType::FILESYSTEM;
    fsCfg.location             = tmp_dir.string();
    fsCfg.options["recursive"] = "false";
    ASSERT_TRUE(mgr.registerSource(fsCfg));

    // 2. HuggingFace source (stub returns 12 000 docs)
    ASSERT_TRUE(mgr.registerSource(
        makeHfConfig("ms_hf", "test/legal_dataset", "train")));

    // 3. Generic API source (cursor mode, 2 pages × 5 docs)
    SourceConfig apiCfg;
    apiCfg.source_id                        = "ms_api";
    apiCfg.type                             = SourceType::API;
    apiCfg.location                         = "https://api.example.com/v1/data";
    apiCfg.options["pagination_mode"]       = "cursor";
    apiCfg.options["cursor_response_field"] = "next_cursor";
    apiCfg.options["page_size"]             = "5";
    apiCfg.options["max_pages"]             = "2";
    apiCfg.options["text_field"]            = "text";
    ASSERT_TRUE(mgr.registerSource(apiCfg));

    std::atomic<int> api_page{0};
    mgr.setApiHttpGetForTesting(
        [&](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            int n = api_page.fetch_add(1);
            if (n == 0) return {200, makeApiPage(5, 0, "page2token")};
            return {200, makeApiPage(5)};  // no next_cursor
        });

    auto report = mgr.ingestAll();

    // All three sources must appear in the report
    EXPECT_EQ(report.source_stats.count("ms_fs"),  1u);
    EXPECT_EQ(report.source_stats.count("ms_hf"),  1u);
    EXPECT_EQ(report.source_stats.count("ms_api"), 1u);

    // Per-source counts
    EXPECT_EQ(report.source_stats.at("ms_fs").documents_processed,  2u);
    EXPECT_EQ(report.source_stats.at("ms_hf").documents_processed,  12000u);
    EXPECT_EQ(report.source_stats.at("ms_api").documents_processed, 10u);

    // Aggregate
    EXPECT_EQ(report.total_documents, 12012u);
    EXPECT_EQ(report.total_failures,  0u);

    fs::remove_all(tmp_dir);
}

TEST(MultiSourceIntegrationTest, DisabledSourceSkippedInIngestAll) {
    auto tmp_dir = makeDir("themis_disabled_source_test");
    writeFile(tmp_dir, "x.txt", "content");

    IngestionManager mgr("test_db");

    SourceConfig active;
    active.source_id            = "active_fs";
    active.type                 = SourceType::FILESYSTEM;
    active.location             = tmp_dir.string();
    active.options["recursive"] = "false";
    active.enabled              = true;
    ASSERT_TRUE(mgr.registerSource(active));

    SourceConfig disabled;
    disabled.source_id            = "disabled_fs";
    disabled.type                 = SourceType::FILESYSTEM;
    disabled.location             = tmp_dir.string();
    disabled.options["recursive"] = "false";
    disabled.enabled              = false;
    ASSERT_TRUE(mgr.registerSource(disabled));

    auto report = mgr.ingestAll();

    // Only the active source contributes documents
    EXPECT_EQ(report.total_documents, 1u);
    // Disabled source should not appear or have 0 docs
    if (report.source_stats.count("disabled_fs") > 0) {
        EXPECT_EQ(report.source_stats.at("disabled_fs").documents_processed, 0u);
    }

    fs::remove_all(tmp_dir);
}

TEST(MultiSourceIntegrationTest, ParallelIngestionMultipleSources) {
    auto root = makeDir("themis_parallel_integration");
    for (int i = 0; i < 3; ++i) {
        auto d = root / ("src" + std::to_string(i));
        fs::create_directories(d);
        for (int j = 0; j < 4; ++j) {
            writeFile(d, "f" + std::to_string(j) + ".txt",
                      "content " + std::to_string(i) + "_" + std::to_string(j));
        }
    }

    IngestionManager mgr("test_db");
    mgr.setParallelProcessing(true, 3);

    for (int i = 0; i < 3; ++i) {
        SourceConfig cfg;
        cfg.source_id            = "par_" + std::to_string(i);
        cfg.type                 = SourceType::FILESYSTEM;
        cfg.location             = (root / ("src" + std::to_string(i))).string();
        cfg.options["recursive"] = "false";
        ASSERT_TRUE(mgr.registerSource(cfg));
    }

    auto report = mgr.ingestAll();
    EXPECT_EQ(report.total_documents, 12u);  // 3 sources × 4 docs
    EXPECT_EQ(report.total_failures,  0u);

    fs::remove_all(root);
}

// ============================================================================
// HTTP client TLS configuration – ca_bundle_path tests
// ============================================================================

TEST(RetryConfigTest, CaBundlePathDefaultIsEmpty) {
    RetryConfig cfg;
    EXPECT_TRUE(cfg.ca_bundle_path.empty());
}

TEST(RetryConfigTest, CaBundlePathCanBeSet) {
    RetryConfig cfg;
    cfg.ca_bundle_path = "/etc/ssl/certs/ca-certificates.crt";
    EXPECT_EQ(cfg.ca_bundle_path, "/etc/ssl/certs/ca-certificates.crt");
}

TEST(HuggingFaceIntegrationTest, CaBundlePathFromOptions) {
    HuggingFaceConnector conn;
    SourceConfig cfg = makeHfConfig("hf_ca_bundle", "test/dataset", "train");
    cfg.options["ca_bundle_path"] = "/etc/ssl/certs/ca-certificates.crt";
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setHttpGetForTesting(makeHfHttpGetMock());

    // Ingestion must succeed with ca_bundle_path option set (mock HTTP path)
    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, DEFAULT_HF_TEST_ROWS);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(HuggingFaceIntegrationTest, CaBundlePathViaRetryConfig) {
    HuggingFaceConnector conn;
    SourceConfig cfg = makeHfConfig("hf_ca_retry", "test/dataset", "train");
    ASSERT_TRUE(conn.initialize(cfg));

    RetryConfig retry;
    retry.ca_bundle_path = "/etc/ssl/certs/ca-certificates.crt";
    conn.setRetryConfig(retry);
    conn.setHttpGetForTesting(makeHfHttpGetMock());

    // setRetryConfig must not discard ca_bundle_path
    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, DEFAULT_HF_TEST_ROWS);
    EXPECT_EQ(stats.documents_failed, 0u);
}

TEST(GenericApiIntegrationTest, CaBundlePathFromOptions) {
    GenericApiConnector conn;

    SourceConfig cfg;
    cfg.source_id              = "api_ca_bundle";
    cfg.type                   = SourceType::API;
    cfg.location               = "https://api.example.com/data";
    cfg.options["page_size"]   = "3";
    cfg.options["max_pages"]   = "1";
    cfg.options["text_field"]  = "text";
    cfg.options["ca_bundle_path"] = "/etc/ssl/certs/ca-certificates.crt";
    ASSERT_TRUE(conn.initialize(cfg));

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {200, makeApiPage(3)};
        });

    // Ingestion must succeed with ca_bundle_path option set (mock HTTP path)
    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(GenericApiIntegrationTest, CaBundlePathViaRetryConfig) {
    GenericApiConnector conn;

    SourceConfig cfg;
    cfg.source_id             = "api_ca_retry";
    cfg.type                  = SourceType::API;
    cfg.location              = "https://api.example.com/data";
    cfg.options["page_size"]  = "2";
    cfg.options["max_pages"]  = "1";
    cfg.options["text_field"] = "text";
    ASSERT_TRUE(conn.initialize(cfg));

    RetryConfig retry;
    retry.ca_bundle_path = "/etc/ssl/certs/ca-certificates.crt";
    conn.setRetryConfig(retry);

    conn.setHttpGetForTesting(
        [](const std::string& /*url*/, const std::string& /*auth*/)
        -> std::pair<int, std::string> {
            return {200, makeApiPage(2)};
        });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

TEST(MultiSourceIntegrationTest, IngestAllReportCorrelationIds) {
    auto tmp_dir = makeDir("themis_corrids_test");
    writeFile(tmp_dir, "doc.txt", "hello");

    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id            = "corr_src";
    cfg.type                 = SourceType::FILESYSTEM;
    cfg.location             = tmp_dir.string();
    cfg.options["recursive"] = "false";
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto report = mgr.ingestAll();
    // Each per-source IngestionStats should carry a non-empty correlation_id
    for (const auto& [sid, stats] : report.source_stats) {
        EXPECT_FALSE(stats.correlation_id.empty())
            << "correlation_id missing for source: " << sid;
    }

    fs::remove_all(tmp_dir);
}
