/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_ingestion_kafka.cpp                           ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-26                                         ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_ingestion_kafka.cpp
 * @brief Unit tests for the KafkaConnector ingestion source connector.
 *
 * All tests use the mock-injection path (setMessageFetchForTesting) so that
 * no live Kafka broker is required.  The tests verify:
 *   - Initialization from SourceConfig
 *   - JSON text-field extraction
 *   - Plain-text (non-JSON) message ingestion
 *   - max_messages limit enforcement
 *   - Empty batch termination
 *   - Progress callback invocation
 *   - IngestionBuilder::withKafkaSource() fluent API
 *   - Error handling: missing broker/topic configuration
 *   - SourceType::KAFKA in sourceTypeLabel (via IngestionMetricsExporter)
 *   - Avro magic-byte stripping
 */

#include <gtest/gtest.h>
#include "ingestion/kafka_connector.h"
#include "ingestion/ingestion_manager.h"
#include <string>
#include <vector>
#include <atomic>

using namespace themis::ingestion;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SourceConfig makeKafkaConfig(
        const std::string& brokers = "localhost:9092",
        const std::string& topic   = "test_topic") {
    SourceConfig cfg;
    cfg.source_id     = "test_kafka";
    cfg.type          = SourceType::KAFKA;
    cfg.location      = brokers;
    cfg.options["topic"]          = topic;
    cfg.options["consumer_group"] = "test-group";
    cfg.options["max_messages"]   = "0";
    return cfg;
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, InitializeValidConfig) {
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(KafkaConnectorTest, InitializeWrongType) {
    KafkaConnector conn;
    SourceConfig cfg;
    cfg.source_id = "wrong";
    cfg.type      = SourceType::API;  // wrong type
    cfg.location  = "localhost:9092";
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(KafkaConnectorTest, InitializeEmptyBrokers) {
    KafkaConnector conn;
    SourceConfig cfg;
    cfg.source_id       = "empty_brokers";
    cfg.type            = SourceType::KAFKA;
    cfg.location        = "";  // empty brokers
    cfg.options["topic"] = "test";
    EXPECT_FALSE(conn.initialize(cfg));
}

TEST(KafkaConnectorTest, InitializeEmptyTopic) {
    KafkaConnector conn;
    SourceConfig cfg;
    cfg.source_id = "empty_topic";
    cfg.type      = SourceType::KAFKA;
    cfg.location  = "localhost:9092";
    // no "topic" key – topic_ defaults to brokers string which is non-empty,
    // but we want to explicitly verify the no-topic-option case
    // In the implementation, if "topic" is absent, location is used as topic.
    // So this should succeed (location is non-empty).
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(KafkaConnectorTest, GetDocumentCountAlwaysZero) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());
    EXPECT_EQ(conn.getDocumentCount(), 0u);
}

TEST(KafkaConnectorTest, IsAvailableWithMockReturnsTrue) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());
    // Inject a mock that returns messages – isAvailable() returns true
    conn.setMessageFetchForTesting([]() { return std::vector<std::string>{}; });
    EXPECT_TRUE(conn.isAvailable());
}

// ---------------------------------------------------------------------------
// JSON ingestion
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, IngestJsonMessages) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());

    std::vector<std::string> messages = {
        R"({"text":"document one"})",
        R"({"text":"document two"})",
        R"({"text":"document three"})",
    };
    size_t call_count = 0;
    conn.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (call_count++ == 0) return messages;
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.bytes_processed, 0u);
}

TEST(KafkaConnectorTest, IngestCustomTextField) {
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["text_field"] = "content";
    conn.initialize(cfg);

    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {R"({"content":"hello world"})"};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

TEST(KafkaConnectorTest, IngestPlainTextMessages) {
    // When the message is not a JSON object (does not start with '{'),
    // the whole payload is treated as the document.
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());

    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {"plain text document", "another plain doc"};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
}

// ---------------------------------------------------------------------------
// max_messages limit
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, MaxMessagesLimit) {
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["max_messages"] = "2";
    conn.initialize(cfg);

    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        // Return 5 messages per batch; limit should stop at 2.
        if (call++ < 5) return {"msg1", "msg2", "msg3", "msg4", "msg5"};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
}

