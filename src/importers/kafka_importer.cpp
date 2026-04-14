/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            kafka_importer.cpp                                 ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:33:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     755                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 59453d3ae6  2026-02-28  feat(importers): Add Kafka consumer importer for real-tim... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// When THEMIS_ENABLE_KAFKA is defined the full librdkafka-backed implementation
// is compiled.  When the macro is absent the importer still compiles but every
// importData() call returns immediately with an UNKNOWN error describing the
// missing build flag.  The mock injection path (setMessageFetchForTesting) and
// the URL-parsing helper are available in all build configurations.

#include "importers/kafka_importer.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_KAFKA
#include <librdkafka/rdkafka.h>
#endif

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace importers {

// ============================================================================
// Constructor / Destructor
// ============================================================================

KafkaImporter::KafkaImporter() = default;

KafkaImporter::~KafkaImporter() {
    cancel();
}

// ============================================================================
// IImporter interface
// ============================================================================

std::vector<std::string> KafkaImporter::getSupportedTypes() const {
    return {"kafka", "kafka-json", "kafka-avro", "kafka-plaintext"};
}

bool KafkaImporter::initialize(const std::string& config) {
    cancelled_ = false;

    if (config.empty() || config == "{}") {
        THEMIS_INFO("Kafka Importer initialized with default config");
        return true;
    }

    try {
        auto cfg = json::parse(config);

        auto opt_str = [&](const std::string& key, std::string& field,
                           const std::string& def) {
            if (cfg.contains(key) && cfg[key].is_string())
                field = cfg[key].get<std::string>();
            else
                field = def;
        };

        opt_str("brokers",           default_brokers_,    default_brokers_);
        opt_str("consumer_group",    consumer_group_,     "themis-import");
        opt_str("message_format",    message_format_,     "json");
        opt_str("text_field",        text_field_,         "text");
        opt_str("security_protocol", security_protocol_,  "plaintext");
        opt_str("sasl_mechanism",    sasl_mechanism_,     "");
        opt_str("sasl_username",     sasl_username_,      "");
        opt_str("sasl_password",     sasl_password_,      "");
        opt_str("ssl_ca_location",   ssl_ca_location_,    "");
        opt_str("auto_offset_reset", auto_offset_reset_,  "earliest");

        if (cfg.contains("poll_timeout_ms") && cfg["poll_timeout_ms"].is_number_integer())
            poll_timeout_ms_ = cfg["poll_timeout_ms"].get<int>();
        if (cfg.contains("max_messages") && cfg["max_messages"].is_number_integer())
            max_messages_ = static_cast<size_t>(cfg["max_messages"].get<uint64_t>());
        if (cfg.contains("session_timeout_ms") && cfg["session_timeout_ms"].is_number_integer())
            session_timeout_ms_ = cfg["session_timeout_ms"].get<int>();

    } catch (const std::exception& e) {
        THEMIS_WARN("Kafka Importer: failed to parse config JSON: {}", e.what());
        return false;
    }

    THEMIS_INFO("Kafka Importer initialized");
    return true;
}

bool KafkaImporter::validateSource(const std::string& source_path,
                                    std::vector<std::string>& errors) {
    std::string brokers, topic;
    if (!parseKafkaUrl(source_path, brokers, topic)) {
        errors.push_back("Invalid Kafka source URL: expected 'kafka://broker:port/topic' "
                         "or a bare topic name. Got: " + source_path);
        return false;
    }

    // Use default brokers from config if the URL didn't supply any.
    if (brokers.empty()) {
        brokers = default_brokers_;
    }

    if (brokers.empty()) {
        errors.push_back("No broker list found. Set 'brokers' in initialize() config "
                         "or use 'kafka://broker:port/topic' URL format.");
        return false;
    }

    if (topic.empty()) {
        errors.push_back("Topic name is empty.");
        return false;
    }

#ifdef THEMIS_ENABLE_KAFKA
    // Lightweight metadata request to verify broker connectivity.
    char errstr[512];
    rd_kafka_conf_t* conf = rd_kafka_conf_new();
    rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(),
                      errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "group.id", consumer_group_.c_str(),
                      errstr, sizeof(errstr));

    rd_kafka_t* rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
                                  errstr, sizeof(errstr));
    if (!rk) {
        errors.push_back(std::string("rd_kafka_new failed: ") + errstr);
        return false;
    }

    const rd_kafka_metadata_t* meta = nullptr;
    rd_kafka_resp_err_t err = rd_kafka_metadata(rk, 1, nullptr, &meta,
                                                poll_timeout_ms_);
    bool ok = (err == RD_KAFKA_RESP_ERR_NO_ERROR);
    if (meta) rd_kafka_metadata_destroy(meta);
    rd_kafka_destroy(rk);

    if (!ok) {
        errors.push_back("Kafka broker unreachable: " + brokers +
                         " (" + rd_kafka_err2str(err) + ")");
        return false;
    }
