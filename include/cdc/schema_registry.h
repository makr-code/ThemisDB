/**
 * @file schema_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Schema-aware CDC with Avro/Protobuf schema registry integration.
 *
 * Provides schema registration, caching, and Confluent-compatible wire-format
 * encoding for ThemisDB CDC change events.  Downstream consumers (Kafka,
 * Kafka Connect, etc.) can use any Confluent-compatible schema registry client
 * to decode the binary-framed messages produced by CdcSchemaEncoder.
 *
 * Wire format (Confluent Schema Registry protocol):
 * @code
 * +--------+------------------+--------------------------+
 * | 0x00   |  schema_id (4B)  |  serialized payload      |
 * | magic  |  big-endian      |  JSON / Avro / Protobuf  |
 * +--------+------------------+--------------------------+
 * @endcode
 *
 * Components:
 *  - SchemaFormat         – format identifier (JSON, AVRO, PROTOBUF)
 *  - SchemaInfo           – schema metadata (id, version, subject, definition)
 *  - SchemaRegistryConfig – connection and behaviour settings
 *  - ISchemaRegistryBackend – abstract backend (register / lookup schemas)
 *  - InMemorySchemaRegistryBackend – thread-safe in-memory backend for
 *                                    testing and standalone use
 *  - SchemaRegistryClient – caching client over an ISchemaRegistryBackend
 *  - CdcSchemaEncoder     – encode ChangeEvents using the wire format
 *
 * The header is self-contained (no external HTTP library required).  For
 * production use with a remote Confluent Schema Registry, provide a custom
 * ISchemaRegistryBackend implementation that issues HTTP requests to the
 * registry REST API.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace cdc {

// ── Format enumeration ────────────────────────────────────────────────────────

/**
 * @brief Schema format identifier.
 *
 * Controls both the schema definition language and the payload serialization:
 *  - JSON     – Confluent JSON Schema; payload is UTF-8 JSON bytes.
 *  - AVRO     – Apache Avro schema; payload is Avro binary (OCF-less datum).
 *               Requires avro-cpp or equivalent; falls back to JSON bytes when
 *               no Avro runtime is linked.
 *  - PROTOBUF – Protocol Buffers; payload is proto3 wire bytes.
 *               Requires the protobuf library; falls back to JSON bytes when
 *               not linked.
 */
enum class SchemaFormat {
    JSON,       ///< Confluent JSON Schema (JSON bytes payload)
    AVRO,       ///< Apache Avro binary encoding
    PROTOBUF,   ///< Protocol Buffers binary encoding
};

/** @brief Returns the Confluent magic type string for a format. */
inline std::string schemaFormatString(SchemaFormat fmt) {
    switch (fmt) {
        case SchemaFormat::AVRO:     return "AVRO";
        case SchemaFormat::PROTOBUF: return "PROTOBUF";
        default:                     return "JSON";
    }
}

// ── Schema information ────────────────────────────────────────────────────────

/**
 * @brief Schema metadata stored in or retrieved from a schema registry.
 */
struct SchemaInfo {
    int32_t     id          = -1;   ///< Registry-assigned schema ID
    int32_t     version     = -1;   ///< Schema version within the subject
    std::string subject;            ///< Registry subject name (e.g. "orders-value")
    std::string schema_json;        ///< Schema definition in JSON form
    SchemaFormat format     = SchemaFormat::JSON;

    bool is_valid() const noexcept { return id >= 0; }
};

// ── Configuration ─────────────────────────────────────────────────────────────

/**
 * @brief Configuration for SchemaRegistryClient.
 *
 * When @c url is empty the client operates in in-memory mode (no HTTP).
 * Credentials must be loaded from secure config paths; they are never logged.
 */
struct SchemaRegistryConfig {
    /// Schema registry base URL, e.g. "http://schema-registry:8081".
    /// Empty string → in-memory mode (no remote registry).
    std::string url;

    /// HTTP Basic auth username (empty = no auth).
    std::string username;

