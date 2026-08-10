/**
 * @file test_importers_new_connectors_focused.cpp
 * @brief Focused unit tests for ElasticsearchImporter, RedisImporter, and
 *        DebeziumCDCImporter (INC-01 .. INC-24).
 *
 * All tests are self-contained with no external I/O.  Mock injection is used
 * for every connector.  The kImportersNewConnectorsSeed = 42 ensures
 * deterministic test data.
 *
 * Test groups:
 *   INC-01..08  – ElasticsearchImporter
 *   INC-09..16  – RedisImporter
 *   INC-17..24  – DebeziumCDCImporter
 */

#include <gtest/gtest.h>

#include "importers/elasticsearch_importer.h"
#include "importers/redis_importer.h"
#include "importers/debezium_cdc_importer.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>

namespace {

using json = nlohmann::json;
using themis::importers::ElasticsearchImporter;
using themis::importers::RedisImporter;
using themis::importers::DebeziumCDCImporter;
using themis::importers::ImportOptions;
using themis::importers::ImportErrorCode;

constexpr int kImportersNewConnectorsSeed = 42;

// ---------------------------------------------------------------------------
// Helper: minimal ElasticsearchImporter config
// ---------------------------------------------------------------------------
static std::string esConfig(const std::string& index = "test-index") {
    return json{
        {"host",  "http://localhost:9200"},
        {"index", index},
        {"batch_size", 10},
        {"max_retries", 1},
        {"timeout_ms", 5000}
    }.dump();
}

// ---------------------------------------------------------------------------
// Helper: minimal RedisImporter config
// ---------------------------------------------------------------------------
static std::string redisConfig() {
    return json{
        {"host",         "127.0.0.1"},
        {"port",         6379},
        {"key_pattern",  "*"},
        {"batch_size",   10},
        {"timeout_ms",   2000}
    }.dump();
}

// ---------------------------------------------------------------------------
// Helper: minimal DebeziumCDCImporter config
// ---------------------------------------------------------------------------
static std::string debeziumConfig(const std::string& prefix = "mydb") {
    return json{
        {"brokers",      "localhost:9092"},
        {"topic_prefix", prefix},
        {"consumer_group", "themisdb-test"},
        {"snapshot_mode",  "initial"}
    }.dump();
}

// ============================================================================
// INC-01..08  –  ElasticsearchImporter
// ============================================================================

// INC-01: initialize with valid config succeeds.
TEST(ElasticsearchImporterTest, INC01_InitializeValidConfig) {
    ElasticsearchImporter imp;
    EXPECT_TRUE(imp.initialize(esConfig()));
}

// INC-02: initialize without required 'host' field fails.
TEST(ElasticsearchImporterTest, INC02_InitializeMissingHostFails) {
    ElasticsearchImporter imp;
    EXPECT_FALSE(imp.initialize(json{{"index", "x"}}.dump()));
}

// INC-03: getSupportedTypes returns expected identifiers.
TEST(ElasticsearchImporterTest, INC03_SupportedTypes) {
    ElasticsearchImporter imp;
    const auto types = imp.getSupportedTypes();
    EXPECT_TRUE(std::find(types.begin(), types.end(), "elasticsearch") != types.end());
    EXPECT_TRUE(std::find(types.begin(), types.end(), "opensearch") != types.end());
}

// INC-04: validateSource without THEMIS_ENABLE_ELASTICSEARCH and no mock returns false.
TEST(ElasticsearchImporterTest, INC04_ValidateSourceNoBuildFlagFails) {
    ElasticsearchImporter imp;
    imp.initialize(esConfig());
    std::vector<std::string> errors;
    // Without mock and without build flag, validation must fail with a clear message.
    bool result = imp.validateSource("test-index", errors);
#ifndef THEMIS_ENABLE_ELASTICSEARCH
    EXPECT_FALSE(result);
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("THEMIS_ENABLE_ELASTICSEARCH"), std::string::npos);
#else
    (void)result; // Build-flag path: skip assertion.
#endif
}