#else
    // Without librdkafka we can only validate the URL format.
#endif

    return true;
}

ImportStats KafkaImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();

    // Permission check
    if (options.permission_check &&
        !options.permission_check("import", "write")) {
        addError(stats, ImportErrorCode::PERMISSION_DENIED,
                 ImportErrorSeverity::CRITICAL,
                 "Permission denied by permission_check callback");
        return stats;
    }

    std::string brokers, topic;
    if (!parseKafkaUrl(source_path, brokers, topic)) {
        addError(stats, ImportErrorCode::FILE_NOT_FOUND,
                 ImportErrorSeverity::CRITICAL,
                 "Invalid Kafka source URL: " + source_path);
        return stats;
    }

    if (brokers.empty()) brokers = default_brokers_;

    if (brokers.empty() && !message_fn_) {
        addError(stats, ImportErrorCode::FILE_NOT_FOUND,
                 ImportErrorSeverity::CRITICAL,
                 "No broker list configured. Set 'brokers' in initialize() or "
                 "use 'kafka://broker:port/topic' URL.");
        return stats;
    }

    if (topic.empty()) {
        addError(stats, ImportErrorCode::FILE_NOT_FOUND,
                 ImportErrorSeverity::CRITICAL,
                 "Topic name is empty in source URL: " + source_path);
        return stats;
    }

    THEMIS_INFO("Kafka Importer: starting import from topic '{}' (brokers: '{}', "
                "dry_run: {})", topic, brokers.empty() ? "<mock>" : brokers,
                options.dry_run);

    emitSpan(options, "import_total",
             {{"source", "kafka"}, {"topic", topic}}, 0.0);

    cancelled_ = false;

    if (message_fn_) {
        consumeFromMock(topic, options, stats, progress_callback);
    } else {
#ifdef THEMIS_ENABLE_KAFKA
        consumeFromKafka(brokers, topic, options, stats, progress_callback);
#else
        addError(stats, ImportErrorCode::UNKNOWN,
                 ImportErrorSeverity::CRITICAL,
                 "KafkaImporter requires THEMIS_ENABLE_KAFKA at build time");
#endif
    }

    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds =
        std::chrono::duration<double>(end_time - start_time).count();

    emitMetric(options, "themisdb_import_rows_total",
               {{"source", "kafka"}, {"topic", topic},
                {"status", "imported"}},
               static_cast<double>(stats.imported_records));
    emitMetric(options, "themisdb_import_rows_total",
               {{"source", "kafka"}, {"topic", topic}, {"status", "failed"}},
               static_cast<double>(stats.failed_records));
    emitMetric(options, "themisdb_import_duration_seconds",
               {{"source", "kafka"}, {"topic", topic}},
               stats.elapsed_seconds);

    emitSpan(options, "import_total",
             {{"source", "kafka"}, {"topic", topic},
              {"rows", std::to_string(stats.imported_records)}},
             stats.elapsed_seconds);

    THEMIS_INFO("Kafka Importer: completed – {} imported, {} failed, {:.3f}s",
                stats.imported_records, stats.failed_records,
                stats.elapsed_seconds);

    return stats;
}

std::shared_ptr<ImportHandle> KafkaImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options
) {
    auto handle = std::make_shared<ImportHandle>();

    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        handle->id = "kafka-import-" + std::to_string(ms) + "-" +
                     std::to_string(reinterpret_cast<uintptr_t>(handle.get()) & 0xFFFF);
    }
    handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    handle->running.store(true);
    handle->setStage("pending");

    auto promise = std::make_shared<std::promise<ImportStats>>();
    handle->future = promise->get_future().share();

    std::weak_ptr<ImportHandle> weak_handle = handle;
    ProgressCallback progress_cb = [weak_handle](const std::string& stage,
                                                   size_t current,
                                                   size_t total) {
        if (auto h = weak_handle.lock()) {
            h->current_records.store(current);
            h->total_records.store(total);
            h->setStage(stage);
        }
    };

    std::thread([this, source_path, options, progress_cb,
                 handle, promise]() mutable {
        ImportStats stats;
        try {
            stats = this->importData(source_path, options, progress_cb);
        } catch (const std::exception& e) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = std::string("Unhandled exception in async Kafka import: ")
                           + e.what();
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        } catch (...) {
            ImportError err;
            err.code     = ImportErrorCode::UNKNOWN;
            err.severity = ImportErrorSeverity::CRITICAL;
            err.message  = "Unknown exception in async Kafka import worker";
            stats.structured_errors.push_back(err);
            stats.errors.push_back(err.message);
        }
        handle->running.store(false);
        handle->setStage("completed");
        handle->finished_at_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        promise->set_value(std::move(stats));
    }).detach();

    return handle;
}

