/**
 * @file test_ingestion_database.cpp
 * @brief Unit tests for the DatabaseConnector ingestion source connector.
 *
 * All tests use the mock-injection path (setRowFetchForTesting) so that no
 * real ODBC driver or database server is required.  The tests verify:
 *   - Initialization from SourceConfig with JDBC URL
 *   - Row-to-text extraction (text_columns option)
 *   - Full-JSON fallback when text_columns is not set
 *   - max_rows limit enforcement
 *   - Empty batch termination
 *   - Progress callback invocation
 *   - Empty-text rows counted as failures
 *   - Exception in row fetch handled gracefully
 *   - IngestionBuilder::withDatabaseSource() fluent API
 *   - Error handling: missing query and table configuration
 *   - Wrong SourceType rejected by initialize()
 *   - SourceType::DATABASE in sourceTypeLabel (via IngestionMetricsExporter)
 *   - RetryConfig passthrough
 */

#include <gtest/gtest.h>
#include "ingestion/database_connector.h"
#include "ingestion/ingestion_manager.h"
#include <string>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <unordered_map>

using namespace themis::ingestion;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SourceConfig makeDbConfig(
        const std::string& jdbc_url    = "jdbc:postgresql://localhost:5432/testdb",
        const std::string& table       = "documents",
        const std::string& query       = "",
        const std::string& text_cols   = "") {
    SourceConfig cfg;
    cfg.source_id        = "test_db";
    cfg.type             = SourceType::DATABASE;
    cfg.location         = jdbc_url;
    if (!table.empty()) {
      cfg.options["table"]        = table;
    }
    if (!query.empty()) {
      cfg.options["query"]        = query;
    }
    if (!text_cols.empty()) {
      cfg.options["text_columns"] = text_cols;
    }
    return cfg;
}

static DatabaseConnector::DbRow makeRow(
        std::initializer_list<std::pair<std::string, std::string>> cols) {
    DatabaseConnector::DbRow row;
    for (auto& kv : cols) {
      row[kv.first] = kv.second;
    }
    return row;
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, InitializeValidConfig) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig();
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(DatabaseConnectorTest, InitializeWithQuery) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:mysql://host:3306/db",
                            "",
                            "SELECT id, body FROM articles");
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(DatabaseConnectorTest, InitializeWrongType) {
    DatabaseConnector conn;
    SourceConfig cfg;
    cfg.source_id = "wrong";
    cfg.type      = SourceType::API;
    cfg.location  = "jdbc:postgresql://localhost:5432/db";
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(DatabaseConnectorTest, InitializeMissingTableAndQuery) {
    DatabaseConnector conn;
    SourceConfig cfg;
    cfg.source_id = "no_table";
    cfg.type      = SourceType::DATABASE;
    cfg.location  = "jdbc:postgresql://localhost:5432/db";
    // Neither table nor query set → should fail
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(DatabaseConnectorTest, InitializeSqliteUrl) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:sqlite:/tmp/test.db", "items");
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(DatabaseConnectorTest, InitializeSqlServerUrl) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig(
        "jdbc:sqlserver://dbhost:1433;databaseName=sales",
        "", "SELECT * FROM orders");
    EXPECT_TRUE(conn.initialize(cfg));
}

// ---------------------------------------------------------------------------
// isAvailable (mock path always returns true)
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, IsAvailableWithMock) {
    DatabaseConnector conn;
    conn.initialize(makeDbConfig());
    // Inject an empty mock so the availability check uses the mock path
    conn.setRowFetchForTesting([]() -> std::vector<DatabaseConnector::DbRow> {
        return {};
    });
    EXPECT_TRUE(conn.isAvailable());
}

// ---------------------------------------------------------------------------
// getDocumentCount
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, GetDocumentCountWithMockReturnsZero) {
    DatabaseConnector conn;
    conn.initialize(makeDbConfig());
    conn.setRowFetchForTesting([]() -> std::vector<DatabaseConnector::DbRow> {
        return {};
    });
    // Mock path always returns 0 (count unknown)
    EXPECT_EQ(conn.getDocumentCount(), 0u);
}