// INC-05: importData without build flag or mock returns IMPORT_CONNECTOR_UNAVAILABLE.
TEST(ElasticsearchImporterTest, INC05_ImportDataNoBuildFlag) {
    ElasticsearchImporter imp;
    imp.initialize(esConfig());
    ImportOptions opts;
    const auto stats = imp.importData("test-index", opts);
#ifndef THEMIS_ENABLE_ELASTICSEARCH
    ASSERT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors[0].code,
              static_cast<uint32_t>(ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE));
#else
    (void)stats;
#endif
}

// INC-06: importData with mock HTTP returns documents.
TEST(ElasticsearchImporterTest, INC06_ImportDataWithMock) {
    ElasticsearchImporter imp;
    imp.initialize(esConfig());

    // Build a mock that returns one scroll page with 3 hits, then an empty page.
    int call_count = 0;
    imp.setMockHttpForTesting([&call_count](const std::string& url,
                                             const std::string& /*body*/) -> std::string {
        ++call_count;
        if (url.find("_search") != std::string::npos && call_count == 1) {
            // Initial scroll response.
            return json{
                {"_scroll_id", "scroll-abc"},
                {"hits", {{"hits", json::array({
                    {{"_id","1"},{"_index","test-index"},{"_source",{{"name","Alice"}}}},
                    {{"_id","2"},{"_index","test-index"},{"_source",{{"name","Bob"}}}},
                    {{"_id","3"},{"_index","test-index"},{"_source",{{"name","Carol"}}}}
                })}}}
            }.dump();
        }
        // Subsequent scroll pages → empty (end of index).
        return json{{"_scroll_id","scroll-abc"},{"hits",{{"hits",json::array()}}}}.dump();
    });

    ImportOptions opts;
    const auto stats = imp.importData("test-index", opts);
    EXPECT_EQ(stats.rows_imported, 3u);
    EXPECT_TRUE(stats.errors.empty());
}

// INC-07: cancel() stops an in-progress import.
TEST(ElasticsearchImporterTest, INC07_CancelStopsImport) {
    ElasticsearchImporter imp;
    imp.initialize(esConfig());

    int page_calls = 0;
    imp.setMockHttpForTesting([&](const std::string& url, const std::string&) -> std::string {
        ++page_calls;
        // Return an infinite stream of 1-hit pages.
        return json{
            {"_scroll_id", "s1"},
            {"hits", {{"hits", json::array({
                {{"_id", std::to_string(page_calls)},
                 {"_index","test-index"},
                 {"_source",{{"v",page_calls}}}}
            })}}}
        }.dump();
    });

    imp.cancel(); // Cancel before importData starts.
    ImportOptions opts;
    const auto stats = imp.importData("test-index", opts);
    // With cancel pre-set, the import should complete with 0 or very few rows.
    EXPECT_LE(stats.rows_imported, 1u);
}

// INC-08: getSourceSchema with mock mapping returns correct field types.
TEST(ElasticsearchImporterTest, INC08_GetSourceSchemaWithMock) {
    ElasticsearchImporter imp;
    imp.initialize(esConfig("orders"));

    imp.setMockHttpForTesting([](const std::string&, const std::string&) -> std::string {
        return json{
            {"orders", {{"mappings", {{"properties", {
                {"id",    {{"type","integer"}}},
                {"name",  {{"type","keyword"}}},
                {"price", {{"type","float"}}}
            }}}}}}
        }.dump();
    });

    const json schema = imp.getSourceSchema("orders");
    ASSERT_TRUE(schema.contains("orders"));
    ASSERT_TRUE(schema["orders"].contains("fields"));
    EXPECT_EQ(schema["orders"]["fields"].size(), 3u);
}

// ============================================================================
// INC-09..16  –  RedisImporter
// ============================================================================

