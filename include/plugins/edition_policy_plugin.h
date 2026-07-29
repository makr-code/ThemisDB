/**
 * @file edition_policy_plugin.h
 * @brief Plugin interface for signed edition-upgrade resource-limit policies.
 *
 * An IEditionPolicyPlugin carries signed license claims and factory methods
 * for IVRAMPolicy, IShardLimitPolicy, ILLMResourcePolicy, ITenantQuotaPolicy,
 * IQueryLimitPolicy, IConnectionPolicy, IStorageOpsPolicy, and IRateLimitPolicy
 * implementations.  The PluginManager activates the policies into EditionManager
 * after verifying the plugin's signature against the SignedPluginRepository and
 * confirming that the claimed limits do not exceed the compile-time edition ceilings.
 *
 * Activation flow:
 * @code
 *   PluginManager::activatePolicyPlugin(name, repo)
 *       → verify entry in SignedPluginRepository
 *       → validate license claim via RuntimeLicenseGate
 *       → EditionManager::installVRAMPolicy(...)
 *       → EditionManager::installShardPolicy(...)
 *       → EditionManager::installLLMResourcePolicy(...)
 *       → EditionManager::installTenantQuotaPolicy(...)
 *       → EditionManager::installQueryLimitPolicy(...)
 *       → EditionManager::installConnectionPolicy(...)
 *       → EditionManager::installStorageOpsPolicy(...)
 *       → EditionManager::installRateLimitPolicy(...)
 * @endcode
 *
 * @note Group 6 (WASM sandbox memory) is a Security-Boundary and is intentionally
 *       absent from this interface.  WASM memory limits are enforced at the
 *       compile-time/kernel level and must not receive a plugin-policy override.
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "themis/gpu/ivram_policy.h"
#include "themis/sharding/ishard_limit_policy.h"
#include "themis/llm/illm_resource_policy.h"
#include "themis/tenant/itenant_quota_policy.h"
#include "themis/query/iquery_limit_policy.h"
#include "themis/network/iconnection_policy.h"
#include "themis/storage/istorage_ops_policy.h"
#include "themis/ratelimit/irate_limit_policy.h"

#include <memory>
#include <string>

namespace themis {
namespace plugins {

/**
 * @brief Base interface for edition-upgrade resource-limit plugins.
 *
 * Plugins of type PluginType::RESOURCE_LIMIT_POLICY must implement this
 * interface.  After successful signature verification and ceiling validation
 * by PluginManager::activatePolicyPlugin(), the policies returned by the
 * factory methods are installed in EditionManager.
 *
 * ### Security contract
 *  - All claimed limits must satisfy: claimed ≤ compile-time ceiling, or the
 *    ceiling is unlimited (0 or -1), otherwise EditionManager rejects the install.
 *  - A null factory return signals "this plugin does not provide this policy".
 *  - PluginManager::activatePolicyPlugin rejects any RESOURCE_LIMIT_POLICY
 *    plugin whose manifest entry is absent from SignedPluginRepository or
 *    whose Ed25519 signature fails verification.
 *  - WASM sandbox memory (Group 6) must not be part of any plugin policy.
 *
 * ### Thread safety
 * Implementations must be individually thread-safe.
 */
class IEditionPolicyPlugin : public IThemisPlugin {
public:
    // -------------------------------------------------------------------------
    // VRAM and sharding (existing axes)
    // -------------------------------------------------------------------------

    /**
     * @brief Factory: construct the VRAM allocation policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IVRAMPolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<gpu::IVRAMPolicy> createVRAMPolicy() = 0;

    /**
     * @brief Factory: construct the shard-limit policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IShardLimitPolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<sharding::IShardLimitPolicy> createShardPolicy() = 0;

    /**
     * @brief License claim string identifying the edition tier being unlocked.
     *
     * Validated against RuntimeLicenseGate during plugin activation.
     * Required JSON fields:
     * @code
     * { "edition": "<community|enterprise|hyperscaler>",
     *   "vram_gb": <n>,
     *   "max_nodes": <n> }
     * @endcode
     *
     * @return Non-empty JSON string encoding the license claim.
     */
    [[nodiscard]] virtual std::string getLicenseClaim() const = 0;

    /**
     * @brief Declared maximum GPU VRAM in GiB that this plugin activates.
     *
     * @return VRAM limit in GiB; -1 signals "no VRAM policy provided".
     */
    [[nodiscard]] virtual int claimedMaxVRAMGB() const noexcept = 0;

    /**
     * @brief Declared maximum shard-node count that this plugin activates.
     *
     * @return Node limit; -1 signals "no shard policy provided".
     */
    [[nodiscard]] virtual int claimedMaxNodes() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Group 4: LLM resource policy
    // -------------------------------------------------------------------------

