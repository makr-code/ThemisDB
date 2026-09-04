/**
 * @file schema_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=7, M=24, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/schema_manager.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "index/secondary_index.h"
#include "index/secondary_index_metadata_cache.h"
#include <spdlog/spdlog.h>
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>

namespace themis {

// ============================================================================
// JSON Serialization Helpers
// ============================================================================

json SchemaManager::PropertyInfo::toJSON() const {
    json j;
    j["name"] = name;
    j["type"] = type;
    j["indexed"] = indexed;
    j["nullable"] = nullable;
    if (!index_type.empty()) {
        j["index_type"] = index_type;
    }
    return j;
}

json SchemaManager::IndexInfo::toJSON() const {
    json j;
    j["name"] = name;
    j["type"] = type;
    j["unique"] = unique;
    if (!columns.empty()) {
        j["columns"] = columns;
    }
    return j;
}

json SchemaManager::TableSchema::toJSON() const {
    json j;
    j["name"] = name;
    j["type"] = type;
    j["estimated_row_count"] = estimated_row_count;
    
    json props = json::array();
    for (const auto& prop : properties) {
        props.push_back(prop.toJSON());
    }
    j["properties"] = props;
    
    json idxs = json::array();
    for (const auto& idx : indexes) {
        idxs.push_back(idx.toJSON());
    }
    j["indexes"] = idxs;
    
    return j;
}

json SchemaManager::RelationshipSchema::toJSON() const {
    json j;
    j["name"] = name;
    j["from_table"] = from_table;
    j["to_table"] = to_table;
    
    json props = json::array();
    for (const auto& prop : properties) {
        props.push_back(prop.toJSON());
    }
    j["properties"] = props;
    
    return j;
}

json SchemaManager::DatabaseMetadata::toJSON() const {
    json j;
    j["version"] = version;
    j["table_count"] = table_count;
    j["total_rows"] = total_rows;
    j["capabilities"] = capabilities;
    
    // Format timestamp using thread-safe localtime conversion.
    const auto ts = std::chrono::system_clock::to_time_t(last_refresh);
    std::tm tm_buf{};
    char buf[100] = {};
#if defined(_WIN32)
    if (::localtime_s(&tm_buf, &ts) == 0) {
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        j["last_refresh"] = buf;
    } else {
        j["last_refresh"] = "1970-01-01 00:00:00";
    }
#else
    if (::localtime_r(&ts, &tm_buf) != nullptr) {
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        j["last_refresh"] = buf;
    } else {
        j["last_refresh"] = "1970-01-01 00:00:00";
    }
#endif
    
    return j;
}

// ============================================================================
// Constructor
// ============================================================================

SchemaManager::SchemaManager(
    RocksDBWrapper& db,
    SecondaryIndexManager* index_mgr
) : db_(db), index_mgr_(index_mgr) {
    loadCustomSchemas();  // Load persisted custom schemas on startup
    spdlog::debug("SchemaManager: Initialized");
}

// ============================================================================
// Public API - Schema Discovery
// ============================================================================

std::vector<SchemaManager::TableSchema> SchemaManager::getAllTables() {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    
    // Check if cache is valid
    if (!isCacheValid()) {
        // Need to rebuild - upgrade to write lock
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(cache_mutex_);
        
        // Double-check after acquiring write lock (another thread might have rebuilt)
        if (!isCacheValid()) {
            buildCache();
        }
    }
    
    // Extract tables from cache
    std::vector<TableSchema> tables = {};

    tables.reserve(table_cache_.size());
    for (const auto& [name, schema] : table_cache_) {
        tables.push_back(schema);
    }
    
    spdlog::debug("SchemaManager: getAllTables() returned {} tables", tables.size());
    return tables;
}

std::optional<SchemaManager::TableSchema> SchemaManager::getTable(std::string_view name) {
    // Optimistic read path: take shared lock first.
    // If the cache is stale, upgrade to a unique (write) lock to rebuild, then
    // read directly under that lock instead of downgrading back to shared — which
    // avoids an unbounded blocking re-lock that has no timeout guarantee.
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    if (!isCacheValid()) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(cache_mutex_);
        if (!isCacheValid()) {
            buildCache();
        }
        // Read under the write lock (unique_lock grants full access) — no re-lock needed.
        auto it = table_cache_.find(std::string(name));
        if (it != table_cache_.end()) {
            spdlog::debug("SchemaManager: getTable('{}') found", name);
            return it->second;
        }
        spdlog::debug("SchemaManager: getTable('{}') not found", name);
        return std::nullopt;
    }

    auto it = table_cache_.find(std::string(name));
    if (it != table_cache_.end()) {
        spdlog::debug("SchemaManager: getTable('{}') found", name);
        return it->second;
    }
    
    spdlog::debug("SchemaManager: getTable('{}') not found", name);
    return std::nullopt;
}

std::vector<SchemaManager::RelationshipSchema> SchemaManager::getAllRelationships() {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    if (!isCacheValid()) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(cache_mutex_);
        if (!isCacheValid()) {
            buildCache();
        }
        // Read under the write lock — no re-lock needed.
        std::vector<RelationshipSchema> relationships = {};

        relationships.reserve(rel_cache_.size());
        for (const auto& [name, schema] : rel_cache_) {
            relationships.push_back(schema);
        }
        spdlog::debug("SchemaManager: getAllRelationships() returned {} relationships", relationships.size());
        return relationships;
    }
    
    std::vector<RelationshipSchema> relationships = {};

    relationships.reserve(rel_cache_.size());
    for (const auto& [name, schema] : rel_cache_) {
        relationships.push_back(schema);
    }
    
    spdlog::debug("SchemaManager: getAllRelationships() returned {} relationships", relationships.size());
    return relationships;
}

SchemaManager::DatabaseMetadata SchemaManager::getDatabaseMetadata() {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    if (!isCacheValid()) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(cache_mutex_);
        if (!isCacheValid()) {
            buildCache();
        }
        // Fall through: write_lock still held; read cache below under unique_lock.
    }
    
    DatabaseMetadata metadata;
    
    // Version from compile-time constant
    #ifdef THEMIS_VERSION_STRING
    metadata.version = THEMIS_VERSION_STRING;
    #else
    metadata.version = "1.5.0";
    #endif
    
    metadata.table_count = table_cache_.size();
    metadata.total_rows = 0;
    for (const auto& [name, schema] : table_cache_) {
        metadata.total_rows += schema.estimated_row_count;
    }
    
    metadata.last_refresh = last_refresh_;
    
    // Capabilities based on build flags
    std::vector<std::string> caps;
    #ifdef THEMIS_LLM_ENABLED
    caps.push_back("llm");
    #endif
    #ifdef THEMIS_GEO_ENABLED
    caps.push_back("geo");
    #endif
    #ifdef THEMIS_ENABLE_GPU
    caps.push_back("gpu");
    #endif
    #ifdef THEMIS_ENABLE_GRPC
    caps.push_back("grpc");
    #endif
    #ifdef THEMIS_ENABLE_MCP
    caps.push_back("mcp");
    #endif
    #ifdef THEMIS_ENABLE_GRAPHQL
    caps.push_back("graphql");
    #endif
    caps.push_back("multi-model");
    caps.push_back("transactions");
    caps.push_back("secondary-indexes");
    caps.push_back("fulltext-search");
    
    metadata.capabilities = caps;
    
    spdlog::debug("SchemaManager: getDatabaseMetadata() - {} tables, {} rows",
                  metadata.table_count, metadata.total_rows);
    
    return metadata;
}

void SchemaManager::refreshCache() {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    buildCache();
    spdlog::info("SchemaManager: Cache manually refreshed");
}

void SchemaManager::setCacheTTL(std::chrono::seconds ttl) {
    cache_ttl_ = ttl;
    spdlog::debug("SchemaManager: Cache TTL set to {} seconds", ttl.count());
}

void SchemaManager::setChangefeed(Changefeed* changefeed) {
    changefeed_ = changefeed;
    if (changefeed_) {
        spdlog::info("SchemaManager: Changefeed registered for schema change notifications");
    } else {
        spdlog::debug("SchemaManager: Changefeed deregistered");
    }
}

void SchemaManager::recordMutation(std::string_view table_name) {
    std::lock_guard<std::mutex> lock(mutation_mutex_);
    auto now = std::chrono::system_clock::now();
    auto& log = mutation_log_[std::string(table_name)];
    log.push_back(now);

    // Prune timestamps that have fallen outside the measurement window
    auto cutoff = now - adaptive_ttl_config_.window;
    while (!log.empty() && log.front() < cutoff) {
        log.pop_front();
    }

    spdlog::debug("SchemaManager: Mutation recorded for table '{}' (window count: {})",
                  table_name, log.size());
}

void SchemaManager::enableAdaptiveTTL(AdaptiveTTLConfig config) {
    std::lock_guard<std::mutex> lock(mutation_mutex_);
    adaptive_ttl_config_ = config;
    mutation_log_.clear();
    adaptive_ttl_enabled_ = true;
    spdlog::info("SchemaManager: Adaptive TTL enabled (min={}s, max={}s, window={}s, scale={})",
                 config.min_ttl.count(), config.max_ttl.count(),
                 config.window.count(), config.scale_factor);
}

void SchemaManager::disableAdaptiveTTL() {
    std::lock_guard<std::mutex> lock(mutation_mutex_);
    adaptive_ttl_enabled_ = false;
    spdlog::info("SchemaManager: Adaptive TTL disabled; using fixed TTL of {}s",
                 cache_ttl_.count());
}

std::chrono::seconds SchemaManager::getEffectiveTTL() const {
    if (!adaptive_ttl_enabled_) {
        return cache_ttl_;
    }
    std::lock_guard<std::mutex> lock(mutation_mutex_);
    return computeAdaptiveTTL();
}

std::chrono::seconds SchemaManager::computeAdaptiveTTL() const {
    // Caller must hold mutation_mutex_
    auto now = std::chrono::system_clock::now();
    auto window_secs = static_cast<double>(adaptive_ttl_config_.window.count());
    auto cutoff = now - adaptive_ttl_config_.window;

    double max_rate = 0.0;
    for (auto& [table, log] : mutation_log_) {
        // Prune stale entries so subsequent calls and .size() are accurate
        while (!log.empty() && log.front() < cutoff) {
            log.pop_front();
        }
        double rate = (window_secs > 0.0) ? static_cast<double>(log.size()) / window_secs : 0.0;
        if (rate > max_rate) {
          max_rate = rate;
        }
    }

    // effective_ttl = base_ttl / (1 + scale * max_rate), clamped to [min_ttl, max_ttl]
    double base = static_cast<double>(cache_ttl_.count());
    double effective = base / (1.0 + adaptive_ttl_config_.scale_factor * max_rate);
    double min_s = static_cast<double>(adaptive_ttl_config_.min_ttl.count());
    double max_s = static_cast<double>(adaptive_ttl_config_.max_ttl.count());
    int64_t clamped = static_cast<int64_t>(std::round(std::max(min_s, std::min(max_s, effective))));

    spdlog::debug("SchemaManager: Adaptive TTL computed: {}s (max_rate={:.4f} mut/s)",
                  clamped, max_rate);
    return std::chrono::seconds(clamped);
}

void SchemaManager::notifySchemaChange(std::string_view table_name, std::string_view event_kind) {
    if (!changefeed_) {
        return;
    }
    try {
        Changefeed::ChangeEvent ev;
        ev.key = "schema:" + std::string(table_name);
        ev.type = (event_kind == "schema_deleted")
                    ? Changefeed::ChangeEventType::EVENT_DELETE
                    : Changefeed::ChangeEventType::EVENT_PUT;
        ev.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        ev.metadata = {
            {"table", std::string(table_name)},
            {"event", std::string(event_kind)}
        };
        changefeed_->recordEvent(std::move(ev));
        spdlog::debug("SchemaManager: Emitted '{}' notification for table '{}'", event_kind, table_name);
    } catch (const std::exception& e) {
        spdlog::warn("SchemaManager: Failed to emit schema change notification: {}", e.what());
    }
}

// ============================================================================
// JSON Export API
// ============================================================================

json SchemaManager::toJSON() {
    auto tables = getAllTables();
    auto relationships = getAllRelationships();
    auto metadata = getDatabaseMetadata();
    
    json result;
    result["status"] = "success";
    result["metadata"] = metadata.toJSON();
    
    json tables_json = json::array();
    for (const auto& table : tables) {
        tables_json.push_back(table.toJSON());
    }
    result["tables"] = tables_json;
    
    json rels_json = json::array();
    for (const auto& rel : relationships) {
        rels_json.push_back(rel.toJSON());
    }
    result["relationships"] = rels_json;
    
    return result;
}

json SchemaManager::tableToJSON(std::string_view table_name) {
    auto table_opt = getTable(table_name);
    
    if (table_opt) {
        json result;
        result["status"] = "success";
        result["table"] = table_opt->toJSON();
        return result;
    } else {
        json result;
        result["status"] = "error";
        result["message"] = std::string("Table not found: ") + std::string(table_name);
        return result;
    }
}

json SchemaManager::getCapabilitiesJSON() {
    auto metadata = getDatabaseMetadata();
    
    json result;
    result["status"] = "success";
    result["version"] = metadata.version;
    result["capabilities"] = metadata.capabilities;
    
    return result;
}

// ============================================================================
// Internal Implementation
// ============================================================================

std::vector<std::string> SchemaManager::discoverTableNames() {
    std::set<std::string> table_names;
    
    try {
        // Scan all keys and extract table names from key prefixes
        auto it_result = db_.newIterator();
        if (!it_result) {
            spdlog::warn("SchemaManager: Failed to create iterator: {}", it_result.error().message());
            return {};
        }
        auto it = std::move(it_result.value());
        
        it->SeekToFirst();
        while (it->Valid()) {
            std::string key = it->key().ToString();
            
            // Parse key to extract table name
            // Key formats:
            // - "table_name:pk" (relational)
            // - "collection:pk" (document)
            // - "node:pk" (graph node)
            // - "edge:pk" (graph edge)
            // - "idx:table:column:value:pk" (index - skip)
            // - "graph:out:pk:edge" (outdex - skip)
            // - "graph:in:pk:edge" (index - skip)
            
            static const std::array<std::string_view, 18> kInternalPrefixes = {
                "idx:", "ridx:", "sidx:", "gidx:", "ttlidx:", "ftidx:", "pidx:", "cidx:",
                "idxmeta:", "ridxmeta:", "sidxmeta:", "gidxmeta:", "ttlidxmeta:", "ftidxmeta:", "pidxmeta:", "cidxmeta:",
                "graph:", "config:"
            };

            const bool is_internal = std::any_of(kInternalPrefixes.begin(), kInternalPrefixes.end(),
                [&]([[maybe_unused]] std::string_view prefix) {
                    return key.rfind(prefix, 0) == 0;
                });

            if (is_internal) {
                // Skip index, graph and config internal keys
                it->Next();
                continue;
            }
            
            // Extract table name (everything before first ':')
            size_t colon_pos = key.find(':');
            if (colon_pos != std::string::npos && colon_pos > 0) {
                std::string table_name = key.substr(0, colon_pos);
                table_names.insert(table_name);
            }
            
            it->Next();
        }
        
        spdlog::debug("SchemaManager: Discovered {} table names", table_names.size());
        
    } catch (const std::exception& e) {
        spdlog::error("SchemaManager: Exception during table discovery: {}", e.what());
    }
    
    return std::vector<std::string>(table_names.begin(), table_names.end());
}

std::vector<SchemaManager::PropertyInfo> SchemaManager::discoverProperties(
    std::string_view table_name,
    size_t sample_size
) {
    static const std::set<std::string> kInternalBinaryKeyspaces = {
        "dek",
        "kek"
    };
    if (kInternalBinaryKeyspaces.contains(std::string(table_name))) {
        spdlog::debug("SchemaManager: Skipping property sampling for internal keyspace '{}'", table_name);
        return {};
    }

    std::map<std::string, PropertyInfo> property_map;
    
    try {
        // Build key prefix for this table
        std::string prefix = std::string(table_name) + ":";
        
        auto it_result = db_.newIterator();
        if (!it_result) {
            spdlog::warn("SchemaManager: Failed to create iterator for properties: {}", it_result.error().message());
            return {};
        }
        auto it = std::move(it_result.value());
        
        it->Seek(prefix);
        
        size_t sampled = 0;
        while (it->Valid() && sampled < sample_size) {
            std::string key = it->key().ToString();
            
            // Check if key matches our table prefix
            if (key.find(prefix) != 0) {
                break;  // No more keys for this table
            }
            
            // Parse entity to discover properties
            try {
                std::string pk = key.substr(prefix.length());
                const auto value = it->value();
                std::vector<uint8_t> blob(value.data(), value.data() + value.size());
                
                // Try to deserialize as BaseEntity
                BaseEntity entity = BaseEntity::deserialize(pk, blob);
                
                // Get all fields
                auto fields = entity.getAllFields();
                
                for (const auto& [field_name, field_value] : fields) {
                    // Determine type
                    std::string type_str = "null";
                    
                    if (std::holds_alternative<bool>(field_value)) {
                        type_str = "boolean";
                    } else if (std::holds_alternative<int64_t>(field_value)) {
                        type_str = "integer";
                    } else if (std::holds_alternative<double>(field_value)) {
                        type_str = "double";
                    } else if (std::holds_alternative<std::string>(field_value)) {
                        type_str = "string";
                    } else if (std::holds_alternative<std::vector<float>>(field_value)) {
                        type_str = "vector";
                    } else if (std::holds_alternative<std::vector<uint8_t>>(field_value)) {
                        type_str = "binary";
                    }
                    
                    // Update or create property info
                    auto& prop = property_map[field_name];
                    if (prop.name.empty()) {
                        prop.name = field_name;
                        prop.type = type_str;
                    } else {
                        // If types differ, mark as mixed (use "string" as fallback)
                        if (prop.type != type_str && type_str != "null") {
                            if (prop.type != "null") {
                                prop.type = "string";  // Mixed types -> string
                            } else {
                                prop.type = type_str;
                            }
                        }
                    }
                }
                
                sampled++;
                
            } catch (const std::exception& e) {
                spdlog::debug("SchemaManager: Failed to parse entity: {}", e.what());
            }
            
            it->Next();
        }
        
        spdlog::debug("SchemaManager: Discovered {} properties from {} samples for table '{}'",
                      property_map.size(), sampled, table_name);
        
    } catch (const std::exception& e) {
        spdlog::error("SchemaManager: Exception during property discovery: {}", e.what());
    }
    
    // Convert to vector
    std::vector<PropertyInfo> properties = {};

    properties.reserve(property_map.size());
    for (const auto& [name, prop] : property_map) {
        properties.push_back(prop);
    }
    
    return properties;
}

std::vector<SchemaManager::IndexInfo> SchemaManager::discoverIndexes(
    std::string_view table_name
) {
    std::vector<IndexInfo> indexes;
    
    if (!index_mgr_) {
        spdlog::debug("SchemaManager: No index manager available");
        return indexes;
    }
    
    try {
        // Try to get from metadata cache
        auto& cache = SecondaryIndexMetadataCache::instance();
        auto metadata_opt = cache.get(table_name);
        
        if (metadata_opt) {
            const auto& metadata = *metadata_opt;
            
            // Regular indexes
            for (const auto& col : metadata.regular_indexes) {
                IndexInfo idx;
                idx.name = col;
                idx.type = "regular";
                idx.columns.push_back(col);
                
                auto it = metadata.regular_unique.find(col);
                if (it != metadata.regular_unique.end()) {
                    idx.unique = it->second;
                }
                
                indexes.push_back(idx);
            }
            
            // Range indexes
            for (const auto& col : metadata.range_indexes) {
                IndexInfo idx;
                idx.name = col;
                idx.type = "range";
                idx.columns.push_back(col);
                indexes.push_back(idx);
            }
            
            // Sparse indexes
            for (const auto& col : metadata.sparse_indexes) {
                IndexInfo idx;
                idx.name = col;
                idx.type = "sparse";
                idx.columns.push_back(col);
                
                auto it = metadata.sparse_unique.find(col);
                if (it != metadata.sparse_unique.end()) {
                    idx.unique = it->second;
                }
                
                indexes.push_back(idx);
            }
            
            // Geo indexes
            for (const auto& col : metadata.geo_indexes) {
                IndexInfo idx;
                idx.name = col;
                idx.type = "geo";
                idx.columns.push_back(col);
                indexes.push_back(idx);
            }
            
            // TTL indexes
            for (const auto& col : metadata.ttl_indexes) {
                IndexInfo idx;
                idx.name = col;
                idx.type = "ttl";
                idx.columns.push_back(col);
                indexes.push_back(idx);
            }
            
            // Fulltext indexes
            for (const auto& col : metadata.fulltext_indexes) {
                IndexInfo idx;
                idx.name = col;
                idx.type = "fulltext";
                idx.columns.push_back(col);
                indexes.push_back(idx);
            }

            // Partial indexes
            for (const auto& col : metadata.partial_indexes) {
                IndexInfo idx;
                idx.name = col;
                idx.type = "partial";
                idx.columns.push_back(col);

                auto it = metadata.partial_unique.find(col);
                if (it != metadata.partial_unique.end()) {
                    idx.unique = it->second;
                }

                indexes.push_back(idx);
            }
        }

        spdlog::debug("SchemaManager: Discovered {} indexes for table '{}'",
                      indexes.size(), table_name);
        
    } catch (const std::exception& e) {
        spdlog::error("SchemaManager: Exception during index discovery: {}", e.what());
    }
    
    return indexes;
}

size_t SchemaManager::estimateRowCount(std::string_view table_name) {
    size_t count = 0;
    
    try {
        std::string prefix = std::string(table_name) + ":";
        
        auto it_result = db_.newIterator();
        if (!it_result) {
            return 0;
        }
        auto it = std::move(it_result.value());
        
        it->Seek(prefix);
        
        while (it->Valid()) {
            std::string key = it->key().ToString();
            
            if (key.find(prefix) != 0) {
                break;  // No more keys for this table
            }
            
            count++;
            it->Next();
        }
        
    } catch (const std::exception& e) {
        spdlog::error("SchemaManager: Exception during row count: {}", e.what());
    }
    
    return count;
}

std::string SchemaManager::determineTableType(std::string_view table_name) {
    std::string name_lower(table_name);
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    
    // Heuristics based on naming conventions
    if (name_lower == "node" || name_lower == "nodes") {
        return "graph_node";
    } else if (name_lower == "edge" || name_lower == "edges") {
        return "graph_edge";
    } else if (name_lower.find("vector") != std::string::npos ||
               name_lower.find("embedding") != std::string::npos) {
        return "vector";
    } else if (name_lower.find("collection") != std::string::npos ||
               name_lower.find("document") != std::string::npos) {
        return "document";
    }
    
    // Default to relational
    return "relational";
}

bool SchemaManager::isCacheValid() const {
    if (table_cache_.empty()) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_refresh_);
    
    if (adaptive_ttl_enabled_) {
        std::lock_guard<std::mutex> lock(mutation_mutex_);
        return elapsed < computeAdaptiveTTL();
    }

    return elapsed < cache_ttl_;
}

void SchemaManager::buildCache() {
    spdlog::debug("SchemaManager: Building cache...");
    
    auto start = std::chrono::steady_clock::now();
    
    // Clear existing cache
    table_cache_.clear();
    rel_cache_.clear();
    
    // Discover all tables
    auto table_names = discoverTableNames();
    
    // Build schema for each discovered table
    for (const auto& table_name : table_names) {
        TableSchema schema;
        schema.name = table_name;
        schema.type = determineTableType(table_name);
        schema.properties = discoverProperties(table_name);
        schema.indexes = discoverIndexes(table_name);

        // Optional fallback probing is disabled by default because some builds
        // may expose unsafe index-manager states for arbitrary discovered keys.
        // Re-enable only for controlled diagnostics via env toggle.
        const bool enable_fallback_index_probe = [] {
            const char* raw = std::getenv("THEMIS_SCHEMA_INDEX_FALLBACK_PROBE");
            return raw != nullptr && std::string_view(raw) == "1";
        }();
        if (schema.indexes.empty() && index_mgr_ && enable_fallback_index_probe) {
            std::set<std::string> seen;
            auto add_index = [&](const std::string& col, const std::string& type, bool unique) {
                const std::string key = type + ":" + col;
                if (!seen.insert(key).second) {
                    return;
                }
                IndexInfo idx;
                idx.name = col;
                idx.type = type;
                idx.unique = unique;
                idx.columns.push_back(col);
                schema.indexes.push_back(std::move(idx));
            };

            for (const auto& prop : schema.properties) {
                const std::string& col = prop.name;
                if (col.empty()) {
                    continue;
                }

                if (index_mgr_->hasIndex(table_name, col)) {
                    add_index(col, "regular", false);
                }
                if (index_mgr_->hasRangeIndex(table_name, col)) {
                    add_index(col, "range", false);
                }
                if (index_mgr_->hasSparseIndex(table_name, col)) {
                    add_index(col, "sparse", false);
                }
                if (index_mgr_->hasGeoIndex(table_name, col)) {
                    add_index(col, "geo", false);
                }
                if (index_mgr_->hasTTLIndex(table_name, col)) {
                    add_index(col, "ttl", false);
                }
                if (index_mgr_->hasFulltextIndex(table_name, col)) {
                    add_index(col, "fulltext", false);
                }
                if (index_mgr_->hasPartialIndex(table_name, col)) {
                    add_index(col, "partial", false);
                }
            }
        }

        for (const auto& idx : schema.indexes) {
            for (const auto& col : idx.columns) {
                auto it = std::find_if(schema.properties.begin(), schema.properties.end(),
                                       [&]([[maybe_unused]] const PropertyInfo& prop) {
                                           return prop.name == col;
                                       });
                if (it != schema.properties.end()) {
                    it->indexed = true;
                    it->index_type = idx.type;
                }
            }
        }

        schema.estimated_row_count = estimateRowCount(table_name);
        
        table_cache_[table_name] = schema;
        
        // If it's an edge table, also add to relationships
        if (schema.type == "graph_edge") {
            RelationshipSchema rel;
            rel.name = table_name;
            rel.from_table = "node";  // Default - could be enhanced
            rel.to_table = "node";
            rel.properties = schema.properties;
            
            rel_cache_[table_name] = rel;
        }
    }
    
    // Merge custom schemas (override discovered schemas)
    for (const auto& [table_name, custom_schema] : custom_schemas_) {
        table_cache_[table_name] = custom_schema;
        
        // Also add to relationships if it's an edge type
        if (custom_schema.type == "graph_edge") {
            RelationshipSchema rel;
            rel.name = table_name;
            rel.from_table = "node";
            rel.to_table = "node";
            rel.properties = custom_schema.properties;
            
            rel_cache_[table_name] = rel;
        }
    }
    
    last_refresh_ = std::chrono::system_clock::now();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    spdlog::info("SchemaManager: Cache built in {}ms - {} tables ({} custom), {} relationships",
                 duration.count(), table_cache_.size(), custom_schemas_.size(), rel_cache_.size());
}

// ============================================================================
// Schema Management API (PUT/PATCH)
// ============================================================================

bool SchemaManager::setTableSchema(std::string_view table_name, const TableSchema& schema) {
    // Validate schema
    std::string validation_error = validateSchema(schema);
    if (!validation_error.empty()) {
        spdlog::error("SchemaManager: Invalid schema for table '{}': {}", 
                      table_name, validation_error);
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    // Save to custom schemas map and RocksDB
    custom_schemas_[std::string(table_name)] = schema;
    saveCustomSchema(table_name, schema);
    
    // Also update the cache so it's immediately available
    table_cache_[std::string(table_name)] = schema;
    lock.unlock();
    
    spdlog::info("SchemaManager: Stored custom schema for table '{}'", table_name);
    notifySchemaChange(table_name, "schema_created");
    return true;
}

bool SchemaManager::patchTableSchema(std::string_view table_name, const json& updates) {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    // First check if we have a custom schema
    auto custom_it = custom_schemas_.find(std::string(table_name));
    if (custom_it == custom_schemas_.end()) {
        // Check if table exists in discovered schemas
        auto cache_it = table_cache_.find(std::string(table_name));
        if (cache_it == table_cache_.end()) {
            spdlog::warn("SchemaManager: Table '{}' not found for patch", table_name);
            return false;
        }
        // Promote discovered schema to custom schema for modification
        // This allows PATCH to work on both auto-discovered and custom schemas.
        // Once promoted, the custom schema will override discovery in future calls.
        custom_schemas_[std::string(table_name)] = cache_it->second;
        custom_it = custom_schemas_.find(std::string(table_name));
    }
    
    TableSchema& schema = custom_it->second;
    
    // Apply updates
    if (updates.contains("type") && updates["type"].is_string()) {
        schema.type = updates["type"].get<std::string>();
    }
    
    if (updates.contains("properties") && updates["properties"].is_array()) {
        // Merge or replace properties
        std::map<std::string, PropertyInfo> prop_map = {};

        for (const auto& prop : schema.properties) {
            prop_map[prop.name] = prop;
        }
        
        for (const auto& upd_prop : updates["properties"]) {
            if (!upd_prop.contains("name")) {
              continue;
            }
            
            std::string prop_name = upd_prop["name"].get<std::string>();
            PropertyInfo& info = prop_map[prop_name];
            info.name = prop_name;
            
            if (upd_prop.contains("type")) {
                info.type = upd_prop["type"].get<std::string>();
            }
            if (upd_prop.contains("indexed")) {
                info.indexed = upd_prop["indexed"].get<bool>();
            }
            if (upd_prop.contains("nullable")) {
                info.nullable = upd_prop["nullable"].get<bool>();
            }
            if (upd_prop.contains("index_type")) {
                info.index_type = upd_prop["index_type"].get<std::string>();
            }
        }
        
        schema.properties.clear();
        for (const auto& [name, prop] : prop_map) {
            schema.properties.push_back(prop);
        }
    }
    
    if (updates.contains("indexes") && updates["indexes"].is_array()) {
        // Similar merge logic for indexes
        std::map<std::string, IndexInfo> idx_map = {};

        for (const auto& idx : schema.indexes) {
            idx_map[idx.name] = idx;
        }
        
        for (const auto& upd_idx : updates["indexes"]) {
            if (!upd_idx.contains("name")) {
              continue;
            }
            
            std::string idx_name = upd_idx["name"].get<std::string>();
            IndexInfo& info = idx_map[idx_name];
            info.name = idx_name;
            
            if (upd_idx.contains("type")) {
                info.type = upd_idx["type"].get<std::string>();
            }
            if (upd_idx.contains("unique")) {
                info.unique = upd_idx["unique"].get<bool>();
            }
            if (upd_idx.contains("columns")) {
                info.columns.clear();
                for (const auto& col : upd_idx["columns"]) {
                    info.columns.push_back(col.get<std::string>());
                }
            }
        }
        
        schema.indexes.clear();
        for (const auto& [name, idx] : idx_map) {
            schema.indexes.push_back(idx);
        }
    }
    
    // Validate updated schema
    std::string validation_error = validateSchema(schema);
    if (!validation_error.empty()) {
        spdlog::error("SchemaManager: Invalid patched schema for table '{}': {}", 
                      table_name, validation_error);
        return false;
    }
    
    // Save and update cache
    saveCustomSchema(table_name, schema);
    table_cache_[std::string(table_name)] = schema;
    lock.unlock();
    
    spdlog::info("SchemaManager: Patched schema for table '{}'", table_name);
    notifySchemaChange(table_name, "schema_updated");
    return true;
}

bool SchemaManager::deleteTableSchema(std::string_view table_name) {
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    auto it = custom_schemas_.find(std::string(table_name));
    if (it == custom_schemas_.end()) {
        spdlog::debug("SchemaManager: No custom schema to delete for table '{}'", table_name);
        return false;
    }
    
    // Remove from custom schemas
    custom_schemas_.erase(it);
    
    // Remove from RocksDB
    std::string key = "config:schema:" + std::string(table_name);
    bool result = db_.del(key);
    if (!result) {
        spdlog::warn("SchemaManager: Failed to delete schema from storage for table '{}'", table_name);
    }
    
    // Rebuild cache to reflect discovered schema (if any)
    buildCache();
    lock.unlock();
    
    spdlog::info("SchemaManager: Deleted custom schema for table '{}'", table_name);
    notifySchemaChange(table_name, "schema_deleted");
    return true;
}

std::string SchemaManager::validateSchema(const TableSchema& schema) const {
    // Check required fields
    if (schema.name.empty()) {
        return "Table name is required";
    }
    
    // Validate table name (alphanumeric, underscores, hyphens only)
    for (char c : schema.name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            return "Table name contains invalid characters (use alphanumeric, _, -)";
        }
    }
    
    // Validate type
    static const std::set<std::string> valid_types = {
        "relational", "document", "graph_node", "graph_edge", "vector"
    };
    if (valid_types.find(schema.type) == valid_types.end()) {
        return "Invalid table type (must be: relational, document, graph_node, graph_edge, or vector)";
    }
    
    // Validate properties
    std::set<std::string> prop_names = {};

    for (const auto& prop : schema.properties) {
        if (prop.name.empty()) {
            return "Property name cannot be empty";
        }
        
        // Check for duplicate property names
        if (prop_names.count(prop.name) > 0) {
            return "Duplicate property name: " + prop.name;
        }
        prop_names.insert(prop.name);
        
        // Validate property type
        static const std::set<std::string> valid_prop_types = {
            "string", "integer", "double", "boolean", "vector", "binary", "null"
        };
        if (valid_prop_types.find(prop.type) == valid_prop_types.end()) {
            return "Invalid property type for '" + prop.name + "' (must be: string, integer, double, boolean, vector, binary, or null)";
        }
        
        // Validate index_type if present
        if (!prop.index_type.empty()) {
            static const std::set<std::string> valid_index_types = {
                "regular", "range", "sparse", "geo", "ttl", "fulltext"
            };
            if (valid_index_types.find(prop.index_type) == valid_index_types.end()) {
                return "Invalid index_type for '" + prop.name + "'";
            }
        }
    }
    
    // Validate indexes
    std::set<std::string> idx_names = {};

    for (const auto& idx : schema.indexes) {
        if (idx.name.empty()) {
            return "Index name cannot be empty";
        }
        
        // Check for duplicate index names
        if (idx_names.count(idx.name) > 0) {
            return "Duplicate index name: " + idx.name;
        }
        idx_names.insert(idx.name);
        
        // Validate index type
        static const std::set<std::string> valid_idx_types = {
            "regular", "range", "sparse", "geo", "ttl", "fulltext", "composite"
        };
        if (valid_idx_types.find(idx.type) == valid_idx_types.end()) {
            return "Invalid index type for '" + idx.name + "'";
        }
        
        // Validate columns exist
        if (idx.columns.empty()) {
            return "Index '" + idx.name + "' must have at least one column";
        }
        
        for (const auto& col : idx.columns) {
            if (prop_names.find(col) == prop_names.end()) {
                return "Index '" + idx.name + "' references non-existent property: " + col;
            }
        }
    }
    
    return "";  // Valid
}

SchemaManager::TableSchema SchemaManager::parseTableSchema(const json& j) {
    TableSchema schema;
    
    if (!j.contains("name") || !j["name"].is_string()) {
        throw std::runtime_error("Schema must contain 'name' field");
    }
    schema.name = j["name"].get<std::string>();
    
    if (j.contains("type") && j["type"].is_string()) {
        schema.type = j["type"].get<std::string>();
    } else {
        schema.type = "relational";  // Default
    }
    
    if (j.contains("properties") && j["properties"].is_array()) {
        for (const auto& prop_json : j["properties"]) {
            PropertyInfo prop;
            
            if (!prop_json.contains("name") || !prop_json["name"].is_string()) {
                throw std::runtime_error("Property must have 'name' field");
            }
            prop.name = prop_json["name"].get<std::string>();
            
            if (prop_json.contains("type") && prop_json["type"].is_string()) {
                prop.type = prop_json["type"].get<std::string>();
            } else {
                prop.type = "string";  // Default
            }
            
            if (prop_json.contains("indexed") && prop_json["indexed"].is_boolean()) {
                prop.indexed = prop_json["indexed"].get<bool>();
            }
            
            if (prop_json.contains("nullable") && prop_json["nullable"].is_boolean()) {
                prop.nullable = prop_json["nullable"].get<bool>();
            }
            
            if (prop_json.contains("index_type") && prop_json["index_type"].is_string()) {
                prop.index_type = prop_json["index_type"].get<std::string>();
            }
            
            schema.properties.push_back(prop);
        }
    }
    
    if (j.contains("indexes") && j["indexes"].is_array()) {
        for (const auto& idx_json : j["indexes"]) {
            IndexInfo idx;
            
            if (!idx_json.contains("name") || !idx_json["name"].is_string()) {
                throw std::runtime_error("Index must have 'name' field");
            }
            idx.name = idx_json["name"].get<std::string>();
            
            if (idx_json.contains("type") && idx_json["type"].is_string()) {
                idx.type = idx_json["type"].get<std::string>();
            } else {
                idx.type = "regular";  // Default
            }
            
            if (idx_json.contains("unique") && idx_json["unique"].is_boolean()) {
                idx.unique = idx_json["unique"].get<bool>();
            }
            
            if (idx_json.contains("columns") && idx_json["columns"].is_array()) {
                for (const auto& col : idx_json["columns"]) {
                    if (col.is_string()) {
                        idx.columns.push_back(col.get<std::string>());
                    }
                }
            } else {
                // Default: use index name as column
                idx.columns.push_back(idx.name);
            }
            
            schema.indexes.push_back(idx);
        }
    }
    
    if (j.contains("estimated_row_count") && j["estimated_row_count"].is_number()) {
        schema.estimated_row_count = j["estimated_row_count"].get<size_t>();
    }
    
    return schema;
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

void SchemaManager::loadCustomSchemas() {
    try {
        // Scan RocksDB for config:schema:* keys
        auto it_result = db_.newIterator();
        if (!it_result) {
            spdlog::warn("SchemaManager: Failed to create iterator for loading schemas");
            return;
        }
        auto it = std::move(it_result.value());
        
        std::string prefix = "config:schema:";
        it->Seek(prefix);
        
        int loaded = 0;
        while (it->Valid()) {
            std::string key = it->key().ToString();
            
            if (key.find(prefix) != 0) {
                break;  // No more schema configs
            }
            
            // Extract table name
            std::string table_name = key.substr(prefix.length());
            
            // Parse schema JSON
            try {
                const auto value = it->value();
                std::string schema_json(value.data(), value.data() + value.size());
                json j = json::parse(schema_json);
                TableSchema schema = parseTableSchema(j);
                
                custom_schemas_[table_name] = schema;
                loaded++;
                
            } catch (const std::exception& e) {
                spdlog::warn("SchemaManager: Failed to parse custom schema for '{}': {}", 
                            table_name, e.what());
            }
            
            it->Next();
        }
        
        if (loaded > 0) {
            spdlog::info("SchemaManager: Loaded {} custom schemas", loaded);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("SchemaManager: Exception loading custom schemas: {}", e.what());
    }
}

void SchemaManager::saveCustomSchema(std::string_view table_name, const TableSchema& schema) {
    try {
        std::string key = "config:schema:" + std::string(table_name);
        json j = schema.toJSON();
        std::string value = j.dump();
        
                bool result = db_.put(key, std::vector<uint8_t>(value.begin(), value.end()));
        if (!result) {
            spdlog::error("SchemaManager: Failed to save schema for '{}'", table_name);
        } else {
            spdlog::debug("SchemaManager: Saved custom schema for '{}'", table_name);
        }
        
    } catch (const std::exception& e) {
        spdlog::error("SchemaManager: Exception saving custom schema: {}", e.what());
    }
}

} // namespace themis