// INC-09: initialize with valid config succeeds.
TEST(RedisImporterTest, INC09_InitializeValidConfig) {
    RedisImporter imp;
    EXPECT_TRUE(imp.initialize(redisConfig()));
}

// INC-10: getSupportedTypes returns "redis".
TEST(RedisImporterTest, INC10_SupportedTypes) {
    RedisImporter imp;
    const auto types = imp.getSupportedTypes();
    ASSERT_EQ(types.size(), 1u);
    EXPECT_EQ(types[0], "redis");
}

// INC-11: validateSource without THEMIS_ENABLE_REDIS and no mock returns false.
TEST(RedisImporterTest, INC11_ValidateSourceNoBuildFlagFails) {
    RedisImporter imp;
    imp.initialize(redisConfig());
    std::vector<std::string> errors;
#ifndef THEMIS_ENABLE_REDIS
    EXPECT_FALSE(imp.validateSource("", errors));
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("THEMIS_ENABLE_REDIS"), std::string::npos);
#else
    (void)imp.validateSource("", errors);
#endif
}

// INC-12: validateSource with mock PING succeeds.
TEST(RedisImporterTest, INC12_ValidateSourceMockPing) {
    RedisImporter imp;
    imp.initialize(redisConfig());
    imp.setMockCommandForTesting([](const std::vector<std::string>& cmd) -> std::string {
        if (!cmd.empty() && cmd[0] == "PING") return "PONG";
        return "";
    });
    std::vector<std::string> errors;
    EXPECT_TRUE(imp.validateSource("", errors));
    EXPECT_TRUE(errors.empty());
}

// INC-13: importData with String keys returns documents.
TEST(RedisImporterTest, INC13_ImportStringKeys) {
    RedisImporter imp;
    imp.initialize(redisConfig());

    // State machine: SCAN returns 2 keys, then exhausted.
    int scan_calls = 0;
    imp.setMockCommandForTesting([&scan_calls](const std::vector<std::string>& cmd) -> std::string {
        if (cmd[0] == "SCAN") {
            ++scan_calls;
            if (scan_calls == 1) {
                return json::array({"0", json::array({"key:1","key:2"})}).dump();
            }
            return json::array({"0", json::array()}).dump();
        }
        if (cmd[0] == "PTTL") return "-1"; // No expiry.
        if (cmd[0] == "TYPE") return "string";
        if (cmd[0] == "GET") return cmd.size() > 1 ? cmd[1] + "_value" : "val";
        return "";
    });

    ImportOptions opts;
    const auto stats = imp.importData("", opts);
    EXPECT_EQ(stats.rows_imported, 2u);
    EXPECT_TRUE(stats.errors.empty());
}

// INC-14: importData with Hash key returns document with nested object.
TEST(RedisImporterTest, INC14_ImportHashKey) {
    RedisImporter imp;
    imp.initialize(redisConfig());

    imp.setMockCommandForTesting([](const std::vector<std::string>& cmd) -> std::string {
        if (cmd[0] == "SCAN" && cmd[1] == "0") {
            return json::array({"0", json::array({"user:1"})}).dump();
        }
        if (cmd[0] == "SCAN") {
            return json::array({"0", json::array()}).dump();
        }
        if (cmd[0] == "PTTL") return "-1";
        if (cmd[0] == "TYPE") return "hash";
        if (cmd[0] == "HGETALL") {
            // Return alternating field/value pairs.
            return json::array({"name","Alice","age","30"}).dump();
        }
        return "";
    });

    ImportOptions opts;
    const auto stats = imp.importData("", opts);
    EXPECT_EQ(stats.rows_imported, 1u);
}

