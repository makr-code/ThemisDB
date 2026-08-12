/**
 * @file schema_cdc.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "replication/schema_cdc.h"
#include <stdexcept>

#include <chrono>
#include <memory>
#include "utils/logger.h"

namespace themisdb {
namespace replication {

// ─────────────────────────────────────────────────────────────────────────────
// SchemaAwareCDCBridge
// ─────────────────────────────────────────────────────────────────────────────

SchemaAwareCDCBridge::SchemaAwareCDCBridge(
    std::shared_ptr<ReplicationManager> repl_mgr,
    const themis::cdc::SchemaRegistryConfig& reg_cfg)
    : repl_mgr_(std::move(repl_mgr))
    , reg_cfg_(reg_cfg)
{
    // Build the shared registry client used by all collection encoders
    registry_ = std::make_shared<themis::cdc::SchemaRegistryClient>(reg_cfg_);
}

SchemaAwareCDCBridge::~SchemaAwareCDCBridge() {
    stop();
}

uint32_t SchemaAwareCDCBridge::registerCollection(
    const std::string& collection,
    const std::string& schema_def)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Determine schema definition to use
    std::string def = schema_def;
    if (def.empty()) {
        switch (reg_cfg_.default_format) {
            case themis::cdc::SchemaFormat::AVRO:
                def = themis::cdc::CdcSchemaEncoder::defaultAvroSchema(collection);
                break;
            case themis::cdc::SchemaFormat::PROTOBUF:
                def = themis::cdc::CdcSchemaEncoder::defaultProtobufSchema(collection);
                break;
            default:
                def = themis::cdc::CdcSchemaEncoder::defaultJsonSchema(collection);
                break;
        }
    }

    // Register the schema and cache the resulting ID
    const std::string subject =
        collection + "-" + themis::cdc::schemaFormatString(reg_cfg_.default_format);
    int32_t schema_id = registry_->ensureSchema(subject, def, reg_cfg_.default_format);
    registered_[collection] = static_cast<uint32_t>(schema_id);
    return static_cast<uint32_t>(schema_id);
}

void SchemaAwareCDCBridge::deregisterCollection(const std::string& collection) {
    std::lock_guard<std::mutex> lock(mutex_);
    registered_.erase(collection);
}

uint64_t SchemaAwareCDCBridge::subscribe(
    const std::string& collection,
    EncodedCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t id = next_sub_id_.fetch_add(1, std::memory_order_relaxed);
    subscriptions_.push_back(Subscription{id, collection, std::move(callback)});
    return id;
}

void SchemaAwareCDCBridge::unsubscribe(uint64_t subscription_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
            [subscription_id](const Subscription& s) {
                return s.id == subscription_id;
            }),
        subscriptions_.end());
}

void SchemaAwareCDCBridge::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) return;
    if (repl_mgr_) {
        repl_mgr_->addListener(shared_from_this());
    }
    started_ = true;
}

void SchemaAwareCDCBridge::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    started_ = false;
    // Note: ReplicationManager does not expose removeListener; the bridge
    // becomes a no-op by ignoring events after stopped_ is false.
}

SchemaAwareCDCBridge::Stats SchemaAwareCDCBridge::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    Stats s = stats_;
    s.events_skipped = skipped_events_.load(std::memory_order_relaxed);
    return s;
}

void SchemaAwareCDCBridge::onWALEntryApplied(const WALEntry& wal_entry) {
    // Snapshot state under lock – do NOT hold mutex_ while acquiring
    // stats_mutex_ to prevent lock ordering inversion.
    uint32_t schema_id = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_) return;
        auto it = registered_.find(wal_entry.collection);
        if (it == registered_.end()) {
            // Increment skip counter lock-free (no nested lock needed)
            skipped_events_.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        schema_id = it->second;
    }

    // Convert WALEntry → CDC ChangeEvent
    themis::Changefeed::ChangeEvent ev;
    ev.sequence     = wal_entry.sequence_number;
    ev.key          = wal_entry.collection + ":" + wal_entry.document_id;
    ev.timestamp_ms = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
        .count());

    if (wal_entry.operation == "DELETE") {
        ev.type  = themis::Changefeed::ChangeEventType::EVENT_DELETE;
        ev.value = std::nullopt;
    } else {
        ev.type  = themis::Changefeed::ChangeEventType::EVENT_PUT;
        ev.value = wal_entry.data;
    }

    // Encode using the schema registry client
    std::vector<uint8_t> payload_bytes;
    try {
        themis::cdc::CdcSchemaEncoder encoder(registry_.get());
        auto encoded = encoder.encode(ev, wal_entry.collection);
        payload_bytes = std::move(encoded.data);
    } catch (...) {
        THEMIS_WARN("schema_cdc: unhandled exception caught");
        std::lock_guard<std::mutex> slock(stats_mutex_);
        ++stats_.encoding_errors;
        return;
    }

    // Build output event
    SchemaEncodedEvent out;
    out.collection  = wal_entry.collection;
    out.operation   = wal_entry.operation;
    out.document_id = wal_entry.document_id;
    out.sequence    = wal_entry.sequence_number;
    out.schema_id   = schema_id;
    out.payload     = std::move(payload_bytes);

    {
        std::lock_guard<std::mutex> slock(stats_mutex_);
        ++stats_.events_encoded;
    }

    dispatch(out);
}

void SchemaAwareCDCBridge::dispatch(const SchemaEncodedEvent& ev) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& sub : subscriptions_) {
        if (sub.collection.empty() || sub.collection == ev.collection) {
            sub.callback(ev);
        }
    }
}

}  // namespace replication
}  // namespace themisdb

