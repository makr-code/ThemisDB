/**
 * @file schema_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <deque>
#include <optional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;
class Changefeed;

using json = nlohmann::json;

/// Configuration for adaptive TTL based on table mutation rate.
///
/// Used with SchemaManager::enableAdaptiveTTL().
/// When adaptive TTL is enabled, the effective cache TTL is computed as:
///   effective_ttl = clamp(base_ttl / (1 + scale_factor * rate), min_ttl, max_ttl)
/// where rate = mutations per second observed within the measurement window.
struct AdaptiveTTLConfig {
    std::chrono::seconds min_ttl{5};    ///< Minimum TTL used when mutation rate is very high
    std::chrono::seconds max_ttl{300};  ///< Maximum TTL used when mutation rate is zero
    std::chrono::seconds window{60};    ///< Sliding window for mutation rate measurement
    double scale_factor{1.0};           ///< Sensitivity: higher → TTL shrinks faster with rate
};

/// SchemaManager - Database Self-Awareness and Schema Discovery
///
/// Provides comprehensive schema introspection capabilities enabling ThemisDB
/// to understand its own structure and answer questions about stored data.
///
/// Features:
/// - Automatic table/collection discovery via RocksDB key scanning
/// - Property type detection from stored entities
/// - Index metadata collection from SecondaryIndexManager
/// - Relationship discovery (graph edges)
/// - Thread-safe caching with configurable TTL (default 60s)
/// - JSON export for REST API and MCP integration
///
/// Architecture:
///   SchemaManager
///   ├─→ RocksDB Key Scanning (table discovery)
///   ├─→ BaseEntity Parsing (property types)
///   └─→ SecondaryIndexManager (index metadata)
///
/// Thread-Safety:
/// - Uses std::shared_mutex for read-heavy cache access
/// - Multiple concurrent readers, single writer
/// - Safe for concurrent getAllTables() calls
///
/// Performance:
/// - Discovery time: <100ms for typical schemas (tested up to 100 tables)
/// - Cache hit rate: >90% expected
/// - Memory overhead: <50 MB for 100 tables
///
/// Usage:
///   SchemaManager schema_mgr(db, index_mgr);
///   auto tables = schema_mgr.getAllTables();
///   auto schema_json = schema_mgr.toJSON();
///
/// Version: 1.5.0
/// Issue: makr-code/ThemisDB#1
class SchemaManager {
public:
    /// Property type information
    struct PropertyInfo {
        std::string name;                           // Property name
        std::string type;                           // Type: "string", "integer", "double", "boolean", "vector", "binary", "null"
        bool indexed = false;                       // Has secondary index
        bool nullable = true;                       // Can be null/missing
        std::string index_type;                     // "regular", "range", "sparse", "geo", "ttl", "fulltext"
        
        json toJSON() const;
    };

    /// Index information
    struct IndexInfo {
        std::string name;                           // Index name (column name)
        std::string type;                           // "regular", "range", "sparse", "geo", "ttl", "fulltext", "composite"
        std::vector<std::string> columns;           // Column list (for composite indexes)
        bool unique = false;                        // Unique constraint
        
        json toJSON() const;
    };

    /// Table/collection schema
    struct TableSchema {
        std::string name;                           // Table/collection name
        std::string type;                           // "relational", "document", "graph_node", "graph_edge", "vector"
        std::vector<PropertyInfo> properties;       // Properties/columns
        std::vector<IndexInfo> indexes;             // Secondary indexes
        size_t estimated_row_count = 0;             // Approximate row count
        
        json toJSON() const;
    };

    /// Relationship/edge schema
    struct RelationshipSchema {
        std::string name;                           // Edge type/relationship name
        std::string from_table;                     // Source node type
        std::string to_table;                       // Target node type
        std::vector<PropertyInfo> properties;       // Edge properties
        
        json toJSON() const;
    };

    /// Database-level metadata
    struct DatabaseMetadata {
        std::string version;                        // ThemisDB version
        size_t table_count = 0;                     // Total tables/collections
        size_t total_rows = 0;                      // Total entities (approx)
        std::vector<std::string> capabilities;      // Enabled features
        std::chrono::system_clock::time_point last_refresh;
        
        json toJSON() const;
    };

    /// Constructor
    /// @param db RocksDB wrapper for key scanning
    /// @param index_mgr Secondary index manager (optional, for index metadata)
    explicit SchemaManager(
        RocksDBWrapper& db,
        SecondaryIndexManager* index_mgr = nullptr
    );

    /// Destructor
    ~SchemaManager() = default;

    // Disable copy, allow move
    SchemaManager(const SchemaManager&) = delete;
    SchemaManager& operator=(const SchemaManager&) = delete;
    SchemaManager(SchemaManager&&) noexcept = default;
    SchemaManager& operator=(SchemaManager&&) noexcept = default;

    // ========================================================================
    // Public API - Schema Discovery
    // ========================================================================

    /// Get all tables/collections
    /// Returns cached data if available and not expired
    std::vector<TableSchema> getAllTables();

    /// Get specific table schema by name
    /// @param name Table/collection name
    /// @return Table schema or nullopt if not found
    std::optional<TableSchema> getTable(std::string_view name);

    /// Get all relationships (graph edges)
    /// Returns edge types discovered in the database
    std::vector<RelationshipSchema> getAllRelationships();

    /// Get database-level metadata
    /// Includes version, capabilities, statistics
    DatabaseMetadata getDatabaseMetadata();

    /// Force refresh of schema cache
    /// Rescans RocksDB and rebuilds cache
    /// Call this after structural changes (create/drop table)
    void refreshCache();

    /// Set cache TTL (Time-To-Live)
    /// @param ttl Cache expiration time.
    void setCacheTTL(std::chrono::seconds ttl);

    /// Register a Changefeed for real-time schema change notifications.
    /// When set, every schema mutation (create/update/delete) emits a
    /// ChangeEvent with key "schema:{table_name}" into the given changefeed.
    /// @param changefeed Non-owning pointer; may be nullptr to disable notifications.
    void setChangefeed(Changefeed* changefeed);

    /// Record a data mutation (insert / update / delete) for a table.
    /// When adaptive TTL is enabled, high-frequency mutations cause the cache
    /// to expire sooner so that stale statistics are refreshed more quickly.
    /// This method is thread-safe and non-blocking.
    /// @param table_name Name of the table that was mutated.
    void recordMutation(std::string_view table_name);

    /// Enable adaptive TTL mode.
    /// The effective cache TTL is recomputed on every cache-validity check
    /// based on the per-table mutation rate observed in a sliding window.
    /// Calling this method resets any previously collected mutation history.
    /// @param config Adaptive TTL parameters (uses defaults if omitted).
    void enableAdaptiveTTL(AdaptiveTTLConfig config = {});

    /// Disable adaptive TTL and revert to the fixed TTL set by setCacheTTL().
    void disableAdaptiveTTL();

    /// Return the currently effective cache TTL.
    /// When adaptive TTL is disabled, equals the value set by setCacheTTL().
    /// When adaptive TTL is enabled, returns the rate-adjusted value.
    std::chrono::seconds getEffectiveTTL() const;

    // ========================================================================
    // JSON Export API
    // ========================================================================

    /// Export full schema as JSON
    /// Format compatible with MCP and REST API
    json toJSON();

    /// Export single table as JSON
    /// @param table_name Table/collection name
    json tableToJSON(std::string_view table_name);

    /// Export database capabilities as JSON
    /// Lists enabled features based on build flags
    json getCapabilitiesJSON();

    // ========================================================================
    // Schema Management API (PUT/PATCH)
    // ========================================================================

    /// Store/update custom schema for a table
    /// @param table_name Table/collection name
    /// @param schema Custom schema definition (JSON)
    /// @return true on success, false on validation failure
    bool setTableSchema(std::string_view table_name, const TableSchema& schema);

    /// Partial update of existing schema
    /// @param table_name Table/collection name
    /// @param updates JSON object with fields to update
    /// @return true on success, false if table not found or validation failure
    bool patchTableSchema(std::string_view table_name, const json& updates);

    /// Delete custom schema for a table
    /// @param table_name Table/collection name
    /// @return true if deleted, false if not found
    bool deleteTableSchema(std::string_view table_name);

    /// Validate table schema structure
    /// @param schema Schema to validate
    /// @return Error message if invalid, empty string if valid
    std::string validateSchema(const TableSchema& schema) const;

    /// Parse TableSchema from JSON
    /// @param j JSON object
    /// @return TableSchema or throws on parse error
    static TableSchema parseTableSchema(const json& j);

private:
    // ========================================================================
    // Internal Implementation
    // ========================================================================

    /// Discover all tables by scanning RocksDB keys
    /// Scans key prefixes to identify table/collection names
    std::vector<std::string> discoverTableNames();

    /// Discover properties for a table by sampling entities
    /// Parses BaseEntity objects to detect property types
    /// @param table_name Table/collection name
    /// @param sample_size Number of entities to sample (default: 100)
    std::vector<PropertyInfo> discoverProperties(
        std::string_view table_name,
        size_t sample_size = 100
    );

    /// Discover indexes for a table from SecondaryIndexManager
    /// @param table_name Table/collection name
    std::vector<IndexInfo> discoverIndexes(std::string_view table_name);

    /// Estimate row count for a table
    /// Uses RocksDB iterator to count keys with table prefix
    /// @param table_name Table/collection name
    size_t estimateRowCount(std::string_view table_name);

    /// Determine table type from key schema
    /// @param table_name Table/collection name
    /// @return "relational", "document", "graph_node", "graph_edge", "vector"
    std::string determineTableType(std::string_view table_name);

    /// Check if cache is valid (not expired)
    bool isCacheValid() const;

    /// Build cache from scratch
    void buildCache();

    /// Emit a schema change event to the registered changefeed (if any).
    /// @param table_name Table that changed
    /// @param event_kind "schema_created", "schema_updated", or "schema_deleted"
    void notifySchemaChange(std::string_view table_name, std::string_view event_kind);

    /// Load custom schemas from RocksDB
    void loadCustomSchemas();

    /// Save custom schema to RocksDB
    /// @param table_name Table name
    /// @param schema Schema to save
    void saveCustomSchema(std::string_view table_name, const TableSchema& schema);

    /// Compute the adaptive TTL from current per-table mutation rates.
    /// Caller must hold mutation_mutex_.
    std::chrono::seconds computeAdaptiveTTL() const;

    // ========================================================================
    // Member Variables
    // ========================================================================

    RocksDBWrapper& db_;                                    // Database wrapper
    SecondaryIndexManager* index_mgr_;                      // Index manager (optional)
    Changefeed* changefeed_ = nullptr;                      // Changefeed for schema notifications (optional)

    // Cache
    std::map<std::string, TableSchema> table_cache_;        // Table name -> schema
    std::map<std::string, RelationshipSchema> rel_cache_;   // Edge type -> schema
    std::chrono::system_clock::time_point last_refresh_;    // Last cache refresh time
    std::chrono::seconds cache_ttl_{60};                    // Cache TTL (default: 60s)

    // Custom schemas (persisted in RocksDB under "config:schema:{table_name}")
    std::map<std::string, TableSchema> custom_schemas_;     // User-defined schemas

    // Thread safety
    mutable std::shared_mutex cache_mutex_;                 // Read-write lock for cache

    // Adaptive TTL
    bool adaptive_ttl_enabled_ = false;                     // Whether adaptive TTL is active
    AdaptiveTTLConfig adaptive_ttl_config_;                 // Adaptive TTL parameters
    // Per-table mutation timestamps within the sliding window
    mutable std::map<std::string, std::deque<std::chrono::system_clock::time_point>> mutation_log_;
    mutable std::mutex mutation_mutex_;                     // Protects mutation_log_
};

} // namespace themis
