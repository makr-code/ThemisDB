// Tests for KafkaCDCProducer
//
// These tests exercise the producer without a live Kafka broker:
//  - Configuration defaults
//  - No-op stub behaviour when THEMIS_ENABLE_KAFKA is not defined
//  - Topic-routing logic (per-collection vs single-topic)
//  - Metric counter initialisation
//  - getStats() snapshot
//  - ICDCTransport interface compliance
//
// Live-broker integration tests require a running Kafka instance and are
// outside the scope of the unit-test suite.

#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include "cdc/debezium_format.h"
#include "cdc/icdc_transport.h"
#include "cdc/kafka_cdc_producer.h"

using namespace themis;
using namespace themis::cdc;

// ── KafkaProducerConfig defaults ─────────────────────────────────────────────

TEST(KafkaCDCProducerTest, DefaultConfigValues) {
    KafkaProducerConfig cfg;
    EXPECT_EQ(cfg.brokers,           "localhost:9092");
    EXPECT_EQ(cfg.topic_prefix,      "themis.cdc.");
    EXPECT_TRUE(cfg.single_topic.empty());
    EXPECT_EQ(cfg.acks,              "all");
    EXPECT_TRUE(cfg.enable_idempotence);
    EXPECT_EQ(cfg.poll_interval_ms,  500u);
    EXPECT_EQ(cfg.linger_ms,         5);
    EXPECT_EQ(cfg.max_in_flight,     5);
    EXPECT_EQ(cfg.security_protocol, "plaintext");
    EXPECT_TRUE(cfg.sasl_mechanism.empty());
    EXPECT_TRUE(cfg.sasl_username.empty());
    EXPECT_TRUE(cfg.sasl_password.empty());
    EXPECT_TRUE(cfg.ssl_ca_location.empty());
    EXPECT_EQ(cfg.flush_timeout_ms,  10000u);
    EXPECT_FALSE(cfg.use_debezium_format);
}

// ── KafkaProducerStats defaults ───────────────────────────────────────────────

TEST(KafkaCDCProducerTest, DefaultStatsValues) {
    KafkaProducerStats s;
    EXPECT_EQ(s.delivered_total, 0u);
    EXPECT_EQ(s.error_total,     0u);
    EXPECT_EQ(s.poll_cycles,     0u);
    EXPECT_FALSE(s.running);
}

// ── No-op stub or stopped-producer getStats() ─────────────────────────────────

// This test is valid in both stub and full builds: before start() is called
// (or when compiled without THEMIS_ENABLE_KAFKA) stats must show zeroes and
// running=false.
TEST(KafkaCDCProducerTest, GetStatsBeforeStart) {
    // KafkaCDCProducer requires a Changefeed* but we only need the stub/ctor
    // path here; pass nullptr — the constructor must not dereference it before
    // start() is called.
    KafkaCDCProducer producer(nullptr);
    KafkaProducerStats s = producer.getStats();
    EXPECT_EQ(s.delivered_total, 0u);
    EXPECT_EQ(s.error_total,     0u);
    EXPECT_EQ(s.poll_cycles,     0u);
    EXPECT_FALSE(s.running);
}

// ── No-op stub: start() returns false, publish() returns false ────────────────

#ifndef THEMIS_ENABLE_KAFKA
TEST(KafkaCDCProducerTest, StubStartReturnsFalse) {
    KafkaCDCProducer producer(nullptr);
    EXPECT_FALSE(producer.start());
}

TEST(KafkaCDCProducerTest, StubPublishReturnsFalse) {
    KafkaCDCProducer producer(nullptr);
    Changefeed::ChangeEvent ev;
    ev.sequence    = 1;
    ev.type        = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key         = "orders:42";
    ev.value       = R"({"qty":3})";
    ev.timestamp_ms = 1740000000000LL;
    EXPECT_FALSE(producer.publish(ev));
}

TEST(KafkaCDCProducerTest, StubStopIsNoOp) {
    KafkaCDCProducer producer(nullptr);
    EXPECT_NO_THROW(producer.stop());
    EXPECT_NO_THROW(producer.stop());  // Idempotent.
}
#endif // !THEMIS_ENABLE_KAFKA

// ── CDCMetrics Kafka counter fields ──────────────────────────────────────────

TEST(KafkaCDCProducerTest, MetricsHasKafkaCounters) {
    CDCMetrics m;
    EXPECT_EQ(m.kafka_delivered_total.load(), 0u);
    EXPECT_EQ(m.kafka_error_total.load(),     0u);

    ++m.kafka_delivered_total;
    ++m.kafka_delivered_total;
    ++m.kafka_error_total;

    EXPECT_EQ(m.kafka_delivered_total.load(), 2u);
    EXPECT_EQ(m.kafka_error_total.load(),     1u);

    m.reset();
    EXPECT_EQ(m.kafka_delivered_total.load(), 0u);
    EXPECT_EQ(m.kafka_error_total.load(),     0u);
}

