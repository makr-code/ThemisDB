/**
 * @file edition_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Edition Manager
 * ========================
 * Runtime feature-gating for Community / Enterprise / Hyperscaler editions.
 *
 * The EditionManager is the single authoritative source for feature and
 * resource-limit decisions at runtime.  It combines two independent checks:
 *
 *   1. Compile-time gate  – edition::IsFeatureEnabled() from edition.h
 *      If the binary was built for Community edition, Enterprise-only features
 *      are never allowed regardless of any runtime state.
 *
 *   2. Runtime gate – license::RuntimeLicenseGate::instance()
 *      For Enterprise/Hyperscaler binaries, the active license must also be
 *      valid ("active" or "grace") for Enterprise-only features to be used.
 *
 * Usage:
 *
 *   // Feature check (returns bool + optional error message):
 *   std::string err;
 *   if (!EditionManager::instance().isFeatureAvailable("field_encryption", err)) {
 *       return {ErrorCode::LicenseRequired, err};
 *   }
 *
 *   // Resource-limit check:
 *   if (!EditionManager::instance().checkNodeLimit(requested_nodes, err)) {
 *       return {ErrorCode::EditionLimitExceeded, err};
 *   }
 */

#pragma once

#include "themis/edition.h"
#include "themis/export.h"
#include "themis/gpu/ivram_policy.h"
#include "themis/sharding/ishard_limit_policy.h"
#include "themis/llm/illm_resource_policy.h"
#include "themis/tenant/itenant_quota_policy.h"
#include "themis/query/iquery_limit_policy.h"
#include "themis/network/iconnection_policy.h"
#include "themis/storage/istorage_ops_policy.h"
#include "themis/ratelimit/irate_limit_policy.h"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace edition {

// ============================================================================
// EditionManager
// ============================================================================

/**
 * @brief Process-wide singleton that enforces edition feature and resource
 *        limits at runtime.
 *
 * All public methods are thread-safe.
 */
class THEMIS_BASE_API EditionManager {
public:
    /// Retrieve the process-wide singleton instance.
    static EditionManager& instance();

    // Non-copyable / non-movable (singleton)
    EditionManager(const EditionManager&)            = delete;
    EditionManager& operator=(const EditionManager&) = delete;

    // -------------------------------------------------------------------------
    // Feature availability
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true if the named feature is available at runtime.
     *
     * The check is two-stage:
     *   1. Compile-time: edition::IsFeatureEnabled(feature_name) must be true.
     *   2. Runtime: license::RuntimeLicenseGate::instance() must allow it.
     *
     * Features that are not in the Enterprise gate list (i.e. Community
     * features) are always allowed.
     *
     * @param feature_name  One of: "enterprise_plugins", "multi_master",
     *                      "field_encryption", "rbac", "hsm", or any unknown
     *                      feature name (unknown → always allowed).
     */
    bool isFeatureAvailable(std::string_view feature_name) const;

    /**
     * @brief Like isFeatureAvailable() but also populates @p error_out with a
     *        human-readable explanation when returning false.
     */
    bool isFeatureAvailable(std::string_view feature_name,
                            std::string& error_out) const;

    // -------------------------------------------------------------------------
    // Resource-limit checks
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true if @p requested_nodes is within the edition limit.
     *
     * COMMUNITY:    up to SHARDING_MAX_NODES
     * ENTERPRISE:   up to SHARDING_MAX_NODES
     * HYPERSCALER:  unlimited (always returns true)
     *
     * @param requested_nodes  Number of shard nodes the caller wants to use.
     * @param error_out        Populated with an error string on failure.
     */
    bool checkNodeLimit(int requested_nodes, std::string& error_out) const;

