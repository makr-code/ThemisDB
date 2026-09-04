/**
 * @file test_ingestion_checkpoint.cpp
 * @brief Unit tests for CheckpointStore, incremental ingestion mode,
 *        GenericApiConnector, and IngestionManager checkpoint integration.
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include "ingestion/api_connector.h"
#include <filesystem>
#include <fstream>
#include <utility>

using namespace themis::ingestion;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Shared mock HTTP GET: returns the same simulated response that was
// previously hard-coded in the apiHttpGet stub.  Inject via
// setHttpGetForTesting() / setApiHttpGetForTesting() so tests are
// independent of real network connectivity.
// ---------------------------------------------------------------------------
static ApiHttpGetFn makeMockHttpGet() {
    return [](const std::string& /*url*/,
              const std::string& /*auth*/) -> std::pair<int, std::string> {
        return {200,
                R"({"total":6,"next_cursor":"cursor_page2","items":[)"
                R"({"text":"doc alpha"},)"
                R"({"text":"doc beta"},)"
                R"({"text":"doc gamma"}]})"};
    };
}

// ============================================================================
// Test fixture – temporary directory for checkpoint files
// ============================================================================

class CheckpointTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "themis_cp_test";
        fs::create_directories(tmp_dir_);
    }
    void TearDown() override {
        std::error_code ec = {};
        fs::remove_all(tmp_dir_, ec);
    }

    fs::path tmp_dir_;
};

// ============================================================================
// CheckpointStore – unit tests
// ============================================================================

TEST_F(CheckpointTest, WriteAndRead) {
    CheckpointStore store(tmp_dir_.string());

    IngestionCheckpoint cp;
    cp.source_id       = "hf_legal";
    cp.processed_count = 5000;
    cp.byte_offset     = 102400;
    cp.cursor          = "page_42";
    cp.timestamp       = "2026-02-20T16:00:00Z";

    ASSERT_TRUE(store.write(cp));

    IngestionCheckpoint out;
    ASSERT_TRUE(store.read("hf_legal", out));

    EXPECT_EQ(out.source_id,       "hf_legal");
    EXPECT_EQ(out.processed_count, 5000u);
    EXPECT_EQ(out.byte_offset,     102400u);
    EXPECT_EQ(out.cursor,          "page_42");
    EXPECT_EQ(out.timestamp,       "2026-02-20T16:00:00Z");
}

TEST_F(CheckpointTest, ExistsAfterWrite) {
    CheckpointStore store(tmp_dir_.string());
    EXPECT_FALSE(store.exists("new_source"));

    IngestionCheckpoint cp;
    cp.source_id = "new_source";
    store.write(cp);

    EXPECT_TRUE(store.exists("new_source"));
}

TEST_F(CheckpointTest, ClearRemovesFile) {
    CheckpointStore store(tmp_dir_.string());

    IngestionCheckpoint cp;
    cp.source_id = "ephemeral";
    store.write(cp);

    ASSERT_TRUE(store.exists("ephemeral"));
    EXPECT_TRUE(store.clear("ephemeral"));
    EXPECT_FALSE(store.exists("ephemeral"));
}

TEST_F(CheckpointTest, ClearNonExistentReturnsFalse) {
    CheckpointStore store(tmp_dir_.string());
    EXPECT_FALSE(store.clear("ghost_source"));
}

TEST_F(CheckpointTest, ReadNonExistentReturnsFalse) {
    CheckpointStore store(tmp_dir_.string());
    IngestionCheckpoint out;
    EXPECT_FALSE(store.read("missing", out));
}

TEST_F(CheckpointTest, OverwriteUpdatesValues) {
    CheckpointStore store(tmp_dir_.string());

    IngestionCheckpoint cp;
    cp.source_id       = "my_src";
    cp.processed_count = 100;
    store.write(cp);

    cp.processed_count = 200;
    cp.cursor          = "next_page";
    store.write(cp);

    IngestionCheckpoint out;
    ASSERT_TRUE(store.read("my_src", out));
    EXPECT_EQ(out.processed_count, 200u);
    EXPECT_EQ(out.cursor,          "next_page");
}

