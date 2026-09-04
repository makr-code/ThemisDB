/**
 * @file adapter_capability_announcement.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>
#include "governance/gdpr_subject_rights.h"

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// AdapterDomainType — coarse domain category broadcast with each announcement
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Coarse-grained domain category for a LoRA adapter.
 *
 * Used by `AdaptiveShardRouter` to score domain affinity when routing
 * queries to shards.  Values are intentionally broad so that a shard can
 * advertise without exposing tenant or data details.
 */
enum class AdapterDomainType {
    GENERAL,            ///< No specific specialisation
    SECURITY_MONITOR,   ///< Security anomaly detection / IntentAlert domain
    SCHEMA_ADVISOR,     ///< Schema evolution / dead-weight analysis
    TRANSACTION,        ///< Transaction conflict prediction / batch hints
    MULTI_TENANT,       ///< Multi-tenant workload fingerprinting
    EXPLAINABILITY,     ///< AI decision explanation / DBA dialog
    VECTOR_SEARCH,      ///< ANN / vector-index specialisation
    PROCESS_MINING,     ///< Process mining / conformance checking
    GEOSPATIAL,         ///< Geospatial query optimisation
    LEGAL,              ///< Legal document analysis / contract intelligence
    MEDICAL,            ///< Medical / healthcare NLP and clinical decision support
    CUSTOM              ///< Shard-defined domain (see custom_domain_label)
};

/// Convert `AdapterDomainType` to a human-readable string.
inline std::string adapterDomainTypeToString(AdapterDomainType t) {
    switch (t) {
        case AdapterDomainType::GENERAL:          return "GENERAL";
        case AdapterDomainType::SECURITY_MONITOR: return "SECURITY_MONITOR";
        case AdapterDomainType::SCHEMA_ADVISOR:   return "SCHEMA_ADVISOR";
        case AdapterDomainType::TRANSACTION:      return "TRANSACTION";
        case AdapterDomainType::MULTI_TENANT:     return "MULTI_TENANT";
        case AdapterDomainType::EXPLAINABILITY:   return "EXPLAINABILITY";
        case AdapterDomainType::VECTOR_SEARCH:    return "VECTOR_SEARCH";
        case AdapterDomainType::PROCESS_MINING:   return "PROCESS_MINING";
        case AdapterDomainType::GEOSPATIAL:       return "GEOSPATIAL";
        case AdapterDomainType::LEGAL:            return "LEGAL";
        case AdapterDomainType::MEDICAL:          return "MEDICAL";
        case AdapterDomainType::CUSTOM:           return "CUSTOM";
    }
    return "UNKNOWN";
}

// ─────────────────────────────────────────────────────────────────────────────
// AdapterCapabilityAnnouncement
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Gossip payload broadcast by each shard to advertise its active LoRA
 *        adapter capabilities.
 *
 * Serialised via `toJson()` / `fromJson()` and embedded in
 * `GossipMessage::payload` with `message_type = "adapter_capability"`.
 *
 * Privacy contract:
 *  - `shard_id` identifies the broadcasting shard (not individual tenants).
 *  - No raw training data or model weights are included.
 *  - `performance_delta_p99_ms` is an aggregate delta, not per-query logs.
 */
struct AdapterCapabilityAnnouncement {
    // Identity
    std::string shard_id;                   ///< Broadcasting shard identifier
    std::string adapter_id;                 ///< Adapter identifier in AdapterRegistry
    std::string adapter_version;            ///< Semantic version, e.g. "v1.3.0"

    // Domain
    AdapterDomainType domain_type = AdapterDomainType::GENERAL;
    std::string custom_domain_label;        ///< Only used when domain_type == CUSTOM

    // Performance metrics (aggregate — no per-query detail)
    double performance_delta_p99_ms = 0.0; ///< p99 latency delta vs. base (negative = faster)
    double accuracy_delta           = 0.0; ///< Accuracy delta vs. base model [−1, 1]
    size_t training_samples         = 0;   ///< Number of samples used in last training round

    // Federated round bookkeeping
    uint64_t    federation_round    = 0;    ///< Last federated round this adapter participated in
    std::string last_global_delta_version;  ///< Version of last applied global gradient delta

    // Liveness
    std::chrono::system_clock::time_point announced_at;

    /// When true, this announcement signals that the adapter has been unloaded
    /// and is no longer available on the originating shard.
    bool is_withdrawal = false;

    // ── Serialisation ────────────────────────────────────────────────────────

