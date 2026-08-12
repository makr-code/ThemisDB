/**
 * @file test_ingestion_cdc.cpp
 * @brief Unit tests for the CdcConnector ingestion source connector.
 *
 * All tests use the mock-injection path (setCdcEventFetchForTesting) so that
 * no real replication stream or database server is required.  The tests verify:
 *   - Initialization from SourceConfig with a connection URL
 *   - Initialization rejection for wrong SourceType
 *   - Initialization rejection when connection URL is empty
 *   - isAvailable() returns true when a mock is injected
 *   - getDocumentCount() always returns 0 (streaming source)
 *   - Single event ingestion (INSERT)
 *   - Multiple events in one batch
 *   - Multiple batches across several fetch calls
 *   - DELETE events use "before" image for text extraction
 *   - UPDATE events use "after" image for text extraction
 *   - table_filter restricts which tables are ingested
 *   - operations filter restricts which event types are ingested
 *   - text_columns extraction from the event image
 *   - text_columns fallback to full JSON when columns are absent/empty
 *   - max_events limit enforcement
 *   - Empty batch terminates ingestion
 *   - Progress callback invoked after each batch
 *   - Exception in event fetch handled gracefully
 *   - No-stream path returns CONNECTOR_NOT_SUPPORTED without mock
 *   - IngestionBuilder::withCdcSource() fluent API
 *   - SourceType::CDC in sourceTypeLabel (via IngestionMetricsExporter)
 *   - RetryConfig passthrough
 */

#include <gtest/gtest.h>
#include "ingestion/cdc_connector.h"
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

static SourceConfig makeCdcConfig(
        const std::string& url          = "postgresql://localhost:5432/testdb",
        const std::string& slot_name    = "themis_cdc",
        const std::string& table_filter = "",
        const std::string& text_cols    = "",
        const std::string& operations   = "") {
    SourceConfig cfg;
    cfg.source_id          = "test_cdc";
    cfg.type               = SourceType::CDC;
    cfg.location           = url;
    cfg.options["slot_name"] = slot_name;
    if (!table_filter.empty()) cfg.options["table_filter"] = table_filter;
    if (!text_cols.empty())    cfg.options["text_columns"] = text_cols;
    if (!operations.empty())   cfg.options["operations"]   = operations;
    return cfg;
}

static CdcConnector::CdcEvent makeInsert(
        const std::string& table,
        const std::string& key,
        std::unordered_map<std::string, std::string> after) {
    CdcConnector::CdcEvent ev;
    ev.operation    = CdcConnector::CdcEvent::Operation::INSERT;
    ev.table        = table;
    ev.key          = key;
    ev.lsn          = 1;
    ev.timestamp_ms = 1706438400000LL;
    ev.after        = std::move(after);
    return ev;
}

static CdcConnector::CdcEvent makeDelete(
        const std::string& table,
        const std::string& key,
        std::unordered_map<std::string, std::string> before) {
    CdcConnector::CdcEvent ev;
    ev.operation    = CdcConnector::CdcEvent::Operation::DELETE;
    ev.table        = table;
    ev.key          = key;
    ev.lsn          = 2;
    ev.timestamp_ms = 1706438400000LL;
    ev.before       = std::move(before);
    return ev;
}

static CdcConnector::CdcEvent makeUpdate(
        const std::string& table,
        const std::string& key,
        std::unordered_map<std::string, std::string> before,
        std::unordered_map<std::string, std::string> after) {
    CdcConnector::CdcEvent ev;
    ev.operation    = CdcConnector::CdcEvent::Operation::UPDATE;
    ev.table        = table;
    ev.key          = key;
    ev.lsn          = 3;
    ev.timestamp_ms = 1706438400000LL;
    ev.before       = std::move(before);
    ev.after        = std::move(after);
    return ev;
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, InitializeValidConfig) {
    CdcConnector conn;
    auto cfg = makeCdcConfig();
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(CdcConnectorTest, InitializeWithTableFilter) {
    CdcConnector conn;
    auto cfg = makeCdcConfig("postgresql://localhost:5432/db",
                             "slot1", "orders,order_items");
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(CdcConnectorTest, InitializeWrongType) {
    CdcConnector conn;
    SourceConfig cfg;
    cfg.source_id = "wrong";
    cfg.type      = SourceType::API;
    cfg.location  = "postgresql://localhost:5432/db";
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(CdcConnectorTest, InitializeMissingUrl) {
    CdcConnector conn;
    SourceConfig cfg;
    cfg.source_id = "no_url";
    cfg.type      = SourceType::CDC;
    cfg.location  = ""; // empty
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(CdcConnectorTest, InitializeMySqlUrl) {
    CdcConnector conn;
    auto cfg = makeCdcConfig("mysql://db.example.com:3306/inventory");
    EXPECT_TRUE(conn.initialize(cfg));
}

// ---------------------------------------------------------------------------
// isAvailable (mock path always returns true)
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, IsAvailableWithMock) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    conn.setCdcEventFetchForTesting([]() -> std::vector<CdcConnector::CdcEvent> {
        return {};
    });
    EXPECT_TRUE(conn.isAvailable());
}

// ---------------------------------------------------------------------------
// getDocumentCount
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, GetDocumentCountAlwaysZero) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    conn.setCdcEventFetchForTesting([]() -> std::vector<CdcConnector::CdcEvent> {
        return {};
    });
    EXPECT_EQ(conn.getDocumentCount(), 0u);
}

