/*
 * Tests for Debezium-compatible change event envelope format.
 *
 * Validates that DebeziumFormatter:
 *  1. Maps EVENT_PUT (no before_snapshot)  → op 'c' (CREATE)
 *  2. Maps EVENT_PUT (with before_snapshot) → op 'u' (UPDATE)
 *  3. Maps EVENT_DELETE                     → op 'd' (DELETE)
 *  4. Maps EVENT_TRANSACTION_COMMIT         → op 'r' (READ)
 *  5. Populates before/after JSON from snapshot fields
 *  6. Falls back to the 'value' field for 'after' when after_snapshot absent
 *  7. Derives the collection name from the event key prefix
 *  8. Accepts an explicit collection name override
 *  9. Serializes source metadata correctly
 * 10. Produces a schema block when include_schema = true
 * 11. Handles invalid JSON in snapshot fields gracefully (_raw fallback)
 * 12. Preserves the redacted state in the source.snapshot field
 */

#include <gtest/gtest.h>
#include "cdc/debezium_format.h"
#include "cdc/changefeed.h"

using namespace themis;
using namespace themis::cdc;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

Changefeed::ChangeEvent makePutEvent(
    const std::string& key,
    const std::string& value,
    std::optional<std::string> before_snap = std::nullopt,
    std::optional<std::string> after_snap  = std::nullopt)
{
    Changefeed::ChangeEvent ev;
    ev.sequence        = 1;
    ev.type            = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key             = key;
    ev.value           = value;
    ev.timestamp_ms    = 1740000000000LL;
    ev.before_snapshot = std::move(before_snap);
    ev.after_snapshot  = std::move(after_snap);
    return ev;
}

Changefeed::ChangeEvent makeDeleteEvent(
    const std::string& key,
    std::optional<std::string> before_snap = std::nullopt)
{
    Changefeed::ChangeEvent ev;
    ev.sequence        = 2;
    ev.type            = Changefeed::ChangeEventType::EVENT_DELETE;
    ev.key             = key;
    ev.timestamp_ms    = 1740000000001LL;
    ev.before_snapshot = std::move(before_snap);
    return ev;
}

Changefeed::ChangeEvent makeTxCommitEvent() {
    Changefeed::ChangeEvent ev;
    ev.sequence     = 3;
    ev.type         = Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT;
    ev.key          = "orders:99";
    ev.timestamp_ms = 1740000000002LL;
    return ev;
}

}  // anonymous namespace

// ---------------------------------------------------------------------------
// Operation code mapping
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, PutWithoutBefore_MapsToCreate) {
    DebeziumFormatter fmt;
    auto env = fmt.toEnvelope(makePutEvent("orders:1", R"({"qty":5})"));
    EXPECT_EQ(env.op, DebeziumOp::CREATE);

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["op"].get<std::string>(), "c");
}

TEST(DebeziumFormatterTest, PutWithBefore_MapsToUpdate) {
    DebeziumFormatter fmt;
    const std::string before = R"({"qty":3})";
    const std::string after  = R"({"qty":5})";
    auto env = fmt.toEnvelope(
        makePutEvent("orders:1", after, before, after));
    EXPECT_EQ(env.op, DebeziumOp::UPDATE);

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["op"].get<std::string>(), "u");
}

TEST(DebeziumFormatterTest, Delete_MapsToDelete) {
    DebeziumFormatter fmt;
    const std::string before = R"({"qty":5})";
    auto env = fmt.toEnvelope(makeDeleteEvent("orders:1", before));
    EXPECT_EQ(env.op, DebeziumOp::DELETE);

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["op"].get<std::string>(), "d");
}

TEST(DebeziumFormatterTest, TransactionRollback_MapsToRead) {
    DebeziumFormatter fmt;
    Changefeed::ChangeEvent ev;
    ev.sequence     = 4;
    ev.type         = Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK;
    ev.key          = "orders:99";
    ev.timestamp_ms = 1740000000003LL;
    auto env = fmt.toEnvelope(ev);
    EXPECT_EQ(env.op, DebeziumOp::READ);

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["op"].get<std::string>(), "r");
}

TEST(DebeziumFormatterTest, TransactionCommit_MapsToRead) {
    DebeziumFormatter fmt;
    auto env = fmt.toEnvelope(makeTxCommitEvent());
    EXPECT_EQ(env.op, DebeziumOp::READ);

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["op"].get<std::string>(), "r");
}

