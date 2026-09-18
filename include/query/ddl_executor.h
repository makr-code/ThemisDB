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

class SchemaRegistry {
public:
    SchemaRegistry()  = default;
    ~SchemaRegistry() = default;

    // Non-copyable, movable
    SchemaRegistry(const SchemaRegistry&)            = delete;
    SchemaRegistry& operator=(const SchemaRegistry&) = delete;
    SchemaRegistry(SchemaRegistry&&)                 noexcept = default;
    SchemaRegistry& operator=(SchemaRegistry&&)      noexcept = default;

    // ── Query ────────────────────────────────────────────────────────────────

    [[nodiscard]] bool hasCollection(const std::string& name) const;

    [[nodiscard]] bool hasIndex(const std::string& collection,
                                const std::string& index_name) const;

    [[nodiscard]] bool hasView(const std::string& name) const;

    [[nodiscard]] nlohmann::json collectionOptions(const std::string& name) const;

    [[nodiscard]] std::vector<std::string> collections() const;

    [[nodiscard]] std::vector<std::string> views() const;

    // ── Mutate ───────────────────────────────────────────────────────────────

    void addCollection(const std::string& name, const nlohmann::json& options = {});

    /**
     * @brief Drop Collection.
     * @param[in] name Input parameter.
     */
    void dropCollection(const std::string& name);

    /**
     * @brief Add Index.
     * @param[in] collection Input parameter.
     * @param[in] def Input parameter.
     */
    void addIndex(const std::string& collection, const IndexDef& def);

    /**
     * @brief Drop Index.
     * @param[in] collection Input parameter.
     * @param[in] index_name Name of the index.
     */
    void dropIndex(const std::string& collection, const std::string& index_name);

    /**
     * @brief Add View.
     * @param[in] name Input parameter.
     * @param[in] body Input parameter.
     */
    void addView(const std::string& name, const std::string& body);

    /**
     * @brief Drop View.
     * @param[in] name Input parameter.
     */
    void dropView(const std::string& name);

    /**
     * @brief Alter Collection.
     * @param[in] name Input parameter.
     * @param[in] options Input parameter.
     */
    void alterCollection(const std::string& name, const nlohmann::json& options);

private:
    mutable std::mutex mu_;

    std::unordered_map<std::string, nlohmann::json> collections_;

    std::unordered_map<std::string,
                       std::unordered_map<std::string, IndexDef>> indexes_;

    std::unordered_map<std::string, std::string> views_;
};

// ============================================================================
// DDLExecutor
// ============================================================================

class DDLExecutor {
public:
    /**
     * @brief DDLExecutor.
     * @param[in,out] registry Input/output parameter.
     * @return Return value.
     */
    explicit DDLExecutor(SchemaRegistry& registry);

    DDLExecutor(const DDLExecutor&)            = delete;
    DDLExecutor& operator=(const DDLExecutor&) = delete;

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
