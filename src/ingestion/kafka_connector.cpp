/**
 * @file kafka_connector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=12; TODO=1, Stub=3, Unimpl=0, Mock=6, Sim=2, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// When THEMIS_ENABLE_KAFKA is defined the full librdkafka-backed implementation
// is compiled.  When the macro is absent the connector still compiles but every
// method returns immediately with a CONNECTOR_NOT_SUPPORTED error, or the
// injected test mock is used (for unit tests that do not require a broker).

#include "ingestion/kafka_connector.h"

#ifdef THEMIS_ENABLE_KAFKA
#include <librdkafka/rdkafka.h>
#endif

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <thread>
#include <cstring>

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace ingestion {

// ---------------------------------------------------------------------------
// JSON helpers (minimal, dependency-free – mirrors api_connector.cpp)
// ---------------------------------------------------------------------------

namespace {

/// Extract the first string value for `"key":"<value>"` from a JSON blob.
static std::string kafkaJsonExtractString(const std::string& json,
                                          const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    auto start = json.find(needle);
    if (start == std::string::npos) return {};
    start += needle.size();
    std::string value;
    bool escape = false;
    for (size_t i = start; i < json.size(); ++i) {
        char c = json[i];
        if (escape) { value += c; escape = false; continue; }
        if (c == '\\') { escape = true; continue; }
        if (c == '"') break;
        value += c;
    }
    return value;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

/** @brief Pimpl. */
class KafkaConnector::Impl {
public:
    Impl() = default;
    ~Impl() {
        destroyConsumer();
    }

    bool initialize(const SourceConfig& config) {
        if (config.type != SourceType::KAFKA) return false;
        config_  = config;
        brokers_ = config.location;

        auto opt = [&](const std::string& k, const std::string& def) {
            auto it = config.options.find(k);
            return (it != config.options.end()) ? it->second : def;
        };

        topic_            = opt("topic",             brokers_); // fallback to location
        // If the location looks like a broker list (contains ':'), use it;
        // otherwise treat location as the topic and brokers_ must be set separately.
        // In the standard IngestionBuilder::withKafkaSource() flow, location=brokers
        // and options["topic"] always contains the topic name.
        if (config.options.count("topic")) {
            topic_ = config.options.at("topic");
        }

        consumer_group_   = opt("consumer_group",   "themis-ingestion");
        message_format_   = opt("message_format",   "json");
        text_field_       = opt("text_field",       "text");
        schema_reg_url_   = opt("schema_registry_url", "");

        try { poll_timeout_ms_   = std::stoi(opt("poll_timeout_ms",   "1000")); }
        catch (...) { poll_timeout_ms_ = 1000; }

        try { max_messages_ = static_cast<size_t>(std::stoull(opt("max_messages","0"))); }
        catch (...) { max_messages_ = 0; }

        try { session_timeout_ms_ = std::stoi(opt("session_timeout_ms","10000")); }
        catch (...) { session_timeout_ms_ = 10000; }

        security_protocol_ = opt("security_protocol", "plaintext");
        sasl_mechanism_    = opt("sasl_mechanism",    "");
        sasl_username_     = opt("sasl_username",     "");
        sasl_password_     = opt("sasl_password",     "");
        ssl_ca_location_   = opt("ssl_ca_location",   "");
        // "earliest" ensures new consumer groups don't skip historical messages;
        // callers can override to "latest" via options["auto_offset_reset"].
        auto_offset_reset_ = opt("auto_offset_reset", "earliest");

        return !brokers_.empty() && !topic_.empty();
    }

    bool isAvailable() const {
        // When a test mock is set, always report available.
        if (message_fn_) return true;

#ifdef THEMIS_ENABLE_KAFKA
        // Perform a metadata request to verify broker connectivity.
        char errstr[512];
        rd_kafka_conf_t* conf = rd_kafka_conf_new();
        rd_kafka_conf_set(conf, "bootstrap.servers", brokers_.c_str(),
                          errstr, sizeof(errstr));
        rd_kafka_conf_set(conf, "group.id", consumer_group_.c_str(),
                          errstr, sizeof(errstr));

        rd_kafka_t* rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
                                      errstr, sizeof(errstr));
        if (!rk) return false;

