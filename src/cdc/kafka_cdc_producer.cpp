/**
 * @file kafka_cdc_producer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB - CDC Kafka Producer (implementation)
 *
 * See include/cdc/kafka_cdc_producer.h for class documentation.
 *
 * When THEMIS_ENABLE_KAFKA is defined, this file provides the full librdkafka-
 * backed implementation.  When the macro is absent the translation unit is
 * intentionally empty; the no-op stub is defined inline in the header.
 *
 * PERMANENT HARDWARE FALLBACK NOTE (librdkafka not available):
 * Purpose: Allow ThemisDB to be built without librdkafka.  All
 *   KafkaCdcProducer methods are defined inline as no-ops in
 *   include/cdc/kafka_cdc_producer.h when `THEMIS_ENABLE_KAFKA` is not set.
 *   This lets CDC change-event publishing be compiled out cleanly without
 *   affecting the rest of the CDC pipeline.
 * Activation: `THEMIS_ENABLE_KAFKA` not defined at compile time (default for
 *   CPU-only / bare-metal builds without a Kafka broker).
 * Production Delta: Header fallback path handles the non-Kafka behavior:
 *   start()/publish() return false by default, and can be explicitly bridged
 *   via injected callbacks in tests/dev builds (see STUB #98 bridge APIs).
 *   Downstream consumers will not receive real-time change feeds unless a
 *   real Kafka backend is enabled.
 * Hardware requirement: librdkafka (`apt install librdkafka-dev`) +
 *   `-DTHEMIS_ENABLE_KAFKA=1` in CMake.
 * Roadmap ref: src/cdc/FUTURE_ENHANCEMENTS.md §"Kafka CDC Producer Activation"
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef THEMIS_ENABLE_KAFKA

#include "cdc/kafka_cdc_producer.h"
#include "cdc/debezium_format.h"
#include "utils/logger.h"

#include <chrono>
#include <thread>

namespace themis {
namespace cdc {

// ── DeliveryReportCb ──────────────────────────────────────────────────────────

void KafkaCDCProducer::DeliveryReportCb::dr_cb(RdKafka::Message& message) {
    if (message.err() != RdKafka::ERR_NO_ERROR) {
        THEMIS_WARN("KafkaCDCProducer: delivery error for key='{}' topic='{}': {}",
                    message.key() ? *message.key() : "<none>",
                    message.topic_name(),
                    message.errstr());
        ++producer_.error_total_;
        if (producer_.metrics_) {
            ++producer_.metrics_->kafka_error_total;
        }
    } else {
        ++producer_.delivered_total_;
        if (producer_.metrics_) {
            ++producer_.metrics_->kafka_delivered_total;
        }
    }
}

// ── Construction / destruction ────────────────────────────────────────────────

KafkaCDCProducer::KafkaCDCProducer(Changefeed* changefeed,
                                     KafkaProducerConfig config,
                                     CDCMetrics* metrics)
    : changefeed_(changefeed)
    , config_(std::move(config))
    , metrics_(metrics)
{
    THEMIS_INFO("KafkaCDCProducer created (brokers={}, topic_prefix={}, single_topic='{}')",
                config_.brokers, config_.topic_prefix,
                config_.single_topic.empty() ? "(per-collection)" : config_.single_topic);
}

KafkaCDCProducer::~KafkaCDCProducer() {
    stop();
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

bool KafkaCDCProducer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return true;  // Already running.
    }

    if (!changefeed_) {
        THEMIS_ERROR("KafkaCDCProducer: cannot start without a valid Changefeed");
        return false;
    }

    // ── Build librdkafka configuration ──────────────────────────────────────
    std::string errstr;
    std::unique_ptr<RdKafka::Conf> conf(
        RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));

    auto set = [&](const std::string& key, const std::string& value) -> bool {
        if (conf->set(key, value, errstr) != RdKafka::Conf::CONF_OK) {
            THEMIS_ERROR("KafkaCDCProducer: failed to set '{}': {}", key, errstr);
            return false;
        }
        return true;
    };

    if (!set("bootstrap.servers", config_.brokers)) return false;
    if (!set("acks", config_.acks)) return false;
    if (!set("enable.idempotence",
             config_.enable_idempotence ? "true" : "false")) return false;
    if (!set("linger.ms", std::to_string(config_.linger_ms))) return false;
    if (!set("max.in.flight.requests.per.connection",
             std::to_string(config_.max_in_flight))) return false;

    if (!config_.security_protocol.empty()) {
        if (!set("security.protocol", config_.security_protocol)) return false;
    }
    if (!config_.sasl_mechanism.empty()) {
        if (!set("sasl.mechanism", config_.sasl_mechanism)) return false;
    }
    if (!config_.sasl_username.empty()) {
        if (!set("sasl.username", config_.sasl_username)) return false;
    }
    if (!config_.sasl_password.empty()) {
        // Password is set but never logged.
        if (!set("sasl.password", config_.sasl_password)) return false;
    }
    if (!config_.ssl_ca_location.empty()) {
        if (!set("ssl.ca.location", config_.ssl_ca_location)) return false;
    }

    // Register delivery-report callback.
    dr_cb_ = std::make_unique<DeliveryReportCb>(*this);
    if (conf->set("dr_cb", dr_cb_.get(), errstr) != RdKafka::Conf::CONF_OK) {
        THEMIS_ERROR("KafkaCDCProducer: failed to set dr_cb: {}", errstr);
        return false;
    }

    // ── Create producer ─────────────────────────────────────────────────────
    producer_.reset(RdKafka::Producer::create(conf.get(), errstr));
    if (!producer_) {
        THEMIS_ERROR("KafkaCDCProducer: failed to create producer: {}", errstr);
        return false;
    }

    // Initialise sequence watermark from the latest already-processed event.
    last_sequence_.store(changefeed_->getLatestSequence(), std::memory_order_relaxed);

    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&KafkaCDCProducer::pollingThread, this);

    THEMIS_INFO("KafkaCDCProducer: started (brokers={})", config_.brokers);
    return true;
}

void KafkaCDCProducer::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;  // Already stopped.
    }

    if (thread_.joinable()) {
        thread_.join();
    }

    if (producer_) {
        // Flush remaining queued messages before destruction.
        RdKafka::ErrorCode rc = producer_->flush(
            static_cast<int>(config_.flush_timeout_ms));
        if (rc != RdKafka::ERR_NO_ERROR) {
            THEMIS_WARN("KafkaCDCProducer: flush timed out ({}); {} message(s) may not have been delivered",
                        RdKafka::err2str(rc),
                        producer_->outq_len());
        }
        producer_.reset();
    }

    {
        std::lock_guard<std::mutex> lk(topic_mutex_);
        topic_cache_.clear();
    }

    dr_cb_.reset();
    THEMIS_INFO("KafkaCDCProducer: stopped");
}

// ── Topic resolution ──────────────────────────────────────────────────────────

std::string KafkaCDCProducer::topicForEvent(
    const Changefeed::ChangeEvent& event) const
{
    if (!config_.single_topic.empty()) {
        return config_.single_topic;
    }
    // Derive collection name from key: "collection:rest-of-key".
    const auto colon = event.key.find(':');
    const std::string collection =
        (colon != std::string::npos) ? event.key.substr(0, colon) : event.key;
    return config_.topic_prefix + collection;
}

RdKafka::Topic* KafkaCDCProducer::getOrCreateTopic(
    const std::string& topic_name)
{
    std::lock_guard<std::mutex> lk(topic_mutex_);
    auto it = topic_cache_.find(topic_name);
    if (it != topic_cache_.end()) {
        return it->second.get();
    }

    std::string errstr;
    std::unique_ptr<RdKafka::Conf> tconf(
        RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
    std::unique_ptr<RdKafka::Topic> topic(
        RdKafka::Topic::create(producer_.get(), topic_name, tconf.get(), errstr));

    if (!topic) {
        THEMIS_ERROR("KafkaCDCProducer: failed to create topic handle '{}': {}",
                     topic_name, errstr);
        return nullptr;
    }

    RdKafka::Topic* raw = topic.get();
    topic_cache_.emplace(topic_name, std::move(topic));
    return raw;
}

// ── Manual publish ────────────────────────────────────────────────────────────

bool KafkaCDCProducer::publish(const Changefeed::ChangeEvent& event) {
    if (!producer_) {
        return false;
    }

    const std::string topic_name = topicForEvent(event);
    RdKafka::Topic* topic = getOrCreateTopic(topic_name);
    if (!topic) {
        return false;
    }

    // Serialise value to UTF-8.
    // Use Debezium envelope format when configured; fall back to native JSON.
    std::string payload;
    if (config_.use_debezium_format) {
        DebeziumFormatter fmt(config_.debezium_config);
        // Pass an empty collection name so the formatter derives it from the
        // event key prefix (consistent with topicForEvent() routing logic).
        payload = fmt.toJson(event).dump();
    } else {
        payload = event.toJson().dump();
    }
    const std::string& key = event.key;

    RdKafka::ErrorCode rc = producer_->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,         // automatic partition assignment
        RdKafka::Producer::RK_MSG_COPY,       // copy payload
        const_cast<char*>(payload.data()),
        payload.size(),
        &key,
        nullptr);                             // no opaque

    if (rc != RdKafka::ERR_NO_ERROR) {
        if (rc == RdKafka::ERR__QUEUE_FULL) {
            // Back-pressure: drain queue and retry once.
            producer_->poll(100 /* ms */);
            rc = producer_->produce(
                topic,
                RdKafka::Topic::PARTITION_UA,
                RdKafka::Producer::RK_MSG_COPY,
                const_cast<char*>(payload.data()),
                payload.size(),
                &key,
                nullptr);
        }
        if (rc != RdKafka::ERR_NO_ERROR) {
            THEMIS_WARN("KafkaCDCProducer: produce failed for key='{}': {}",
                        key, RdKafka::err2str(rc));
            ++error_total_;
            if (metrics_) ++metrics_->kafka_error_total;
            return false;
        }
    }

    // Allow librdkafka to handle delivery reports without blocking.
    producer_->poll(0);
    return true;
}