// ---------------------------------------------------------------------------
// Before / after document fields
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, Create_NullBefore_AfterFromValue) {
    DebeziumFormatter fmt;
    const std::string value = R"({"name":"Alice"})";
    // No after_snapshot provided: formatter falls back to the value field
    auto env = fmt.toEnvelope(makePutEvent("users:1", value));

    EXPECT_TRUE(env.before.is_null());
    ASSERT_FALSE(env.after.is_null());
    EXPECT_EQ(env.after["name"].get<std::string>(), "Alice");

    auto j = env.toJson();
    EXPECT_TRUE(j["payload"]["before"].is_null());
    EXPECT_EQ(j["payload"]["after"]["name"].get<std::string>(), "Alice");
}

TEST(DebeziumFormatterTest, Update_BeforeAndAfterFromSnapshots) {
    DebeziumFormatter fmt;
    const std::string before_val = R"({"status":"pending"})";
    const std::string after_val  = R"({"status":"active"})";
    auto env = fmt.toEnvelope(
        makePutEvent("orders:42", after_val, before_val, after_val));

    ASSERT_FALSE(env.before.is_null());
    ASSERT_FALSE(env.after.is_null());
    EXPECT_EQ(env.before["status"].get<std::string>(), "pending");
    EXPECT_EQ(env.after["status"].get<std::string>(),  "active");

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["before"]["status"].get<std::string>(), "pending");
    EXPECT_EQ(j["payload"]["after"]["status"].get<std::string>(),  "active");
}

TEST(DebeziumFormatterTest, Delete_BeforeFromSnapshot_AfterIsNull) {
    DebeziumFormatter fmt;
    const std::string before_val = R"({"qty":10})";
    auto env = fmt.toEnvelope(makeDeleteEvent("inventory:SKU-01", before_val));

    ASSERT_FALSE(env.before.is_null());
    EXPECT_EQ(env.before["qty"].get<int>(), 10);
    EXPECT_TRUE(env.after.is_null());

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["before"]["qty"].get<int>(), 10);
    EXPECT_TRUE(j["payload"]["after"].is_null());
}

// ---------------------------------------------------------------------------
// Collection / table name derivation
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, CollectionDerivedFromKeyPrefix) {
    DebeziumFormatter fmt;
    auto env = fmt.toEnvelope(makePutEvent("orders:42", R"({})"));
    EXPECT_EQ(env.source.table, "orders");
}

TEST(DebeziumFormatterTest, ExplicitCollectionOverridesKeyPrefix) {
    DebeziumFormatter fmt;
    auto env = fmt.toEnvelope(makePutEvent("orders:42", R"({})"), "purchase_orders");
    EXPECT_EQ(env.source.table, "purchase_orders");
}

TEST(DebeziumFormatterTest, KeyWithoutColonUsesFullKeyAsCollection) {
    DebeziumFormatter fmt;
    auto env = fmt.toEnvelope(makePutEvent("globalconfig", R"({})"));
    EXPECT_EQ(env.source.table, "globalconfig");
}

// ---------------------------------------------------------------------------
// Source metadata
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, SourceBlock_DefaultConfig) {
    DebeziumFormatter fmt;
    auto ev  = makePutEvent("orders:1", R"({})");
    ev.sequence = 77;
    auto env = fmt.toEnvelope(ev);

    EXPECT_EQ(env.source.connector, "themisdb");
    EXPECT_EQ(env.source.name,      "themis");
    EXPECT_EQ(env.source.db,        "themisdb");
    EXPECT_EQ(env.source.table,     "orders");
    EXPECT_EQ(env.source.ts_ms,     ev.timestamp_ms);
    EXPECT_EQ(env.source.sequence,  77ULL);
    EXPECT_EQ(env.source.snapshot,  "false");
}

TEST(DebeziumFormatterTest, SourceBlock_CustomConfig) {
    DebeziumFormatter::Config cfg;
    cfg.server_name = "prod-cluster";
    cfg.db_name     = "myapp";
    cfg.version     = "2.0.0";
    DebeziumFormatter fmt(cfg);

    auto env = fmt.toEnvelope(makePutEvent("users:1", R"({})"));
    EXPECT_EQ(env.source.name,      "prod-cluster");
    EXPECT_EQ(env.source.db,        "myapp");
    EXPECT_EQ(env.source.version,   "2.0.0");
}

TEST(DebeziumFormatterTest, SourceBlock_SerializesToJson) {
    DebeziumFormatter fmt;
    auto ev = makePutEvent("orders:1", R"({})");
    ev.sequence = 42;
    auto j = fmt.toJson(ev);

    const auto& src = j["payload"]["source"];
    EXPECT_EQ(src["connector"].get<std::string>(), "themisdb");
    EXPECT_EQ(src["table"].get<std::string>(),     "orders");
    EXPECT_EQ(src["sequence"].get<uint64_t>(),     42ULL);
    EXPECT_EQ(src["ts_ms"].get<int64_t>(),         ev.timestamp_ms);
}

