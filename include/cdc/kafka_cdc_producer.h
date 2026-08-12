/**
 * @file kafka_cdc_producer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB - CDC Kafka Producer
 *
 * Bridges the ThemisDB Change Data Capture (CDC) changefeed to an Apache Kafka
 * topic.  Each ChangeEvent is published with ChangeEvent::key as the Kafka
 * message key and ChangeEvent::toJson() (UTF-8) as the Kafka message value.
 *
 * Topic routing:
 *   - per-collection mode (default): one topic per collection,
 *     e.g. "themis.cdc.orders" for collection "orders".
 *   - single-topic mode: all events go to a single configurable topic.
 *
 * The producer requires librdkafka.  When THEMIS_ENABLE_KAFKA is not defined
 * at build time, every method is a documented no-op so the rest of the codebase
 * can reference KafkaCDCProducer unconditionally; the no-op stubs compile to
 * zero overhead and the Kafka dependency is entirely absent.
 *
 * Prometheus metrics:
 *   cdc_kafka_delivered_total  – events successfully acknowledged by the broker
 *   cdc_kafka_error_total      – delivery-error callbacks from librdkafka
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include "cdc/cdc_metrics.h"
#include "cdc/debezium_format.h"
#include "cdc/icdc_transport.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#ifdef THEMIS_ENABLE_KAFKA
#include <librdkafka/rdkafkacpp.h>
#endif

namespace themis {
namespace cdc {

// ── Configuration ─────────────────────────────────────────────────────────────

/**
 * @brief Kafka producer configuration for the CDC bridge.
 *
 * Credentials (sasl_username / sasl_password) must be loaded from
 * config/security/ paths via ConfigPathResolver::resolve(); they are never
 * logged even at DEBUG level.
 */
struct KafkaProducerConfig {
    /// Comma-separated list of initial broker addresses ("host:port,...").
    std::string brokers{"localhost:9092"};

    /// Prefix prepended to collection names to form topic names.
    /// Example: prefix "themis.cdc." + collection "orders" → "themis.cdc.orders".
    std::string topic_prefix{"themis.cdc."};

    /// When non-empty, all events are published to this single topic instead of
    /// per-collection topics.  The ChangeEvent::key is still used as message key.
    std::string single_topic{};

    /// librdkafka acks setting ("all" for strongest durability guarantees).
    std::string acks{"all"};

    /// Enable idempotent producer (requires acks=all; provides exactly-once on LAN).
    bool enable_idempotence{true};

    /// Delivery report poll interval in milliseconds.
    uint32_t poll_interval_ms{500};

    /// Maximum linger time before a batch is flushed (milliseconds).
    int linger_ms{5};

    /// Maximum in-flight requests per connection (must be 1 when idempotence=true).
    int max_in_flight{5};

    /// SASL mechanism ("PLAIN", "SCRAM-SHA-256", "SCRAM-SHA-512", or empty for none).
    std::string sasl_mechanism{};

    /// SASL username (load from config/security/; never log).
    std::string sasl_username{};

    /// SASL password (load from config/security/; never log).
    std::string sasl_password{};

    /// Security protocol ("plaintext", "ssl", "sasl_plaintext", "sasl_ssl").
    std::string security_protocol{"plaintext"};

    /// Path to CA certificate for TLS (empty = use system CA bundle).
    std::string ssl_ca_location{};

    /// Flush timeout when stopping the producer (milliseconds).
    uint32_t flush_timeout_ms{10000};

    /// When true, events are serialized as Debezium-compatible envelopes
    /// instead of the native ThemisDB JSON format.  Set this when publishing
    /// to topics consumed by Kafka Connect transforms or Debezium Server sinks.
    bool use_debezium_format{false};

    /// Debezium formatter configuration (used only when use_debezium_format is true).
    DebeziumFormatter::Config debezium_config{};
};

// ── Statistics ────────────────────────────────────────────────────────────────

