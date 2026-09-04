/**
 * @file federated_blob_router.h
 * @brief Region-aware blob routing and synchronous replication for multi-region deployments.
 * @version 0.1.0
 */

#pragma once

#include "storage/blob_storage_backend.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace storage {

/**
 * @brief Replication target for a federated blob write.
 *
 * Targets marked as required must succeed for the write to commit. Optional
 * targets may fail without rolling back the primary write, but their failure is
 * reported to the caller.
 */
struct FederatedBlobReplicaTarget {
    std::string region;
    bool        required = true;
};

/**
 * @brief Write plan describing how a blob should be placed across regions.
 *
 * @param primary_region Region that receives the canonical primary write.
 * @param replica_targets Additional regions that receive synchronous copies.
 */
struct FederatedBlobWritePlan {
    std::string                               primary_region;
    std::vector<FederatedBlobReplicaTarget>   replica_targets;
};

/**
 * @brief Multi-region placement result for a federated blob write.
 *
 * The route records the blob reference returned by each participating backend so
 * that subsequent reads and deletes can use the correct region-local metadata.
 */
struct FederatedBlobRoute {
    std::string primary_region;
    std::unordered_map<std::string, BlobRef> region_refs;
    std::vector<std::string>                 optional_failures;
};

/**
 * @brief Dedicated router for multi-region blob placement and failover reads.
 *
 * This first production cut provides:
 * - explicit per-region backend registration
 * - synchronous primary + replica writes
 * - fail-closed rollback when a required replica write fails
 * - preferred-region reads with fallback to other healthy regions
 * - multi-region deletion by replaying recorded region-local references
 *
 * Edge cases:
 * - Empty region names and null backend registrations are rejected.
 * - Writes fail fast when the primary region is unknown or unavailable.
 * - Reads fail only after every eligible region returns an error.
 */
