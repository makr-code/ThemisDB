#include "importers/debezium_cdc_importer.h"
#include "importers/importer_common.h"
#include "utils/logger.h"

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>

// When THEMIS_ENABLE_DEBEZIUM is defined the full librdkafka-backed
// implementation is compiled.  Without it every importData() call returns
// IMPORT_CONNECTOR_UNAVAILABLE with a message describing the missing build flag.
// The mock injection path (setMockEventsForTesting), the envelope parser, and
// the broker-sanitisation helper are available in all build configurations.

#ifdef THEMIS_ENABLE_DEBEZIUM
#include <librdkafka/rdkafkacpp.h>
#endif

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace importers {

// ============================================================================
// Phase-2-hardening helpers
// ============================================================================
namespace {

/// Maps Kafka/broker error patterns to ImporterErrorCode.
static ImportErrorCode mapDebeziumErrorToCode(const std::string& error_msg) {
    const auto lower = [](std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    const std::string lmsg = lower(error_msg);

    if (lmsg.find("broker") != std::string::npos ||
        lmsg.find("connection refused") != std::string::npos ||
        lmsg.find("transport") != std::string::npos ||
        lmsg.find("all brokers down") != std::string::npos) {
        return ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    }
    if (lmsg.find("timeout") != std::string::npos ||
        lmsg.find("timed out") != std::string::npos) {
        return ImportErrorCode::DEADLINE_EXCEEDED;
    }
    if (lmsg.find("schema") != std::string::npos ||
        lmsg.find("parse") != std::string::npos ||
        lmsg.find("deserialization") != std::string::npos) {
        return ImportErrorCode::IMPORT_SCHEMA_MISMATCH;
    }
    return ImportErrorCode::UNKNOWN;
}

} // anonymous namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

DebeziumCDCImporter::DebeziumCDCImporter() = default;
DebeziumCDCImporter::~DebeziumCDCImporter() = default;

// ============================================================================
// IImporter – getSupportedTypes
// ============================================================================

std::vector<std::string> DebeziumCDCImporter::getSupportedTypes() const {
    return {"debezium", "debezium-cdc", "cdc"};
}

// ============================================================================
// IImporter – initialize
// ============================================================================

bool DebeziumCDCImporter::initialize(const std::string& config_json) {
    try {
        const json cfg = json::parse(config_json);

        if (!cfg.contains("brokers") || cfg["brokers"].get<std::string>().empty()) {
            THEMIS_WARN("DebeziumCDCImporter::initialize: 'brokers' is required");
            return false;
        }
        config_.brokers        = cfg["brokers"].get<std::string>();
        config_.topic_prefix   = cfg.value("topic_prefix",   std::string{});
        config_.consumer_group = cfg.value("consumer_group", std::string{"themisdb-cdc"});
        config_.auto_offset    = cfg.value("auto_offset",    std::string{"latest"});
        config_.schema_registry_url = cfg.value("schema_registry", std::string{});
        config_.tls            = cfg.value("tls",            false);
        config_.max_batch_size = cfg.value("max_batch_size", 500);
        config_.poll_timeout_ms = cfg.value("poll_timeout_ms", 100);
        config_.snapshot_mode  = cfg.value("snapshot_mode",  std::string{"initial"});
        config_.dead_letter_topic = cfg.value("dead_letter_topic", std::string{});

        // Table filter list.
        config_.table_filter.clear();
        if (cfg.contains("tables") && cfg["tables"].is_array()) {
            for (const auto& t : cfg["tables"]) {
                config_.table_filter.push_back(t.get<std::string>());
            }
        }

        // SASL password is consumed here and used only during connect; never stored.
        return true;
    } catch (const std::exception& e) {
        THEMIS_WARN("DebeziumCDCImporter::initialize parse error: " +
                    std::string(e.what()));
        return false;
    }
}

// ============================================================================
// IImporter – validateSource
// ============================================================================

bool DebeziumCDCImporter::validateSource(const std::string& source_path,
                                          std::vector<std::string>& errors) {
#ifndef THEMIS_ENABLE_DEBEZIUM
    if (mock_events_.empty()) {
        errors.push_back(
            "DebeziumCDCImporter: THEMIS_ENABLE_DEBEZIUM is not defined. "
            "Rebuild with -DTHEMIS_ENABLE_DEBEZIUM=ON to enable the full "
            "librdkafka-backed connector.");
        return false;
    }
#endif

    const std::string brokers = source_path.empty() ? config_.brokers : source_path;
    if (brokers.empty()) {
        errors.push_back("DebeziumCDCImporter: 'brokers' is required.");
        return false;
    }

    if (config_.topic_prefix.empty() && mock_events_.empty()) {
        errors.push_back("DebeziumCDCImporter: 'topic_prefix' is required.");
        return false;
    }

#ifdef THEMIS_ENABLE_DEBEZIUM
    // Production path: metadata request to validate broker reachability.
    // Real librdkafka metadata call would go here.
#else
    // Mock path: if we have mock events, validation passes.
#endif

    return true;
}

// ============================================================================
// Static helpers
// ============================================================================

/*static*/
std::string DebeziumCDCImporter::sanitiseBrokers(const std::string& brokers) {
    // Brokers string: "host1:9092,host2:9092" — no credentials to strip here.
    // SASL credentials are separate config fields and are never in the brokers string.
    return brokers;
}

/*static*/
DebeziumCDCImporter::ChangeOp DebeziumCDCImporter::mapOpChar(
    const std::string& op_str) {
    if (op_str == "c") return ChangeOp::Create;
    if (op_str == "u") return ChangeOp::Update;
    if (op_str == "d") return ChangeOp::Delete;
    if (op_str == "r") return ChangeOp::Read;
    return ChangeOp::Unknown;
}

/*static*/
DebeziumCDCImporter::CDCEvent DebeziumCDCImporter::parseDebeziumEnvelope(
    const json& envelope, std::string& error_out) {

    CDCEvent event;
    try {
        // Support both flat payload and nested "payload" wrapper.
        const json& payload = envelope.contains("payload") ? envelope["payload"] : envelope;

        event.op     = mapOpChar(payload.value("op", std::string{""}));
        event.before = payload.value("before", json{});
        event.after  = payload.value("after",  json{});

        if (payload.contains("source")) {
            const auto& src = payload["source"];
            event.table         = src.value("table", std::string{});
            event.source_ts_ms  = src.value("ts_ms", int64_t{0});
            // Compose fully-qualified table name: schema.table
            const std::string schema = src.value("schema", std::string{});
            if (!schema.empty() && !event.table.empty()) {
                event.table = schema + "." + event.table;
            }
        }

        // Transaction ID (optional, Debezium 1.5+)
        if (payload.contains("transaction") && !payload["transaction"].is_null()) {
            event.transaction_id = payload["transaction"].value("id", std::string{});
        }

    } catch (const std::exception& e) {
        error_out = "parseDebeziumEnvelope: " + std::string(e.what());
        event.op = ChangeOp::Unknown;
    }
    return event;
}

bool DebeziumCDCImporter::tableAllowed(const std::string& table) const {
    if (config_.table_filter.empty()) return true;
    return std::any_of(config_.table_filter.begin(), config_.table_filter.end(),
                       [&](const std::string& f) {
                           return table.find(f) != std::string::npos;
                       });
}

// ============================================================================
// IImporter – importData
// ============================================================================

ImportStats DebeziumCDCImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback) {

    cancelled_.store(false);

    // Delegate to streamEvents() with a ThemisDB-write sink.
    ImportStats stats{};
    const auto start_time = std::chrono::steady_clock::now();

    // Delegate to streamEvents() which returns detailed stats including
    // structured_errors for the no-build-flag guard path. Return the
    // stream result directly so callers (and tests) observe connector
    // unavailability correctly.
    return streamEvents(options,
        [&](const CDCEvent& event) -> bool {
            if (cancelled_.load(std::memory_order_relaxed)) return false;

            ++stats.total_records;

            if (event.op == ChangeOp::Unknown) {
                ++stats.skipped_records;
                return true;
            }

            if (!tableAllowed(event.table)) {
                ++stats.skipped_records;
                return true;
            }

            // In production: dispatch to ThemisDB storage layer.
            ++stats.imported_records;

            if (progress_callback) {
                progress_callback("stream", stats.total_records, 0);
            }
            return true;
        });
}

// ============================================================================
// streamEvents – core CDC delivery loop
// ============================================================================

ImportStats DebeziumCDCImporter::streamEvents(const ImportOptions& options,
                                               CDCEventCallback callback) {
    ImportStats stats{};
    const auto start_time = std::chrono::steady_clock::now();

    const auto deadline = (options.deadline_ms > 0)
        ? std::optional<std::chrono::steady_clock::time_point>(
              start_time + std::chrono::milliseconds(options.deadline_ms))
        : std::nullopt;

    // ---- Mock path (for unit tests) ----------------------------------------
    if (!mock_events_.empty()) {
        for (auto& event : mock_events_) {
            if (cancelled_.load(std::memory_order_relaxed)) break;
            if (deadline && std::chrono::steady_clock::now() >= *deadline) break;
            ++stats.total_records;

            if (!tableAllowed(event.table)) {
                ++stats.skipped_records;
                continue;
            }

            if (!callback(event)) break;
            ++stats.imported_records;
        }
        mock_events_.clear();
        const auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
        stats.rows_imported = stats.imported_records;
        stats.rows_skipped = stats.skipped_records;
        stats.rows_quarantined = stats.quarantined_records;
        return stats;
    }

    // ---- Build guard check --------------------------------------------------
#ifndef THEMIS_ENABLE_DEBEZIUM
    ImportError err;
    err.code     = ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
    err.message  = "DebeziumCDCImporter: THEMIS_ENABLE_DEBEZIUM is not defined. "
                   "Rebuild with -DTHEMIS_ENABLE_DEBEZIUM=ON to enable the full "
                   "librdkafka-backed connector. Brokers: " +
                   sanitiseBrokers(config_.brokers);
    err.severity = ImportErrorSeverity::CRITICAL;
    stats.structured_errors.push_back(err);
    stats.errors.push_back(err.message);
    const auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
    stats.rows_imported = stats.imported_records;
    stats.rows_skipped = stats.skipped_records;
    stats.rows_quarantined = stats.quarantined_records;
    return stats;
#else
    // ---- Production path (librdkafka C++ API) --------------------------------

    std::string errstr;

    // 1. Build and apply consumer configuration.
    std::unique_ptr<RdKafka::Conf> conf(
        RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));

    auto setConf = [&](const std::string& key, const std::string& value) -> bool {
        if (conf->set(key, value, errstr) != RdKafka::Conf::CONF_OK) {
            THEMIS_WARN("DebeziumCDCImporter: conf '{}' error: {}", key, errstr);
            return false;
        }
        return true;
    };

    if (!setConf("bootstrap.servers",  config_.brokers)        ||
        !setConf("group.id",           config_.consumer_group) ||
        !setConf("auto.offset.reset",  config_.auto_offset)    ||
        !setConf("enable.auto.commit", "false")                 ||
        !setConf("session.timeout.ms", "30000")) {
        ImportError err;
        err.code     = ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
        err.message  = "DebeziumCDCImporter: configuration error: " + errstr;
        err.severity = ImportErrorSeverity::CRITICAL;
        stats.structured_errors.push_back(err);
        stats.errors.push_back(err.message);
        const auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
        stats.rows_imported = stats.imported_records;
        stats.rows_skipped = stats.skipped_records;
        stats.rows_quarantined = stats.quarantined_records;
        return stats;
    }

    if (config_.tls) {
        setConf("security.protocol", "ssl");
    }

    // 2. Create consumer instance.
    std::unique_ptr<RdKafka::KafkaConsumer> consumer(
        RdKafka::KafkaConsumer::create(conf.get(), errstr));
    if (!consumer) {
        ImportError err;
        err.code     = ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
        err.message  = "DebeziumCDCImporter: failed to create consumer: " + errstr;
        err.severity = ImportErrorSeverity::CRITICAL;
        stats.structured_errors.push_back(err);
        stats.errors.push_back(err.message);
        const auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
        stats.rows_imported = stats.imported_records;
        stats.rows_skipped = stats.skipped_records;
        stats.rows_quarantined = stats.quarantined_records;
        return stats;
    }

    // 3. Subscribe to Debezium topics.
    //    Debezium topic naming: <prefix>.<schema>.<table>
    //    Use a regex pattern to subscribe to all tables under the configured prefix.
    if (config_.topic_prefix.empty()) {
        ImportError err;
        err.code     = ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
        err.message  = "DebeziumCDCImporter: 'topic_prefix' is required for "
                   "the production Kafka path.";
        err.severity = ImportErrorSeverity::CRITICAL;
        stats.structured_errors.push_back(err);
        stats.errors.push_back(err.message);
        const auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
        stats.rows_imported = stats.imported_records;
        stats.rows_skipped = stats.skipped_records;
        stats.rows_quarantined = stats.quarantined_records;
        return stats;
    }

    const std::vector<std::string> topics{
        "^" + config_.topic_prefix + "\\..*"
    };

    const RdKafka::ErrorCode sub_err = consumer->subscribe(topics);
    if (sub_err != RdKafka::ERR_NO_ERROR) {
        ImportError err;
        err.code     = ImportErrorCode::IMPORT_CONNECTOR_UNAVAILABLE;
        err.message  = "DebeziumCDCImporter: subscribe failed: " +
                   RdKafka::err2str(sub_err);
        err.severity = ImportErrorSeverity::CRITICAL;
        stats.structured_errors.push_back(err);
        stats.errors.push_back(err.message);
        const auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
        stats.rows_imported = stats.imported_records;
        stats.rows_skipped = stats.skipped_records;
        stats.rows_quarantined = stats.quarantined_records;
        return stats;
    }

    // 4. Poll loop: consume messages, decode Debezium envelopes, invoke callback.
    static constexpr int kBatchCommitSize = 500;
    int batch_count = 0;
    bool stop = false;

    while (!stop && !cancelled_.load(std::memory_order_relaxed)) {
        if (deadline && std::chrono::steady_clock::now() >= *deadline) {
            break;
        }

        std::unique_ptr<RdKafka::Message> msg(
            consumer->consume(config_.poll_timeout_ms));
        if (!msg) continue;

        switch (msg->err()) {
            case RdKafka::ERR_NO_ERROR: {
                ++stats.total_records;

                const std::string payload(
                    static_cast<const char*>(msg->payload()),
                    msg->len());

                std::string parse_err;
                CDCEvent event;
                try {
                    const json envelope = json::parse(payload);
                    event = parseDebeziumEnvelope(envelope, parse_err);
                } catch (const std::exception& e) {
                    parse_err = std::string("JSON parse: ") + e.what();
                }

                if (!parse_err.empty()) {
                    // Unparseable message — record error and continue.
                    ImportError err;
                    err.code     = ImportErrorCode::IMPORT_SCHEMA_MISMATCH;
                    err.message  = "DebeziumCDCImporter: " + parse_err;
                    err.severity = ImportErrorSeverity::WARNING;
                    stats.structured_errors.push_back(err);
                    stats.errors.push_back(err.message);
                    ++stats.skipped_records;
                    break;
                }

                // Store the Kafka offset in the event for downstream consumers.
                event.offset = static_cast<uint64_t>(msg->offset());

                if (!tableAllowed(event.table)) {
                    ++stats.skipped_records;
                    break;
                }

                if (!callback(event)) {
                    // Callback requested stop.
                    stop = true;
                } else {
                    ++stats.imported_records;
                }
                ++batch_count;

                // At-least-once: commit after each batch window.
                if (batch_count >= kBatchCommitSize) {
                    consumer->commitSync();
                    batch_count = 0;
                }
                break;
            }
            case RdKafka::ERR__TIMED_OUT:
                // Normal poll timeout; no messages currently available.
                break;
            case RdKafka::ERR__PARTITION_EOF:
                // All partitions at end-of-log; continue streaming for new events.
                break;
            default: {
                const ImportErrorCode code =
                    mapDebeziumErrorToCode(msg->errstr());
                ImportError err;
                err.code     = code;
                err.message  = "DebeziumCDCImporter: consumer error: " +
                               msg->errstr();
                err.severity = ImportErrorSeverity::ERROR;
                stats.structured_errors.push_back(err);
                stats.errors.push_back(err.message);
                break;
            }
        }
    }

    // 5. Flush remaining uncommitted offsets, then close the consumer.
    if (batch_count > 0) {
        consumer->commitSync();
    }
    consumer->close();

    const auto end_time2 = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time2 - start_time).count();
    stats.rows_imported = stats.imported_records;
    stats.rows_skipped = stats.skipped_records;
    stats.rows_quarantined = stats.quarantined_records;
    return stats;