// ---------------------------------------------------------------------------
// Basic ingestion via mock
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, IngestSingleRow) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                            "", "body");
    conn.initialize(cfg);

    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        if (calls++ == 0) {
            return {makeRow({{"id","1"}, {"body","hello world"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.bytes_processed, 0u);
}

TEST(DatabaseConnectorTest, IngestMultipleRows) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                            "", "body");
    conn.initialize(cfg);

    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        if (calls++ == 0) {
            return {
                makeRow({{"id","1"}, {"body","doc one"}}),
                makeRow({{"id","2"}, {"body","doc two"}}),
                makeRow({{"id","3"}, {"body","doc three"}})
            };
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

// ---------------------------------------------------------------------------
// Text-column extraction
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, TextColumnExtraction) {
    DatabaseConnector conn;
    // text_columns = "title,body" → concatenated
    auto cfg = makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                            "", "title,body");
    conn.initialize(cfg);

    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        if (calls++ == 0) {
            return {makeRow({{"id","1"}, {"title","Hello"}, {"body","World"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

TEST(DatabaseConnectorTest, NoTextColumnsUsesJsonFallback) {
    DatabaseConnector conn;
    // No text_columns → serialize whole row as JSON
    auto cfg = makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs");
    conn.initialize(cfg);

    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        if (calls++ == 0) {
            return {makeRow({{"id","42"}, {"name","Widget"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed, 0u);
}

TEST(DatabaseConnectorTest, EmptyTextColumnFallsBackToJson) {
    DatabaseConnector conn;
    // text_columns = "summary" but that column is empty → falls back to JSON
    auto cfg = makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                            "", "summary");
    conn.initialize(cfg);

    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        if (calls++ == 0) {
            return {makeRow({{"id","1"}, {"summary",""}, {"body","rich content"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    // Falls back to full JSON which is non-empty → processed
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// max_rows enforcement
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, MaxRowsLimit) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                            "", "body");
    cfg.options["max_rows"] = "2";
    conn.initialize(cfg);

    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        ++calls;
        return {
            makeRow({{"id","1"}, {"body","a"}}),
            makeRow({{"id","2"}, {"body","b"}}),
            makeRow({{"id","3"}, {"body","c"}})
        };
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
}

// ---------------------------------------------------------------------------
// Empty batch terminates ingestion
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, EmptyBatchTerminates) {
    DatabaseConnector conn;
    conn.initialize(makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                                 "", "body"));
    conn.setRowFetchForTesting([]() -> std::vector<DatabaseConnector::DbRow> {
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

// ---------------------------------------------------------------------------
// Progress callback
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, ProgressCallbackInvoked) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                            "", "body");
    cfg.options["batch_size"] = "1"; // callback per row
    conn.initialize(cfg);

    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        if (calls++ == 0) {
            return {makeRow({{"id","1"}, {"body","hello"}})};
        }
        return {};
    });

    std::atomic<int> cb_count{0};
    auto cb = [&](const std::string& /*src*/, size_t /*proc*/,
                  size_t /*total*/, const std::string& /*status*/) {
        ++cb_count;
    };

    conn.ingest("col", cb);
    EXPECT_GE(cb_count.load(), 1);
}

// ---------------------------------------------------------------------------
// Exception in row-fetch handled gracefully
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, ExceptionInRowFetchIsHandled) {
    DatabaseConnector conn;
    conn.initialize(makeDbConfig("jdbc:postgresql://localhost:5432/db", "docs",
                                 "", "body"));
    int calls = 0;
    conn.setRowFetchForTesting([&]() -> std::vector<DatabaseConnector::DbRow> {
        if (calls++ == 0) {
            throw std::runtime_error("simulated DB error");
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors.back().code, IngestionErrorCode::INTERNAL_ERROR);
}

// ---------------------------------------------------------------------------
// No-ODBC path: CONNECTOR_NOT_SUPPORTED without mock
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, NoOdbcWithoutMockReturnsNotSupported) {
    DatabaseConnector conn;
    conn.initialize(makeDbConfig());
    // No mock injected → production ODBC path
    // Without THEMIS_ENABLE_ODBC (not defined in test builds) should return error
#ifndef THEMIS_ENABLE_ODBC
    auto stats = conn.ingest("col", nullptr);
    bool has_not_supported = false;
    for (const auto& e : stats.errors) {
        if (e.code == IngestionErrorCode::CONNECTOR_NOT_SUPPORTED) {
            has_not_supported = true;
            break;
        }
    }
    EXPECT_TRUE(has_not_supported);
#endif
}

// ---------------------------------------------------------------------------
// IngestionBuilder::withDatabaseSource fluent API
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, BuilderRegistersSource) {
    auto mgr = IngestionBuilder("test_db_conn")
        .withDatabaseSource("my_db",
                            "jdbc:postgresql://localhost:5432/shop",
                            {{"table","products"},
                             {"text_columns","description"},
                             {"max_rows","10"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "my_db");
    EXPECT_EQ(sources[0].type, SourceType::DATABASE);
    EXPECT_EQ(sources[0].location, "jdbc:postgresql://localhost:5432/shop");
    EXPECT_EQ(sources[0].options.at("table"), "products");
    EXPECT_EQ(sources[0].options.at("text_columns"), "description");
}

TEST(DatabaseConnectorTest, BuilderDefaultPriority) {
    auto mgr = IngestionBuilder("conn")
        .withDatabaseSource("db_src",
                            "jdbc:mysql://host:3306/mydb",
                            {{"table","t"}})
        .build();
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].priority, 5);
}

TEST(DatabaseConnectorTest, BuilderCustomPriority) {
    auto mgr = IngestionBuilder("conn")
        .withDatabaseSource("db_src",
                            "jdbc:mysql://host:3306/mydb",
                            {{"table","t"}},
                            8)
        .build();
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].priority, 8);
}

// ---------------------------------------------------------------------------
// SourceType::DATABASE in sourceTypeLabel (IngestionMetricsExporter)
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, MetricsExporterHandlesDatabaseType) {
    IngestionReport report;
    IngestionStats stats;
    stats.documents_processed = 5;
    stats.documents_failed    = 1;
    stats.bytes_processed     = 1024;
    stats.elapsed_seconds     = 0.5;
    report.source_stats["my_db"] = stats;
    report.total_documents    = 5;
    report.total_failures     = 1;
    report.total_time_seconds = 0.5;

    IngestionMetricsExporter exporter;
    std::string text = exporter.exportText(report);
    EXPECT_FALSE(text.empty());
    // Exporter should not crash with DATABASE type
    EXPECT_NE(text.find("my_db"), std::string::npos);
}

// ---------------------------------------------------------------------------
// RetryConfig passthrough
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, RetryConfigPassthrough) {
    DatabaseConnector conn;
    conn.initialize(makeDbConfig());
    RetryConfig rc;
    rc.max_attempts     = 7;
    rc.initial_delay_ms = 100.0;
    // setRetryConfig must not throw
    EXPECT_NO_THROW(conn.setRetryConfig(rc));
}

// ---------------------------------------------------------------------------
// Multiple JDBC URL formats parse correctly (initialization smoke tests)
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, MySqlJdbcUrlInitializes) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:mysql://db.example.com:3306/inventory",
                            "items");
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(DatabaseConnectorTest, SqlServerJdbcUrlInitializes) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig(
        "jdbc:sqlserver://host:1433;databaseName=sales",
        "", "SELECT * FROM orders");
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(DatabaseConnectorTest, MinimalJdbcUrlWithTable) {
    DatabaseConnector conn;
    auto cfg = makeDbConfig("jdbc:postgresql://localhost/mydb", "tbl");
    EXPECT_TRUE(conn.initialize(cfg));
}

// ---------------------------------------------------------------------------
// Security: credentials must not appear in error messages
// ---------------------------------------------------------------------------

TEST(DatabaseConnectorTest, CredentialsAbsentFromErrors) {
    // When ingest() fails (no ODBC, no mock) the error message must not
    // contain the password that was passed in the source config.
#ifndef THEMIS_ENABLE_ODBC
    static const std::string kPassword = "supersecret123";
    DatabaseConnector conn;
    SourceConfig cfg;
    cfg.source_id          = "sec_test";
    cfg.type               = SourceType::DATABASE;
    cfg.location           = "jdbc:postgresql://localhost:5432/db";
    cfg.options["table"]   = "data";
    cfg.options["username"] = "reader";
    cfg.options["password"] = kPassword;
    conn.initialize(cfg);

    auto stats = conn.ingest("col", nullptr);
    for (const auto& err : stats.errors) {
        EXPECT_EQ(err.message.find(kPassword), std::string::npos)
            << "Password leaked in error message: " << err.message;
        EXPECT_EQ(err.details.find(kPassword), std::string::npos)
            << "Password leaked in error details: " << err.details;
    }
#endif
}