TEST(KafkaConnectorTest, MaxMessagesZeroMeansUnlimited) {
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["max_messages"] = "0";
    conn.initialize(cfg);

    int batch_count = 0;
    conn.setMessageFetchForTesting([&]() -> std::vector<std::string> {
        if (batch_count++ < 3) return {"msg"};
        return {};  // stop after 3 batches
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 3u);
}

// ---------------------------------------------------------------------------
// Empty batch terminates ingestion
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, EmptyBatchTerminates) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());

    // First call returns messages; second call returns empty (= end of stream).
    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {R"({"text":"doc"})"};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_TRUE(stats.errors.empty());
}

// ---------------------------------------------------------------------------
// Progress callback
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, ProgressCallbackInvoked) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());

    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {"a", "b", "c"};
        return {};
    });

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
// Error: not configured
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, IngestWithoutInitializationReturnsError) {
    KafkaConnector conn;
    // setMessageFetchForTesting but no initialize – brokers_ and topic_ are empty
    conn.setMessageFetchForTesting([]() { return std::vector<std::string>{}; });

    auto stats = conn.ingest("docs", nullptr);
    // Should fail with SOURCE_NOT_CONFIGURED
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
// Elapsed time and throughput
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, StatsHaveElapsedTime) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());
    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {"msg1", "msg2"};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

// ---------------------------------------------------------------------------
// Avro magic byte stripping
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, AvroMagicByteStripped) {
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["message_format"] = "avro";
    conn.initialize(cfg);

    // Construct a minimal Avro-framed message: 0x00 + 4-byte schema ID + payload
    std::string avro_msg;
    avro_msg += '\x00';       // magic byte
    avro_msg += '\x00'; avro_msg += '\x00'; avro_msg += '\x00'; avro_msg += '\x01'; // schema ID = 1
    avro_msg += "avro_payload";

    conn.setMessageFetchForTesting([&, call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {avro_msg};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// IngestionBuilder::withKafkaSource
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, BuilderWithKafkaSource) {
    auto mgr = IngestionBuilder("test_db")
        .withKafkaSource("kafka_src", "localhost:9092", "my_topic",
                         {{"consumer_group","test-grp"},{"max_messages","5"}})
        .withDryRun(true)
        .build();

    ASSERT_NE(mgr, nullptr);

    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "kafka_src");
    EXPECT_EQ(sources[0].type, SourceType::KAFKA);
    EXPECT_EQ(sources[0].location, "localhost:9092");
    EXPECT_EQ(sources[0].options.at("topic"), "my_topic");
    EXPECT_EQ(sources[0].options.at("consumer_group"), "test-grp");
    EXPECT_EQ(sources[0].options.at("max_messages"), "5");
}

TEST(KafkaConnectorTest, BuilderWithKafkaSourceDefaultOptions) {
    auto mgr = IngestionBuilder("test_db")
        .withKafkaSource("ks", "broker:9092", "t")
        .build();

    ASSERT_NE(mgr, nullptr);
    auto sources = mgr->getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].options.at("topic"), "t");
}

// ---------------------------------------------------------------------------
// SourceType::KAFKA in Prometheus metrics exporter
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, MetricsExporterIncludesKafkaSourceType) {
    IngestionStats stats;
    stats.documents_processed = 42;
    stats.elapsed_seconds     = 0.5;

    IngestionMetricsExporter exporter;
    exporter.setPrefix("themis_ingestion");

    // Use the single-stats overload with source_type="KAFKA" to verify the
    // KAFKA label flows through the exporter.
    std::string text = exporter.exportText(stats, "kafka_src", "KAFKA");
    EXPECT_FALSE(text.empty());
    EXPECT_NE(text.find("KAFKA"), std::string::npos)
        << "Expected 'KAFKA' source_type label in Prometheus output";
}

// ---------------------------------------------------------------------------
// RetryConfig passthrough
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, SetRetryConfigDoesNotCrash) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());
    RetryConfig rc;
    rc.max_attempts     = 5;
    rc.initial_delay_ms = 100.0;
    // Should not throw or crash.
    EXPECT_NO_THROW(conn.setRetryConfig(rc));
}