#endif
}

// ============================================================================
// IImporter – importDataAsync
// ============================================================================

std::shared_ptr<ImportHandle> DebeziumCDCImporter::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options) {

    auto handle = std::make_shared<ImportHandle>();
    handle->id  = "debezium-cdc-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());

    handle->future = std::async(std::launch::async,
        [this, source_path, options]() -> ImportStats {
            return importData(source_path, options, nullptr);
        });

    return handle;
}

// ============================================================================
// IImporter – cancel
// ============================================================================

void DebeziumCDCImporter::cancel() {
    cancelled_.store(true, std::memory_order_release);
}

// ============================================================================
// IImporter – getSourceSchema
// ============================================================================

json DebeziumCDCImporter::getSourceSchema(const std::string& source_path) {
    (void)source_path;

    if (mock_events_.empty()) {
        // In production: read the first message from each topic, extract the
        // Debezium schema envelope, and convert to ThemisDB schema format.
        return json::object();
    }

    // Build a schema summary from the injected mock events.
    json schema = json::object();
    for (const auto& event : mock_events_) {
        if (event.table.empty()) continue;
        if (schema.contains(event.table)) continue;

        json fields = json::array();
        const json& sample = event.after.is_null() ? event.before : event.after;
        if (sample.is_object()) {
            for (auto& [fname, fval] : sample.items()) {
                json f;
                f["name"] = fname;
                f["type"] = fval.is_number_integer() ? "int64"
                          : fval.is_number()          ? "float64"
                          : fval.is_boolean()          ? "bool"
                          : "string";
                fields.push_back(std::move(f));
            }
        }
        schema[event.table] = {{"fields", fields}};
    }
    return schema;
}

// ============================================================================
// Testing support
// ============================================================================

void DebeziumCDCImporter::setMockEventsForTesting(std::vector<CDCEvent> events) {
    mock_events_ = std::move(events);
}

} // namespace importers
} // namespace themis