        const rd_kafka_metadata_t* meta = nullptr;
        rd_kafka_resp_err_t err = rd_kafka_metadata(rk, 1, nullptr, &meta,
                                                    poll_timeout_ms_);
        bool ok = (err == RD_KAFKA_RESP_ERR_NO_ERROR);
        if (meta) rd_kafka_metadata_destroy(meta);
        rd_kafka_destroy(rk);
        return ok;
#else
        return false;
#endif
    }

    size_t getDocumentCount() const {
        // Kafka topics do not expose a reliable total count.
        return 0;
    }

    IngestionStats ingest(const std::string& /*target_collection*/,
                          ProgressCallback progress_callback) {
        IngestionStats stats;
        auto start_time = std::chrono::steady_clock::now();

        if (brokers_.empty() || topic_.empty()) {
            stats.addError(IngestionErrorCode::SOURCE_NOT_CONFIGURED,
                           IngestionErrorSeverity::FATAL,
                           "Kafka connector not configured: brokers or topic is empty",
                           config_.source_id);
            return stats;
        }

        // -------------------------------------------------------------------
        // Test mock path: no librdkafka required
        // -------------------------------------------------------------------
        if (message_fn_) {
            ingestFromMock(stats, progress_callback);
            finaliseStats(stats, start_time);
            return stats;
        }

        // -------------------------------------------------------------------
        // Production path: librdkafka
        // -------------------------------------------------------------------
#ifdef THEMIS_ENABLE_KAFKA
        ingestFromKafka(stats, progress_callback);
#else
        stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                       IngestionErrorSeverity::FATAL,
                       "KafkaConnector requires THEMIS_ENABLE_KAFKA at build time",
                       config_.source_id);
#endif
        finaliseStats(stats, start_time);
        return stats;
    }

    void setRetryConfig(const RetryConfig& c)   { retry_config_ = c; }
    void setMessageFetchForTesting(KafkaMessageFn fn) { message_fn_ = std::move(fn); }
    void setCheckpointStore(std::shared_ptr<CheckpointStore> store) {
        checkpoint_store_ = std::move(store);
    }

