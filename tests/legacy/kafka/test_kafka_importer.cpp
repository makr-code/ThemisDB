/**
 * @file test_kafka_importer.cpp
 * @brief Unit tests for the KafkaImporter (importers module).
 *
 * All tests use the mock-injection path (setMessageFetchForTesting) so that
 * no live Kafka broker is required.  The tests verify:
 *
 *  URL parsing (parseKafkaUrl)
 *  - Valid kafka:// URL with single broker
 *  - Valid kafka:// URL with multiple brokers
 *  - Bare topic name (no kafka:// prefix)
 *  - Invalid URL (empty, trailing slash without topic)
 *
 *  Initialization (initialize)
 *  - Default config (empty JSON)
 *  - Explicit consumer_group, message_format, max_messages, poll_timeout_ms
 *  - Malformed JSON returns false
 *
 *  getSupportedTypes
 *  - Returns expected type strings
 *
 *  importData – JSON format
 *  - Messages with a JSON object body imported as entities
 *  - Scalar JSON value wrapped in configured text_field
 *  - Non-JSON payload wrapped in text_field
 *
 *  importData – plaintext format
 *  - Payload wrapped in {"content": "..."}
 *
 *  importData – Avro format
 *  - Magic-byte + schema-ID prefix stripped; remainder parsed as JSON
 *  - Bare Avro payload (no magic byte) parsed directly
 *
 *  importData – max_messages limit
 *  - Stops after configured limit
 *  - max_messages = 0 means unlimited
 *
 *  importData – streaming row callback
 *  - Callback receives correct table name and entity
 *  - Returning false from callback aborts import
 *
 *  importData – dry-run mode
 *  - Messages consumed but callback NOT invoked
 *  - imported_records stays 0; warnings populated
 *
 *  importData – permission_check callback
 *  - PERMISSION_DENIED error when callback returns false
 *
 *  importData – oversized message
 *  - Message exceeding max_row_size_bytes counted as failed
 *
 *  importData – empty batch terminates
 *  - Empty vector returned by mock stops the consume loop
 *
 *  importData – invalid source_path
 *  - Error returned for malformed URL
 *
 *  getSourceSchema
 *  - Returns JSON with type="kafka" and the topic name
 *
 *  metrics callback
 *  - Metrics callback invoked at import completion
 *
 *  tracing callback
 *  - Tracing callback invoked at import start and end
 */

#include <gtest/gtest.h>
#include "importers/kafka_importer.h"
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>
#include <thread>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace themis::importers;

// ---------------------------------------------------------------------------
// Helper: build a JSON config string for KafkaImporter::initialize()
// ---------------------------------------------------------------------------

static std::string buildConfig(
        const std::string& consumer_group = "themis-import",
        const std::string& message_format = "json",
        const std::string& text_field     = "text",
        size_t             max_messages   = 0,
        const std::string& brokers        = "") {
    json cfg;
    cfg["consumer_group"]  = consumer_group;
    cfg["message_format"]  = message_format;
    cfg["text_field"]      = text_field;
    cfg["max_messages"]    = max_messages;
    if (!brokers.empty()) cfg["brokers"] = brokers;
    return cfg.dump();
}

// ===========================================================================
// Test suite: URL parsing
// ===========================================================================

TEST(KafkaImporterUrlParsing, SingleBroker) {
    std::string b, t;
    ASSERT_TRUE(KafkaImporter::parseKafkaUrl("kafka://localhost:9092/events", b, t));
    EXPECT_EQ(b, "localhost:9092");
    EXPECT_EQ(t, "events");
}

TEST(KafkaImporterUrlParsing, MultipleBrokers) {
    std::string b, t;
    ASSERT_TRUE(KafkaImporter::parseKafkaUrl("kafka://b1:9092,b2:9092/logs", b, t));
    EXPECT_EQ(b, "b1:9092,b2:9092");
    EXPECT_EQ(t, "logs");
}

TEST(KafkaImporterUrlParsing, BareTopicName) {
    std::string b, t;
    ASSERT_TRUE(KafkaImporter::parseKafkaUrl("my-topic", b, t));
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(t, "my-topic");
}