TEST_F(CheckpointTest, SourceIdWithSpecialCharsIsSanitised) {
    // Dots and hyphens should be preserved; slashes become underscores
    CheckpointStore store(tmp_dir_.string());
    IngestionCheckpoint cp;
    cp.source_id = "org/dataset-v1.0";
    ASSERT_TRUE(store.write(cp));

    // Read back using the same source_id string
    IngestionCheckpoint out;
    EXPECT_TRUE(store.read("org/dataset-v1.0", out));
    EXPECT_EQ(out.source_id, "org/dataset-v1.0");
}

TEST_F(CheckpointTest, EmptyCursorRoundtrips) {
    CheckpointStore store(tmp_dir_.string());
    IngestionCheckpoint cp;
    cp.source_id = "no_cursor";
    cp.cursor    = "";
    store.write(cp);

    IngestionCheckpoint out;
    ASSERT_TRUE(store.read("no_cursor", out));
    EXPECT_EQ(out.cursor, "");
}

TEST_F(CheckpointTest, CorruptFileReturnsFalse) {
    // Write a file with no source_id= line so read() should return false
    // (guards against treating empty/corrupt checkpoint files as valid)
    auto corrupt_path = tmp_dir_ / "corrupt_src.checkpoint";
    {
        std::ofstream f(corrupt_path);
        f << "processed_count=42\n";  // missing source_id=
    }
    CheckpointStore store(tmp_dir_.string());
    IngestionCheckpoint out;
    EXPECT_FALSE(store.read("corrupt_src", out));
}

// ============================================================================
// IngestionManager – checkpoint integration tests
// ============================================================================

TEST_F(CheckpointTest, SetCheckpointDirDoesNotCrash) {
    IngestionManager mgr("test_db");
    // Just verifying no exception/crash when a valid directory is set
    EXPECT_NO_THROW(mgr.setCheckpointDir(tmp_dir_.string()));
}

TEST_F(CheckpointTest, GetCheckpointReturnsFalseWhenNoneSet) {
    IngestionManager mgr("test_db");
    IngestionCheckpoint out;
    // No checkpoint dir set – should return false gracefully
    EXPECT_FALSE(mgr.getCheckpoint("any_source", out));
}

TEST_F(CheckpointTest, ClearCheckpointReturnsFalseWhenNoneSet) {
    IngestionManager mgr("test_db");
    EXPECT_FALSE(mgr.clearCheckpoint("any_source"));
}

TEST_F(CheckpointTest, IncrementalModeDefaultOff) {
    IngestionManager mgr("test_db");
    EXPECT_FALSE(mgr.isIncrementalMode());
}

TEST_F(CheckpointTest, IncrementalModeToggle) {
    IngestionManager mgr("test_db");
    mgr.enableIncrementalMode(true);
    EXPECT_TRUE(mgr.isIncrementalMode());
    mgr.enableIncrementalMode(false);
    EXPECT_FALSE(mgr.isIncrementalMode());
}

TEST_F(CheckpointTest, CheckpointWrittenAfterSuccessfulIngestSource) {
    // Register a real filesystem source with a temp file so ingest() succeeds
    fs::path data_dir = tmp_dir_ / "docs";
    fs::create_directories(data_dir);
    {
        std::ofstream f(data_dir / "a.txt");
        f << "hello world";
    }

    IngestionManager mgr("test_db");
    mgr.setCheckpointDir(tmp_dir_.string());
    mgr.enableIncrementalMode(true);
    mgr.registerSource({
        "fs_src", SourceType::FILESYSTEM, data_dir.string(), 5, true, {}
    });

    auto stats = mgr.ingestSource("fs_src");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_TRUE(stats.errors.empty() || stats.documents_failed == 0u);

    // Checkpoint should now exist
    IngestionCheckpoint out;
    ASSERT_TRUE(mgr.getCheckpoint("fs_src", out));
    EXPECT_EQ(out.source_id, "fs_src");
    EXPECT_EQ(out.processed_count, 1u);
}