void KafkaImporter::cancel() {
    cancelled_.store(true);
    THEMIS_INFO("Kafka Importer: import cancelled");
}

json KafkaImporter::getSourceSchema(const std::string& source_path) {
    std::string brokers, topic;
    parseKafkaUrl(source_path, brokers, topic);
    return json{
        {"type",   "kafka"},
        {"topic",  topic},
        {"schema", nullptr}
    };
}

void KafkaImporter::setMessageFetchForTesting(KafkaMessageFn fn) {
    message_fn_ = std::move(fn);
}

// ============================================================================
// URL parsing
// ============================================================================

bool KafkaImporter::parseKafkaUrl(const std::string& url,
                                   std::string& brokers,
                                   std::string& topic) {
    brokers.clear();
    topic.clear();

    if (url.empty()) return false;

    const std::string prefix = "kafka://";
    if (url.substr(0, prefix.size()) == prefix) {
        // Format: kafka://broker:9092/topic  or kafka://b1,b2/topic
        std::string rest = url.substr(prefix.size());
        auto slash_pos = rest.rfind('/');
        if (slash_pos == std::string::npos || slash_pos == rest.size() - 1) {
            // No topic after the slash, or slash is last char.
            return false;
        }
        brokers = rest.substr(0, slash_pos);
        topic   = rest.substr(slash_pos + 1);
    } else {
        // Bare topic name (no kafka:// prefix); brokers come from config.
        topic = url;
    }

    return !topic.empty();
}

// ============================================================================
// Mock-based consume loop
// ============================================================================

void KafkaImporter::consumeFromMock(const std::string& topic,
                                     const ImportOptions& options,
                                     ImportStats& stats,
                                     ProgressCallback& progress_cb) {
    size_t consumed = 0;
    bool abort_requested = false;
    try {
        while (!cancelled_.load() && !abort_requested) {
            if (max_messages_ > 0 && consumed >= max_messages_) break;

            auto batch = message_fn_();
            if (batch.empty()) break;

            for (auto& payload : batch) {
                if (cancelled_.load() || abort_requested) break;
                if (max_messages_ > 0 && consumed >= max_messages_) break;

                ++stats.total_records;

                if (options.max_row_size_bytes > 0 &&
                    payload.size() > options.max_row_size_bytes) {
                    addError(stats, ImportErrorCode::ROW_TOO_LARGE,
                             ImportErrorSeverity::WARNING,
                             "Message exceeds max_row_size_bytes limit",
                             "message " + std::to_string(consumed));
                    ++stats.failed_records;
                    ++consumed;
                    continue;
                }

                json entity = extractEntity(payload);
                if (entity.is_null()) {
                    ++stats.skipped_records;
                    ++consumed;
                    continue;
                }

                if (!options.dry_run) {
                    if (options.streaming_row_callback) {
                        bool cont = options.streaming_row_callback(topic, entity);
                        ++stats.imported_records;
                        if (!cont) {
                            THEMIS_INFO("Kafka Importer: streaming callback requested abort");
                            abort_requested = true;
                            break;
                        }
                    } else {
                        ++stats.imported_records;
                    }
                } else {
                    // dry-run: count but don't deliver
                    stats.warnings.push_back("dry-run: message " +
                                             std::to_string(consumed) + " parsed OK");
                }

                ++consumed;
            }

            reportProgress(progress_cb, "consuming", stats.imported_records, 0);
        }
    } catch (const std::exception& e) {
        addError(stats, ImportErrorCode::UNKNOWN,
                 ImportErrorSeverity::CRITICAL,
                 "Exception in Kafka mock consume: " + std::string(e.what()));
    }
}

// ============================================================================
// librdkafka consume loop
// ============================================================================