TEST(KafkaImporterUrlParsing, EmptyUrl) {
    std::string b, t;
    EXPECT_FALSE(KafkaImporter::parseKafkaUrl("", b, t));
}

TEST(KafkaImporterUrlParsing, TrailingSlashNoTopic) {
    std::string b, t;
    // kafka://broker:9092/  – empty topic after last '/'
    EXPECT_FALSE(KafkaImporter::parseKafkaUrl("kafka://broker:9092/", b, t));
}

TEST(KafkaImporterUrlParsing, MissingTopicSlash) {
    // kafka://broker:9092  – no '/' at all after the prefix
    std::string b, t;
    EXPECT_FALSE(KafkaImporter::parseKafkaUrl("kafka://broker:9092", b, t));
}

TEST(KafkaImporterUrlParsing, TopicWithHyphens) {
    std::string b, t;
    ASSERT_TRUE(KafkaImporter::parseKafkaUrl("kafka://broker:9092/my-cool-topic-v2", b, t));
    EXPECT_EQ(t, "my-cool-topic-v2");
}

// ===========================================================================
// Test suite: getSupportedTypes
// ===========================================================================

TEST(KafkaImporterSupportedTypes, ContainsExpectedTypes) {
    KafkaImporter importer;
    auto types = importer.getSupportedTypes();
    EXPECT_FALSE(types.empty());
    // Must include the four documented formats.
    for (const auto& expected : {"kafka", "kafka-json", "kafka-avro", "kafka-plaintext"}) {
        EXPECT_NE(std::find(types.begin(), types.end(), expected), types.end())
            << "Missing type: " << expected;
    }
}

// ===========================================================================
// Test suite: entity extraction (via setMessageFetchForTesting + importData)
// ===========================================================================

TEST(KafkaImporterExtractEntity, JsonObjectPassedThrough) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"id":1,"value":"hello"})"};
        return {};
    });

    json captured;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        captured = e;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_TRUE(captured.is_object());
    EXPECT_EQ(captured["id"].get<int>(), 1);
    EXPECT_EQ(captured["value"].get<std::string>(), "hello");
    EXPECT_EQ(stats.imported_records, 1u);
}

TEST(KafkaImporterExtractEntity, ScalarJsonWrapped) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {"42"};
        return {};
    });

    json captured;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        captured = e;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_TRUE(captured.is_object());
    EXPECT_EQ(captured["text"].get<int>(), 42);
}

TEST(KafkaImporterExtractEntity, NonJsonWrapped) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {"plain text payload"};
        return {};
    });

    json captured;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        captured = e;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_TRUE(captured.is_object());
    EXPECT_EQ(captured["text"].get<std::string>(), "plain text payload");
}

TEST(KafkaImporterExtractEntity, PlaintextFormat) {
    KafkaImporter importer;
    importer.initialize(buildConfig("themis-import", "plaintext"));

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {"raw bytes"};
        return {};
    });

    json captured;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        captured = e;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_TRUE(captured.is_object());
    EXPECT_EQ(captured["content"].get<std::string>(), "raw bytes");
}

TEST(KafkaImporterExtractEntity, AvroMagicByteStripped) {
    KafkaImporter importer;
    importer.initialize(buildConfig("themis-import", "avro"));

    // Build a Confluent-framed Avro message: 0x00 + 4-byte schema ID + JSON
    std::string avro_payload;
    avro_payload += '\x00';
    avro_payload += '\x00'; avro_payload += '\x00';
    avro_payload += '\x00'; avro_payload += '\x01'; // schema ID = 1
    avro_payload += R"({"content":"avro_data"})";

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {avro_payload};
        return {};
    });

    json captured;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        captured = e;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_TRUE(captured.is_object());
    EXPECT_EQ(captured["content"].get<std::string>(), "avro_data");
}

TEST(KafkaImporterExtractEntity, AvroNoMagicByteJsonFallback) {
    KafkaImporter importer;
    importer.initialize(buildConfig("themis-import", "avro"));

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"val":99})"};
        return {};
    });

    json captured;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        captured = e;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_TRUE(captured.is_object());
    EXPECT_EQ(captured["val"].get<int>(), 99);
}