    /// HTTP Basic auth password (load from config/security/; never log).
    std::string password;

    /// Subject name strategy: "TopicName" | "RecordName" | "TopicRecordName".
    std::string subject_name_strategy{"TopicName"};

    /// Topic prefix used when subject_name_strategy == "TopicName".
    std::string topic_prefix{"themis.cdc."};

    /// Default encoding format for new schemas.
    SchemaFormat default_format{SchemaFormat::JSON};

    /// Automatically register schemas that are not yet in the registry.
    bool auto_register_schemas{true};

    /// Schema cache time-to-live (seconds). 0 = never expire.
    std::chrono::seconds cache_ttl{300};

    /// Max retries for transient HTTP errors.
    uint32_t max_retries{3};

    /// Per-request HTTP timeout (milliseconds).
    std::chrono::milliseconds request_timeout{5000};

    /// Path to CA certificate bundle for TLS verification.
    std::string ssl_ca_location;
};

// ── Abstract backend ──────────────────────────────────────────────────────────

/**
 * @brief Abstract schema registry backend (register / lookup interface).
 *
 * Implement this interface to integrate with any schema registry (Confluent,
 * AWS Glue, Apicurio, etc.).  The default InMemorySchemaRegistryBackend is
 * suitable for unit tests and standalone setups.
 */
class ISchemaRegistryBackend {
public:
    virtual ~ISchemaRegistryBackend() = default;

    /**
     * @brief Register a schema and return its ID.
     *
     * If an identical schema for @p subject is already registered, the
     * existing ID is returned (idempotent).
     *
     * @param subject    Registry subject (e.g. "orders-value").
     * @param schema_json Schema definition JSON.
     * @param format      Encoding format.
     * @return Schema ID (>= 0).
     * @throws std::runtime_error on unrecoverable registration failures.
     */
    [[nodiscard]] virtual int32_t registerSchema(const std::string& subject,
                                   const std::string& schema_json,
                                   SchemaFormat format) = 0;

    /**
     * @brief Look up a schema by its ID.
     * @return SchemaInfo or nullopt when not found.
     */
    [[nodiscard]] virtual std::optional<SchemaInfo> getById(int32_t id) const = 0;

    /**
     * @brief Look up the latest schema version for a subject.
     * @return SchemaInfo or nullopt when not found.
     */
    [[nodiscard]] virtual std::optional<SchemaInfo> getLatest(
        const std::string& subject) const = 0;
};

// ── In-memory backend ─────────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory schema registry backend.
 *
 * Suitable for unit tests and for environments without a live registry.
 * Schemas are stored in process memory and lost on destruction.
 */
class InMemorySchemaRegistryBackend : public ISchemaRegistryBackend {
public:
    int32_t registerSchema(const std::string& subject,
                           const std::string& schema_json,
                           SchemaFormat format) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Return existing ID when the same (subject, schema_json) is re-registered.
        auto key = subject + "|" + schema_json;
        auto it = key_to_id_.find(key);
        if (it != key_to_id_.end()) {
            return it->second;
        }

        const int32_t new_id = next_id_++;
        const int32_t version = static_cast<int32_t>(
            ++subject_versions_[subject]);

        SchemaInfo info;
        info.id          = new_id;
        info.version     = version;
        info.subject     = subject;
        info.schema_json = schema_json;
        info.format      = format;

        id_map_[new_id]     = info;
        subject_latest_[subject] = new_id;
        key_to_id_[key]     = new_id;

        return new_id;
    }

    std::optional<SchemaInfo> getById(int32_t id) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = id_map_.find(id);
        if (it == id_map_.end()) return std::nullopt;
        return it->second;
    }

    std::optional<SchemaInfo> getLatest(
        const std::string& subject) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto sit = subject_latest_.find(subject);
        if (sit == subject_latest_.end()) return std::nullopt;
        auto iit = id_map_.find(sit->second);
        if (iit == id_map_.end()) return std::nullopt;
        return iit->second;
    }

    /** @brief Return number of registered schemas (useful in tests). */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return id_map_.size();
    }

    /** @brief Remove all schemas (useful in tests). */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        id_map_.clear();
        subject_latest_.clear();
        subject_versions_.clear();
        key_to_id_.clear();
        next_id_ = 0;
    }