    [[nodiscard]] nlohmann::json toJson() const {
        using nlohmann::json;
        return json{
            {"shard_id",                   shard_id},
            {"adapter_id",                 adapter_id},
            {"adapter_version",            adapter_version},
            {"domain_type",                adapterDomainTypeToString(domain_type)},
            {"custom_domain_label",        custom_domain_label},
            {"performance_delta_p99_ms",   performance_delta_p99_ms},
            {"accuracy_delta",             accuracy_delta},
            {"training_samples",           training_samples},
            {"federation_round",           federation_round},
            {"last_global_delta_version",  last_global_delta_version},
            {"is_withdrawal",              is_withdrawal},
            {"announced_at_ms",
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 announced_at.time_since_epoch()).count()}
        };
    }

    [[nodiscard]] static AdapterCapabilityAnnouncement fromJson(const nlohmann::json& j) {
        AdapterCapabilityAnnouncement a;
        a.shard_id                  = j.value("shard_id", "");
        a.adapter_id                = j.value("adapter_id", "");
        a.adapter_version           = j.value("adapter_version", "");
        a.custom_domain_label       = j.value("custom_domain_label", "");
        a.performance_delta_p99_ms  = j.value("performance_delta_p99_ms", 0.0);
        a.accuracy_delta            = j.value("accuracy_delta", 0.0);
        a.training_samples          = j.value<size_t>("training_samples", 0);
        a.federation_round          = j.value<uint64_t>("federation_round", 0);
        a.last_global_delta_version = j.value("last_global_delta_version", "");
        a.is_withdrawal             = j.value("is_withdrawal", false);

        const std::string dt = j.value("domain_type", "GENERAL");
        if      (dt == "SECURITY_MONITOR") {
          a.domain_type = AdapterDomainType::SECURITY_MONITOR;
        }
        else if (dt == "SCHEMA_ADVISOR")   a.domain_type = AdapterDomainType::SCHEMA_ADVISOR;
        else if (dt == "TRANSACTION")      a.domain_type = AdapterDomainType::TRANSACTION;
        else if (dt == "MULTI_TENANT")     a.domain_type = AdapterDomainType::MULTI_TENANT;
        else if (dt == "EXPLAINABILITY")   a.domain_type = AdapterDomainType::EXPLAINABILITY;
        else if (dt == "VECTOR_SEARCH")    a.domain_type = AdapterDomainType::VECTOR_SEARCH;
        else if (dt == "PROCESS_MINING")   a.domain_type = AdapterDomainType::PROCESS_MINING;
        else if (dt == "GEOSPATIAL")       a.domain_type = AdapterDomainType::GEOSPATIAL;
        else if (dt == "LEGAL")            a.domain_type = AdapterDomainType::LEGAL;
        else if (dt == "MEDICAL")          a.domain_type = AdapterDomainType::MEDICAL;
        else if (dt == "CUSTOM")           a.domain_type = AdapterDomainType::CUSTOM;
        else                               a.domain_type = AdapterDomainType::GENERAL;

        const auto ms = j.value<int64_t>("announced_at_ms", 0);
        a.announced_at = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(ms));
        return a;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GossipAdapterPublisher
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Publishes `AdapterCapabilityAnnouncement` messages into the gossip
 *        network after each training round.
 *
 * The publisher is the single integration point between the training layer
 * (`IncrementalLoRATrainer`, `AdapterRegistry`) and the gossip transport.
 * It wraps the announcement in a `GossipMessage` with
 * `message_type = "adapter_capability"` and forwards it to the protocol.
 *
 * Usage:
 * @code
 *   GossipAdapterPublisher publisher(shard_id, gossip_protocol);
 *   publisher.announce(announcement);
 * @endcode
 *
 * Thread safety: `announce()` is thread-safe (mutex-protected).
 */
class GossipAdapterPublisher {
public:
    /**
     * @brief Callback invoked when a remote shard broadcasts an announcement.
     *
     * Consumers (e.g. `AdaptiveShardRouter`) register via
     * `setAnnouncementCallback()` to receive remote announcements.
     */
    using AnnouncementCallback =
        std::function<void(const AdapterCapabilityAnnouncement&)>;

    /**
     * @brief Construct publisher for a specific shard.
     * @param local_shard_id   Shard identifier for outbound messages.
     * @param gossip_message_fn  Callable that accepts a `nlohmann::json` payload
     *                           and dispatches it via the live GossipProtocol.
     *                           Signature: `void(nlohmann::json payload)`.
     */
    explicit GossipAdapterPublisher(
        std::string local_shard_id,
        std::function<void(nlohmann::json)> gossip_message_fn);

    ~GossipAdapterPublisher();

    GossipAdapterPublisher(const GossipAdapterPublisher&)            = delete;
    GossipAdapterPublisher& operator=(const GossipAdapterPublisher&) = delete;
    GossipAdapterPublisher(GossipAdapterPublisher&&)                 noexcept = default;
    GossipAdapterPublisher& operator=(GossipAdapterPublisher&&)      noexcept = default;

    /**
     * @brief Broadcast an adapter capability announcement to all peers.
     *
     * Stamps `announced_at` with the current wall-clock time, serialises the
     * announcement as JSON, and dispatches it via the gossip message function.
     *
     * @param announcement  Capability data to broadcast.
     */
    void announce(AdapterCapabilityAnnouncement announcement);

    /**
     * @brief Handle an inbound gossip message that may contain an announcement.
     *
     * Call this from the `GossipProtocol::handleMessage()` dispatch path when
     * `message_type == "adapter_capability"`.  Deserialises the payload and
     * invokes the registered `AnnouncementCallback`.
     *
     * @param payload  JSON payload from the incoming `GossipMessage`.
     */
    void handleInboundMessage(const nlohmann::json& payload);

    /**
     * @brief Register callback for inbound remote announcements.
     * @param cb  Callback invoked on the calling thread of `handleInboundMessage`.
     */
    void setAnnouncementCallback(AnnouncementCallback cb);

    /**
     * @brief Return the most recently sent announcement (or nullopt if none).
     */
    [[nodiscard]] std::optional<AdapterCapabilityAnnouncement> lastAnnouncement() const;

    /**
     * @brief GDPR erase: clear buffered announcement payload (DK-OR).
     *
     * Clears `last_announcement_` and increments `erase_count_`.
     */
    themis::governance::StoreErasureResult erase(
        const std::string& subject_id = "",
        themis::governance::Regulation regulation = themis::governance::Regulation::GDPR);

    [[nodiscard]] size_t eraseCount() const;

private:
    std::string                          local_shard_id_;
    std::function<void(nlohmann::json)>  gossip_message_fn_;
    AnnouncementCallback                 on_announcement_;
    mutable std::mutex                   mutex_;
    std::optional<AdapterCapabilityAnnouncement> last_announcement_;
    size_t                               erase_count_{0}; ///< DK-OR: GDPR erase ops
};

} // namespace themis::distributed_knowledge