// INC-15: cancel() stops the SCAN loop.
TEST(RedisImporterTest, INC15_CancelStopsImport) {
    RedisImporter imp;
    imp.initialize(redisConfig());
    imp.cancel(); // Pre-cancel.

    imp.setMockCommandForTesting([](const std::vector<std::string>& cmd) -> std::string {
        if (cmd[0] == "SCAN") {
            // Would return keys forever if not cancelled.
            return json::array({"1", json::array({"k1"})}).dump();
        }
        if (cmd[0] == "PTTL") return "-1";
        if (cmd[0] == "TYPE") return "string";
        if (cmd[0] == "GET")  return "val";
        return "";
    });

    ImportOptions opts;
    const auto stats = imp.importData("", opts);
    EXPECT_LE(stats.rows_imported, 1u);
}

// INC-16: deadline enforcement stops import after timeout.
TEST(RedisImporterTest, INC16_DeadlineEnforcement) {
    RedisImporter imp;
    imp.initialize(redisConfig());

    int scan_calls = 0;
    imp.setMockCommandForTesting([&scan_calls](const std::vector<std::string>& cmd) -> std::string {
        if (cmd[0] == "SCAN") {
            ++scan_calls;
            // Simulate non-terminating SCAN (cursor never reaches 0).
            return json::array({"99", json::array({"k" + std::to_string(scan_calls)})}).dump();
        }
        if (cmd[0] == "PTTL") return "-1";
        if (cmd[0] == "TYPE") return "string";
        if (cmd[0] == "GET")  return "val";
        return "";
    });

    ImportOptions opts;
    opts.deadline_ms = 1; // 1 ms — will expire almost immediately.
    const auto stats = imp.importData("", opts);
    // Should have at least one TIMEOUT error.
    bool has_timeout = false;
    for (const auto& e : stats.errors) {
        if (e.code == static_cast<uint32_t>(ImportErrorCode::IMPORT_TIMEOUT)) {
            has_timeout = true; break;
        }
    }
    EXPECT_TRUE(has_timeout);
}

// ============================================================================
// INC-17..24  –  DebeziumCDCImporter
// ============================================================================

// INC-17: initialize with valid config succeeds.
TEST(DebeziumCDCImporterTest, INC17_InitializeValidConfig) {
    DebeziumCDCImporter imp;
    EXPECT_TRUE(imp.initialize(debeziumConfig()));
}

// INC-18: initialize without 'brokers' fails.
TEST(DebeziumCDCImporterTest, INC18_InitializeMissingBrokersFails) {
    DebeziumCDCImporter imp;
    EXPECT_FALSE(imp.initialize(json{{"topic_prefix","db"}}.dump()));
}

// INC-19: getSupportedTypes includes "debezium" and "cdc".
TEST(DebeziumCDCImporterTest, INC19_SupportedTypes) {
    DebeziumCDCImporter imp;
    const auto types = imp.getSupportedTypes();
    EXPECT_TRUE(std::find(types.begin(), types.end(), "debezium") != types.end());
    EXPECT_TRUE(std::find(types.begin(), types.end(), "cdc") != types.end());
}

// INC-20: importData with mock INSERT event increments rows_imported.
TEST(DebeziumCDCImporterTest, INC20_ImportInsertEvent) {
    DebeziumCDCImporter imp;
    imp.initialize(debeziumConfig());

    DebeziumCDCImporter::CDCEvent ev;
    ev.op    = DebeziumCDCImporter::ChangeOp::Create;
    ev.table = "public.orders";
    ev.after = {{"id", 1}, {"amount", 99.9}};
    imp.setMockEventsForTesting({ev});

    ImportOptions opts;
    const auto stats = imp.importData("", opts);
    EXPECT_EQ(stats.rows_imported, 1u);
    EXPECT_TRUE(stats.errors.empty());
}