    /**
     * @brief Returns true if @p requested_vram_gb is within the edition limit.
     *
     * COMMUNITY:    up to GPU_MAX_VRAM_GB
     * ENTERPRISE:   up to GPU_MAX_VRAM_GB
     * HYPERSCALER:  unlimited (always returns true)
     *
     * @param requested_vram_gb  VRAM to allocate in gigabytes.
     * @param error_out          Populated with an error string on failure.
     */
    bool checkVRAMLimit(int requested_vram_gb, std::string& error_out) const;

    // -------------------------------------------------------------------------
    // Edition information
    // -------------------------------------------------------------------------

    /// Returns the compile-time edition type.
    EditionType getEditionType() const noexcept;

    /// Returns the compile-time edition name string (e.g. "COMMUNITY").
    std::string_view getEditionName() const noexcept;

    /// Maximum shard nodes for this edition (-1 = unlimited).
    int getMaxNodes() const noexcept;

    /// Maximum GPU VRAM in GB for this edition (-1 = unlimited).
    int getMaxVRAMGB() const noexcept;

    // -------------------------------------------------------------------------
    // Feature enumeration
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the names of all known gated features that are currently
     *        available (both compile-time ON and runtime license valid).
     */
    std::vector<std::string> getAvailableFeatures() const;

    /**
     * @brief Returns the names of all known gated features that are currently
     *        unavailable (compile-time OFF or runtime license invalid).
     */
    std::vector<std::string> getUnavailableFeatures() const;

    // -------------------------------------------------------------------------
    // Upgrade guidance
    // -------------------------------------------------------------------------

    /**
     * @brief Returns a human-readable upgrade message for the given feature.
     *
     * The message explains which edition supports the feature and how to
     * obtain it.  Returns an empty string if the feature is already available.
     */
    std::string getUpgradeMessage(std::string_view feature_name) const;

    // -------------------------------------------------------------------------
    // Runtime resource-limit policies (signed edition-upgrade plugins)
    // -------------------------------------------------------------------------

    /**
     * @brief Install a signed VRAM policy supplied by an edition-upgrade plugin.
     *
     * The policy is accepted only when @p claimed_max_vram_gb does not exceed
     * the compile-time ceiling (@c GPU_MAX_VRAM_GB), or the ceiling is -1
     * (Hyperscaler — unlimited).  A rejected install is logged and returns @c false;
     * the binary retains its compile-time default.
     *
     * Thread-safe.
     *
     * @param policy              The IVRAMPolicy implementation provided by the plugin.
     * @param claimed_max_vram_gb Maximum VRAM (GiB) the policy declares it allows.
     *                            Must be <= GPU_MAX_VRAM_GB compile-time ceiling, or -1.
     * @return true   Policy accepted and installed.
     * @return false  Policy rejected — ceiling exceeded.
     */
    bool installVRAMPolicy(std::shared_ptr<gpu::IVRAMPolicy> policy,
                           int claimed_max_vram_gb);

    /**
     * @brief Install a signed shard-limit policy supplied by an edition-upgrade plugin.
     *
     * Enforces the compile-time ceiling @c SHARDING_MAX_NODES (Defense in Depth).
     * A rejected install is logged; the binary retains its default node limit.
     *
     * Thread-safe.
     *
     * @param policy            The IShardLimitPolicy implementation provided by the plugin.
     * @param claimed_max_nodes Maximum shard-node count the policy declares it allows.
     *                          Must be <= SHARDING_MAX_NODES compile-time ceiling, or -1.
     * @return true   Policy accepted and installed.
     * @return false  Policy rejected — ceiling exceeded.
     */
    bool installShardPolicy(std::shared_ptr<sharding::IShardLimitPolicy> policy,
                            int claimed_max_nodes);

    /**
     * @brief Remove any previously installed VRAM policy (reverts to compile-time default).
     * Thread-safe.
     */
    void clearVRAMPolicy();

    /**
     * @brief Remove any previously installed shard-limit policy (reverts to compile-time default).
     * Thread-safe.
     */
    void clearShardPolicy();