// ── Background polling thread ─────────────────────────────────────────────────

void KafkaCDCProducer::pollingThread() {
    const auto interval =
        std::chrono::milliseconds(config_.poll_interval_ms);

    THEMIS_INFO("KafkaCDCProducer: polling thread started "
                "(interval={}ms, from_sequence={})",
                config_.poll_interval_ms,
                last_sequence_.load(std::memory_order_relaxed));

    while (running_.load(std::memory_order_acquire)) {
        ++poll_cycles_;

        Changefeed::ListOptions opts;
        opts.from_sequence = last_sequence_.load(std::memory_order_relaxed);
        opts.limit         = 1000;  // Batch up to 1 000 events per poll cycle.

        std::vector<Changefeed::ChangeEvent> events =
            changefeed_->listEvents(opts);

        for (const auto& ev : events) {
            if (publish(ev)) {
                last_sequence_.store(ev.sequence, std::memory_order_relaxed);
            }
        }

        // Give librdkafka time to service delivery reports.
        if (producer_) {
            producer_->poll(static_cast<int>(config_.poll_interval_ms));
        }

        if (events.empty()) {
            std::this_thread::sleep_for(interval);
        }
    }

    THEMIS_INFO("KafkaCDCProducer: polling thread stopped");
}

// ── Observability ─────────────────────────────────────────────────────────────

KafkaProducerStats KafkaCDCProducer::getStats() const {
    KafkaProducerStats s;
    s.delivered_total = delivered_total_.load(std::memory_order_relaxed);
    s.error_total     = error_total_.load(std::memory_order_relaxed);
    s.poll_cycles     = poll_cycles_.load(std::memory_order_relaxed);
    s.running         = running_.load(std::memory_order_relaxed);
    return s;
}

} // namespace cdc
} // namespace themis

#endif // THEMIS_ENABLE_KAFKA