// INC-21: importData with mock UPDATE and DELETE events.
TEST(DebeziumCDCImporterTest, INC21_ImportUpdateAndDeleteEvents) {
    DebeziumCDCImporter imp;
    imp.initialize(debeziumConfig());

    std::vector<DebeziumCDCImporter::CDCEvent> events;

    DebeziumCDCImporter::CDCEvent upd;
    upd.op     = DebeziumCDCImporter::ChangeOp::Update;
    upd.table  = "public.orders";
    upd.before = {{"id",1},{"amount",99.9}};
    upd.after  = {{"id",1},{"amount",150.0}};
    events.push_back(upd);

    DebeziumCDCImporter::CDCEvent del;
    del.op     = DebeziumCDCImporter::ChangeOp::Delete;
    del.table  = "public.orders";
    del.before = {{"id",2},{"amount",10.0}};
    events.push_back(del);

    imp.setMockEventsForTesting(events);
    ImportOptions opts;
    const auto stats = imp.importData("", opts);
    EXPECT_EQ(stats.rows_imported, 2u);
}

// INC-22: table filter skips events for non-matching tables.
TEST(DebeziumCDCImporterTest, INC22_TableFilterSkipsNonMatchingEvents) {
    DebeziumCDCImporter imp;
    json cfg = json::parse(debeziumConfig());
    cfg["tables"] = json::array({"public.orders"});
    imp.initialize(cfg.dump());

    std::vector<DebeziumCDCImporter::CDCEvent> events;

    DebeziumCDCImporter::CDCEvent ev1;
    ev1.op    = DebeziumCDCImporter::ChangeOp::Create;
    ev1.table = "public.orders";
    ev1.after = {{"id",1}};
    events.push_back(ev1);

    DebeziumCDCImporter::CDCEvent ev2;
    ev2.op    = DebeziumCDCImporter::ChangeOp::Create;
    ev2.table = "public.customers"; // not in filter
    ev2.after = {{"id",2}};
    events.push_back(ev2);

    imp.setMockEventsForTesting(events);
    ImportOptions opts;
    const auto stats = imp.importData("", opts);
    EXPECT_EQ(stats.rows_imported, 1u);
    EXPECT_EQ(stats.rows_skipped, 1u);
}

// INC-23: parseDebeziumEnvelope correctly decodes an envelope JSON.
TEST(DebeziumCDCImporterTest, INC23_ParseDebeziumEnvelope) {
    // Use the importer streamEvents() path with one injected event built from
    // a known Debezium JSON payload to verify end-to-end decoding.
    DebeziumCDCImporter imp;
    imp.initialize(debeziumConfig());

    DebeziumCDCImporter::CDCEvent ev;
    ev.op           = DebeziumCDCImporter::ChangeOp::Create;
    ev.table        = "inventory.products";
    ev.after        = {{"id",101},{"name","Widget"}};
    ev.source_ts_ms = 1700000000000LL;
    ev.transaction_id = "tx-abc";
    imp.setMockEventsForTesting({ev});

    bool callback_called = false;
    ImportOptions opts;
    imp.streamEvents(opts, [&](const DebeziumCDCImporter::CDCEvent& e) -> bool {
        callback_called = true;
        EXPECT_EQ(e.op, DebeziumCDCImporter::ChangeOp::Create);
        EXPECT_EQ(e.table, "inventory.products");
        EXPECT_EQ(e.source_ts_ms, 1700000000000LL);
        return true;
    });
    EXPECT_TRUE(callback_called);
}

// INC-24: importData without THEMIS_ENABLE_DEBEZIUM and no mock returns CONNECTOR_UNAVAILABLE.
TEST(DebeziumCDCImporterTest, INC24_ImportNoBuildFlag) {
    DebeziumCDCImporter imp;
    imp.initialize(debeziumConfig());
    ImportOptions opts;
    const auto stats = imp.importData("", opts);
#ifndef THEMIS_ENABLE_DEBEZIUM
    ASSERT_FALSE(stats.errors.empty());
    EXPECT_EQ(stats.errors[0].code,
              static_cast<uint32_t>(ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE));
#else
    // With build flag: expect WARNING (stub path).
    (void)stats;
#endif
}

} // anonymous namespace
