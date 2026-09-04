/**
 * @file policy.cpp
 * @brief ThemisDB Replication Policy Implementation
 *
 * Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/policy.h"

#include <algorithm>
#include <set>

namespace themisdb {
namespace replication {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/** Count healthy replicas in a vector (HEALTHY or UNKNOWN). */
static int countHealthy(const std::vector<ReplicaInfo>& replicas) {
    int cnt = 0;
    for (const auto& r : replicas) {
        if (r.health_status == HealthStatus::HEALTHY ||
            r.health_status == HealthStatus::UNKNOWN)
            ++cnt;
    }
    return cnt;
}

/** Collect the set of distinct datacenter labels present among replicas. */
static std::set<std::string> collectDatacenters(
    const std::vector<ReplicaInfo>& replicas)
{
    std::set<std::string> dcs = {};

    for (const auto& r : replicas) {
        if (!r.datacenter.empty()) {
          dcs.insert(r.datacenter);
        }
    }
    return dcs;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ReplicationPolicy::ReplicationPolicy(
    std::shared_ptr<ReplicationManager> manager)
    : manager_(std::move(manager))
{}

// ---------------------------------------------------------------------------
// Default policy
// ---------------------------------------------------------------------------

/*static*/ ReplicationPolicy::Policy ReplicationPolicy::defaultPolicy()
{
    Policy p;
    p.name = "__default__";
    return p;
}

// ---------------------------------------------------------------------------
// Policy management
// ---------------------------------------------------------------------------

void ReplicationPolicy::definePolicy(
    const std::string& policy_name,
    const Policy& policy)
{
    std::lock_guard<std::mutex> lock(policies_mutex_);
    policies_[policy_name] = policy;
}

bool ReplicationPolicy::removePolicy(const std::string& policy_name)
{
    std::lock_guard<std::mutex> lock(policies_mutex_);
    return policies_.erase(policy_name) > 0;
}

bool ReplicationPolicy::assignPolicy(
    const std::string& collection,
    const std::string& policy_name)
{
    std::lock_guard<std::mutex> lock(policies_mutex_);
    if (policies_.find(policy_name) == policies_.end()) {
      return false;
    }
    assignments_[collection] = policy_name;
    return true;
}

ReplicationPolicy::Policy
ReplicationPolicy::getPolicy(const std::string& collection) const
{
    std::lock_guard<std::mutex> lock(policies_mutex_);
    const auto asn_it = assignments_.find(collection);
    if (asn_it == assignments_.end()) {
      return defaultPolicy();
    }
    const auto pol_it = policies_.find(asn_it->second);
    if (pol_it == policies_.end()) {
      return defaultPolicy();
    }
    return pol_it->second;
}

std::vector<std::string> ReplicationPolicy::listPolicies() const
{
    std::lock_guard<std::mutex> lock(policies_mutex_);
    std::vector<std::string> names = {};

    names.reserve(policies_.size());
    for (const auto& kv : policies_) {
      names.push_back(kv.first);
    }
    return names;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

ReplicationPolicy::ValidationResult
ReplicationPolicy::validatePolicy(const Policy& policy) const
{
    ValidationResult result;
    result.is_valid = true;

    const auto replicas   = manager_->getReplicas();
    const int  healthy    = countHealthy(replicas);
    const int  total      = static_cast<int>(replicas.size());
    const auto datacenters = collectDatacenters(replicas);
    const int  dc_count   = static_cast<int>(datacenters.size());

    // 1. Minimum replicas
    if (healthy < static_cast<int>(policy.min_replicas)) {
        result.is_valid = false;
        result.violations.push_back(
            "Requires at least " + std::to_string(policy.min_replicas) +
            " healthy replicas but only " + std::to_string(healthy) +
            " are healthy.");
    }

    // 2. Desired replicas (warning, not a hard violation)
    if (healthy < static_cast<int>(policy.desired_replicas)) {
        result.recommendations.push_back(
            "Desired " + std::to_string(policy.desired_replicas) +
            " replicas; only " + std::to_string(healthy) + " are available.");
    }

    // 3. Minimum datacenters
    if (dc_count < static_cast<int>(policy.min_datacenters)) {
        result.is_valid = false;
        result.violations.push_back(
            "Requires replicas in at least " +
            std::to_string(policy.min_datacenters) +
            " datacenters; only " + std::to_string(dc_count) + " found.");
    }

    // 4. Required datacenters
    for (const auto& req_dc : policy.required_datacenters) {
        if (datacenters.find(req_dc) == datacenters.end()) {
            result.is_valid = false;
            result.violations.push_back(
                "Required datacenter '" + req_dc + "' has no replicas.");
        }
    }

    // 5. Write quorum feasibility
    if (static_cast<int>(policy.write_quorum) > healthy) {
        result.is_valid = false;
        result.violations.push_back(
            "write_quorum=" + std::to_string(policy.write_quorum) +
            " exceeds healthy replica count (" + std::to_string(healthy) + ").");
    }

    // 6. SYNC mode requires all replicas to be healthy
    if (policy.mode == ReplicationMode::SYNC && healthy < total) {
        result.is_valid = false;
        result.violations.push_back(
            "SYNC mode requires all " + std::to_string(total) +
            " replicas to be healthy, but only " +
            std::to_string(healthy) + " are.");
    }

    // Advisory recommendations
    if (policy.enable_pitr && policy.wal_retention < std::chrono::hours(24)) {
        result.recommendations.push_back(
            "PITR is enabled but WAL retention is less than 24 hours; "
            "consider increasing wal_retention for longer recovery windows.");
    }

    return result;
}

} // namespace replication
} // namespace themisdb

