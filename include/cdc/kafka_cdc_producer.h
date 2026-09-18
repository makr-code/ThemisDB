/**
 * @file kafka_cdc_producer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
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

struct KafkaProducerConfig {
    std::string brokers{"localhost:9092"};

    std::string topic_prefix{"themis.cdc."};

    std::string single_topic{};

    std::string acks{"all"};

    bool enable_idempotence{true};

    uint32_t poll_interval_ms{500};

    int linger_ms{5};

    int max_in_flight{5};

    std::string sasl_mechanism{};

    std::string sasl_username{};

    std::string sasl_password{};

    std::string security_protocol{"plaintext"};

    std::string ssl_ca_location{};

    uint32_t flush_timeout_ms{10000};

    bool use_debezium_format{false};

    DebeziumFormatter::Config debezium_config{};
};

// ── Statistics ────────────────────────────────────────────────────────────────

struct KafkaProducerStats {
    uint64_t delivered_total{0};    ///< cdc_kafka_delivered_total
    uint64_t error_total{0};        ///< cdc_kafka_error_total
    uint64_t poll_cycles{0};        ///< number of poll() iterations
    bool     running{false};        ///< true while background thread is active
};

// ── Producer class ────────────────────────────────────────────────────────────

#ifdef THEMIS_ENABLE_KAFKA

class KafkaCDCProducer : public ICDCTransport {
public:
    explicit KafkaCDCProducer(Changefeed* changefeed,
                               KafkaProducerConfig config = {},
                               CDCMetrics* metrics = nullptr);

    ~KafkaCDCProducer();

    // Non-copyable, non-movable.
    KafkaCDCProducer(const KafkaCDCProducer&) = delete;
    KafkaCDCProducer& operator=(const KafkaCDCProducer&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────

    bool start() override;

    void stop() override;

    // ── Manual publish ─────────────────────────────────────────────────────

    bool publish(const Changefeed::ChangeEvent& event) override;

    /**
     * @brief ── Observability ──────────────────────────────────────────────────────
     * @return Return value.
     */

    KafkaProducerStats getStats() const;

private:
    /**
     * @brief ── Internal helpers ───────────────────────────────────────────────────
     * @param[in] event Input parameter.
     * @return Return value.
     */

    std::string topicForEvent(const Changefeed::ChangeEvent& event) const;

    /**
     * @brief Polling Thread.
     */
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

    mutable std::mutex topic_mutex_;
    std::unordered_map<std::string, std::unique_ptr<RdKafka::Topic>> topic_cache_;

    std::atomic<bool>  running_{false};
    std::thread        thread_;

    std::atomic<uint64_t> last_sequence_{0};

    // Prometheus counters (mirrored in CDCMetrics when metrics_ != null)
    std::atomic<uint64_t> delivered_total_{0};
    std::atomic<uint64_t> error_total_{0};
    std::atomic<uint64_t> poll_cycles_{0};

    /**
     * @brief Get Or Create Topic.
     * @param[in] topic_name Name of the topic.
     * @return Pointer to the result.
     */
    RdKafka::Topic* getOrCreateTopic(const std::string& topic_name);
};

#else // !THEMIS_ENABLE_KAFKA ── no-op stub ────────────────────────────────────

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
    using StartFn   = std::function<bool()>;
    using PublishFn = std::function<bool(const Changefeed::ChangeEvent&)>;

    /**
     * @brief Set Start Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), s_start_fn_mutex_(), s_start_fn_(), std::move().
     */
    static void setStartFn(StartFn fn) {
        std::lock_guard<std::mutex> lk(s_start_fn_mutex_());
        s_start_fn_() = std::move(fn);
    }

    /**
     * @brief Set Publish Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lk(), s_publish_fn_mutex_(), s_publish_fn_(), std::move().
     */
    static void setPublishFn(PublishFn fn) {
        std::lock_guard<std::mutex> lk(s_publish_fn_mutex_());
        s_publish_fn_() = std::move(fn);
    }

private:
    /**
     * @brief Static storage via function-local statics so they are lazily initialised and avoid static-initialisation-order issues.
     * @return Return value.
     * @details Implements s_start_fn_mutex_ without additional internal calls.
     */
    static std::mutex&   s_start_fn_mutex_()   { static std::mutex m; return m; }
    /**
     * @brief S start fn.
     * @return Return value.
     * @details Implements s_start_fn_ without additional internal calls.
     */
    static StartFn&      s_start_fn_()          { static StartFn f; return f; }
    /**
     * @brief S publish fn mutex.
     * @return Return value.
     * @details Implements s_publish_fn_mutex_ without additional internal calls.
     */
    static std::mutex&   s_publish_fn_mutex_()  { static std::mutex m; return m; }
    /**
     * @brief S publish fn.
     * @return Return value.
     * @details Implements s_publish_fn_ without additional internal calls.
     */
    static PublishFn&    s_publish_fn_()         { static PublishFn f; return f; }
};

#endif // THEMIS_ENABLE_KAFKA

} // namespace cdc
} // namespace themis
