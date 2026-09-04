/**
 * @file schema_cdc.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "replication/replication_manager.h"
#include "cdc/schema_registry.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace replication {

// ─────────────────────────────────────────────────────────────────────────────
// SchemaEncodedEvent
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single CDC event encoded with the Confluent wire format.
 *
 * The `payload` field contains the full wire-format bytes (magic byte +
 * 4-byte schema ID + serialised event body).  Downstream Kafka producers,
 * Kafka Connect sinks, and other Confluent-compatible consumers can decode
 * this directly.
 */
struct SchemaEncodedEvent {
    std::string collection;     ///< Source collection name
    std::string operation;      ///< "INSERT" | "UPDATE" | "DELETE"
    std::string document_id;    ///< Document key
    uint64_t    sequence{0};    ///< WAL sequence number
    uint32_t    schema_id{0};   ///< Registered schema ID
    std::vector<uint8_t> payload; ///< Full Confluent wire-format bytes
};

// ─────────────────────────────────────────────────────────────────────────────
// SchemaAwareCDCBridge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Bridges WAL-based CDC events with schema-encoded output.
 *
 * Wraps a `ReplicationManager` (or its `CDCManager`) and intercepts WAL
 * entries.  For each entry whose collection is registered, the bridge:
 *  1. Resolves (or auto-registers) the collection's schema via
 *     `SchemaRegistryClient`.
 *  2. Encodes the event using `CdcSchemaEncoder`.
 *  3. Dispatches the resulting `SchemaEncodedEvent` to all registered
 *     subscriber callbacks.
 */
class SchemaAwareCDCBridge : public IReplicationListener,
                             public std::enable_shared_from_this<SchemaAwareCDCBridge> {
public:
    using EncodedCallback = std::function<void(const SchemaEncodedEvent&)>;

    /**
     * @brief Construct the bridge.
     *
     * @param repl_mgr  The replication manager whose CDC events to capture.
     * @param reg_cfg   Schema registry configuration (URL, format, auth …).
     */
    SchemaAwareCDCBridge(
        std::shared_ptr<ReplicationManager> repl_mgr,
        const themis::cdc::SchemaRegistryConfig& reg_cfg);

    ~SchemaAwareCDCBridge();

    // ── Collection registration ─────────────────────────────────────────────

    /**
     * @brief Register a collection for schema-encoded CDC output.
     *
     * If `schema_def` is empty the encoder uses the default Avro/JSON/Protobuf
     * template from `CdcSchemaEncoder::defaultAvroSchema()` etc.
     *
     * @param collection  Collection name to watch.
     * @param schema_def  Optional explicit schema definition string.
     * @return Registered schema ID.
     */
    uint32_t registerCollection(
        const std::string& collection,
        const std::string& schema_def = "");

    /**
     * @brief Deregister a collection.  Future WAL entries for that collection
     *        are silently dropped.
     */
    void deregisterCollection(const std::string& collection);

    // ── Subscription ────────────────────────────────────────────────────────

    /**
     * @brief Subscribe to encoded CDC events for @p collection.
     *
     * Use an empty string to subscribe to all registered collections.
     *
     * @return Subscription ID that can be passed to unsubscribe().
     */
    uint64_t subscribe(const std::string& collection,
                       EncodedCallback callback);

    /** @brief Unsubscribe a previously registered callback. */
    void unsubscribe(uint64_t subscription_id);

    // ── Lifecycle ───────────────────────────────────────────────────────────

    /**
     * @brief Register this bridge as a listener on the replication manager.
     */
    void start();

    /**
     * @brief Deregister from the replication manager.
     */
    void stop();

    // ── Statistics ──────────────────────────────────────────────────────────

    struct Stats {
        uint64_t events_encoded{0};    ///< Total events successfully encoded
        uint64_t events_skipped{0};    ///< Events from unregistered collections
        uint64_t encoding_errors{0};   ///< Encoding failures
    };

    Stats getStats() const;

    // ── IReplicationListener (only WAL entries matter) ──────────────────────

    void onWALEntryApplied(const WALEntry& entry) override;
    void onRoleChange(ReplicationRole, ReplicationRole) override {}
    void onLeaderElected(const std::string&) override {}
    void onReplicaAdded(const ReplicaInfo&) override {}
    void onReplicaRemoved(const std::string&) override {}
    void onConflictDetected(const std::string&) override {}
    void onReplicationLagWarning(int64_t) override {}
    void onReplicaHealthChanged(const std::string&,
                                HealthStatus, HealthStatus) override {}
    void onFailoverStarted(const std::string&,
                           const std::string&) override {}
    void onFailoverCompleted(const std::string&, bool) override {}
    void onNetworkPartitionDetected(
        const std::vector<std::string>&) override {}

private:
    std::shared_ptr<ReplicationManager>           repl_mgr_;
    themis::cdc::SchemaRegistryConfig             reg_cfg_;
    std::shared_ptr<themis::cdc::SchemaRegistryClient> registry_;

    mutable std::mutex mutex_;

    // Registered collections: collection -> schema_id
    std::unordered_map<std::string, uint32_t> registered_;

    struct Subscription {
        uint64_t       id = 0;
        std::string    collection;  // empty = all
        EncodedCallback callback;
    };
    std::vector<Subscription> subscriptions_;
    std::atomic<uint64_t>     next_sub_id_{1};

    // Stats are maintained with individual atomics to avoid nested locking
    // (mutex_ must never be held while acquiring stats_mutex_).
    mutable std::mutex stats_mutex_;
    Stats stats_;
    std::atomic<uint64_t> skipped_events_{0};  // incremented lock-free

    bool started_{false};

    void dispatch(const SchemaEncodedEvent& ev);
};

}  // namespace replication
}  // namespace themisdb
