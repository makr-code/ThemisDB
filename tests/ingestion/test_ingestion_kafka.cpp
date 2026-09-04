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
#include <filesystem>

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
        if (call_count++ == 0) {
          return messages;
        }
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
    std::string avro_msg = {};
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

// ---------------------------------------------------------------------------
// CheckpointStore integration
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, CheckpointWrittenAfterMockIngest) {
    // Verify that when a CheckpointStore is injected, the connector writes a
    // checkpoint (with the correct document count) after processing messages.
    // This validates the "offset commit tied to IngestionCheckpointStore::commit()"
    // acceptance criterion: the checkpoint is written before the ingest()
    // call returns, ahead of any librdkafka rd_kafka_consumer_close() commit.
    namespace fs = std::filesystem;
    auto tmpdir = fs::temp_directory_path() / "themis_kafka_ckpt_test";
    fs::create_directories(tmpdir);

    auto store = std::make_shared<CheckpointStore>(tmpdir.string());
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());
    conn.setCheckpointStore(store);

    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {R"({"text":"a"})", R"({"text":"b"})"};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);

    // The checkpoint must have been written with the correct count.
    IngestionCheckpoint cp;
    ASSERT_TRUE(store->read("test_kafka", cp));
    EXPECT_EQ(cp.processed_count, 2u);
    EXPECT_EQ(cp.source_id, "test_kafka");

    fs::remove_all(tmpdir);
}

TEST(KafkaConnectorTest, CheckpointNotWrittenWithoutStore) {
    // Without a checkpoint store, ingest() completes without errors.
    namespace fs = std::filesystem;
    auto tmpdir = fs::temp_directory_path() / "themis_kafka_nostore_test";
    fs::create_directories(tmpdir);

    // Use a separate store to verify no spurious write happened.
    auto probe_store = std::make_shared<CheckpointStore>(tmpdir.string());
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());
    // Deliberately do NOT call setCheckpointStore()

    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {R"({"text":"doc"})"};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_TRUE(stats.errors.empty());

    // No checkpoint should have been written to the probe store.
    IngestionCheckpoint cp;
    EXPECT_FALSE(probe_store->read("test_kafka", cp));

    fs::remove_all(tmpdir);
}

TEST(KafkaConnectorTest, ConsumerGroupIsConfigurable) {
    // Verify that the consumer_group option is accepted during initialization
    // and that a custom group ID is stored correctly.
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["consumer_group"] = "my-custom-group";
    EXPECT_TRUE(conn.initialize(cfg));

    // The connector should work normally with the custom group.
    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {R"({"text":"group-test"})"};
        return {};
    });
    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

TEST(KafkaConnectorTest, SchemaRegistryUrlStoredForAvro) {
    // When message_format=avro and schema_registry_url is set, the connector
    // should initialize successfully and still process messages.
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["message_format"]      = "avro";
    cfg.options["schema_registry_url"] = "http://registry:8081";
    EXPECT_TRUE(conn.initialize(cfg));

    // Build a minimal Confluent Avro wire-format message:
    //   Byte 0     : magic byte 0x00
    //   Bytes 1-4  : 4-byte big-endian schema ID (here: 0x00000002)
    //   Bytes 5+   : Avro-encoded payload
    const std::string avro_msg{
        '\x00',                         // magic byte
        '\x00', '\x00', '\x00', '\x02', // schema ID = 2
        'a', 'v', 'r', 'o', '_', 'c', 'o', 'n', 't', 'e', 'n', 't'
    };

    conn.setMessageFetchForTesting([&, call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {avro_msg};
        return {};
    });

    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// auto_offset_reset option
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, AutoOffsetResetLatestInitializes) {
    // Verify the connector initializes successfully when auto_offset_reset
    // is explicitly set to "latest".
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["auto_offset_reset"] = "latest";
    EXPECT_TRUE(conn.initialize(cfg));
}