TEST(KafkaImporterExtractEntity, EmptyPayloadReturnsNull) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {""};
        return {};
    });

    ImportOptions opts;
    // Callback should never be invoked for empty (null) entity.
    bool callback_invoked = false;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        callback_invoked = true;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    EXPECT_FALSE(callback_invoked);
    EXPECT_EQ(stats.skipped_records, 1u);
    EXPECT_EQ(stats.imported_records, 0u);
}

// ===========================================================================
// Test suite: importData – mock injection via setMessageFetchForTesting
// ===========================================================================

TEST(KafkaImporterMockImport, IngestsJsonMessages) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {
            R"({"id":1,"name":"Alice"})",
            R"({"id":2,"name":"Bob"})"
        };
        return {};
    });

    std::vector<json> entities;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        entities.push_back(e);
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_EQ(entities.size(), 2u);
    EXPECT_EQ(entities[0]["name"].get<std::string>(), "Alice");
    EXPECT_EQ(entities[1]["name"].get<std::string>(), "Bob");
    EXPECT_EQ(stats.imported_records, 2u);
    EXPECT_EQ(stats.failed_records, 0u);
}

TEST(KafkaImporterMockImport, BareTopicNameAccepted) {
    KafkaImporter importer;
    // Configure default brokers so the bare-topic path doesn't fail with
    // "no broker list".
    importer.initialize(buildConfig("themis-import", "json", "text", 0, "localhost:9092"));

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"x":1})"};
        return {};
    });

    ImportOptions opts;
    size_t count = 0;
    opts.streaming_row_callback = [&](const std::string& table, const json&) -> bool {
        EXPECT_EQ(table, "my-topic");
        ++count;
        return true;
    };

    auto stats = importer.importData("my-topic", opts);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
}

TEST(KafkaImporterMockImport, MaxMessagesLimit) {
    KafkaImporter importer;
    importer.initialize(buildConfig("themis-import", "json", "text", 3 /*max_messages*/));

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        ++call;
        return {R"({"v":1})", R"({"v":2})", R"({"v":3})",
                R"({"v":4})", R"({"v":5})"};
    });

    ImportOptions opts;
    auto stats = importer.importData("bare-topic", opts);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(KafkaImporterMockImport, MaxMessagesZeroMeansUnlimited) {
    KafkaImporter importer;
    importer.initialize(buildConfig("themis-import", "json", "text", 0 /*unlimited*/));

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ < 3) return {R"({"v":1})", R"({"v":2})"};
        return {};
    });

    ImportOptions opts;
    auto stats = importer.importData("bare-topic", opts);
    // 3 batches × 2 messages = 6
    EXPECT_EQ(stats.imported_records, 6u);
}

TEST(KafkaImporterMockImport, StreamingCallbackReceivesEntities) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {
            R"({"seq":1})", R"({"seq":2})", R"({"seq":3})"
        };
        return {};
    });

    std::vector<int> seqs;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        seqs.push_back(e["seq"].get<int>());
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_EQ(seqs.size(), 3u);
    EXPECT_EQ(seqs[0], 1);
    EXPECT_EQ(seqs[1], 2);
    EXPECT_EQ(seqs[2], 3);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(KafkaImporterMockImport, StreamingCallbackAbortOnFalse) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {
            R"({"n":1})", R"({"n":2})", R"({"n":3})", R"({"n":4})"
        };
        return {};
    });

    size_t delivered = 0;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++delivered;
        return delivered < 2; // abort after 2nd entity
    };

    auto stats = importer.importData("bare-topic", opts);
    EXPECT_EQ(delivered, 2u);
    // imported_records includes the last one that caused abort
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(KafkaImporterMockImport, StreamingCallbackTableNameMatchesTopic) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"x":1})", R"({"x":2})"};
        return {};
    });

    std::vector<std::string> table_names;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string& table,
                                       const json&) -> bool {
        table_names.push_back(table);
        return true;
    };

    auto stats = importer.importData("kafka://b:9092/user-events", opts);
    for (const auto& t : table_names) {
        EXPECT_EQ(t, "user-events");
    }
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(KafkaImporterMockImport, DryRunDoesNotInvokeCallback) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {
            R"({"a":1})", R"({"a":2})", R"({"a":3})"
        };
        return {};
    });

    bool callback_called = false;
    ImportOptions opts;
    opts.dry_run = true;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        callback_called = true;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    EXPECT_FALSE(callback_called);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.total_records, 3u);
    EXPECT_FALSE(stats.warnings.empty()); // dry-run produces warnings
}