/// Snapshot of KafkaCDCProducer counters.
struct KafkaProducerStats {
    uint64_t delivered_total{0};    ///< cdc_kafka_delivered_total
    uint64_t error_total{0};        ///< cdc_kafka_error_total
    uint64_t poll_cycles{0};        ///< number of poll() iterations
    bool     running{false};        ///< true while background thread is active
};

// ── Producer class ────────────────────────────────────────────────────────────

#ifdef THEMIS_ENABLE_KAFKA

/**
 * @brief Kafka-compatible CDC producer: polls the ThemisDB changefeed and
 *        publishes change events to Apache Kafka.
 *
 * Typical lifecycle:
 * @code
 *   KafkaProducerConfig cfg;
 *   cfg.brokers = "kafka-broker:9092";
 *   KafkaCDCProducer producer(&changefeed, cfg, &metrics);
 *   producer.start();
 *   // ... ThemisDB running ...
 *   producer.stop();
 * @endcode
 *
 * Thread-safety: start()/stop() must not be called concurrently.  getStats()
 * is safe to call from any thread.
 */
class KafkaCDCProducer : public ICDCTransport {
public:
    /**
     * @brief Construct the producer.
     * @param changefeed    Changefeed to poll (not owned; must outlive producer).
     * @param config        Producer configuration.
     * @param metrics       Optional shared metrics sink (not owned; may be null).
     */
    explicit KafkaCDCProducer(Changefeed* changefeed,
                               KafkaProducerConfig config = {},
                               CDCMetrics* metrics = nullptr);

    ~KafkaCDCProducer();

    // Non-copyable, non-movable.
    KafkaCDCProducer(const KafkaCDCProducer&) = delete;
    KafkaCDCProducer& operator=(const KafkaCDCProducer&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────

    /**
     * @brief Create the librdkafka producer and start the background polling
     *        thread.  No-op if already started.
     * @return true on success, false if librdkafka initialisation failed.
     */
    bool start() override;

    /**
     * @brief Flush pending messages, stop the background thread, and destroy
     *        the librdkafka producer.  No-op if already stopped.
     */
    void stop() override;

    // ── Manual publish ─────────────────────────────────────────────────────

    /**
     * @brief Publish a single ChangeEvent to Kafka immediately.
     *
     * Uses the topic selected by topicForEvent().  Non-blocking: delivery
     * confirmation is handled asynchronously via the delivery-report callback.
     *
     * @param event  Event to publish.
     * @return true if the message was enqueued, false on error.
     */
    bool publish(const Changefeed::ChangeEvent& event) override;

    // ── Observability ──────────────────────────────────────────────────────

    /**
     * @brief Return a snapshot of producer statistics.
     */
    KafkaProducerStats getStats() const;

private:
    // ── Internal helpers ───────────────────────────────────────────────────

    /// Returns the Kafka topic name for a given ChangeEvent.
    std::string topicForEvent(const Changefeed::ChangeEvent& event) const;

    /// Background polling thread: fetches new events and calls publish().
    void pollingThread();

    // ── Delivery report callback (friend class) ────────────────────────────

    class DeliveryReportCb : public RdKafka::DeliveryReportCb {
    public:
        explicit DeliveryReportCb(KafkaCDCProducer& producer) : producer_(producer) {}
        void dr_cb(RdKafka::Message& message) override;
    private:
        KafkaCDCProducer& producer_;
    };

    // ── Members ────────────────────────────────────────────────────────────

    Changefeed* changefeed_;
    KafkaProducerConfig config_;
    CDCMetrics* metrics_;  ///< Not owned; may be null.

    std::unique_ptr<DeliveryReportCb>   dr_cb_;
    std::unique_ptr<RdKafka::Producer>  producer_;

    /// Cache of RdKafka::Topic handles, keyed by topic name.
    mutable std::mutex topic_mutex_;
    std::unordered_map<std::string, std::unique_ptr<RdKafka::Topic>> topic_cache_;

    std::atomic<bool>  running_{false};
    std::thread        thread_;

    /// Sequence watermark: the producer polls events after this sequence.
    std::atomic<uint64_t> last_sequence_{0};

    // Prometheus counters (mirrored in CDCMetrics when metrics_ != null)
    std::atomic<uint64_t> delivered_total_{0};
    std::atomic<uint64_t> error_total_{0};
    std::atomic<uint64_t> poll_cycles_{0};

    /// Get or create a cached RdKafka::Topic handle.
    RdKafka::Topic* getOrCreateTopic(const std::string& topic_name);
};

#else // !THEMIS_ENABLE_KAFKA ── no-op stub ────────────────────────────────────

/**
 * @brief No-op stub compiled when THEMIS_ENABLE_KAFKA is not defined.
 *
 * All methods are inline no-ops so the rest of the codebase can reference
 * KafkaCDCProducer without introducing a Kafka dependency.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Preserve compile-time API compatibility without linking librdkafka.
 * Activation: Compiled when THEMIS_ENABLE_KAFKA is not defined.
 * Production Delta: start()/publish() delegate to injected fns when set;
 *   otherwise return false and no CDC events are emitted to Kafka.
 * Removal Plan: Keep as optional-build fallback; remove only if Kafka becomes a mandatory runtime dependency.
 */
class KafkaCDCProducer : public ICDCTransport {
public:
    explicit KafkaCDCProducer(Changefeed* /*changefeed*/,
                               KafkaProducerConfig /*config*/ = {},
                               CDCMetrics* /*metrics*/ = nullptr) {}
    ~KafkaCDCProducer() = default;