    // --- Group 4: LLM resource policy ---

    /**
     * @brief Install a signed LLM resource policy supplied by an edition-upgrade plugin.
     *
     * Defense in Depth: @p claimed_max_context_tokens must satisfy
     *   (LLM_MAX_CONTEXT_TOKENS == 0 || claimed ≤ LLM_MAX_CONTEXT_TOKENS);
     * @p claimed_max_model_instances must satisfy
     *   (LLM_MAX_MODEL_INSTANCES == -1 || claimed ≤ LLM_MAX_MODEL_INSTANCES);
     * @p claimed_max_vram_per_model_mb must satisfy
     *   (LLM_MAX_VRAM_PER_MODEL_MB == 0 || claimed ≤ LLM_MAX_VRAM_PER_MODEL_MB).
     * A rejected install is logged and returns false.
     *
     * Thread-safe.
     *
     * @param policy                       LLM resource policy implementation.
     * @param claimed_max_context_tokens   Max context tokens per inference; 0 = unlimited.
     * @param claimed_max_model_instances  Max concurrently loaded models; -1 = unlimited.
     * @param claimed_max_vram_per_model_mb Max VRAM per model in MiB; 0 = unlimited.
     * @return true on acceptance, false when a ceiling is exceeded.
     */
    bool installLLMResourcePolicy(std::shared_ptr<llm::ILLMResourcePolicy> policy,
                                  int64_t claimed_max_context_tokens,
                                  int32_t claimed_max_model_instances,
                                  int64_t claimed_max_vram_per_model_mb);

    /**
     * @brief Remove any previously installed LLM resource policy.
     * Thread-safe.
     */
    void clearLLMResourcePolicy();

    // --- Group 1+3: Tenant quota policy ---

    /**
     * @brief Install a signed tenant-quota policy supplied by an edition-upgrade plugin.
     *
     * Defense in Depth: each claimed value is validated against the corresponding
     * compile-time ceiling in edition.h (0 = unlimited ceilings always accept).
     * A rejected install is logged and returns false.
     *
     * Thread-safe.
     *
     * @param policy                        Tenant quota policy implementation.
     * @param claimed_max_storage_bytes     Max storage per tenant in bytes; 0 = unlimited.
     * @param claimed_max_documents         Max documents per tenant; 0 = unlimited.
     * @param claimed_max_collections       Max collections per tenant; 0 = unlimited.
     * @param claimed_max_concurrent_queries Max concurrent queries per tenant; 0 = unlimited.
     * @param claimed_max_rps               Max requests per second per tenant; 0 = unlimited.
     * @return true on acceptance, false when a ceiling is exceeded.
     */
    bool installTenantQuotaPolicy(std::shared_ptr<tenant::ITenantQuotaPolicy> policy,
                                  uint64_t claimed_max_storage_bytes,
                                  uint64_t claimed_max_documents,
                                  uint32_t claimed_max_collections,
                                  uint32_t claimed_max_concurrent_queries,
                                  uint32_t claimed_max_rps);

    /**
     * @brief Remove any previously installed tenant-quota policy.
     * Thread-safe.
     */
    void clearTenantQuotaPolicy();

    // --- Group 2: Query limit policy ---

    /**
     * @brief Install a signed query-limit policy supplied by an edition-upgrade plugin.
     *
     * Defense in Depth: each claimed value is validated against the corresponding
     * compile-time ceiling in edition.h (0 = unlimited ceilings always accept).
     * A rejected install is logged and returns false.
     *
     * Thread-safe.
     *
     * @param policy                    Query limit policy implementation.
     * @param claimed_max_depth         Max GraphQL/AQL depth; 0 = unlimited.
     * @param claimed_max_complexity    Max query complexity score; 0 = unlimited.
     * @param claimed_max_payload_bytes Max request payload bytes; 0 = unlimited.
     * @param claimed_max_result_rows   Max result rows per query; 0 = unlimited.
     * @return true on acceptance, false when a ceiling is exceeded.
     */
    bool installQueryLimitPolicy(std::shared_ptr<query::IQueryLimitPolicy> policy,
                                 uint32_t claimed_max_depth,
                                 uint32_t claimed_max_complexity,
                                 uint64_t claimed_max_payload_bytes,
                                 uint64_t claimed_max_result_rows);

