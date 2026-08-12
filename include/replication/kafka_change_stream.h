/**
 * @file kafka_change_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
// Change stream Kafka export interface
#include <string>
#include <vector>
#include <chrono>

namespace themis { namespace replication {

struct KafkaProducerConfig {
    std::vector<std::string> bootstrap_servers;
    std::string topic_prefix = "themisdb.changes.";
    int acks = -1;
    int retries = 3;
    int batch_size_bytes = 65536;
    int linger_ms = 5;
    bool enable_idempotence = true;
    std::string compression_type = "lz4";
    std::string sasl_mechanism;
    std::string sasl_username;
    std::string sasl_password;
    bool ssl_enabled = false;
    std::string ssl_ca_cert_path;
};

enum class ChangeEventFormat {
    JSON,
    AVRO,
    DEBEZIUM_JSON,
    PROTOBUF,
};

struct KafkaChangeStreamConfig {
    KafkaProducerConfig producer;
    ChangeEventFormat format = ChangeEventFormat::DEBEZIUM_JSON;
    std::vector<std::string> collections_to_stream;
    bool include_before_image = true;
    bool include_after_image = true;
    std::string schema_registry_url;
};

struct KafkaStreamStats {
    size_t events_published = 0;
    size_t events_failed = 0;
    size_t bytes_sent = 0;
    double avg_publish_latency_ms = 0.0;
    bool is_connected = false;
};

/** @brief I kafka change stream exporter component. */
class IKafkaChangeStreamExporter {
public:
    virtual ~IKafkaChangeStreamExporter() = default;
    [[nodiscard]] virtual bool configure(const KafkaChangeStreamConfig& config) = 0;
    [[nodiscard]] virtual bool start() = 0;
    [[nodiscard]] virtual bool stop() = 0;
    [[nodiscard]] virtual bool isRunning() const = 0;
    [[nodiscard]] virtual KafkaStreamStats getStats() const = 0;
    [[nodiscard]] virtual bool flush(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) = 0;
};

}} // namespace themis::replication