    KafkaCDCProducer(const KafkaCDCProducer&) = delete;
    KafkaCDCProducer& operator=(const KafkaCDCProducer&) = delete;

    bool start() override {
        StartFn fn;
        { std::lock_guard<std::mutex> lk(s_start_fn_mutex_()); fn = s_start_fn_(); }
        if (fn) { try { return fn(); } catch (...) { return false; } }
        return false;
    }

    void stop() override {}

    bool publish(const Changefeed::ChangeEvent& event) override {
        PublishFn fn;
        { std::lock_guard<std::mutex> lk(s_publish_fn_mutex_()); fn = s_publish_fn_(); }
        if (fn) { try { return fn(event); } catch (...) { return false; } }
        return false;
    }

    KafkaProducerStats getStats() const { return {}; }

    // -----------------------------------------------------------------------
    // Injectable bridge (STUB #98)
    // -----------------------------------------------------------------------
    /// Callback type for start(): return true when the Kafka producer has
    /// started successfully.
    using StartFn   = std::function<bool()>;
    /// Callback type for publish(): return true when the event was accepted
    /// by the Kafka producer.
    using PublishFn = std::function<bool(const Changefeed::ChangeEvent&)>;

    /// Register a start callback used by `start()` in non-Kafka builds.
    /// Pass an empty `std::function` to revert to the always-false fallback.
    /// Thread-safe.
    static void setStartFn(StartFn fn) {
        std::lock_guard<std::mutex> lk(s_start_fn_mutex_());
        s_start_fn_() = std::move(fn);
    }

    /// Register a publish callback used by `publish()` in non-Kafka builds.
    /// Pass an empty `std::function` to revert to the always-false fallback.
    /// Thread-safe.
    static void setPublishFn(PublishFn fn) {
        std::lock_guard<std::mutex> lk(s_publish_fn_mutex_());
        s_publish_fn_() = std::move(fn);
    }

private:
    // Static storage via function-local statics so they are lazily initialised
    // and avoid static-initialisation-order issues.
    static std::mutex&   s_start_fn_mutex_()   { static std::mutex m; return m; }
    static StartFn&      s_start_fn_()          { static StartFn f; return f; }
    static std::mutex&   s_publish_fn_mutex_()  { static std::mutex m; return m; }
    static PublishFn&    s_publish_fn_()         { static PublishFn f; return f; }
};

#endif // THEMIS_ENABLE_KAFKA

} // namespace cdc
} // namespace themis