    /**
     * @brief Remove any previously installed query-limit policy.
     * Thread-safe.
     */
    void clearQueryLimitPolicy();

    // --- Group 2+3: Connection policy ---

    /**
     * @brief Install a signed connection policy supplied by an edition-upgrade plugin.
     *
     * Defense in Depth: each claimed value is validated against the corresponding
     * compile-time ceiling in edition.h (0 = unlimited ceilings always accept).
     * A rejected install is logged and returns false.
     *
     * Thread-safe.
     *
     * @param policy                       Connection policy implementation.
     * @param claimed_max_http2_streams    Max HTTP/2 streams per connection; 0 = unlimited.
     * @param claimed_max_sse_connections  Max total SSE connections; 0 = unlimited.
     * @param claimed_max_total_connections Max total server connections; 0 = unlimited.
     * @param claimed_max_sse_events_per_sec Max SSE events/sec; 0 = unlimited.
     * @return true on acceptance, false when a ceiling is exceeded.
     */
    bool installConnectionPolicy(std::shared_ptr<network::IConnectionPolicy> policy,
                                 uint32_t claimed_max_http2_streams,
                                 uint32_t claimed_max_sse_connections,
                                 uint32_t claimed_max_total_connections,
                                 uint32_t claimed_max_sse_events_per_sec);

    /**
     * @brief Remove any previously installed connection policy.
     * Thread-safe.
     */
    void clearConnectionPolicy();

    // --- Group 5: Storage operations policy ---

    /**
     * @brief Install a signed storage-operations policy supplied by an edition-upgrade plugin.
     *
     * Defense in Depth: each claimed value is validated against the corresponding
     * compile-time ceiling in edition.h (-1 = unlimited for counts, 0 = unlimited for rates).
     * A rejected install is logged and returns false.
     *
     * Thread-safe.
     *
     * @param policy                              Storage ops policy implementation.
     * @param claimed_max_background_jobs         Max concurrent background jobs; -1 = unlimited.
     * @param claimed_max_compaction_bytes_per_sec Max compaction I/O rate in bytes/s; 0 = unlimited.
     * @param claimed_max_concurrent_snapshots    Max concurrent snapshots; -1 = unlimited.
     * @return true on acceptance, false when a ceiling is exceeded.
     */
    bool installStorageOpsPolicy(std::shared_ptr<storage::IStorageOpsPolicy> policy,
                                 int32_t  claimed_max_background_jobs,
                                 uint64_t claimed_max_compaction_bytes_per_sec,
                                 int32_t  claimed_max_concurrent_snapshots);

    /**
     * @brief Remove any previously installed storage-operations policy.
     * Thread-safe.
     */
    void clearStorageOpsPolicy();

    // --- Group 3: Global rate-limit policy ---

    /**
     * @brief Install a signed global rate-limit policy supplied by an edition-upgrade plugin.
     *
     * Defense in Depth: @p claimed_max_global_rps must satisfy
     *   (RATE_LIMIT_MAX_GLOBAL_RPS == 0 || claimed ≤ RATE_LIMIT_MAX_GLOBAL_RPS).
     * A rejected install is logged and returns false.
     *
     * Thread-safe.
     *
     * @param policy                  Rate-limit policy implementation.
     * @param claimed_max_global_rps  Max global requests per second; 0 = unlimited.
     * @return true on acceptance, false when the ceiling is exceeded.
     */
    bool installRateLimitPolicy(std::shared_ptr<ratelimit::IRateLimitPolicy> policy,
                                uint64_t claimed_max_global_rps);

