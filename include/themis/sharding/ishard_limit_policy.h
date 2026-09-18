/**
 * @file ishard_limit_policy.h
 * @brief Abstract shard-node admission and accounting policy for ThemisDB distributed clustering.
 *
 * Defines the policy contract that governs how many shard nodes the runtime
 * is allowed to operate concurrently.  Analogous to IVRAMPolicy for GPU
 * memory budgets — both interfaces feed into EditionManager's resource-limit
 * checks and can be installed at runtime by signed edition-upgrade plugins.
 */

#pragma once

#include <string>

namespace themis {
namespace sharding {

/**
 * @brief Abstract shard-node admission and accounting policy.
 *
 * All shard-count enforcement in ThemisDB routes through this interface so
 * that edition limits, signed plugin upgrades, and operational caps share a
 * single, auditable enforcement surface.
 *
 * Hierarchy overview:
 * @code
 *   IShardLimitPolicy   (this file)
 *   └── EditionManager::installShardPolicy()
 *       └── EditionManager::checkNodeLimit()
 *           └── ShardingManager — enforces on shard join
 * @endcode
 *
 * Thread safety: all implementations must be individually thread-safe.
 *
 * @note A policy's maxNodes() must never exceed the compile-time edition
 *       constant edition::SHARDING_MAX_NODES (enforced by
 *       EditionManager::installShardPolicy before accepting the policy).
 *       On Hyperscaler binaries (SHARDING_MAX_NODES == -1) the ceiling is
 *       unlimited and any positive maxNodes() value is accepted.
 */
class IShardLimitPolicy {
public:
    virtual ~IShardLimitPolicy() = default;

    // Non-copyable, non-movable by default.
    IShardLimitPolicy(const IShardLimitPolicy&)            = delete;
    IShardLimitPolicy& operator=(const IShardLimitPolicy&) = delete;

    /**
     * @brief Declared maximum number of shard nodes this policy allows.
     *
     * This value is validated against the compile-time ceiling by
     * EditionManager::installShardPolicy before the policy is accepted.
     *
     * @return Maximum node count; -1 signals unlimited (only valid for
     *         Hyperscaler edition binaries where SHARDING_MAX_NODES == -1).
     */
    [[nodiscard]] virtual int maxNodes() const noexcept = 0;

    /**
     * @brief Return true iff the cluster is permitted to reach @p requested_total nodes.
     *
     * The policy may consult activeNodeCount() to make the admission decision.
     * Does not modify accounting state — call onNodeAdded() only after
     * the node has actually joined.
     *
     * @param requested_total  Intended total cluster size after the pending join.
     * @return true when the expansion is permitted under the current policy.
     */
    [[nodiscard]] virtual bool canExpand(int requested_total) const = 0;

    /**
     * @brief Notify the policy that a shard node with @p shard_id has joined.
     *
     * Updates internal accounting.  Implementations must be safe to call
     * concurrently from multiple threads.
     *
     * @param shard_id  Opaque, unique shard identifier (non-empty string).
     */
    virtual void onNodeAdded(const std::string& shard_id) = 0;

    /**
     * @brief Notify the policy that the shard node @p shard_id has left.
     *
     * Updates internal accounting.  Implementations must clamp to zero on
     * mismatched calls to prevent underflow.
     *
     * @param shard_id  Opaque shard identifier previously passed to onNodeAdded().
     */
    virtual void onNodeRemoved(const std::string& shard_id) = 0;

    /**
     * @brief Return the number of nodes currently tracked as active.
     *
     * @return Non-negative active node count.
     */
    [[nodiscard]] virtual int activeNodeCount() const = 0;

    /**
     * @brief Return true when this policy permits multi-node sharding.
     *
     * Implementations should return false when maxNodes() == 1 (single-node
     * Community edition) or when sharding has been administratively disabled.
     */
    [[nodiscard]] virtual bool isShardingEnabled() const noexcept = 0;

protected:
    IShardLimitPolicy() = default;
    IShardLimitPolicy(IShardLimitPolicy&&) noexcept = default;
    IShardLimitPolicy& operator=(IShardLimitPolicy&&) noexcept = default;
};

} // namespace sharding
} // namespace themis
