/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_catalog.cpp                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:34:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     146                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8cd57d682a  2026-02-27  feat(metadata): add DistributedMetadataCatalog for distri... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/distributed_catalog.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {

DistributedMetadataCatalog::DistributedMetadataCatalog(
    themisdb::sharding::MetadataShardRouter& router)
    : router_(router) {}

// ============================================================================
// Write API
// ============================================================================

bool DistributedMetadataCatalog::publishSchema(
    const SchemaManager::TableSchema& schema)
{
    if (schema.name.empty()) {
        spdlog::warn("DistributedMetadataCatalog::publishSchema: empty table name, skipping");
        return false;
    }

    nlohmann::json value = schema.toJSON();

    bool ok = router_.put(
        themisdb::sharding::MetadataPartitionKey::SCHEMA,
        schema.name,
        value);

    if (ok) {
        ++publish_count_;
        spdlog::debug("DistributedMetadataCatalog: published schema '{}'", schema.name);
    } else {
        spdlog::warn("DistributedMetadataCatalog: failed to publish schema '{}'", schema.name);
    }
    return ok;
}

size_t DistributedMetadataCatalog::syncFromSchemaManager(SchemaManager& schema_mgr)
{
    auto tables = schema_mgr.getAllTables();
    size_t synced = 0;

    for (const auto& schema : tables) {
        if (publishSchema(schema)) {
            ++synced;
        }
    }

    ++sync_count_;
    spdlog::info("DistributedMetadataCatalog: sync complete, {}/{} schemas published",
                 synced, tables.size());
    return synced;
}

bool DistributedMetadataCatalog::removeSchema(const std::string& table_name)
{
    if (table_name.empty()) {
        return false;
    }

    bool ok = router_.remove(
        themisdb::sharding::MetadataPartitionKey::SCHEMA,
        table_name);

    if (ok) {
        ++remove_count_;
        spdlog::debug("DistributedMetadataCatalog: removed schema '{}'", table_name);
    }
    return ok;
}

// ============================================================================
// Read API
// ============================================================================

std::optional<SchemaManager::TableSchema>
DistributedMetadataCatalog::fetchSchema(const std::string& table_name) const
{
    ++fetch_count_;

    auto entry = router_.get(
        themisdb::sharding::MetadataPartitionKey::SCHEMA,
        table_name);

    if (!entry.has_value()) {
        return std::nullopt;
    }

    try {
        return SchemaManager::parseTableSchema(entry->value);
    } catch (const std::exception& ex) {
        spdlog::error("DistributedMetadataCatalog: failed to parse schema for '{}': {}",
                      table_name, ex.what());
        return std::nullopt;
    }
}

std::vector<std::string> DistributedMetadataCatalog::listTableNames() const
{
    auto keys = router_.listKeys(themisdb::sharding::MetadataPartitionKey::SCHEMA);
    std::sort(keys.begin(), keys.end());
    return keys;
}

// ============================================================================
// Diagnostics
// ============================================================================

nlohmann::json DistributedMetadataCatalog::getStatistics() const
{
    return {
        {"publish_count", publish_count_.load()},
        {"fetch_count",   fetch_count_.load()},
        {"remove_count",  remove_count_.load()},
        {"sync_count",    sync_count_.load()},
    };
}

} // namespace themis