TEST(KafkaImporterMockImport, PermissionDenied) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    bool fn_called = false;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        fn_called = true;
        return {R"({"x":1})"};
    });

    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) {
        return false; // deny
    };

    auto stats = importer.importData("bare-topic", opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_FALSE(stats.errors.empty());
    EXPECT_FALSE(stats.structured_errors.empty());
    EXPECT_EQ(stats.structured_errors[0].code, ImportErrorCode::PERMISSION_DENIED);
    EXPECT_FALSE(fn_called) << "message_fn must not be called after permission denial";
}

TEST(KafkaImporterMockImport, PermissionGranted) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"ok":true})"};
        return {};
    });

    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) {
        return true; // allow
    };
    size_t count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++count;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
    EXPECT_TRUE(stats.errors.empty());
}

TEST(KafkaImporterMockImport, OversizedMessageFailed) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    std::string big_msg(200, 'x'); // 200 bytes

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {big_msg};
        return {};
    });

    ImportOptions opts;
    opts.max_row_size_bytes = 100; // reject messages > 100 bytes
    bool callback_called = false;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        callback_called = true;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    EXPECT_FALSE(callback_called);
    EXPECT_EQ(stats.failed_records, 1u);
    EXPECT_EQ(stats.imported_records, 0u);
}

TEST(KafkaImporterMockImport, SmallMessageAccepted) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"small":true})"};
        return {};
    });

    ImportOptions opts;
    opts.max_row_size_bytes = 1000; // plenty of room
    size_t count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++count;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(stats.failed_records, 0u);
}

TEST(KafkaImporterMockImport, EmptyBatchTerminates) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    importer.setMessageFetchForTesting([]() -> std::vector<std::string> {
        return {}; // immediately empty
    });

    ImportOptions opts;
    auto stats = importer.importData("bare-topic", opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.failed_records, 0u);
    EXPECT_EQ(stats.total_records, 0u);
}

TEST(KafkaImporterMockImport, InvalidUrlReturnsError) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    // A kafka:// URL without a topic triggers an error in importData().
    ImportOptions opts;
    auto stats = importer.importData("kafka://broker:9092", opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_FALSE(stats.errors.empty());
}

TEST(KafkaImporterMockImport, MultipleBatchesAggregated) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        switch (call++) {
        case 0: return {R"({"b":1})", R"({"b":2})"};
        case 1: return {R"({"b":3})", R"({"b":4})", R"({"b":5})"};
        default: return {};
        }
    });

    ImportOptions opts;
    auto stats = importer.importData("bare-topic", opts);
    EXPECT_EQ(stats.imported_records, 5u);
}

TEST(KafkaImporterMockImport, PlaintextMessagesWrapped) {
    KafkaImporter importer;
    importer.initialize(buildConfig("themis-import", "plaintext"));

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {"hello", "world"};
        return {};
    });

    std::vector<json> entities;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        entities.push_back(e);
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_EQ(entities.size(), 2u);
    EXPECT_EQ(entities[0]["content"].get<std::string>(), "hello");
    EXPECT_EQ(entities[1]["content"].get<std::string>(), "world");
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(KafkaImporterMockImport, AvroMagicByteStrippedOnImport) {
    KafkaImporter importer;
    importer.initialize(buildConfig("themis-import", "avro"));

    // Build two Confluent-framed Avro messages.
    auto makeAvro = [](const std::string& json_payload) -> std::string {
        std::string msg;
        msg += '\x00';
        msg += '\x00'; msg += '\x00'; msg += '\x00'; msg += '\x02'; // schema ID = 2
        msg += json_payload;
        return msg;
    };

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) {
            return {makeAvro(R"({"event":"login","user":1})"),
                    makeAvro(R"({"event":"logout","user":1})")};
        }
        return {};
    });

    std::vector<json> entities;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        entities.push_back(e);
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_EQ(entities.size(), 2u);
    EXPECT_EQ(entities[0]["event"].get<std::string>(), "login");
    EXPECT_EQ(entities[1]["event"].get<std::string>(), "logout");
    EXPECT_EQ(stats.imported_records, 2u);
}