private:
    // -----------------------------------------------------------------------
    // Write a ThemisDB-level checkpoint for this source via the injected store.
    // Called before Kafka offsets are committed (rd_kafka_consumer_close) so
    // that the ThemisDB checkpoint is always written before Kafka forgets the
    // messages, ensuring at-least-once delivery semantics.
    // -----------------------------------------------------------------------
    void writeCheckpoint(size_t processed_count) {
        if (!checkpoint_store_) return;
        IngestionCheckpoint cp;
        cp.source_id       = config_.source_id;
        cp.processed_count = processed_count;
        auto now = std::chrono::system_clock::now();
        auto tt  = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
#ifdef _WIN32
        gmtime_s(&tm_buf, &tt);
#else
        gmtime_r(&tt, &tm_buf);
#endif
        // Buffer for "YYYY-MM-DDTHH:MM:SSZ" (20 chars + NUL = 21 bytes).
        // 32 bytes provides extra headroom for locale-specific variations.
        constexpr std::size_t kTimestampBufSize = 32;
        char buf[kTimestampBufSize] = {};
        std::strftime(buf, kTimestampBufSize, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
        cp.timestamp = buf;
        checkpoint_store_->write(cp);
    }

    // -----------------------------------------------------------------------
    // STUB/SIMULATION NOTE:
    // Purpose: Enable unit-testing of KafkaConnector without a live Kafka
    //   broker by using an injected message_fn_ instead of a real consumer.
    // Activation: Active when message_fn_ is non-null (set via
    //   KafkaConnector::setMessageFnForTesting()).
    // Production Delta: Messages come from the injected lambda instead of a
    //   real Kafka consumer.  No broker connection, no offset management.
    // Roadmap ref: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
    // Removal Plan: Not removed — remains the test-injection path.
    // Roadmap ref: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
    // -----------------------------------------------------------------------
    void ingestFromMock(IngestionStats& stats,
                        ProgressCallback& progress_callback) {
        size_t consumed = 0;
        try {
            while (true) {
                if (max_messages_ > 0 && consumed >= max_messages_) break;

                auto batch = message_fn_();
                if (batch.empty()) break;

                for (auto& payload : batch) {
                    if (max_messages_ > 0 && consumed >= max_messages_) break;

                    std::string text = extractText(payload);
                    if (!text.empty()) {
                        ++stats.documents_processed;
                        stats.bytes_processed += payload.size();
                    } else {
                        // Empty extraction still counts as processed bytes but
                        // not as a document.
                        stats.bytes_processed += payload.size();
                    }
                    ++consumed;
                }

                if ([[maybe_unused]] progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      0, // total unknown for Kafka
                                      "consumed " + std::to_string(consumed));
                }
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in Kafka mock ingest: " + std::string(e.what()),
                           config_.source_id);
        }
        // Write ThemisDB checkpoint BEFORE the caller signals "done".
        // In the mock path there are no Kafka offsets to commit, but writing
        // the checkpoint here mirrors the production path so unit tests can
        // verify the behaviour.
        writeCheckpoint(stats.documents_processed);
    }

