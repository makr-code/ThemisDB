/**
 * @file geo_policy.cpp
 * @brief Implementation of SpatialQueryPolicy validation and helper functions.
 *
 * See `include/geo/geo_policy.h` for the public API contract.
 */


#include "geo/geo_policy.h"

#include <algorithm>
#include <sstream>

namespace themis {
namespace geo {

SpatialQueryPolicyValidation
validateSpatialQueryPolicy(const SpatialQueryPolicy& policy)
{
    SpatialQueryPolicyValidation result;

    if (policy.max_depth < 0) {
        result.violations.push_back("max_depth must be >= 0 (0 = implementation default)");
    }

    if (policy.timeout.count() < 0) {
        result.violations.push_back("timeout must be >= 0 ms (0 = disabled)");
    }

    for (const auto& name : policy.allowed_collections) {
        if (name.empty()) {
            result.violations.push_back(
                "allowed_collections contains an empty string; "
                "all collection names must be non-empty");
            break; // report once per policy
        }
    }

    return result;
}

SpatialQueryPolicy defaultSpatialQueryPolicy() noexcept
{
    return SpatialQueryPolicy{};
}

bool isSpatialCollectionPermitted(const SpatialQueryPolicy& policy,
                                  const std::string& collection_name)
{
    // An empty allowlist means all collections are permitted (open policy).
    if (policy.allowed_collections.empty()) {
        return true;
    }
    return std::find(policy.allowed_collections.cbegin(),
                     policy.allowed_collections.cend(),
                     collection_name) != policy.allowed_collections.cend();
}

} // namespace geo
} // namespace themis