TEST_F(CheckpointTest, ClearCheckpointViaManager) {
    fs::path data_dir = tmp_dir_ / "docs2";
    fs::create_directories(data_dir);
    { std::ofstream f(data_dir / "x.txt"); f << "data"; }

    IngestionManager mgr("test_db");
    mgr.setCheckpointDir(tmp_dir_.string());
    mgr.enableIncrementalMode(true);
    mgr.registerSource({"fs2", SourceType::FILESYSTEM, data_dir.string(), 5, true, {}});
    mgr.ingestSource("fs2");

    IngestionCheckpoint out;
    ASSERT_TRUE(mgr.getCheckpoint("fs2", out));
    EXPECT_TRUE(mgr.clearCheckpoint("fs2"));
    EXPECT_FALSE(mgr.getCheckpoint("fs2", out));
}

// ============================================================================
// GenericApiConnector – unit tests
// ============================================================================

TEST(ApiConnectorTest, InitializeWithApiType) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id = "my_api";
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/v1/docs";
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(ApiConnectorTest, InitializeFailsForNonApiType) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id = "fs_source";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp";
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(ApiConnectorTest, IsAvailableAfterValidInit) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id = "api1";
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/items";
    conn.initialize(cfg);
    conn.setHttpGetForTesting(makeMockHttpGet());
    // Mock always returns 200
    EXPECT_TRUE(conn.isAvailable());
}

TEST(ApiConnectorTest, IsAvailableReturnsFalseWithoutEndpoint) {
    GenericApiConnector conn;
    // Not initialized – endpoint is empty
    EXPECT_FALSE(conn.isAvailable());
}

TEST(ApiConnectorTest, GetDocumentCountFromSimulatedResponse) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id = "api_count";
    cfg.type      = SourceType::API;
    cfg.location  = "https://api.example.com/items";
    conn.initialize(cfg);
    conn.setHttpGetForTesting(makeMockHttpGet());
    // Mock body has "total":6
    EXPECT_EQ(conn.getDocumentCount(), 6u);
}

TEST(ApiConnectorTest, IngestReturnsDocumentsFromSimulatedPages) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id             = "api_ingest";
    cfg.type                  = SourceType::API;
    cfg.location              = "https://api.example.com/docs";
    cfg.options["page_size"]  = "3";
    cfg.options["text_field"] = "text";
    // max_pages=1 prevents infinite loop: the mock endpoint always returns
    // exactly page_size docs, so without a page cap the termination condition
    // (docs.size() < page_size_) would never be true.
    cfg.options["max_pages"]  = "1";
    conn.initialize(cfg);
    conn.setHttpGetForTesting(makeMockHttpGet());
    conn.setPageSize(3);

    auto stats = conn.ingest("test_collection", nullptr);
    EXPECT_GT(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST(ApiConnectorTest, IngestRespectMaxPages) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id            = "api_max_pages";
    cfg.type                 = SourceType::API;
    cfg.location             = "https://api.example.com/docs";
    cfg.options["max_pages"] = "1";
    cfg.options["page_size"] = "3";
    conn.initialize(cfg);
    conn.setHttpGetForTesting(makeMockHttpGet());

    auto stats = conn.ingest("col", nullptr);
    // With max_pages=1 and 3 docs per mock page, exactly 3 docs expected
    EXPECT_EQ(stats.documents_processed, 3u);
}

TEST(ApiConnectorTest, SetApiKeyDoesNotCrash) {
    GenericApiConnector conn;
    EXPECT_NO_THROW(conn.setApiKey("secret-token-abc"));
}