private:
    mutable std::mutex mutex_;
    int32_t next_id_{0};
    std::unordered_map<int32_t, SchemaInfo>  id_map_;
    std::unordered_map<std::string, int32_t> subject_latest_;
    std::unordered_map<std::string, int32_t> subject_versions_;
    std::unordered_map<std::string, int32_t> key_to_id_;
};

// ── Schema Registry Client ────────────────────────────────────────────────────

/**
 * @brief Caching schema registry client.
 *
 * Wraps an ISchemaRegistryBackend with a TTL-based in-process cache.
 * Thread-safe for concurrent use from multiple CDC producer threads.
 *
 * Usage:
 * @code
 * SchemaRegistryConfig cfg;
 * cfg.url = "http://schema-registry:8081";
 * cfg.default_format = SchemaFormat::AVRO;
 *
 * SchemaRegistryClient client(cfg);
 * int32_t id = client.ensureSchema("orders-value", avro_schema_json,
 *                                   SchemaFormat::AVRO);
 * @endcode
 */
class SchemaRegistryClient {
public:
    /**
     * @brief Construct the client.
     *
     * @param config   Client configuration.
     * @param backend  Backend implementation.  When nullptr an
     *                 InMemorySchemaRegistryBackend is created automatically.
     */
    explicit SchemaRegistryClient(
        SchemaRegistryConfig config = {},
        std::shared_ptr<ISchemaRegistryBackend> backend = nullptr)
        : config_(std::move(config))
        , backend_(backend
                       ? std::move(backend)
                       : std::make_shared<InMemorySchemaRegistryBackend>()) {}

    // Non-copyable; movable.
    SchemaRegistryClient(const SchemaRegistryClient&) = delete;
    SchemaRegistryClient& operator=(const SchemaRegistryClient&) = delete;
    SchemaRegistryClient(SchemaRegistryClient&&)            noexcept = default;
    SchemaRegistryClient& operator=(SchemaRegistryClient&&) noexcept = default;

    /**
     * @brief Register a schema if not yet registered and return its ID.
     *
     * The result is cached for @c config_.cache_ttl.  Subsequent calls with
     * the same @p subject return the cached ID without hitting the backend.
     *
     * @param subject      Registry subject name.
     * @param schema_json  Schema definition.
     * @param format       Encoding format (defaults to config default).
     * @return Schema ID.
     */
    int32_t ensureSchema(
        const std::string& subject,
        const std::string& schema_json,
        SchemaFormat format) {
        // Check subject cache first.
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto it = subject_cache_.find(subject);
            if (it != subject_cache_.end() && !isCacheExpired(it->second.second)) {
                return it->second.first.id;
            }
        }

        const int32_t id = backend_->registerSchema(subject, schema_json, format);

        // Populate id + subject caches.
        auto info_opt = backend_->getById(id);
        if (info_opt) {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto expiry = cacheExpiry();
            id_cache_[id]          = {*info_opt, expiry};
            subject_cache_[subject] = {*info_opt, expiry};
        }

