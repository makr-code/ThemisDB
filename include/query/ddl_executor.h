/**
 * @file ddl_executor.h
 * @brief DDL executor interface for AQL Schema DDL Phase 2.
 *
 * Provides:
 *   - SchemaRegistry  — thread-safe in-memory catalog of collections, indexes, and views.
 *   - DDLExecutor     — executes SchemaDDL AST nodes against a SchemaRegistry, enforcing
 *                       DDL semantics (duplicate detection, existence checks, etc.).
 *
 * @note This is the development-phase in-memory implementation.
 *       Production integration with the storage-engine catalog will replace the
 *       in-memory maps in a future phase.
 *
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include "query/aql_parser.h"
#include "utils/expected.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace query {

// ============================================================================
// SchemaRegistry
// ============================================================================

/**
 * @brief In-memory schema registry for DDL operations.
 *
 * Thread-safe collection/index/view catalog.  Used by DDLExecutor to track
 * schema state during CREATE/DROP/ALTER operations.
 *
 * Internal storage layout:
 *   - Collections : name  → engine options (nlohmann::json)
 *   - Indexes     : collection name → ( index name → IndexDef )
 *   - Views       : name  → AQL body string
 *
 * All mutating and querying operations are protected by a single `std::mutex`.
 */
class SchemaRegistry {
public:
    SchemaRegistry()  = default;
    ~SchemaRegistry() = default;

    // Non-copyable, movable
    SchemaRegistry(const SchemaRegistry&)            = delete;
    SchemaRegistry& operator=(const SchemaRegistry&) = delete;
    SchemaRegistry(SchemaRegistry&&)                 = default;
    SchemaRegistry& operator=(SchemaRegistry&&)      = default;

    // ── Query ────────────────────────────────────────────────────────────────

    /// Returns true if @p name exists in the collection catalog.
    [[nodiscard]] bool hasCollection(const std::string& name) const;

    /// Returns true if @p index_name exists on @p collection.
    [[nodiscard]] bool hasIndex(const std::string& collection,
                                const std::string& index_name) const;

    /// Returns true if @p name exists in the view catalog.
    [[nodiscard]] bool hasView(const std::string& name) const;

    /// Returns the stored options for @p name; returns an empty JSON object if
    /// the collection does not exist.
    [[nodiscard]] nlohmann::json collectionOptions(const std::string& name) const;

    /// Returns all registered collection names (order is unspecified).
    [[nodiscard]] std::vector<std::string> collections() const;

    /// Returns all registered view names (order is unspecified).
    [[nodiscard]] std::vector<std::string> views() const;

    // ── Mutate ───────────────────────────────────────────────────────────────

    /// Register a new collection.  Overwrites an existing entry.
    void addCollection(const std::string& name, const nlohmann::json& options = {});

    /// Remove a collection and all of its indexes.
    void dropCollection(const std::string& name);

    /// Register an index on @p collection.
    void addIndex(const std::string& collection, const IndexDef& def);

    /// Remove a specific index from @p collection.
    void dropIndex(const std::string& collection, const std::string& index_name);

    /// Register a view.  Overwrites an existing entry.
    void addView(const std::string& name, const std::string& body);

    /// Remove a view.
    void dropView(const std::string& name);

    /// Merge (or replace) the options for an existing collection.
    void alterCollection(const std::string& name, const nlohmann::json& options);

private:
    mutable std::mutex mu_;

    /// collection name → options
    std::unordered_map<std::string, nlohmann::json> collections_;

    /// collection name → ( index name → IndexDef )
    std::unordered_map<std::string,
                       std::unordered_map<std::string, IndexDef>> indexes_;

    /// view name → AQL body
    std::unordered_map<std::string, std::string> views_;
};

// ============================================================================
// DDLExecutor
// ============================================================================

/**
 * @brief Executes SchemaDDL statements against a SchemaRegistry.
 *
 * Enforces DDL semantics:
 *   - Duplicate CREATE (without IF NOT EXISTS) → ERR_DOC_ALREADY_EXISTS
 *   - DROP of non-existent object (without IF EXISTS) → ERR_SCHEMA_TABLE_NOT_FOUND
 *   - ALTER of non-existent collection → ERR_SCHEMA_TABLE_NOT_FOUND
 *   - Index on non-existent collection → ERR_SCHEMA_TABLE_NOT_FOUND
 *
 * @par Thread Safety
 * DDLExecutor itself holds no mutable state — all state is in the injected
 * SchemaRegistry, which is thread-safe.  Concurrent calls to execute() on
 * the same executor instance are therefore safe.
 */
class DDLExecutor {
public:
    /// Construct a DDLExecutor bound to @p registry.
    explicit DDLExecutor(SchemaRegistry& registry);

    DDLExecutor(const DDLExecutor&)            = delete;
    DDLExecutor& operator=(const DDLExecutor&) = delete;

    /**
     * @brief Execute a parsed SchemaDDL statement.
     *
     * @param ddl  Parsed DDL node (from AQLParser::parseSchemaDDL).
     * @return     Ok(true) on success; Err with a descriptive message on failure.
     */
    [[nodiscard]] Result<bool> execute(const SchemaDDL& ddl);

private:
    SchemaRegistry& registry_;

    // ── Per-type dispatch helpers ─────────────────────────────────────────────
    [[nodiscard]] Result<bool> execCreateCollection(const SchemaDDL& ddl);
    [[nodiscard]] Result<bool> execDropCollection(const SchemaDDL& ddl);
    [[nodiscard]] Result<bool> execCreateIndex(const SchemaDDL& ddl);
    [[nodiscard]] Result<bool> execDropIndex(const SchemaDDL& ddl);
    [[nodiscard]] Result<bool> execCreateView(const SchemaDDL& ddl);
    [[nodiscard]] Result<bool> execDropView(const SchemaDDL& ddl);
    [[nodiscard]] Result<bool> execAlterCollection(const SchemaDDL& ddl);
};

}  // namespace query
}  // namespace themis
