/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            kafka_change_stream.h                              ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
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

class IKafkaChangeStreamExporter {
public:
    virtual ~IKafkaChangeStreamExporter() = default;
    virtual bool configure(const KafkaChangeStreamConfig& config) = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool isRunning() const = 0;
    virtual KafkaStreamStats getStats() const = 0;
    virtual bool flush(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) = 0;
};

}} // namespace themis::replication
