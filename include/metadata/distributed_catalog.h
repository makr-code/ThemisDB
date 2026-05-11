/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_catalog.h                              ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:45:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     129                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "metadata/schema_manager.h"
#include "sharding/metadata_shard.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include &lt;optional&gt;
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
    /// Constructor
    /// @param router Distributed shard router (non-owning reference)
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

    /// Publish (create or update) a single table schema to the distributed catalog.
    /// @param schema Table schema to publish
    /// @return true on success, false if the router rejected the write
    bool publishSchema(const SchemaManager::TableSchema& schema);

    /// Sync all table schemas from a local SchemaManager to the distributed catalog.
    /// Iterates all tables returned by schema_mgr.getAllTables() and calls
    /// publishSchema() for each one.
    /// @param schema_mgr Source of truth for local schemas
    /// @return Number of schemas successfully published
    size_t syncFromSchemaManager(SchemaManager& schema_mgr);

    /// Remove a table schema from the distributed catalog.
    /// @param table_name Table name to remove
    /// @return true if removed, false if not found or router rejected the removal
    bool removeSchema(const std::string& table_name);

    // ========================================================================
    // Read API
    // ========================================================================

    /// Fetch a table schema from the distributed catalog.
    /// @param table_name Table name to look up
    /// @return TableSchema if found, std::nullopt otherwise
    std::optional<SchemaManager::TableSchema> fetchSchema(
        const std::string& table_name) const;

    /// List all table names present in the distributed catalog.
    /// Performs a scatter-gather across all shards via the router.
    /// @return Sorted vector of table names
    std::vector<std::string> listTableNames() const;

    // ========================================================================
    // Diagnostics
    // ========================================================================

    /// Return catalog statistics as a JSON object.
    /// Includes counts of published, sync, fetch, and remove operations.
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
