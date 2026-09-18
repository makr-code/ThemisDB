/**
 * @file cache_partition.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

struct ICachePartition {
    /**
     * @brief ICache Partition.
     * @return Return value.
     */
    virtual ~ICachePartition() = default;

    // -----------------------------------------------------------------------
    // Tenant-to-partition mapping
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::string getPartitionId(const std::string& tenant_id) const = 0;

    /**
     * @brief Assign Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] partition_id Identifier of the partition.
     */
    virtual void assignTenant(const std::string& tenant_id,
                              const std::string& partition_id) = 0;

    /**
     * @brief Unassign Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    virtual void unassignTenant(const std::string& tenant_id) = 0;

    [[nodiscard]] virtual std::vector<std::string> listTenants(
        const std::string& partition_id) const = 0;

    // -----------------------------------------------------------------------
    // Partition lifecycle
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::vector<std::string> listPartitions() const = 0;

    /**
     * @brief Resize.
     * @param[in] partition_id Identifier of the partition.
     * @param[in] new_capacity Input parameter.
     */
    virtual void resize(const std::string& partition_id,
                        size_t             new_capacity) = 0;

    /**
     * @brief Evict Partition.
     * @param[in] partition_id Identifier of the partition.
     */
    virtual void evictPartition(const std::string& partition_id) = 0;

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::optional<PartitionStats> getStats(
        const std::string& partition_id) const = 0;

    [[nodiscard]] virtual std::vector<PartitionStats> getAllStats() const = 0;

    [[nodiscard]] virtual size_t getCapacity(const std::string& partition_id) const = 0;
};

} // namespace cache
} // namespace themis