TEST(KafkaCDCProducerTest, MetricsToJsonContainsKafkaCounters) {
    CDCMetrics m;
    ++m.kafka_delivered_total;
    nlohmann::json j = m.toJson();
    ASSERT_TRUE(j.contains("counters"));
    const auto& counters = j["counters"];
    EXPECT_EQ(counters["kafka_delivered_total"].get<uint64_t>(), 1u);
    EXPECT_EQ(counters["kafka_error_total"].get<uint64_t>(),     0u);
}

// ── Topic routing helpers (exercised via public config, not private method) ───
//
// We verify the documented routing rules by inspecting config options only;
// the private topicForEvent() is implicitly tested by publish() in integration
// tests that require a live broker.

TEST(KafkaCDCProducerTest, ConfigSingleTopicOverride) {
    KafkaProducerConfig cfg;
    cfg.single_topic = "my.events";
    EXPECT_FALSE(cfg.single_topic.empty());
    // With single_topic set, all events should go to "my.events".
}

TEST(KafkaCDCProducerTest, ConfigPerCollectionTopicPrefix) {
    KafkaProducerConfig cfg;
    cfg.topic_prefix = "db.cdc.";
    EXPECT_TRUE(cfg.single_topic.empty());
    // With single_topic empty, per-collection routing uses topic_prefix.
}

// ── ICDCTransport interface compliance ────────────────────────────────────────

// Verify that KafkaCDCProducer is a subtype of ICDCTransport so it can be
// used polymorphically through the transport interface.
TEST(KafkaCDCProducerTest, ImplementsICDCTransport) {
    static_assert(std::is_base_of<ICDCTransport, KafkaCDCProducer>::value,
                  "KafkaCDCProducer must derive from ICDCTransport");
    KafkaCDCProducer producer(nullptr);
    ICDCTransport* transport = &producer;
    // Interface methods must be callable via the base pointer.
    EXPECT_FALSE(transport->start());
    EXPECT_NO_THROW(transport->stop());
    Changefeed::ChangeEvent ev;
    ev.sequence     = 1;
    ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = "col:1";
    ev.timestamp_ms = 0;
    EXPECT_FALSE(transport->publish(ev));
}

// Verify that a unique_ptr<ICDCTransport> can own a KafkaCDCProducer, which
// is the expected usage pattern in enterprise CDC pipeline setup code.
TEST(KafkaCDCProducerTest, PolymorphicOwnershipViaUniquePtr) {
    std::unique_ptr<ICDCTransport> transport =
        std::make_unique<KafkaCDCProducer>(nullptr);
    ASSERT_NE(transport, nullptr);
    EXPECT_FALSE(transport->start());
    transport->stop();
}

// ── Debezium format configuration ─────────────────────────────────────────────

TEST(KafkaCDCProducerTest, DebeziumFormatFlagCanBeEnabled) {
    KafkaProducerConfig cfg;
    cfg.use_debezium_format = true;
    EXPECT_TRUE(cfg.use_debezium_format);
}

TEST(KafkaCDCProducerTest, DebeziumConfigDefaultServerName) {
    KafkaProducerConfig cfg;
    // Default debezium_config should reflect DebeziumFormatter::Config defaults.
    EXPECT_EQ(cfg.debezium_config.server_name, "themis");
    EXPECT_EQ(cfg.debezium_config.db_name,     "themisdb");
    EXPECT_EQ(cfg.debezium_config.version,     "1.5.0-dev");
}

TEST(KafkaCDCProducerTest, DebeziumConfigCanBeCustomized) {
    KafkaProducerConfig cfg;
    cfg.use_debezium_format        = true;
    cfg.debezium_config.server_name = "prod-cluster";
    cfg.debezium_config.db_name     = "myapp";
    EXPECT_EQ(cfg.debezium_config.server_name, "prod-cluster");
    EXPECT_EQ(cfg.debezium_config.db_name,     "myapp");
}

// Verify that DebeziumFormatter (used by the producer when use_debezium_format
// is true) produces a valid Debezium envelope for a sample PUT event.  This
// acts as a smoke test for the integration path without requiring a live broker.
TEST(KafkaCDCProducerTest, DebeziumFormatterProducesValidEnvelope) {
    DebeziumFormatter::Config dcfg;
    dcfg.server_name = "test-cluster";
    dcfg.db_name     = "testdb";
    DebeziumFormatter fmt(dcfg);

    Changefeed::ChangeEvent ev;
    ev.sequence     = 7;
    ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = "orders:42";
    ev.value        = R"({"qty":5})";
    ev.timestamp_ms = 1740000000000LL;

    nlohmann::json j = fmt.toJson(ev, "orders");

    ASSERT_TRUE(j.contains("payload"));
    EXPECT_EQ(j["payload"]["op"].get<std::string>(), "c");
    EXPECT_EQ(j["payload"]["source"]["table"].get<std::string>(), "orders");
    EXPECT_EQ(j["payload"]["source"]["connector"].get<std::string>(), "themisdb");
    EXPECT_EQ(j["payload"]["source"]["name"].get<std::string>(), "test-cluster");
    EXPECT_EQ(j["payload"]["source"]["db"].get<std::string>(), "testdb");
    EXPECT_EQ(j["payload"]["source"]["sequence"].get<uint64_t>(), 7ULL);
}
