/**
 * @file distributed_catalog.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
                 synced,static_cast<int>(tables.size()));
    return synced;
}

bool DistributedMetadataCatalog::removeSchema(const std::string& table_name)
{
    if (table_name.empty()) {
        return false;
    }

    if (!router_.get(themisdb::sharding::MetadataPartitionKey::SCHEMA, table_name).has_value()) {
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