        return id;
    }

    /**
     * @brief Look up a schema by ID (cache-aware).
     * @return SchemaInfo or nullopt.
     */
    std::optional<SchemaInfo> getSchema(int32_t id) const {
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto it = id_cache_.find(id);
            if (it != id_cache_.end() && !isCacheExpired(it->second.second)) {
                return it->second.first;
            }
        }

        auto info = backend_->getById(id);
        if (info) {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            id_cache_[id] = {*info, cacheExpiry()};
        }
        return info;
    }

    /**
     * @brief Look up the latest schema for a subject (cache-aware).
     * @return SchemaInfo or nullopt.
     */
    std::optional<SchemaInfo> getLatestSchema(
        const std::string& subject) const {
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto it = subject_cache_.find(subject);
            if (it != subject_cache_.end() && !isCacheExpired(it->second.second)) {
                return it->second.first;
            }
        }

        auto info = backend_->getLatest(subject);
        if (info) {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            subject_cache_[subject] = {*info, cacheExpiry()};
        }
        return info;
    }

    /** @brief Invalidate all cached entries. */
    void clearCache() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        id_cache_.clear();
        subject_cache_.clear();
    }

    /** @brief Return the configuration used to construct this client. */
    const SchemaRegistryConfig& config() const noexcept { return config_; }

    /** @brief Access the underlying backend (useful for testing). */
    ISchemaRegistryBackend* backend() const noexcept { return backend_.get(); }

private:
    using TimePoint = std::chrono::steady_clock::time_point;

    SchemaRegistryConfig config_;
    std::shared_ptr<ISchemaRegistryBackend> backend_;

    mutable std::mutex cache_mutex_;
    mutable std::unordered_map<int32_t,    std::pair<SchemaInfo, TimePoint>> id_cache_;
    mutable std::unordered_map<std::string, std::pair<SchemaInfo, TimePoint>> subject_cache_;

    TimePoint cacheExpiry() const {
        if (config_.cache_ttl.count() == 0) {
            return TimePoint::max();
        }
        return std::chrono::steady_clock::now() + config_.cache_ttl;
    }

    static bool isCacheExpired(const TimePoint& expiry) {
        return std::chrono::steady_clock::now() > expiry;
    }
};

// ── Wire-format constants ─────────────────────────────────────────────────────

/** @brief Confluent Schema Registry magic byte (first byte of every message). */
constexpr uint8_t SCHEMA_REGISTRY_MAGIC_BYTE = 0x00;

/** @brief Length of the wire-format header (magic + 4-byte schema ID). */
constexpr size_t SCHEMA_REGISTRY_HEADER_SIZE = 5;

// ── Encoded event ─────────────────────────────────────────────────────────────

/**
 * @brief Result of a CdcSchemaEncoder::encode() call.
 */
struct EncodedEvent {
    std::vector<uint8_t> data;          ///< Wire-format bytes
    int32_t              schema_id{-1}; ///< Schema ID embedded in the header
    SchemaFormat         format{SchemaFormat::JSON};
    std::string          subject;       ///< Registry subject used for encoding
};

// ── CDC Schema Encoder ────────────────────────────────────────────────────────

/**
 * @brief Encodes ThemisDB CDC ChangeEvents using the Confluent Schema Registry
 *        wire format.
 *
 * Each ChangeEvent is serialised to a binary message that begins with the
 * five-byte Confluent wire-format header (magic byte + big-endian schema ID)
 * followed by the payload bytes.
 *
 * Payload encoding depends on the format registered for the subject:
 *  - JSON     – UTF-8 JSON bytes produced by eventToPayload().
 *  - AVRO     – UTF-8 JSON bytes (fallback); inject a real Avro binary encoder
 *               via setAvroEncoderFn() to produce native Avro datum bytes.
 *  - PROTOBUF – UTF-8 JSON bytes (fallback); inject a real Protobuf binary
 *               encoder via setProtobufEncoderFn() to produce proto3 wire bytes.
 *
 * Binary encoder injection:
 * @code
 * encoder.setAvroEncoderFn([](const nlohmann::json& payload) {
 *     // avro-cpp encoding here
 *     return avro::encodeToBytes(payload);
 * });
 * @endcode
 *
 * Schema auto-registration: when @c config.auto_register_schemas is true
 * (the default) the encoder calls @c ensureCollectionSchema() on first use
 * per collection and caches the schema ID.
 *
 * Thread-safe when the underlying SchemaRegistryClient is thread-safe.
 *
 * Usage:
 * @code
 * SchemaRegistryConfig cfg;
 * cfg.auto_register_schemas = true;
 * cfg.default_format = SchemaFormat::AVRO;
 *
 * SchemaRegistryClient client(cfg);
 * CdcSchemaEncoder encoder(&client);
 *
 * Changefeed::ChangeEvent ev = ...;
 * auto encoded = encoder.encode(ev, "orders");
 * // encoded.data = [0x00][schema_id 4B BE][json_bytes]
 * @endcode
 */
