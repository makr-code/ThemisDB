/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_partition.h                                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:02:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     209                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f741f92339  2026-04-12  feat(cache): Phase 6 distribution headers — IDistributedE... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file cache_partition.h
 * @brief Sharded per-tenant cache partition interface.
 *
 * `ICachePartition` defines how a cache's capacity is divided into logical
 * partitions, each assigned to one or more tenants.  Partitions allow
 * per-tenant capacity isolation so that a high-traffic tenant cannot evict
 * entries belonging to other tenants.
 *
 * This complements the per-tenant byte quota enforced by `AdaptiveQueryCache`
 * by providing a coarser-grained partitioning mechanism that is independent
 * of the LRU eviction order.
 *
 * Design constraints:
 *   - All methods are thread-safe; implementations serialise access internally.
 *   - `getPartitionId()` must return a stable ID for the lifetime of the tenant
 *     assignment; reassignment is allowed only via `assignTenant()`.
 *   - `evictPartition()` is a blocking call; it returns after all entries in
 *     the partition have been removed from the local cache tier.
 *   - A tenant assigned to a non-existent partition ID results in a default
 *     "global" partition; implementations must not throw on unknown IDs.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace cache {

// ============================================================================
// PartitionStats — per-partition observable metrics
// ============================================================================

/**
 * @brief Point-in-time statistics for a single cache partition.
 */
struct PartitionStats {
    std::string partition_id;    ///< Unique partition identifier.
    size_t      capacity        = 0; ///< Maximum entries allowed in this partition.
    size_t      current_size    = 0; ///< Current number of entries held.
    uint64_t    hit_count       = 0; ///< Cache hits attributed to this partition.
    uint64_t    miss_count      = 0; ///< Cache misses attributed to this partition.
    uint64_t    eviction_count  = 0; ///< Entries evicted from this partition.
    size_t      tenant_count    = 0; ///< Number of tenants assigned to this partition.
};

// ============================================================================
// ICachePartition — sharded per-tenant cache partition interface
// ============================================================================

/**
 * @brief Pure-virtual interface for sharded per-tenant cache partitioning.
 *
 * A partition manager divides a cache's total capacity into named partitions.
 * Each tenant is assigned to exactly one partition.  The cache implementation
 * uses the partition assignment to:
 *   - Enforce per-partition capacity limits independently of other partitions.
 *   - Route eviction decisions to the correct partition's eviction strategy.
 *   - Expose per-partition statistics for multi-tenant observability.
 *
 * Thread-safety: all public methods are thread-safe.
 */
struct ICachePartition {
    virtual ~ICachePartition() = default;

    // -----------------------------------------------------------------------
    // Tenant-to-partition mapping
    // -----------------------------------------------------------------------

    /**
     * @brief Return the partition ID assigned to @p tenant_id.
     *
     * If the tenant has no explicit assignment, returns the default partition
     * ID (implementation-defined, typically "default").
     *
     * @param tenant_id  Tenant identifier.
     * @return Partition ID string (never empty).
     */
    virtual std::string getPartitionId(const std::string& tenant_id) const = 0;

    /**
     * @brief Assign @p tenant_id to @p partition_id.
     *
     * If @p partition_id does not exist, the implementation should create it
     * with the default capacity.  Reassigning a tenant to a different partition
     * does NOT move existing entries; they remain in the old partition until
     * naturally evicted.
     *
     * @param tenant_id    Tenant to assign.
     * @param partition_id Target partition.
     */
    virtual void assignTenant(const std::string& tenant_id,
                              const std::string& partition_id) = 0;

    /**
     * @brief Remove @p tenant_id's explicit partition assignment.
     *
     * After this call `getPartitionId(tenant_id)` returns the default partition.
     * Existing entries for the tenant are not removed.
     *
     * @param tenant_id  Tenant whose assignment is removed.
     */
    virtual void unassignTenant(const std::string& tenant_id) = 0;

    /**
     * @brief Return all tenant IDs assigned to @p partition_id.
     *
     * Returns an empty vector if the partition does not exist or has no
     * explicit tenant assignments.
     *
     * @param partition_id  Partition to query.
     */
    virtual std::vector<std::string> listTenants(
        const std::string& partition_id) const = 0;

    // -----------------------------------------------------------------------
    // Partition lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Return all partition IDs managed by this instance.
     */
    virtual std::vector<std::string> listPartitions() const = 0;

    /**
     * @brief Resize @p partition_id to @p new_capacity maximum entries.
     *
     * If @p new_capacity < current occupancy of the partition, excess entries
     * are evicted via the partition's eviction strategy before this call returns.
     *
     * @param partition_id  Partition to resize.
     * @param new_capacity  New maximum entry count (0 = unlimited).
     */
    virtual void resize(const std::string& partition_id,
                        size_t             new_capacity) = 0;

    /**
     * @brief Remove all entries from @p partition_id.
     *
     * Blocking call; returns after all entries in the partition have been
     * removed from the local cache tier.  Tenant assignments are preserved.
     *
     * @param partition_id  Partition to evict.
     */
    virtual void evictPartition(const std::string& partition_id) = 0;

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Return statistics for @p partition_id.
     *
     * Returns `std::nullopt` if the partition does not exist.
     *
     * @param partition_id  Partition to query.
     */
    virtual std::optional<PartitionStats> getStats(
        const std::string& partition_id) const = 0;

    /**
     * @brief Return statistics for all partitions.
     *
     * Returned vector is ordered by partition ID lexicographically.
     */
    virtual std::vector<PartitionStats> getAllStats() const = 0;

    /**
     * @brief Return the capacity of @p partition_id.
     *
     * Returns 0 if the partition does not exist or has unlimited capacity.
     *
     * @param partition_id  Partition to query.
     */
    virtual size_t getCapacity(const std::string& partition_id) const = 0;
};

} // namespace cache
} // namespace themis