#ifdef THEMIS_ENABLE_KAFKA
void KafkaImporter::consumeFromKafka(const std::string& brokers,
                                      const std::string& topic,
                                      const ImportOptions& options,
                                      ImportStats& stats,
                                      ProgressCallback& progress_cb) {
    char errstr[512];

    rd_kafka_conf_t* conf = rd_kafka_conf_new();

    auto setConf = [&](const char* key, const char* value) -> bool {
        if (rd_kafka_conf_set(conf, key, value, errstr, sizeof(errstr))
                != RD_KAFKA_CONF_OK) {
            addError(stats, ImportErrorCode::UNKNOWN,
                     ImportErrorSeverity::CRITICAL,
                     std::string("rd_kafka_conf_set '") + key + "' failed: " + errstr);
            return false;
        }
        return true;
    };

    if (!setConf("bootstrap.servers", brokers.c_str())) {
        rd_kafka_conf_destroy(conf); return;
    }
    if (!setConf("group.id", consumer_group_.c_str())) {
        rd_kafka_conf_destroy(conf); return;
    }
    {
        std::string sess = std::to_string(session_timeout_ms_);
        if (!setConf("session.timeout.ms", sess.c_str())) {
            rd_kafka_conf_destroy(conf); return;
        }
    }
    if (!setConf("enable.auto.commit", "false")) {
        rd_kafka_conf_destroy(conf); return;
    }
    if (!setConf("auto.offset.reset", auto_offset_reset_.c_str())) {
        rd_kafka_conf_destroy(conf); return;
    }
    if (!security_protocol_.empty() && security_protocol_ != "plaintext") {
        if (!setConf("security.protocol", security_protocol_.c_str())) {
            rd_kafka_conf_destroy(conf); return;
        }
    }
    if (!sasl_mechanism_.empty()) {
        if (!setConf("sasl.mechanism", sasl_mechanism_.c_str())) {
            rd_kafka_conf_destroy(conf); return;
        }
    }
    if (!sasl_username_.empty()) {
        if (!setConf("sasl.username", sasl_username_.c_str())) {
            rd_kafka_conf_destroy(conf); return;
        }
    }
    if (!sasl_password_.empty()) {
        if (!setConf("sasl.password", sasl_password_.c_str())) {
            rd_kafka_conf_destroy(conf); return;
        }
    }
    if (!ssl_ca_location_.empty()) {
        if (!setConf("ssl.ca.location", ssl_ca_location_.c_str())) {
            rd_kafka_conf_destroy(conf); return;
        }
    }

    rd_kafka_t* rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
                                  errstr, sizeof(errstr));
    if (!rk) {
        rd_kafka_conf_destroy(conf);
        addError(stats, ImportErrorCode::UNKNOWN,
                 ImportErrorSeverity::CRITICAL,
                 std::string("rd_kafka_new failed: ") + errstr);
        return;
    }
    // conf is now owned by rk.

    rd_kafka_poll_set_consumer(rk);

    rd_kafka_topic_partition_list_t* topics =
        rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(topics, topic.c_str(),
                                      RD_KAFKA_PARTITION_UA);
    rd_kafka_resp_err_t sub_err = rd_kafka_subscribe(rk, topics);
    rd_kafka_topic_partition_list_destroy(topics);

    if (sub_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        addError(stats, ImportErrorCode::UNKNOWN,
                 ImportErrorSeverity::CRITICAL,
                 std::string("rd_kafka_subscribe failed: ") +
                 rd_kafka_err2str(sub_err));
        rd_kafka_consumer_close(rk);
        rd_kafka_destroy(rk);
        return;
    }

    size_t consumed = 0;
    int consecutive_timeouts = 0;
    const int kMaxTimeouts = 3;

    try {
        while (!cancelled_.load()) {
            if (max_messages_ > 0 && consumed >= max_messages_) break;

            rd_kafka_message_t* msg =
                rd_kafka_consumer_poll(rk, poll_timeout_ms_);

            if (!msg) {
                if (++consecutive_timeouts >= kMaxTimeouts) break;
                continue;
            }
            consecutive_timeouts = 0;

            if (msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
                rd_kafka_message_destroy(msg);
                break;
            }

            if (msg->err != RD_KAFKA_RESP_ERR_NO_ERROR) {
                addError(stats, ImportErrorCode::UNKNOWN,
                         ImportErrorSeverity::WARNING,
                         std::string("Kafka message error: ") +
                         rd_kafka_message_errstr(msg),
                         "partition " + std::to_string(msg->partition) +
                         " offset " + std::to_string(msg->offset));
                rd_kafka_message_destroy(msg);
                ++stats.failed_records;
                continue;
            }

            if (msg->payload && msg->len > 0) {
                ++stats.total_records;
                std::string payload(static_cast<const char*>(msg->payload),
                                    msg->len);

                if (options.max_row_size_bytes > 0 &&
                    payload.size() > options.max_row_size_bytes) {
                    addError(stats, ImportErrorCode::ROW_TOO_LARGE,
                             ImportErrorSeverity::WARNING,
                             "Message exceeds max_row_size_bytes limit",
                             "partition " + std::to_string(msg->partition) +
                             " offset " + std::to_string(msg->offset));
                    ++stats.failed_records;
                    rd_kafka_message_destroy(msg);
                    ++consumed;
                    continue;
                }

                json entity = extractEntity(payload);
                if (entity.is_null()) {
                    ++stats.skipped_records;
                    rd_kafka_message_destroy(msg);
                    ++consumed;
                    continue;
                }

                if (!options.dry_run) {
                    if (options.streaming_row_callback) {
                        bool cont = options.streaming_row_callback(topic, entity);
                        ++stats.imported_records;
                        if (!cont) {
                            rd_kafka_message_destroy(msg);
                            THEMIS_INFO("Kafka Importer: streaming callback requested abort");
                            break;
                        }
                    } else {
                        ++stats.imported_records;
                    }
                }

                ++consumed;
                reportProgress(progress_cb, "consuming",
                               stats.imported_records, 0);
            }

            rd_kafka_message_destroy(msg);
        }
    } catch (const std::exception& e) {
        addError(stats, ImportErrorCode::UNKNOWN,
                 ImportErrorSeverity::CRITICAL,
                 "Exception in Kafka consume loop: " + std::string(e.what()));
    }

    rd_kafka_consumer_close(rk);
    rd_kafka_destroy(rk);
}
#endif // THEMIS_ENABLE_KAFKA