    /**
     * @brief Remove any previously installed global rate-limit policy.
     * Thread-safe.
     */
    void clearRateLimitPolicy();

    // -------------------------------------------------------------------------
    // Dynamic feature-flag overrides
    // -------------------------------------------------------------------------

    /**
     * @brief Set a runtime override for a named feature flag.
     *
     * Overrides are layered on top of (and cannot bypass) the compile-time
     * edition gate.  An override of @c true for a Community-edition feature
     * that is compile-time disabled is silently stored but will never allow
     * the feature — the compile-time gate still applies.
     *
     * Typical use-cases:
     *  - Staged rollout: set to @c false while validating, flip to @c true
     *    when ready to expose the feature to this node.
     *  - Maintenance mode: set to @c false to block a feature without
     *    redeployment.
     *
     * Thread-safe.
     *
     * @param feature_name  Feature to override (see edition::kGatedFeatureNames).
     * @param enabled       @c true = allow (if edition/license also allow);
     *                      @c false = always block.
     */
    void setFeatureOverride(std::string_view feature_name, bool enabled);

    /**
     * @brief Remove the runtime override for a named feature flag.
     *
     * After calling this, @c isFeatureAvailable(feature_name) reverts to
     * the standard edition + license decision.  A no-op if no override
     * exists for @p feature_name.
     *
     * Thread-safe.
     */
    void clearFeatureOverride(std::string_view feature_name);

    /**
     * @brief Remove all runtime overrides.
     *
     * After this call every feature reverts to the standard edition + license
     * decision.
     *
     * Thread-safe.
     */
    void clearAllFeatureOverrides();

    /**
     * @brief Returns @c true if a runtime override is set for the feature.
     *
     * Does not indicate whether the feature is available — use
     * @c isFeatureAvailable() for that.
     *
     * Thread-safe.
     */
    bool hasFeatureOverride(std::string_view feature_name) const;

    /**
     * @brief Returns the current runtime override value, if any.
     *
     * @return @c std::nullopt when no override is set; @c true / @c false
     *         when an override has been set via @c setFeatureOverride().
     *
     * Thread-safe.
     */
    std::optional<bool> getFeatureOverride(std::string_view feature_name) const;

private:
    EditionManager()  = default;
    ~EditionManager() = default;

    mutable std::mutex                                  overrides_mutex_;
    std::unordered_map<std::string, bool>               overrides_;

    // Resource-limit policy state (protected by policy_mutex_)
    mutable std::mutex                                  policy_mutex_;
    std::shared_ptr<gpu::IVRAMPolicy>                   vram_policy_;
    std::shared_ptr<sharding::IShardLimitPolicy>        shard_policy_;
    /// Effective VRAM limit (GiB) from the installed plugin; -2 = no plugin installed.
    int effective_vram_gb_{ -2 };
    /// Effective max shard nodes from the installed plugin; -2 = no plugin installed.
    int effective_shard_nodes_{ -2 };

    // --- Group 4: LLM resource policy ---
    std::shared_ptr<llm::ILLMResourcePolicy>            llm_resource_policy_;

    // --- Group 1+3: Tenant quota policy ---
    std::shared_ptr<tenant::ITenantQuotaPolicy>         tenant_quota_policy_;

    // --- Group 2: Query limit policy ---
    std::shared_ptr<query::IQueryLimitPolicy>           query_limit_policy_;

    // --- Group 2+3: Connection policy ---
    std::shared_ptr<network::IConnectionPolicy>         connection_policy_;

    // --- Group 5: Storage ops policy ---
    std::shared_ptr<storage::IStorageOpsPolicy>         storage_ops_policy_;

    // --- Group 3: Global rate-limit policy ---
    std::shared_ptr<ratelimit::IRateLimitPolicy>        rate_limit_policy_;
};

} // namespace edition
} // namespace themis