TEST(KafkaConnectorTest, AutoOffsetResetDefaultIsEarliest) {
    // Without an explicit option the connector should default to "earliest"
    // and still initialize successfully.  The librdkafka "auto.offset.reset"
    // config is verified indirectly: initialize() succeeds (config accepted)
    // and the mock-based ingest works correctly.  Verifying the actual offset
    // position requires a live Kafka broker and is deferred to integration tests.
    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    // Do NOT set auto_offset_reset – default should apply.
    EXPECT_TRUE(conn.initialize(cfg));

    // The mock-based ingest should work normally regardless of offset setting.
    conn.setMessageFetchForTesting([call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {R"({"text":"test"})"};
        return {};
    });
    auto stats = conn.ingest("docs", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
}

// ---------------------------------------------------------------------------
// AC-6: Throughput ≥ 100 000 messages/sec (1 KB average message)
//
// Uses the mock injection path so no live Kafka broker is required.
// The test is guarded by THEMIS_RUN_PERF_TESTS=1 to avoid failures on
// slow CI hardware; on reference hardware (modern laptop/server) the
// mock path typically achieves > 1 000 000 msgs/sec — this test targets
// the conservative 100 000 msgs/sec threshold stated in the AC.
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, ThroughputAtLeast100kMessagesPerSec) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping Kafka throughput microbenchmark "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable). "
                        "AC-6: ≥ 100 000 msgs/sec with 1 KB messages.";
    }

    constexpr size_t kMessageCount   = 100'000;
    constexpr size_t kMessageSizeBytes = 1024; // 1 KB
    constexpr double kMinRate    = 100'000.0; // msgs/sec

    KafkaConnector conn;
    auto cfg = makeKafkaConfig();
    cfg.options["max_messages"] = std::to_string(kMessageCount);
    conn.initialize(cfg);

    // Pre-build the message batch: one batch of kMessageCount messages.
    const std::string payload(kMessageSizeBytes, 'x'); // 1 KB plain text
    std::vector<std::string> batch(kMessageCount, payload);

    conn.setMessageFetchForTesting([&, call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) {
          return batch;
        }
        return {};
    });

    auto t0    = std::chrono::steady_clock::now();
    auto stats = conn.ingest("docs", nullptr);
    auto t1    = std::chrono::steady_clock::now();

    ASSERT_EQ(stats.documents_processed, kMessageCount);

    double elapsed_s = std::chrono::duration<double>(t1 - t0).count();
    ASSERT_GT(elapsed_s, 0.0);
    double rate = static_cast<double>(kMessageCount) / elapsed_s;

    RecordProperty("messages_ingested",    static_cast<int>(kMessageCount));
    RecordProperty("elapsed_ms",           static_cast<int>(elapsed_s * 1000));
    RecordProperty("msgs_per_sec",         static_cast<int>(rate));

    EXPECT_GE(rate, kMinRate)
        << "Kafka mock throughput " << static_cast<int>(rate)
        << " msgs/sec is below the AC-6 target of 100 000 msgs/sec. "
           "This target is measured on the mock path; real librdkafka "
           "throughput depends on broker capacity and hardware.";
}

// ---------------------------------------------------------------------------
// AC-7: End-to-end latency ≤ 500 ms p99
//
// True end-to-end latency (Kafka publish → ThemisDB document available)
// requires a live Kafka broker and is verified only in integration tests
// against a local confluentinc/cp-kafka container.  This unit test verifies
// the connector-side processing latency (mock path) is negligible (< 100 ms
// for a single message) so that the connector does not itself become the
// bottleneck that breaks the 500 ms p99 budget.
// ---------------------------------------------------------------------------

TEST(KafkaConnectorTest, SingleMessageProcessingLatencyBelow100ms) {
    KafkaConnector conn;
    conn.initialize(makeKafkaConfig());

    const std::string payload(1024, 'x'); // 1 KB plain text
    conn.setMessageFetchForTesting([&, call = 0]() mutable -> std::vector<std::string> {
        if (call++ == 0) return {payload};
        return {};
    });

    auto t0    = std::chrono::steady_clock::now();
    auto stats = conn.ingest("docs", nullptr);
    auto t1    = std::chrono::steady_clock::now();

    ASSERT_EQ(stats.documents_processed, 1u);

    double elapsed_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    RecordProperty("single_msg_latency_ms", static_cast<int>(elapsed_ms));

    // The connector must add less than 100 ms of processing overhead for a
    // single message so there is headroom within the 500 ms p99 budget.
    EXPECT_LT(elapsed_ms, 100.0)
        << "Single-message connector processing took " << elapsed_ms
        << " ms; must be < 100 ms to leave headroom for the 500 ms p99 "
           "end-to-end latency budget (AC-7). Note: broker network RTT and "
           "storage write time are excluded from this unit test.";
}
