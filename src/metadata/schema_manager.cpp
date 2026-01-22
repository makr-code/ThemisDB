// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/schema_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "index/secondary_index.h"
#include "index/secondary_index_metadata_cache.h"
#include <spdlog/spdlog.h>
#include <algorithm>
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
    
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(last_refresh);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
    j["last_refresh"] = buf;
    
    return j;
}

// ============================================================================
// Constructor
// ============================================================================

SchemaManager::SchemaManager(
    RocksDBWrapper& db,
    SecondaryIndexManager* index_mgr
) : db_(db), index_mgr_(index_mgr) {
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
    std::vector<TableSchema> tables;
    tables.reserve(table_cache_.size());
    for (const auto& [name, schema] : table_cache_) {
        tables.push_back(schema);
    }
    
    spdlog::debug("SchemaManager: getAllTables() returned {} tables", tables.size());
    return tables;
}

std::optional<SchemaManager::TableSchema> SchemaManager::getTable(std::string_view name) {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    
    // Check if cache is valid
    if (!isCacheValid()) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(cache_mutex_);
        if (!isCacheValid()) {
            buildCache();
        }
        write_lock.unlock();
        lock.lock();
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
        write_lock.unlock();
        lock.lock();
    }
    
    std::vector<RelationshipSchema> relationships;
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
        write_lock.unlock();
        lock.lock();
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
            
            if (key.find("idx:") == 0 || key.find("graph:") == 0) {
                // Skip index and graph internal keys
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
                std::vector<uint8_t> blob(it->value().data(), 
                                          it->value().data() + it->value().size());
                
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
    
    // Get index information and mark indexed properties
    auto indexes = discoverIndexes(table_name);
    for (const auto& idx : indexes) {
        for (const auto& col : idx.columns) {
            auto it = property_map.find(col);
            if (it != property_map.end()) {
                it->second.indexed = true;
                it->second.index_type = idx.type;
            }
        }
    }
    
    // Convert to vector
    std::vector<PropertyInfo> properties;
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
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
    
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
    
    // Build schema for each table
    for (const auto& table_name : table_names) {
        TableSchema schema;
        schema.name = table_name;
        schema.type = determineTableType(table_name);
        schema.properties = discoverProperties(table_name);
        schema.indexes = discoverIndexes(table_name);
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
    
    last_refresh_ = std::chrono::system_clock::now();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    spdlog::info("SchemaManager: Cache built in {}ms - {} tables, {} relationships",
                 duration.count(), table_cache_.size(), rel_cache_.size());
}

} // namespace themis