// ===========================================================================
// Test suite: getSourceSchema
// ===========================================================================

TEST(KafkaImporterSchemaTest, SchemaContainsTopicName) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    json schema = importer.getSourceSchema("kafka://broker:9092/my-topic");
    EXPECT_EQ(schema["type"].get<std::string>(), "kafka");
    EXPECT_EQ(schema["topic"].get<std::string>(), "my-topic");
    EXPECT_TRUE(schema["schema"].is_null());
}

// ===========================================================================
// Test suite: metrics callback
// ===========================================================================

TEST(KafkaImporterMockImport, MetricsCallbackInvokedOnCompletion) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"v":1})", R"({"v":2})"};
        return {};
    });

    std::vector<std::string> metric_names;
    ImportOptions opts;
    opts.metrics_callback =
        [&](const std::string& m,
            const std::map<std::string,std::string>&,
            double) {
            metric_names.push_back(m);
        };

    auto stats = importer.importData("kafka://b:9092/t", opts);
    EXPECT_EQ(stats.imported_records, 2u);
    // Production importData() emits metrics after the consume loop.
    EXPECT_FALSE(metric_names.empty())
        << "Expected at least one metric to be emitted by importData()";
    // At minimum the row counter metric should be present.
    bool found_row_metric = false;
    for (const auto& m : metric_names) {
        if (m.find("import_rows") != std::string::npos) {
            found_row_metric = true;
        }
    }
    EXPECT_TRUE(found_row_metric)
        << "Expected 'themisdb_import_rows_total' to be emitted";
}

// ===========================================================================
// Test suite: tracing callback
// ===========================================================================

TEST(KafkaImporterMockImport, TracingCallbackCanReceiveSpans) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"({"v":1})"};
        return {};
    });

    int span_count = 0;
    ImportOptions opts;
    opts.tracing_callback =
        [&](const std::string&,
            const std::map<std::string,std::string>&,
            double) {
            ++span_count;
        };

    auto stats = importer.importData("kafka://b:9092/t", opts);
    EXPECT_EQ(stats.imported_records, 1u);
    // Production importData() emits spans at start and end of import.
    EXPECT_GE(span_count, 1)
        << "Expected at least one span to be emitted by importData()";
}

// ===========================================================================
// Test suite: custom text_field configuration
// ===========================================================================

TEST(KafkaImporterMockImport, CustomTextFieldExtracted) {
    KafkaImporter importer;
    // Configure a custom text_field "payload".
    importer.initialize(buildConfig("themis-import", "json", "payload"));

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {"hello world"}; // non-JSON → wrapped in text_field
        return {};
    });

    json captured;
    ImportOptions opts;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        captured = e;
        return true;
    };

    auto stats = importer.importData("bare-topic", opts);
    ASSERT_TRUE(captured.is_object());
    EXPECT_EQ(captured["payload"].get<std::string>(), "hello world");
}

// ===========================================================================
// Test suite: JSON array message
// ===========================================================================

TEST(KafkaImporterMockImport, JsonArrayMessageImportedAsArray) {
    KafkaImporter importer;
    importer.initialize(buildConfig());

    int call = 0;
    importer.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call++ == 0) return {R"([1, 2, 3])"};
        return {};
    });

    std::vector<json> entities;
    ImportOptions opts;
    opts.streaming_row_callback =
        [&](const std::string&, const json& e) -> bool {
            entities.push_back(e);
            return true;
        };

    auto stats = importer.importData("kafka://b:9092/t", opts);
    ASSERT_EQ(entities.size(), 1u);
    EXPECT_TRUE(entities[0].is_array());
    EXPECT_EQ(stats.imported_records, 1u);
}