// ---------------------------------------------------------------------------
// Timestamp propagation
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, TimestampPropagatedToPayloadAndSource) {
    DebeziumFormatter fmt;
    auto ev = makePutEvent("orders:1", R"({})");
    ev.timestamp_ms = 9876543210LL;
    auto j = fmt.toJson(ev);

    EXPECT_EQ(j["payload"]["ts_ms"].get<int64_t>(),          9876543210LL);
    EXPECT_EQ(j["payload"]["source"]["ts_ms"].get<int64_t>(), 9876543210LL);
}

// ---------------------------------------------------------------------------
// Schema block
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, PayloadOnly_NoSchemaKey) {
    DebeziumFormatter fmt;
    auto j = fmt.toJson(makePutEvent("orders:1", R"({})"));
    EXPECT_FALSE(j.contains("schema"));
    EXPECT_TRUE(j.contains("payload"));
}

TEST(DebeziumFormatterTest, WithSchema_ContainsSchemaAndPayload) {
    DebeziumFormatter fmt;
    auto j = fmt.toJsonWithSchema(makePutEvent("orders:1", R"({})"));
    EXPECT_TRUE(j.contains("schema"));
    EXPECT_TRUE(j.contains("payload"));
}

TEST(DebeziumFormatterTest, SchemaBlock_ContainsEnvelopeName) {
    DebeziumFormatter fmt;
    auto j = fmt.toJsonWithSchema(
        makePutEvent("orders:1", R"({})"), "orders");
    const std::string name = j["schema"]["name"].get<std::string>();
    EXPECT_NE(name.find("orders"), std::string::npos)
        << "schema.name should reference the collection; got: " << name;
    EXPECT_NE(name.find("Envelope"), std::string::npos)
        << "schema.name should end in 'Envelope'; got: " << name;
}

TEST(DebeziumFormatterTest, SchemaBlock_OpFieldPresent) {
    DebeziumFormatter fmt;
    auto j = fmt.toJsonWithSchema(makePutEvent("orders:1", R"({})"));
    ASSERT_TRUE(j["schema"].contains("fields"));
    const auto& fields = j["schema"]["fields"];
    bool found_op = false;
    for (const auto& f : fields) {
        if (f.value("field", "") == "op") {
            found_op = true;
            EXPECT_EQ(f["type"].get<std::string>(), "string");
        }
    }
    EXPECT_TRUE(found_op) << "schema.fields should include 'op'";
}

// ---------------------------------------------------------------------------
// Malformed snapshot JSON – graceful degradation
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, InvalidSnapshotJson_FallsBackToRawKey) {
    DebeziumFormatter fmt;
    const std::string bad_json = "not valid json {{{";
    auto ev = makePutEvent("orders:1", bad_json,
                           std::nullopt, bad_json);
    auto env = fmt.toEnvelope(ev);

    // after should be a JSON object with "_raw" key, not a parse error
    ASSERT_FALSE(env.after.is_null());
    ASSERT_TRUE(env.after.contains("_raw"))
        << "invalid JSON snapshot should produce {\"_raw\": ...}";
    EXPECT_EQ(env.after["_raw"].get<std::string>(), bad_json);
}

// ---------------------------------------------------------------------------
// Redacted events
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, RedactedEvent_SnapshotFieldReflectsRedaction) {
    DebeziumFormatter fmt;
    auto ev = makePutEvent("users:42", "[REDACTED]");
    ev.redacted = true;
    auto env = fmt.toEnvelope(ev);
    EXPECT_EQ(env.source.snapshot, "redacted");

    auto j = env.toJson();
    EXPECT_EQ(j["payload"]["source"]["snapshot"].get<std::string>(), "redacted");
}

// ---------------------------------------------------------------------------
// Transaction field is always present (null)
// ---------------------------------------------------------------------------

TEST(DebeziumFormatterTest, PayloadContainsNullTransactionField) {
    DebeziumFormatter fmt;
    auto j = fmt.toJson(makePutEvent("orders:1", R"({})"));
    ASSERT_TRUE(j["payload"].contains("transaction"));
    EXPECT_TRUE(j["payload"]["transaction"].is_null());
}

// ---------------------------------------------------------------------------
// debeziumOpString helper
// ---------------------------------------------------------------------------

TEST(DebeziumOpStringTest, AllOpCodes) {
    EXPECT_EQ(debeziumOpString(DebeziumOp::CREATE), "c");
    EXPECT_EQ(debeziumOpString(DebeziumOp::UPDATE), "u");
    EXPECT_EQ(debeziumOpString(DebeziumOp::DELETE), "d");
    EXPECT_EQ(debeziumOpString(DebeziumOp::READ),   "r");
}