class CdcSchemaEncoder {
public:
    // ── Default schema templates ───────────────────────────────────────────

    /**
     * @brief Returns the default Avro schema JSON for a CDC event collection.
     *
     * The schema models the `ChangeEvent` payload with optional before/after
     * document snapshots and standard source metadata.
     */
    static std::string defaultAvroSchema(const std::string& collection) {
        nlohmann::json schema = {
            {"type",      "record"},
            {"name",      "CdcEvent"},
            {"namespace", "io.themisdb.cdc." + collection},
            {"doc",       "ThemisDB CDC change event for collection: " + collection},
            {"fields",    nlohmann::json::array({
                {{"name", "sequence"},   {"type", "long"},   {"doc", "Monotonic sequence number"}},
                {{"name", "operation"},  {"type", "string"}, {"doc", "PUT | DELETE | TRANSACTION_COMMIT | TRANSACTION_ROLLBACK"}},
                {{"name", "collection"}, {"type", "string"}, {"doc", "Source collection name"}},
                {{"name", "key"},        {"type", "string"}, {"doc", "Document key"}},
                {{"name", "value"},      {"type", nlohmann::json::array({"null", "string"})},
                                          {"default", nullptr}, {"doc", "Document value (null for DELETE)"}},
                {{"name", "before"},     {"type", nlohmann::json::array({"null", "string"})},
                                          {"default", nullptr}, {"doc", "Document state before this change"}},
                {{"name", "after"},      {"type", nlohmann::json::array({"null", "string"})},
                                          {"default", nullptr}, {"doc", "Document state after this change"}},
                {{"name", "timestamp_ms"}, {"type", "long"}, {"doc", "Event timestamp (epoch ms)"}},
                {{"name", "source"},     {"type", {
                    {"type", "record"},
                    {"name", "CdcSource"},
                    {"fields", nlohmann::json::array({
                        {{"name", "connector"}, {"type", "string"}},
                        {{"name", "db"},        {"type", "string"}},
                        {{"name", "table"},     {"type", "string"}},
                        {{"name", "ts_ms"},     {"type", "long"}}
                    })}
                }}}
            })}
        };
        return schema.dump();
    }

    /**
     * @brief Returns the default JSON Schema for a CDC event collection.
     */
    static std::string defaultJsonSchema(const std::string& collection) {
        nlohmann::json schema = {
            {"$schema",     "http://json-schema.org/draft-07/schema#"},
            {"title",       "CdcEvent"},
            {"description", "ThemisDB CDC change event for collection: " + collection},
            {"type",        "object"},
            {"required",    nlohmann::json::array({"sequence", "operation", "collection", "key", "timestamp_ms"})},
            {"properties",  {
                {"sequence",     {{"type", "integer"}, {"minimum", 0}}},
                {"operation",    {{"type", "string"}, {"enum", nlohmann::json::array({"PUT", "DELETE", "TRANSACTION_COMMIT", "TRANSACTION_ROLLBACK"})}}},
                {"collection",   {{"type", "string"}}},
                {"key",          {{"type", "string"}}},
                {"value",        {{"type", nlohmann::json::array({"string", "null"})}}},
                {"before",       {{"type", nlohmann::json::array({"string", "null"})}}},
                {"after",        {{"type", nlohmann::json::array({"string", "null"})}}},
                {"timestamp_ms", {{"type", "integer"}}},
                {"redacted",     {{"type", "boolean"}, {"default", false}}}
            }}
        };
        return schema.dump();
    }