TEST(ApiConnectorTest, SetPageSizeDoesNotCrash) {
    GenericApiConnector conn;
    EXPECT_NO_THROW(conn.setPageSize(50));
}

TEST(ApiConnectorTest, SetRetryConfigDoesNotCrash) {
    GenericApiConnector conn;
    RetryConfig cfg;
    cfg.max_attempts = 5;
    EXPECT_NO_THROW(conn.setRetryConfig(cfg));
}

TEST(ApiConnectorTest, IngestWithProgressCallback) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id            = "api_cb";
    cfg.type                 = SourceType::API;
    cfg.location             = "https://api.example.com/docs";
    cfg.options["max_pages"] = "1";
    conn.initialize(cfg);
    conn.setHttpGetForTesting(makeMockHttpGet());

    size_t cb_calls = 0;
    auto stats = conn.ingest("col", [&](const std::string&, size_t, size_t,
                                        const std::string&) { ++cb_calls; });
    EXPECT_GE(cb_calls, 1u);
}

// ============================================================================
// IngestionManager – API source wired through connector
// ============================================================================

TEST(IngestionManagerApiTest, RegisterAndIngestApiSource) {
    IngestionManager mgr("test_db");
    mgr.setApiHttpGetForTesting(makeMockHttpGet());
    SourceConfig cfg;
    cfg.source_id            = "api_src";
    cfg.type                 = SourceType::API;
    cfg.location             = "https://api.example.com/items";
    cfg.options["max_pages"] = "1";
    cfg.enabled              = true;
    mgr.registerSource(cfg);

    auto stats = mgr.ingestSource("api_src");
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.documents_processed, 0u);
}