    /**
     * @brief Factory: construct the LLM resource policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe ILLMResourcePolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<llm::ILLMResourcePolicy>
    createLLMResourcePolicy() { return nullptr; }

    /**
     * @brief Declared maximum context tokens per inference; 0 = unlimited, -1 = not provided.
     */
    [[nodiscard]] virtual int64_t claimedMaxContextTokens() const noexcept { return -1; }

    /**
     * @brief Declared maximum concurrently loaded model instances; -1 = not provided.
     */
    [[nodiscard]] virtual int32_t claimedMaxModelInstances() const noexcept { return -1; }

    /**
     * @brief Declared maximum VRAM per model instance in MiB; 0 = unlimited, -1 = not provided.
     */
    [[nodiscard]] virtual int64_t claimedMaxVRAMPerModelMB() const noexcept { return -1; }

    // -------------------------------------------------------------------------
    // Group 1+3: Tenant quota policy
    // -------------------------------------------------------------------------

    /**
     * @brief Factory: construct the tenant-quota policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe ITenantQuotaPolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<tenant::ITenantQuotaPolicy>
    createTenantQuotaPolicy() { return nullptr; }

    /**
     * @brief Declared max storage bytes per tenant; 0 = unlimited.
     *
     * Return 0 when no tenant-quota policy is provided.
     */
    [[nodiscard]] virtual uint64_t claimedTenantMaxStorageBytes() const noexcept { return 0; }

    /**
     * @brief Declared max documents per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t claimedTenantMaxDocuments() const noexcept { return 0; }

    /**
     * @brief Declared max collections per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedTenantMaxCollections() const noexcept { return 0; }

    /**
     * @brief Declared max concurrent queries per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedTenantMaxConcurrentQueries() const noexcept { return 0; }

    /**
     * @brief Declared max requests per second per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedTenantMaxRPS() const noexcept { return 0; }

    // -------------------------------------------------------------------------
    // Group 2: Query limit policy
    // -------------------------------------------------------------------------

    /**
     * @brief Factory: construct the query-limit policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IQueryLimitPolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<query::IQueryLimitPolicy>
    createQueryLimitPolicy() { return nullptr; }

    /**
     * @brief Declared max GraphQL/AQL nesting depth; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedMaxGraphQLDepth() const noexcept { return 0; }

    /**
     * @brief Declared max query complexity score; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedMaxGraphQLComplexity() const noexcept { return 0; }

    /**
     * @brief Declared max request payload bytes; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t claimedMaxPayloadBytes() const noexcept { return 0; }

    /**
     * @brief Declared max result rows per query; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t claimedMaxResultRows() const noexcept { return 0; }

    // -------------------------------------------------------------------------
    // Group 2+3: Connection policy
    // -------------------------------------------------------------------------

    /**
     * @brief Factory: construct the connection policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IConnectionPolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<network::IConnectionPolicy>
    createConnectionPolicy() { return nullptr; }

    /**
     * @brief Declared max HTTP/2 streams per connection; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedMaxHttp2Streams() const noexcept { return 0; }

    /**
     * @brief Declared max total SSE connections; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedMaxSSEConnections() const noexcept { return 0; }

    /**
     * @brief Declared max total server connections; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedMaxTotalConnections() const noexcept { return 0; }

    /**
     * @brief Declared max SSE events per second; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t claimedMaxSSEEventsPerSec() const noexcept { return 0; }

    // -------------------------------------------------------------------------
    // Group 5: Storage operations policy
    // -------------------------------------------------------------------------

    /**
     * @brief Factory: construct the storage-operations policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IStorageOpsPolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<storage::IStorageOpsPolicy>
    createStorageOpsPolicy() { return nullptr; }

    /**
     * @brief Declared max concurrent background jobs; -1 = not provided/unlimited.
     */
    [[nodiscard]] virtual int32_t claimedMaxBackgroundJobs() const noexcept { return -1; }

    /**
     * @brief Declared max compaction I/O rate in bytes/s; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t claimedMaxCompactionBytesPerSec() const noexcept { return 0; }

    /**
     * @brief Declared max concurrent snapshots; -1 = not provided/unlimited.
     */
    [[nodiscard]] virtual int32_t claimedMaxConcurrentSnapshots() const noexcept { return -1; }

    // -------------------------------------------------------------------------
    // Group 3: Global rate-limit policy
    // -------------------------------------------------------------------------

    /**
     * @brief Factory: construct the global rate-limit policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IRateLimitPolicy, or nullptr if not provided.
     */
    [[nodiscard]] virtual std::shared_ptr<ratelimit::IRateLimitPolicy>
    createRateLimitPolicy() { return nullptr; }

    /**
     * @brief Declared max global requests per second; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t claimedMaxGlobalRPS() const noexcept { return 0; }
};

} // namespace plugins
} // namespace themis