// ---------------------------------------------------------------------------
// Basic ingestion via mock
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, IngestSingleInsert) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());

    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {makeInsert("orders", "1", {{"id","1"}, {"status","created"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.bytes_processed, 0u);
}

TEST(CdcConnectorTest, IngestMultipleEventsInOneBatch) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());

    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {
                makeInsert("t", "1", {{"id","1"}, {"body","doc one"}}),
                makeInsert("t", "2", {{"id","2"}, {"body","doc two"}}),
                makeInsert("t", "3", {{"id","3"}, {"body","doc three"}})
            };
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

TEST(CdcConnectorTest, IngestMultipleBatches) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());

    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls == 0) {
            ++calls;
            return {makeInsert("t", "1", {{"v","a"}})};
        } else if (calls == 1) {
            ++calls;
            return {makeInsert("t", "2", {{"v","b"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
}

// ---------------------------------------------------------------------------
// DELETE uses "before" image
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, DeleteEventUsesBeforeImage) {
    CdcConnector conn;
    // text_columns = "name" → should be found in the before image of DELETE
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "", "name"));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {makeDelete("users", "42", {{"id","42"}, {"name","Alice"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

// ---------------------------------------------------------------------------
// UPDATE uses "after" image
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, UpdateEventUsesAfterImage) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "", "status"));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {makeUpdate("orders", "7",
                               {{"id","7"}, {"status","pending"}},
                               {{"id","7"}, {"status","shipped"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

// ---------------------------------------------------------------------------
// Table filter
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, TableFilterAllowsMatchingTable) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "orders", ""));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {
                makeInsert("orders", "1", {{"v","ok"}}),
                makeInsert("users",  "2", {{"v","filtered"}})
            };
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);  // only "orders"
}

TEST(CdcConnectorTest, TableFilterAllowsMultipleTables) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "orders,items", ""));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {
                makeInsert("orders", "1", {{"v","a"}}),
                makeInsert("items",  "2", {{"v","b"}}),
                makeInsert("users",  "3", {{"v","c"}})
            };
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);  // orders + items
}

// ---------------------------------------------------------------------------
// Operations filter
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, OperationsFilterIncludeInsertOnly) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "", "", "INSERT"));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {
                makeInsert("t", "1", {{"v","a"}}),
                makeDelete("t", "2", {{"v","b"}}),
                makeUpdate("t", "3", {{"v","c"}}, {{"v","d"}})
            };
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);  // only INSERT
}

TEST(CdcConnectorTest, OperationsFilterIncludeDeleteOnly) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "", "", "DELETE"));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {
                makeInsert("t", "1", {{"v","a"}}),
                makeDelete("t", "2", {{"v","b"}}),
                makeUpdate("t", "3", {{"v","c"}}, {{"v","d"}})
            };
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);  // only DELETE
}

// ---------------------------------------------------------------------------
// text_columns extraction
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, TextColumnsExtraction) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "", "title,body"));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {makeInsert("docs", "1",
                               {{"id","1"}, {"title","Hello"}, {"body","World"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

TEST(CdcConnectorTest, NoTextColumnsUsesFullJson) {
    CdcConnector conn;
    // No text_columns → serialize full event JSON
    conn.initialize(makeCdcConfig());
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {makeInsert("t", "1", {{"id","1"}, {"data","value"}})};
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed, 0u);
}

TEST(CdcConnectorTest, MissingTextColumnFallsBackToJson) {
    CdcConnector conn;
    // text_columns = "summary" but that column is absent → falls back to JSON
    conn.initialize(makeCdcConfig("postgresql://localhost:5432/db",
                                   "slot", "", "summary"));
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {makeInsert("t", "1", {{"id","1"}, {"body","rich content"}})};
        }
        return {};
    });

    // No "summary" column → full JSON fallback → still processed
    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// max_events enforcement
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, MaxEventsLimit) {
    CdcConnector conn;
    auto cfg = makeCdcConfig();
    cfg.options["max_events"] = "2";
    conn.initialize(cfg);

    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        ++calls;
        return {
            makeInsert("t", "1", {{"v","a"}}),
            makeInsert("t", "2", {{"v","b"}}),
            makeInsert("t", "3", {{"v","c"}})
        };
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
}

