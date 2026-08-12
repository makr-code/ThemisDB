/**
 * @file policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Policy API
 *
 * Provides a flexible mechanism for expressing per-collection replication
 * requirements and validating whether the current cluster topology can
 * satisfy them.
 *
 * Design constraints:
 *   - definePolicy() / assignPolicy() are atomic; partial updates do not
 *     leave the policy store in an inconsistent state.
 *   - validatePolicy() performs a read-only feasibility check; it never
 *     modifies the cluster topology.
 *   - Policy names and collection names are case-sensitive strings; no
 *     whitespace normalisation is applied.
 *
 * Target: v1.7.0
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "replication/replication_manager.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {

/**
 * ReplicationPolicy
 *
 * Manages named replication policies and their assignment to collections.
 *
 * Example:
 * @code
 *   ReplicationPolicy policy_mgr(repl_mgr);
 *
 *   ReplicationPolicy::Policy critical;
 *   critical.name            = "critical";
 *   critical.desired_replicas = 5;
 *   critical.min_datacenters  = 3;
 *   critical.mode             = ReplicationMode::SYNC;
 *   critical.write_quorum     = 3;
 *   critical.enable_pitr      = true;
 *   policy_mgr.definePolicy("critical", critical);
 *
 *   policy_mgr.assignPolicy("financial_transactions", "critical");
 *
 *   auto v = policy_mgr.validatePolicy(critical);
 *   if (!v.is_valid)
 *       for (const auto& viol : v.violations)
 *           std::cerr << "Violation: " << viol << "\n";
 * @endcode
 */
class ReplicationPolicy {
public:
    // -----------------------------------------------------------------------
    // Policy definition
    // -----------------------------------------------------------------------

    struct Policy {
        std::string name;

        // Replica count
        uint32_t min_replicas     = 2;
        uint32_t desired_replicas = 3;
        uint32_t max_replicas     = 5;

        // Geographic distribution
        std::vector<std::string> required_datacenters;  ///< Must have replicas here
        std::vector<std::string> preferred_datacenters; ///< Prefer replicas here
        uint32_t min_datacenters  = 1;                  ///< Minimum distinct DCs

        // Consistency
        ReplicationMode mode        = ReplicationMode::SEMI_SYNC;
        uint32_t write_quorum       = 2;
        uint32_t read_quorum        = 1;

        // Performance
        uint32_t max_replication_lag_ms = 10000;
        bool     enable_compression     = false;
        bool     enable_encryption      = true;

        // Retention
        std::chrono::hours wal_retention = std::chrono::hours(168); ///< 7 days
        bool enable_pitr                 = true;
    };

    // -----------------------------------------------------------------------
    // Validation result
    // -----------------------------------------------------------------------

    struct ValidationResult {
        bool                     is_valid = true;
        std::vector<std::string> violations;      ///< Non-empty when is_valid == false
        std::vector<std::string> recommendations; ///< Suggestions even when valid
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * Construct a ReplicationPolicy manager backed by the given
     * ReplicationManager (used to query the current cluster topology during
     * validation).
     */
    explicit ReplicationPolicy(std::shared_ptr<ReplicationManager> manager);

    // Non-copyable, movable
    ReplicationPolicy(const ReplicationPolicy&)            = delete;
    ReplicationPolicy& operator=(const ReplicationPolicy&) = delete;
    ReplicationPolicy(ReplicationPolicy&&)                 = default;
    ReplicationPolicy& operator=(ReplicationPolicy&&)      = default;

    // -----------------------------------------------------------------------
    // Policy management
    // -----------------------------------------------------------------------

    /**
     * Define (or replace) a named policy.
     * Thread-safe; policy is visible to subsequent calls immediately.
     */
    void definePolicy(const std::string& policy_name, const Policy& policy);

    /**
     * Remove a named policy.  Assignments referencing it remain but will
     * return the default policy until reassigned.
     * Returns false if the policy_name did not exist.
     */
    bool removePolicy(const std::string& policy_name);

    /**
     * Assign a previously defined policy to a collection.
     * Overwrites any existing assignment for that collection.
     * Returns false if policy_name was not previously defined.
     */
    bool assignPolicy(const std::string& collection,
                      const std::string& policy_name);

    /**
     * Get the effective policy for a collection.
     * Returns the default policy when no assignment exists.
     */
    Policy getPolicy(const std::string& collection) const;

    /** List all defined policy names. */
    std::vector<std::string> listPolicies() const;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * Check whether the given policy is currently achievable given the cluster
     * topology reported by the ReplicationManager.
     *
     * Checks performed:
     *   1. Enough healthy replicas to meet desired_replicas.
     *   2. Enough distinct datacenters to meet min_datacenters.
     *   3. required_datacenters each have at least one healthy replica.
     *   4. write_quorum ≤ current healthy replica count.
     *   5. SYNC mode is feasible only when all replicas are healthy.
     *
     * @return ValidationResult with is_valid = true when all checks pass.
     */
    ValidationResult validatePolicy(const Policy& policy) const;

private:
    std::shared_ptr<ReplicationManager> manager_;

    mutable std::mutex                  policies_mutex_;
    std::map<std::string, Policy>       policies_;      ///< name → policy
    std::map<std::string, std::string>  assignments_;   ///< collection → policy name

    static Policy defaultPolicy();
};

} // namespace replication
} // namespace themisdb