    /**
     * @brief Returns the default Protobuf 3 schema for a CDC event collection.
     */
    static std::string defaultProtobufSchema(const std::string& collection) {
        return std::string(R"(syntax = "proto3";
package io.themisdb.cdc.)") + collection + R"(;

message CdcSource {
  string connector = 1;
  string db        = 2;
  string table     = 3;
  int64  ts_ms     = 4;
}

message CdcEvent {
  int64     sequence     = 1;
  string    operation    = 2;
  string    collection   = 3;
  string    key          = 4;
  string    value        = 5;
  string    before       = 6;
  string    after        = 7;
  int64     timestamp_ms = 8;
  CdcSource source       = 9;
  bool      redacted     = 10;
}
)";
    }

    // ── Lifecycle ──────────────────────────────────────────────────────────

    /**
     * @brief Construct the encoder.
     * @param client  Schema registry client (not owned; must outlive encoder).
     */
    explicit CdcSchemaEncoder(SchemaRegistryClient* client)
        : client_(client) {}

    // Non-copyable; movable.
    CdcSchemaEncoder(const CdcSchemaEncoder&) = delete;
    CdcSchemaEncoder& operator=(const CdcSchemaEncoder&) = delete;

    // ── Binary encoder injection ───────────────────────────────────────────

    /**
     * @brief Callable type for a binary payload encoder.
     *
     * Called with the logical JSON payload; must return the native binary
     * encoding (e.g. Avro OCF-less datum or Protobuf wire bytes).
     * Returning an empty vector causes the encoder to fall back to UTF-8 JSON.
     */
    using BinaryEncoderFn =
        std::function<std::vector<uint8_t>(const nlohmann::json& payload)>;

    /**
     * @brief Inject a native Avro binary encoder.
     *
     * When set, `encode()` calls this function for events whose schema format
     * is `SchemaFormat::AVRO`.  If the function returns an empty vector the
     * UTF-8 JSON fallback is used instead.
     *
     * @param fn  Avro encoder callable (or nullptr to remove).
     */
    void setAvroEncoderFn(BinaryEncoderFn fn) {
        avro_encoder_fn_ = std::move(fn);
    }

    /**
     * @brief Inject a native Protobuf binary encoder.
     *
     * When set, `encode()` calls this function for events whose schema format
     * is `SchemaFormat::PROTOBUF`.  If the function returns an empty vector the
     * UTF-8 JSON fallback is used instead.
     *
     * @param fn  Protobuf encoder callable (or nullptr to remove).
     */
    void setProtobufEncoderFn(BinaryEncoderFn fn) {
        protobuf_encoder_fn_ = std::move(fn);
    }

    // ── Encoding ───────────────────────────────────────────────────────────

    /**
     * @brief Encode a ChangeEvent using the Confluent wire format.
     *
     * @param event       CDC change event to encode.
     * @param collection  Collection name.  When empty, derived from the event
     *                    key (first ':'-delimited segment).
     * @return EncodedEvent with wire-format bytes and metadata.
     */
    EncodedEvent encode(const Changefeed::ChangeEvent& event,
                        const std::string& collection = "") const {
        const std::string coll =
            collection.empty() ? collectionFromKey(event.key) : collection;

        const std::string subject = subjectForCollection(coll);
        const int32_t schema_id  = ensureCollectionSchema(coll);

        const nlohmann::json payload = eventToPayload(event, coll);
        const SchemaFormat   fmt     = client_->config().default_format;

        std::vector<uint8_t> payload_bytes;
        if (fmt == SchemaFormat::AVRO && avro_encoder_fn_) {
            payload_bytes = avro_encoder_fn_(payload);
        } else if (fmt == SchemaFormat::PROTOBUF && protobuf_encoder_fn_) {
            payload_bytes = protobuf_encoder_fn_(payload);
        }
        if (payload_bytes.empty()) {
            // JSON fallback for JSON format or when binary encoder not injected / returned empty.
            const std::string json_str = payload.dump();
            payload_bytes.assign(json_str.begin(), json_str.end());
        }

        EncodedEvent result;
        result.data      = buildWireFormat(schema_id, payload_bytes);
        result.schema_id = schema_id;
        result.format    = fmt;
        result.subject   = subject;
        return result;
    }