class FederatedBlobRouter {
public:
    /**
     * @brief Register a backend for a region.
     * @param region Stable region identifier (for example `eu-central-1`).
     * @param backend Backend instance serving that region.
     * @return `ERR_UTIL_INVALID_ARGUMENT` when the region is empty or the backend is null.
     */
    [[nodiscard]] Result<void> registerBackend(const std::string& region,
                                               std::shared_ptr<IBlobStorageBackend> backend) {
        if (region.empty()) {
            return ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                           "federated blob router region must not be empty");
        }
        if (!backend) {
            return ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                           "federated blob router backend must not be null");
        }

        std::lock_guard<std::mutex> lock(mutex_);
        backends_[region] = std::move(backend);
        return OkVoid();
    }

    /**
     * @brief Store a blob in the primary region and synchronously replicate it.
     * @param plan Write placement plan.
     * @param blob_id Stable blob identifier.
     * @param data Blob payload to replicate.
     * @return Region map containing the references returned by each successful backend.
     *
     * If a required replica fails, already-written regions are best-effort rolled
     * back and the method returns an error.
     */
    [[nodiscard]] Result<FederatedBlobRoute> put(const FederatedBlobWritePlan& plan,
                                                 const std::string& blob_id,
                                                 const std::vector<uint8_t>& data) {
        if (plan.primary_region.empty()) {
            return Err<FederatedBlobRoute>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                           "federated blob router primary region must not be empty");
        }
        if (blob_id.empty()) {
            return Err<FederatedBlobRoute>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                           "federated blob router blob_id must not be empty");
        }

        auto primary = backendFor(plan.primary_region);
        if (!primary || !primary->isAvailable()) {
            return Err<FederatedBlobRoute>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                           "primary federated blob backend is unavailable for region " +
                                               plan.primary_region);
        }

        auto primary_result = primary->put(blob_id, data);
        if (!primary_result.has_value()) {
            return Err<FederatedBlobRoute>(primary_result.error().code(),
                                           primary_result.error().message());
        }

        FederatedBlobRoute route;
        route.primary_region = plan.primary_region;
        route.region_refs.emplace(plan.primary_region, primary_result.value());

        for (const auto& target : plan.replica_targets) {
            if (target.region.empty() || target.region == plan.primary_region ||
                route.region_refs.contains(target.region)) {
                continue;
            }

            auto replica = backendFor(target.region);
            if (!replica || !replica->isAvailable()) {
                if (target.required) {
                    rollback(route);
                    return Err<FederatedBlobRoute>(
                        errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                        "required federated blob replica backend is unavailable for region " +
                            target.region);
                }
                route.optional_failures.push_back(target.region);
                continue;
            }

            auto replica_result = replica->put(blob_id, data);
            if (!replica_result.has_value()) {
                if (target.required) {
                    rollback(route);
                    return Err<FederatedBlobRoute>(replica_result.error().code(),
                                                   replica_result.error().message());
                }
                route.optional_failures.push_back(target.region);
                continue;
            }

            route.region_refs.emplace(target.region, replica_result.value());
        }

        return Ok(std::move(route));
    }

    /**
     * @brief Retrieve a blob from the preferred region with cross-region fallback.
     * @param route Region placement metadata returned by put().
     * @param preferred_region Optional region to try first before the primary region.
     * @return Blob bytes on the first successful read, otherwise the last error observed.
     */
    [[nodiscard]] Result<std::vector<uint8_t>> get(const FederatedBlobRoute& route,
                                                   const std::string& preferred_region = "") {
        auto regions = readOrder(route, preferred_region);
        if (regions.empty()) {
            return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                             "federated blob route does not contain any regions");
        }

        errors::ErrorCode last_code = errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED;
        std::string       last_message = "no federated blob regions could satisfy the read";
        for (const auto& region : regions) {
            auto backend_it = route.region_refs.find(region);
            if (backend_it == route.region_refs.end()) {
                continue;
            }

            auto backend = backendFor(region);
            if (!backend || !backend->isAvailable()) {
                continue;
            }

            auto result = backend->get(backend_it->second);
            if (result.has_value()) {
                return result;
            }
            last_code = result.error().code();
            last_message = result.error().message();
        }

        return Err<std::vector<uint8_t>>(last_code, last_message);
    }

    /**
     * @brief Delete all region-local copies recorded in a route.
     * @param route Region placement metadata returned by put().
     * @return `OkVoid()` when all reachable copies were deleted.
     *
     * Returns an error when at least one region-local delete fails.
     */
    [[nodiscard]] Result<void> remove(const FederatedBlobRoute& route) {
        bool        saw_failure = false;
        std::string failure_message;

        for (const auto& [region, ref] : route.region_refs) {
            auto backend = backendFor(region);
            if (!backend) {
                saw_failure = true;
                failure_message = "federated blob backend missing for region " + region;
                continue;
            }

            auto result = backend->remove(ref);
            if (!result.has_value()) {
                saw_failure = true;
                failure_message = result.error().message();
            }
        }

        if (saw_failure) {
            return ErrVoid(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED, failure_message);
        }
        return OkVoid();
    }

    /**
     * @brief Check whether any routed region still contains the blob.
     * @param route Region placement metadata returned by put().
     * @param preferred_region Optional region to check first.
     * @return true on the first positive existence check; false otherwise.
     */
    [[nodiscard]] bool exists(const FederatedBlobRoute& route,
                              const std::string& preferred_region = "") {
        for (const auto& region : readOrder(route, preferred_region)) {
            auto backend_it = route.region_refs.find(region);
            if (backend_it == route.region_refs.end()) {
                continue;
            }

            auto backend = backendFor(region);
            if (backend && backend->isAvailable() && backend->exists(backend_it->second)) {
                return true;
            }
        }
        return false;
    }

private:
    [[nodiscard]] std::shared_ptr<IBlobStorageBackend> backendFor(const std::string& region) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = backends_.find(region);
        return it == backends_.end() ? nullptr : it->second;
    }

    [[nodiscard]] static std::vector<std::string> readOrder(const FederatedBlobRoute& route,
                                                            const std::string& preferred_region) {
        std::vector<std::string> order = {};

        if (!preferred_region.empty() && route.region_refs.contains(preferred_region)) {
            order.push_back(preferred_region);
        }
        if (!route.primary_region.empty() &&
            route.region_refs.contains(route.primary_region) &&
            std::find(order.begin(), order.end(), route.primary_region) == order.end()) {
            order.push_back(route.primary_region);
        }
        for (const auto& [region, _] : route.region_refs) {
            if (std::find(order.begin(), order.end(), region) == order.end()) {
                order.push_back(region);
            }
        }
        return order;
    }

    void rollback(const FederatedBlobRoute& route) {
        for (const auto& [region, ref] : route.region_refs) {
            auto backend = backendFor(region);
            if (backend) {
                static_cast<void>(backend->remove(ref));
            }
        }
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<IBlobStorageBackend>> backends_;
};

} // namespace storage
} // namespace themis
