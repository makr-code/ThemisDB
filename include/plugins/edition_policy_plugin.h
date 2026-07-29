/**
 * @file edition_policy_plugin.h
 * @brief Plugin interface for signed edition-upgrade resource-limit policies.
 *
 * An IEditionPolicyPlugin carries signed license claims and factory methods
 * for IVRAMPolicy and IShardLimitPolicy implementations.  The PluginManager
 * activates the policies into EditionManager after verifying the plugin's
 * signature against the SignedPluginRepository and confirming that the
 * claimed limits do not exceed the compile-time edition ceiling.
 *
 * Activation flow:
 * @code
 *   PluginManager::activatePolicyPlugin(name, repo)
 *       → verify entry in SignedPluginRepository
 *       → validate license claim via RuntimeLicenseGate
 *       → EditionManager::installVRAMPolicy(plugin->createVRAMPolicy(),
 *                                            plugin->claimedMaxVRAMGB())
 *       → EditionManager::installShardPolicy(plugin->createShardPolicy(),
 *                                             plugin->claimedMaxNodes())
 * @endcode
 */

#pragma once

#include "plugins/plugin_interface.h"
#include "themis/gpu/ivram_policy.h"
#include "themis/sharding/ishard_limit_policy.h"

#include <memory>
#include <string>

namespace themis {
namespace plugins {

/**
 * @brief Base interface for edition-upgrade resource-limit plugins.
 *
 * Plugins of type PluginType::RESOURCE_LIMIT_POLICY must implement this
 * interface.  After successful signature verification and ceiling validation
 * by PluginManager::activatePolicyPlugin(), the policies returned by
 * createVRAMPolicy() and createShardPolicy() are installed in EditionManager
 * so that checkVRAMLimit() and checkNodeLimit() delegate to them.
 *
 * ### Security contract
 *  - claimedMaxVRAMGB() must satisfy claimed ≤ edition::GPU_MAX_VRAM_GB,
 *    or -1 when the plugin provides no VRAM policy.
 *    EditionManager::installVRAMPolicy enforces this ceiling.
 *  - claimedMaxNodes() must satisfy claimed ≤ edition::SHARDING_MAX_NODES,
 *    or -1 when the plugin provides no shard policy.
 *    EditionManager::installShardPolicy enforces this ceiling.
 *  - PluginManager::activatePolicyPlugin rejects any RESOURCE_LIMIT_POLICY
 *    plugin whose manifest entry is absent from the SignedPluginRepository
 *    or whose Ed25519 signature fails verification.
 *
 * ### Thread safety
 * Implementations must be individually thread-safe.
 */
class IEditionPolicyPlugin : public IThemisPlugin {
public:
    /**
     * @brief Factory: construct the VRAM allocation policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IVRAMPolicy implementation, or
     *         nullptr when this plugin does not supply a VRAM policy.
     */
    [[nodiscard]] virtual std::shared_ptr<gpu::IVRAMPolicy> createVRAMPolicy() = 0;

    /**
     * @brief Factory: construct the shard-limit policy this plugin provides.
     *
     * @return Shared pointer to a thread-safe IShardLimitPolicy implementation,
     *         or nullptr when this plugin does not supply a shard policy.
     */
    [[nodiscard]] virtual std::shared_ptr<sharding::IShardLimitPolicy> createShardPolicy() = 0;

    /**
     * @brief License claim string identifying the edition tier being unlocked.
     *
     * The claim is validated against RuntimeLicenseGate during plugin activation.
     * Minimum required JSON fields:
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
     * @brief Declared maximum GPU VRAM in gigabytes that this plugin activates.
     *
     * Constraint enforced by EditionManager::installVRAMPolicy:
     *   claimedMaxVRAMGB() ≤ edition::GPU_MAX_VRAM_GB
     *
     * @return VRAM limit in GB; -1 signals "this plugin provides no VRAM policy".
     */
    [[nodiscard]] virtual int claimedMaxVRAMGB() const noexcept = 0;

    /**
     * @brief Declared maximum shard-node count that this plugin activates.
     *
     * Constraint enforced by EditionManager::installShardPolicy:
     *   claimedMaxNodes() ≤ edition::SHARDING_MAX_NODES
     *
     * @return Node limit; -1 signals "this plugin provides no shard policy".
     */
    [[nodiscard]] virtual int claimedMaxNodes() const noexcept = 0;
};

} // namespace plugins
} // namespace themis