    /**
     * @brief Decode the payload portion of a wire-format message to JSON.
     *
     * Strips the five-byte header and interprets the remaining bytes as UTF-8
     * JSON.  Returns nullopt when the magic byte is invalid or the payload
     * cannot be parsed as JSON.
     *
     * @param wire_bytes  Bytes produced by encode().
     * @return Decoded JSON or nullopt on failure.
     */
    std::optional<nlohmann::json> decodeToJson(
        const std::vector<uint8_t>& wire_bytes) const {
        if (wire_bytes.size() < SCHEMA_REGISTRY_HEADER_SIZE) return std::nullopt;
        if (wire_bytes[0] != SCHEMA_REGISTRY_MAGIC_BYTE)    return std::nullopt;

        const auto payload_begin = wire_bytes.begin() + SCHEMA_REGISTRY_HEADER_SIZE;
        const std::string json_str(payload_begin, wire_bytes.end());
        try {
            return nlohmann::json::parse(json_str);
        } catch (const nlohmann::json::parse_error&) {
            return std::nullopt;
        }
    }

    /**
     * @brief Extract the schema ID from a wire-format message header.
     *
     * @param wire_bytes  Bytes produced by encode().
     * @return Schema ID or -1 when the header is invalid.
     */
    static int32_t extractSchemaId(const std::vector<uint8_t>& wire_bytes) {
        if (wire_bytes.size() < SCHEMA_REGISTRY_HEADER_SIZE) return -1;
        if (wire_bytes[0] != SCHEMA_REGISTRY_MAGIC_BYTE)    return -1;
        // Schema ID is stored big-endian at bytes 1–4.
        return static_cast<int32_t>(
            (static_cast<uint32_t>(wire_bytes[1]) << 24) |
            (static_cast<uint32_t>(wire_bytes[2]) << 16) |
            (static_cast<uint32_t>(wire_bytes[3]) <<  8) |
            (static_cast<uint32_t>(wire_bytes[4])       ));
    }

    /**
     * @brief Get (or auto-register) the schema ID for a collection.
     *
     * When auto_register_schemas is true in the client config the default
     * schema template for the configured format is registered the first time
     * a collection is seen.  The ID is cached in-process.
     *
     * @param collection  Collection name.
     * @return Schema ID.
     */
    int32_t ensureCollectionSchema(const std::string& collection) const {
        // Per-collection in-process cache (avoids hitting the backend on every event).
        {
            std::lock_guard<std::mutex> lock(local_cache_mutex_);
            auto it = local_schema_id_cache_.find(collection);
            if (it != local_schema_id_cache_.end()) return it->second;
        }

        const std::string subject = subjectForCollection(collection);
        int32_t id = -1;

        // Try to look up an already-registered schema.
        auto existing = client_->getLatestSchema(subject);
        if (existing) {
            id = existing->id;
        } else if (client_->config().auto_register_schemas) {
            // Auto-register with the default schema template for this format.
            const auto fmt  = client_->config().default_format;
            std::string def_schema;
            switch (fmt) {
                case SchemaFormat::AVRO:     def_schema = defaultAvroSchema(collection); break;
                case SchemaFormat::PROTOBUF: def_schema = defaultProtobufSchema(collection); break;
                default:                     def_schema = defaultJsonSchema(collection); break;
            }
            id = client_->ensureSchema(subject, def_schema, fmt);
        } else {
            throw std::runtime_error(
                "Schema not found for subject '" + subject +
                "' and auto_register_schemas is disabled");
        }

        {
            std::lock_guard<std::mutex> lock(local_cache_mutex_);
            local_schema_id_cache_[collection] = id;
        }
        return id;
    }

