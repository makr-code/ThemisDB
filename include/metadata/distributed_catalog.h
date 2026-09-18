/**
 * @file distributed_catalog.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "metadata/schema_manager.h"
#include "sharding/metadata_shard.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <atomic>

namespace themis {

/// DistributedMetadataCatalog - Distributed schema catalog across metadata shards
///
/// Bridges the local SchemaManager catalog with the distributed MetadataShardRouter,
/// enabling schema metadata to be published to and queried from any shard in the
/// cluster.  All table schemas are stored in the MetadataPartitionKey::SCHEMA
/// partition of the shard router using the table name as the key.
///
/// Architecture:
///   DistributedMetadataCatalog
///   ├─→ MetadataShardRouter  (distributed key/value store, SCHEMA partition)
///   └─→ SchemaManager        (local RocksDB-based catalog, used for bulk sync)
///
/// Thread-Safety:
/// - All public methods are thread-safe. The router provides its own thread
///   safety; operation counters are maintained with std::atomic.
///
/// Usage:
///   DistributedMetadataCatalog catalog(router);
///   catalog.publishSchema(schema);
///   auto fetched = catalog.fetchSchema("users");
///   catalog.syncFromSchemaManager(schema_mgr);
///
/// Issue: makr-code/ThemisDB#1961
class DistributedMetadataCatalog {
public:
    /**
     * @brief Constructor @param router Distributed shard router (non-owning reference)
     * @param[in,out] router Input/output parameter.
     * @return Return value.
     */
    explicit DistributedMetadataCatalog(themisdb::sharding::MetadataShardRouter& router);

    ~DistributedMetadataCatalog() = default;

    // Non-copyable and non-movable (holds a reference and atomic counters)
    DistributedMetadataCatalog(const DistributedMetadataCatalog&) = delete;
    DistributedMetadataCatalog& operator=(const DistributedMetadataCatalog&) = delete;
    DistributedMetadataCatalog(DistributedMetadataCatalog&&) = delete;
    DistributedMetadataCatalog& operator=(DistributedMetadataCatalog&&) = delete;

    // ========================================================================
    // Write API
    // ========================================================================

    /**
     * @brief Publish (create or update) a single table schema to the distributed catalog.
     * @param[in] schema Input parameter.
     * @return True on success.
     * @details @param schema Table schema to publish @return true on success, false if the router rejected the write
     */
    bool publishSchema(const SchemaManager::TableSchema& schema);

    /**
     * @brief Sync all table schemas from a local SchemaManager to the distributed catalog.
     * @param[in,out] schema_mgr Input/output parameter.
     * @return Return value.
     * @details Iterates all tables returned by schema_mgr.getAllTables() and calls publishSchema() for each one. @param schema_mgr Source of truth for local schemas @return Number of schemas successfully published
     */
    size_t syncFromSchemaManager(SchemaManager& schema_mgr);

    /**
     * @brief Remove a table schema from the distributed catalog.
     * @param[in] table_name Input parameter.
     * @return True on success.
     * @details @param table_name Table name to remove @return true if removed, false if not found or router rejected the removal
     */
    bool removeSchema(const std::string& table_name);

    // ========================================================================
    // Read API
    // ========================================================================

    /**
     * @brief Fetch a table schema from the distributed catalog.
     * @param[in] table_name Input parameter.
     * @return Return value.
     * @details @param table_name Table name to look up @return TableSchema if found, std::nullopt otherwise
     */
    std::optional<SchemaManager::TableSchema> fetchSchema(
        const std::string& table_name) const;

    /**
     * @brief List all table names present in the distributed catalog.
     * @return Return value.
     * @details Performs a scatter-gather across all shards via the router. @return Sorted vector of table names
     */
    std::vector<std::string> listTableNames() const;

    // ========================================================================
    // Diagnostics
    // ========================================================================

    /**
     * @brief Return catalog statistics as a JSON object.
     * @return Return value.
     * @details Includes counts of published, sync, fetch, and remove operations.
     */
    nlohmann::json getStatistics() const;

private:
    themisdb::sharding::MetadataShardRouter& router_;

    // Operation counters (atomic for thread safety)
    mutable std::atomic<uint64_t> publish_count_{0};
    mutable std::atomic<uint64_t> fetch_count_{0};
    mutable std::atomic<uint64_t> remove_count_{0};
    mutable std::atomic<uint64_t> sync_count_{0};
};

} // namespace themis