// ============================================================================
// Entity extraction
// ============================================================================

json KafkaImporter::extractEntity(const std::string& payload) const {
    if (payload.empty()) return json(nullptr);

    if (message_format_ == "avro") {
        // Confluent wire format: magic byte (0x00) + 4-byte schema ID + JSON/bytes
        if (payload.size() > 5 &&
            static_cast<unsigned char>(payload[0]) == 0x00) {
            std::string content = payload.substr(5);
            if (content.empty()) return json(nullptr);
            try {
                return json::parse(content);
            } catch (...) {
                return json{{"content", content}};
            }
        }
        // Bare Avro (no magic byte) – treat as JSON.
        try {
            return json::parse(payload);
        } catch (...) {
            return json{{"content", payload}};
        }
    }

    if (message_format_ == "plaintext") {
        return json{{"content", payload}};
    }

    // JSON (default)
    try {
        json parsed = json::parse(payload);
        if (parsed.is_object() || parsed.is_array()) {
            return parsed;
        }
        // Scalar JSON value – wrap it.
        return json{{text_field_, parsed}};
    } catch (...) {
        // Not valid JSON – wrap the raw payload.
        return json{{text_field_, payload}};
    }
}

// ============================================================================
// Helpers
// ============================================================================

void KafkaImporter::addError(ImportStats& stats,
                              ImportErrorCode code,
                              ImportErrorSeverity severity,
                              const std::string& message,
                              const std::string& location) const {
    ImportError err;
    err.code     = code;
    err.severity = severity;
    err.message  = message;
    err.location = location;
    stats.structured_errors.push_back(err);
    stats.errors.push_back(message);
}

void KafkaImporter::emitMetric(const ImportOptions& options,
                                const std::string& metric,
                                const std::map<std::string, std::string>& labels,
                                double value) const {
    if (options.metrics_callback) {
        options.metrics_callback(metric, labels, value);
    }
}

void KafkaImporter::emitSpan(const ImportOptions& options,
                              const std::string& operation,
                              const std::map<std::string, std::string>& attributes,
                              double duration_seconds) const {
    if (options.tracing_callback) {
        options.tracing_callback(operation, attributes, duration_seconds);
    }
}

void KafkaImporter::reportProgress(ProgressCallback& callback,
                                    const std::string& stage,
                                    size_t current,
                                    size_t total) {
    if (callback) {
        callback(stage, current, total);
    }
}

// ============================================================================
// Plugin wrapper
// ============================================================================

KafkaImporterPlugin::KafkaImporterPlugin()
    : importer_(std::make_unique<KafkaImporter>()) {}

plugins::PluginCapabilities KafkaImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.thread_safe        = true;
    return caps;
}

bool KafkaImporterPlugin::initialize(const char* config_json) {
    std::string cfg = config_json ? config_json : "{}";
    return importer_->initialize(cfg);
}

void KafkaImporterPlugin::shutdown() {
    if (importer_) importer_->cancel();
}

} // namespace importers
} // namespace themis