    /**
     * @brief Invalidate the encoder's local schema ID cache.
     *
     * Call this to force re-fetching schema IDs on next encode (e.g. after a
     * schema evolution).
     */
    void clearLocalCache() {
        std::lock_guard<std::mutex> lock(local_cache_mutex_);
        local_schema_id_cache_.clear();
    }

private:
    SchemaRegistryClient* client_;

    BinaryEncoderFn avro_encoder_fn_;
    BinaryEncoderFn protobuf_encoder_fn_;

    mutable std::mutex local_cache_mutex_;
    mutable std::unordered_map<std::string, int32_t> local_schema_id_cache_;

    // ── Helpers ────────────────────────────────────────────────────────────

    /** @brief Derive collection name from a ThemisDB key ("collection:id"). */
    static std::string collectionFromKey(const std::string& key) {
        const auto pos = key.find(':');
        return pos != std::string::npos ? key.substr(0, pos) : key;
    }

    /** @brief Build the registry subject name for a collection. */
    std::string subjectForCollection(const std::string& collection) const {
        return client_->config().topic_prefix + collection + "-value";
    }

    /** @brief Convert a ChangeEventType to a string label. */
    static std::string operationString(Changefeed::ChangeEventType type) {
        switch (type) {
            case Changefeed::ChangeEventType::EVENT_PUT:
                return "PUT";
            case Changefeed::ChangeEventType::EVENT_DELETE:
                return "DELETE";
            case Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT:
                return "TRANSACTION_COMMIT";
            case Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK:
                return "TRANSACTION_ROLLBACK";
            default:
                return "UNKNOWN";
        }
    }

    /**
     * @brief Serialize a ChangeEvent to the JSON payload object.
     *
     * This is the logical event representation.  When full Avro or Protobuf
     * binary encoding is required, replace this method's output with the
     * appropriate binary serializer.
     */
    static nlohmann::json eventToPayload(const Changefeed::ChangeEvent& event,
                                         const std::string& collection) {
        nlohmann::json payload;
        payload["sequence"]     = event.sequence;
        payload["operation"]    = operationString(event.type);
        payload["collection"]   = collection;
        payload["key"]          = event.key;
        payload["timestamp_ms"] = event.timestamp_ms;

        if (event.value.has_value()) {
            payload["value"] = *event.value;
        } else {
            payload["value"] = nullptr;
        }

        if (event.before_snapshot.has_value()) {
            payload["before"] = *event.before_snapshot;
        } else {
            payload["before"] = nullptr;
        }

        if (event.after_snapshot.has_value()) {
            payload["after"] = *event.after_snapshot;
        } else {
            payload["after"] = nullptr;
        }

        if (event.redacted) {
            payload["redacted"] = true;
        }

        payload["source"] = {
            {"connector", "themisdb"},
            {"db",        "themisdb"},
            {"table",     collection},
            {"ts_ms",     event.timestamp_ms}
        };

        return payload;
    }

    /**
     * @brief Build the Confluent wire-format bytes.
     *
     * Layout: [magic(1)][schema_id(4 BE)][payload_bytes...]
     */
    static std::vector<uint8_t> buildWireFormat(
        int32_t schema_id,
        const std::vector<uint8_t>& payload) {

        std::vector<uint8_t> wire;
        wire.reserve(SCHEMA_REGISTRY_HEADER_SIZE + payload.size());

        // Magic byte
        wire.push_back(SCHEMA_REGISTRY_MAGIC_BYTE);

        // 4-byte big-endian schema ID
        const auto uid = static_cast<uint32_t>(schema_id);
        wire.push_back(static_cast<uint8_t>((uid >> 24) & 0xFF));
        wire.push_back(static_cast<uint8_t>((uid >> 16) & 0xFF));
        wire.push_back(static_cast<uint8_t>((uid >>  8) & 0xFF));
        wire.push_back(static_cast<uint8_t>( uid        & 0xFF));

        // Payload
        wire.insert(wire.end(), payload.begin(), payload.end());
        return wire;
    }
};

}  // namespace cdc
}  // namespace themis