TEST(IngestionManagerApiTest, DatabaseSourceStillUnsupported) {
    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "db_src";
    cfg.type      = SourceType::DATABASE;
    cfg.location  = "postgres://localhost/themis";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    auto stats = mgr.ingestSource("db_src");
    // Should get a CONNECTOR_NOT_SUPPORTED error, not crash
    bool found_error = false;
    for (const auto& e : stats.errors) {
        if (e.code == IngestionErrorCode::CONNECTOR_NOT_SUPPORTED) {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST(IngestionManagerApiTest, CursorModeViaSourceConfigOptions) {
    // End-to-end: cursor pagination option flows from SourceConfig through
    // IngestionManager → GenericApiConnector::initialize() → ingest().
    IngestionManager mgr("test_db");
    mgr.setApiHttpGetForTesting(makeMockHttpGet());
    SourceConfig cfg;
    cfg.source_id                        = "cursor_mgr";
    cfg.type                             = SourceType::API;
    cfg.location                         = "https://api.example.com/v2/items";
    cfg.options["pagination_mode"]       = "cursor";
    cfg.options["cursor_response_field"] = "next_cursor";
    cfg.options["page_size"]             = "3";
    cfg.options["max_pages"]             = "2";
    cfg.enabled                          = true;
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto stats = mgr.ingestSource("cursor_mgr");
    EXPECT_EQ(stats.documents_failed,    0u);
    // Two pages × 3 docs each
    EXPECT_EQ(stats.documents_processed, 6u);
    EXPECT_TRUE(stats.errors.empty());
}

// ============================================================================
// GenericApiConnector – cursor-based pagination
// ============================================================================

TEST(ApiConnectorCursorTest, InitializeWithCursorModeOption) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id                        = "cursor_api";
    cfg.type                             = SourceType::API;
    cfg.location                         = "https://api.example.com/v2/docs";
    cfg.options["pagination_mode"]       = "cursor";
    cfg.options["cursor_param"]          = "page_token";
    cfg.options["cursor_response_field"] = "next_page_token";
    cfg.options["page_size"]             = "3";
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(ApiConnectorCursorTest, SetPaginationModeApiDoesNotCrash) {
    GenericApiConnector conn;
    EXPECT_NO_THROW(conn.setPaginationMode(PaginationMode::CURSOR));
    EXPECT_NO_THROW(conn.setPaginationMode(PaginationMode::OFFSET));
}

TEST(ApiConnectorCursorTest, SetCursorResponseFieldDoesNotCrash) {
    GenericApiConnector conn;
    EXPECT_NO_THROW(conn.setCursorResponseField("next_page_token"));
}

TEST(ApiConnectorCursorTest, CursorModeIngestsDocuments) {
    // Mock response always returns 3 docs + "next_cursor":"cursor_page2".
    // Use max_pages=2 to process two pages (6 docs total) before stopping.
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id                        = "cursor_ingest";
    cfg.type                             = SourceType::API;
    cfg.location                         = "https://api.example.com/v2/docs";
    cfg.options["pagination_mode"]       = "cursor";
    cfg.options["cursor_response_field"] = "next_cursor";
    cfg.options["page_size"]             = "3";
    cfg.options["max_pages"]             = "2";
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setHttpGetForTesting(makeMockHttpGet());

    auto stats = conn.ingest("test_collection", nullptr);
    EXPECT_EQ(stats.documents_processed, 6u);
    EXPECT_EQ(stats.documents_failed,    0u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(ApiConnectorCursorTest, CursorModeStopsWhenNoCursorInResponse) {
    // When cursor_response_field points to a field absent in the response,
    // the connector must stop after the first page.
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id                        = "cursor_stop";
    cfg.type                             = SourceType::API;
    cfg.location                         = "https://api.example.com/v2/docs";
    cfg.options["pagination_mode"]       = "cursor";
    // Use a field name that doesn't exist in the mock body
    cfg.options["cursor_response_field"] = "nonexistent_cursor_field";
    cfg.options["page_size"]             = "3";
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setHttpGetForTesting(makeMockHttpGet());

    auto stats = conn.ingest("test_collection", nullptr);
    // Should stop after one page (no cursor field found)
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST(ApiConnectorCursorTest, CursorModeViaSetPaginationMode) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id            = "cursor_via_setter";
    cfg.type                 = SourceType::API;
    cfg.location             = "https://api.example.com/v2/docs";
    cfg.options["page_size"] = "3";
    cfg.options["max_pages"] = "1";
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setHttpGetForTesting(makeMockHttpGet());

    // Override mode and cursor field via setters
    conn.setPaginationMode(PaginationMode::CURSOR);
    conn.setCursorResponseField("nonexistent_cursor_field"); // stop after 1 page

    auto stats = conn.ingest("test_collection", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST(ApiConnectorCursorTest, CursorModeWithProgressCallback) {
    GenericApiConnector conn;
    SourceConfig cfg;
    cfg.source_id                        = "cursor_cb";
    cfg.type                             = SourceType::API;
    cfg.location                         = "https://api.example.com/v2/docs";
    cfg.options["pagination_mode"]       = "cursor";
    cfg.options["cursor_response_field"] = "next_cursor";
    cfg.options["page_size"]             = "3";
    cfg.options["max_pages"]             = "2";
    ASSERT_TRUE(conn.initialize(cfg));
    conn.setHttpGetForTesting(makeMockHttpGet());

    size_t cb_calls = 0;
    auto stats = conn.ingest("col", [&](const std::string&, size_t, size_t,
                                        const std::string&) { ++cb_calls; });
    EXPECT_GE(cb_calls, 1u);
    EXPECT_EQ(stats.documents_processed, 6u);
}

TEST(ApiConnectorCursorTest, PaginationModeEnumValues) {
    // Verify PaginationMode enum is usable in switch/comparison
    PaginationMode m = PaginationMode::OFFSET;
    EXPECT_NE(m, PaginationMode::CURSOR);
    m = PaginationMode::CURSOR;
    EXPECT_EQ(m, PaginationMode::CURSOR);
}