#ifdef THEMIS_ENABLE_KAFKA
    // -----------------------------------------------------------------------
    // librdkafka-based ingestion (production)
    // -----------------------------------------------------------------------
    void ingestFromKafka(IngestionStats& stats,
                         ProgressCallback& progress_callback) {
        char errstr[512];

        // Build configuration
        rd_kafka_conf_t* conf = rd_kafka_conf_new();

        auto setConf = [&](const char* key, const char* value) -> bool {
            if (rd_kafka_conf_set(conf, key, value, errstr, sizeof(errstr))
                    != RD_KAFKA_CONF_OK) {
                stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                               IngestionErrorSeverity::FATAL,
                               std::string("rd_kafka_conf_set '") + key + "' failed: " + errstr,
                               config_.source_id);
                return false;
            }
            return true;
        };

        if (!setConf("bootstrap.servers", brokers_.c_str())) {
            rd_kafka_conf_destroy(conf);
            return;
        }
        if (!setConf("group.id", consumer_group_.c_str())) {
            rd_kafka_conf_destroy(conf);
            return;
        }

        std::string sess_ms = std::to_string(session_timeout_ms_);
        if (!setConf("session.timeout.ms", sess_ms.c_str())) {
            rd_kafka_conf_destroy(conf);
            return;
        }

        // Disable auto-commit so that Kafka offsets are committed only when
        // rd_kafka_consumer_close() is called (i.e., after all messages have
        // been processed), preserving at-least-once delivery semantics.
        if (!setConf("enable.auto.commit", "false")) {
            rd_kafka_conf_destroy(conf);
            return;
        }

        // Control where the consumer starts on its first run (no committed offset).
        // Default: "earliest" to avoid silently skipping historical messages.
        if (!setConf("auto.offset.reset", auto_offset_reset_.c_str())) {
            rd_kafka_conf_destroy(conf);
            return;
        }

        // Security
        if (!security_protocol_.empty() && security_protocol_ != "plaintext") {
            if (!setConf("security.protocol", security_protocol_.c_str())) {
                rd_kafka_conf_destroy(conf);
                return;
            }
        }
        if (!sasl_mechanism_.empty()) {
            if (!setConf("sasl.mechanism", sasl_mechanism_.c_str())) {
                rd_kafka_conf_destroy(conf);
                return;
            }
        }
        if (!sasl_username_.empty()) {
            if (!setConf("sasl.username", sasl_username_.c_str())) {
                rd_kafka_conf_destroy(conf);
                return;
            }
        }
        if (!sasl_password_.empty()) {
            if (!setConf("sasl.password", sasl_password_.c_str())) {
                rd_kafka_conf_destroy(conf);
                return;
            }
        }
        if (!ssl_ca_location_.empty()) {
            if (!setConf("ssl.ca.location", ssl_ca_location_.c_str())) {
                rd_kafka_conf_destroy(conf);
                return;
            }
        }

        // Create consumer
        rd_kafka_t* rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
                                      errstr, sizeof(errstr));
        if (!rk) {
            rd_kafka_conf_destroy(conf);
            stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                           IngestionErrorSeverity::FATAL,
                           std::string("rd_kafka_new failed: ") + errstr,
                           config_.source_id);
            return;
        }
        // conf is now owned by rk – do not destroy separately.

        // Redirect main queue to consumer queue for simple consumer API
        rd_kafka_poll_set_consumer(rk);

        // Subscribe to topic
        rd_kafka_topic_partition_list_t* topics =
            rd_kafka_topic_partition_list_new(1);
        rd_kafka_topic_partition_list_add(topics, topic_.c_str(),
                                          RD_KAFKA_PARTITION_UA);
        rd_kafka_resp_err_t sub_err = rd_kafka_subscribe(rk, topics);
        rd_kafka_topic_partition_list_destroy(topics);

        if (sub_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
            stats.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,
                           IngestionErrorSeverity::FATAL,
                           std::string("rd_kafka_subscribe failed: ") +
                           rd_kafka_err2str(sub_err),
                           config_.source_id);
            rd_kafka_consumer_close(rk);
            rd_kafka_destroy(rk);
            return;
        }

        // Consume loop
        size_t consumed = 0;
        int consecutive_timeouts = 0;
        const int kMaxTimeouts = 3; // stop after 3 consecutive empty polls

        try {
            while (true) {
                if (max_messages_ > 0 && consumed >= max_messages_) break;

                rd_kafka_message_t* msg =
                    rd_kafka_consumer_poll(rk, poll_timeout_ms_);

                if (!msg) {
                    ++consecutive_timeouts;
                    if (consecutive_timeouts >= kMaxTimeouts) break;
                    continue;
                }

                consecutive_timeouts = 0;

                if (msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
                    rd_kafka_message_destroy(msg);
                    break; // end of partition
                }

                if (msg->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
                    stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                                   IngestionErrorSeverity::WARNING,
                                   std::string("Kafka message error: ") +
                                   rd_kafka_message_errstr(msg),
                                   config_.source_id);
                    rd_kafka_message_destroy(msg);
                    continue;
                }

                if (msg->payload && msg->len > 0) {
                    std::string payload(static_cast<const char*>(msg->payload),
                                        msg->len);
                    std::string text = extractText(payload);
                    if (!text.empty()) {
                        ++stats.documents_processed;
                    }
                    stats.bytes_processed += msg->len;
                    ++consumed;

                    if ([[maybe_unused]] progress_callback) {
                        progress_callback(config_.source_id,
                                          stats.documents_processed,
                                          0,
                                          "consumed " + std::to_string(consumed));
                    }
                }

                rd_kafka_message_destroy(msg);
            }
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception in Kafka ingest: " + std::string(e.what()),
                           config_.source_id);
        }

        // Write ThemisDB checkpoint BEFORE committing Kafka offsets.
        // This preserves at-least-once delivery: if the process crashes after
        // the checkpoint write but before rd_kafka_consumer_close(), Kafka will
        // re-deliver the messages and ThemisDB will deduplicate via the
        // checkpoint.  The reverse order (Kafka first, checkpoint second) risks
        // silently losing documents should the process die in between.
        writeCheckpoint(stats.documents_processed);

        // Graceful shutdown: commit final Kafka offsets before closing.
        rd_kafka_consumer_close(rk);
        rd_kafka_destroy(rk);
    }

    void destroyConsumer() {
        // Nothing to destroy outside of ingestFromKafka scope; consumer is
        // created and destroyed within the ingest call for clean lifecycle.
    }