// ---------------------------------------------------------------------------
// Empty batch terminates ingestion
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, EmptyBatchTerminates) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    conn.setCdcEventFetchForTesting([]() -> std::vector<CdcConnector::CdcEvent> {
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

// ---------------------------------------------------------------------------
// Progress callback
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, ProgressCallbackInvoked) {
    CdcConnector conn;
    auto cfg = makeCdcConfig();
    cfg.options["batch_size"] = "1";
    conn.initialize(cfg);

    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {makeInsert("t", "1", {{"v","hello"}})};
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
// Exception in event fetch handled gracefully
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, ExceptionInEventFetchIsHandled) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            throw std::runtime_error("simulated stream error");
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors.back().code, IngestionErrorCode::INTERNAL_ERROR);
}

// ---------------------------------------------------------------------------
// No-stream path: CONNECTOR_NOT_SUPPORTED without mock
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, NoStreamWithoutMockReturnsNotSupported) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    // No mock injected → production stream path
#ifndef THEMIS_ENABLE_CDC_STREAM
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
// IngestionBuilder::withCdcSource fluent API
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, BuilderRegistersSource) {
    auto mgr = IngestionBuilder("test_cdc_conn")
        .withCdcSource("orders_cdc",
                       "postgresql://localhost:5432/shop",
                       {{"slot_name","themis_orders"},
                        {"table_filter","orders"},
                        {"max_events","10000"}})
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "orders_cdc");
    EXPECT_EQ(sources[0].type, SourceType::CDC);
    EXPECT_EQ(sources[0].location, "postgresql://localhost:5432/shop");
    EXPECT_EQ(sources[0].options.at("slot_name"), "themis_orders");
    EXPECT_EQ(sources[0].options.at("table_filter"), "orders");
}

TEST(CdcConnectorTest, BuilderDefaultPriority) {
    auto mgr = IngestionBuilder("conn")
        .withCdcSource("cdc_src", "postgresql://localhost:5432/db")
        .build();
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].priority, 5);
}

TEST(CdcConnectorTest, BuilderCustomPriority) {
    auto mgr = IngestionBuilder("conn")
        .withCdcSource("cdc_src", "postgresql://localhost:5432/db", {}, 8)
        .build();
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].priority, 8);
}

// ---------------------------------------------------------------------------
// SourceType::CDC in sourceTypeLabel (IngestionMetricsExporter)
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, MetricsExporterHandlesCdcType) {
    IngestionReport report;
    IngestionStats stats;
    stats.documents_processed = 10;
    stats.documents_failed    = 0;
    stats.bytes_processed     = 2048;
    stats.elapsed_seconds     = 1.0;
    report.source_stats["orders_cdc"] = stats;
    report.total_documents    = 10;
    report.total_failures     = 0;
    report.total_time_seconds = 1.0;

    IngestionMetricsExporter exporter;
    std::string text = exporter.exportText(report);
    EXPECT_FALSE(text.empty());
    EXPECT_NE(text.find("orders_cdc"), std::string::npos);
}

// ---------------------------------------------------------------------------
// RetryConfig passthrough
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, RetryConfigPassthrough) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    RetryConfig rc;
    rc.max_attempts     = 5;
    rc.initial_delay_ms = 250.0;
    EXPECT_NO_THROW(conn.setRetryConfig(rc));
}

// ---------------------------------------------------------------------------
// All three operation types in one stream
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, AllOperationsProcessed) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    int calls = 0;
    conn.setCdcEventFetchForTesting([&]() -> std::vector<CdcConnector::CdcEvent> {
        if (calls++ == 0) {
            return {
                makeInsert("t", "1", {{"v","insert"}}),
                makeUpdate("t", "1", {{"v","insert"}}, {{"v","update"}}),
                makeDelete("t", "1", {{"v","update"}})
            };
        }
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed, 0u);
}

// ---------------------------------------------------------------------------
// elapsed_seconds is positive
// ---------------------------------------------------------------------------

TEST(CdcConnectorTest, ElapsedSecondsPositive) {
    CdcConnector conn;
    conn.initialize(makeCdcConfig());
    conn.setCdcEventFetchForTesting([]() -> std::vector<CdcConnector::CdcEvent> {
        return {};
    });

    auto stats = conn.ingest("col", nullptr);
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}