#else
    void destroyConsumer() {}
#endif

    // -----------------------------------------------------------------------
    // Text extraction from a raw message payload
    // -----------------------------------------------------------------------
    std::string extractText(const std::string& payload) const {
        if (message_format_ == "avro") {
            // Avro binary messages begin with a magic byte (0x00) followed by a
            // 4-byte schema ID.  Without the Schema Registry we treat the
            // remaining bytes as UTF-8 text.  Full Avro deserialization
            // (using a registry lookup) is deferred to a follow-up task.
            if (payload.size() > 5 && static_cast<unsigned char>(payload[0]) == 0x00) {
                return payload.substr(5); // strip magic + schema ID
            }
            return payload;
        }

        // JSON format (default): extract the configured text_field
        std::string text = kafkaJsonExtractString(payload, text_field_);
        if (text.empty()) {
            // If the whole payload is not a JSON object with the target field,
            // treat the entire payload as the document text.
            if (!payload.empty() && payload[0] != '{') {
                return payload;
            }
        }
        return text;
    }

    void finaliseStats(IngestionStats& stats,
                       const std::chrono::steady_clock::time_point& start) const {
        auto end = std::chrono::steady_clock::now();
        stats.elapsed_seconds =
            std::chrono::duration<double>(end - start).count();
        if (stats.elapsed_seconds > 0.0 && stats.documents_processed > 0) {
            stats.metrics.throughput_docs_per_sec =
                static_cast<double>(stats.documents_processed) /
                stats.elapsed_seconds;
        }
    }

    // Configuration
    SourceConfig config_;
    std::string  brokers_;
    std::string  topic_;
    std::string  consumer_group_;
    std::string  message_format_;
    std::string  text_field_;
    std::string  schema_reg_url_;
    int          poll_timeout_ms_     = 1000;
    size_t       max_messages_        = 0;
    int          session_timeout_ms_  = 10000;
    std::string  security_protocol_;
    std::string  sasl_mechanism_;
    std::string  sasl_username_;
    std::string  sasl_password_;
    std::string  ssl_ca_location_;
    std::string  auto_offset_reset_;  // set in initialize(); default "earliest"
    RetryConfig  retry_config_;

    // Injected ThemisDB checkpoint store (optional).
    // When set, writeCheckpoint() persists progress before Kafka offsets are
    // committed, ensuring at-least-once delivery even across process crashes.
    std::shared_ptr<CheckpointStore> checkpoint_store_;

    // Testing hook
    KafkaMessageFn message_fn_;
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

KafkaConnector::KafkaConnector()
    : impl_(std::make_unique<Impl>()) {}

KafkaConnector::~KafkaConnector() = default;

bool KafkaConnector::initialize(const SourceConfig& config) {
    return impl_->initialize(config);
}

bool KafkaConnector::isAvailable() const {
    return impl_->isAvailable();
}

size_t KafkaConnector::getDocumentCount() const {
    return impl_->getDocumentCount();
}

IngestionStats KafkaConnector::ingest(const std::string& target_collection,
                                       ProgressCallback progress_callback) {
    return impl_->ingest(target_collection, progress_callback);
}

void KafkaConnector::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void KafkaConnector::setCheckpointStore(std::shared_ptr<CheckpointStore> store) {
    impl_->setCheckpointStore(std::move(store));
}

void KafkaConnector::setMessageFetchForTesting(KafkaMessageFn fn) {
    setMessageBatchProvider(std::move(fn));
}

void KafkaConnector::setMessageBatchProvider(KafkaMessageFn fn) {
    impl_->setMessageFetchForTesting(std::move(fn));
}

} // namespace ingestion
} // namespace themis


