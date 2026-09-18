/**
 * @file ddl_executor.cpp
 * @brief Implementation of SchemaRegistry and DDLExecutor for AQL Schema DDL Phase 2.
 *
 * Implements the full CREATE/DROP/ALTER semantics described in the DDL Phase 2
 * specification.  All registry operations are protected by a single mutex so that
 * concurrent DDL statements issued from multiple threads do not corrupt the catalog.
 *
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#include "query/ddl_executor.h"
#include "utils/error_registry.h"
#include <fmt/format.h>

namespace themis {
namespace query {

// ============================================================================
// SchemaRegistry — implementation
// ============================================================================

bool SchemaRegistry::hasCollection(const std::string& name) const {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    return collections_.count(name) > 0;
}

bool SchemaRegistry::hasIndex(const std::string& collection,
                               const std::string& index_name) const {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    auto it = indexes_.find(collection);
    if (it == indexes_.end()) {
      return false;
    }
    return it->second.count(index_name) > 0;
}

bool SchemaRegistry::hasView(const std::string& name) const {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    return views_.count(name) > 0;
}

nlohmann::json SchemaRegistry::collectionOptions(const std::string& name) const {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    auto it = collections_.find(name);
    if (it == collections_.end()) {
      return nlohmann::json::object();
    }
    return it->second;
}

std::vector<std::string> SchemaRegistry::collections() const {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<std::string> result = {};

    result.reserve(collections_.size());
    for (const auto& kv : collections_) {
      result.push_back(kv.first);
    }
    return result;
}

std::vector<std::string> SchemaRegistry::views() const {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<std::string> result = {};

    result.reserve(views_.size());
    for (const auto& kv : views_) {
      result.push_back(kv.first);
    }
    return result;
}

/**
 * @brief Add Collection.
 * @param[in] name Input parameter.
 * @param[in] options Input parameter.
 * @details Calls: lk(), emplace().
 */
void SchemaRegistry::addCollection(const std::string& name, const nlohmann::json& options) {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    collections_[name] = options;
    // Ensure the index sub-map exists even if empty
    indexes_.emplace(name, std::unordered_map<std::string, IndexDef>{});
}

/**
 * @brief Drop Collection.
 * @param[in] name Input parameter.
 * @details Calls: lk(), erase().
 */
void SchemaRegistry::dropCollection(const std::string& name) {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    collections_.erase(name);
    indexes_.erase(name);
}

/**
 * @brief Add Index.
 * @param[in] collection Input parameter.
 * @param[in] def Input parameter.
 * @details Calls: lk().
 */
void SchemaRegistry::addIndex(const std::string& collection, const IndexDef& def) {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    indexes_[collection][def.name] = def;
}

/**
 * @brief Drop Index.
 * @param[in] collection Input parameter.
 * @param[in] index_name Input parameter.
 * @details Calls: lk(), find(), end(), erase().
 */
void SchemaRegistry::dropIndex(const std::string& collection,
                                const std::string& index_name) {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    auto it = indexes_.find(collection);
    if (it != indexes_.end()) {
      it->second.erase(index_name);
    }
}

/**
 * @brief Add View.
 * @param[in] name Input parameter.
 * @param[in] body Input parameter.
 * @details Calls: lk().
 */
void SchemaRegistry::addView(const std::string& name, const std::string& body) {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    views_[name] = body;
}

/**
 * @brief Drop View.
 * @param[in] name Input parameter.
 * @details Calls: lk(), erase().
 */
void SchemaRegistry::dropView(const std::string& name) {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    views_.erase(name);
}

/**
 * @brief Alter Collection.
 * @param[in] name Input parameter.
 * @param[in] options Input parameter.
 * @details Calls: lk(), find(), end(), is_object(), items().
 */
void SchemaRegistry::alterCollection(const std::string& name,
                                      const nlohmann::json& options) {
    /**
     * @brief Lk.
     * @param[in] mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(mu_);
    auto it = collections_.find(name);
    if (it != collections_.end()) {
        // Merge: new keys overwrite, existing keys not mentioned are kept.
        if (options.is_object() && it->second.is_object()) {
            for (auto& [key, val] : options.items()) {
                it->second[key] = val;
            }
        } else {
            it->second = options; // replace wholesale for non-object types
        }
    }
}

// ============================================================================
// DDLExecutor — implementation
// ============================================================================

DDLExecutor::DDLExecutor(SchemaRegistry& registry)
    : registry_(registry) {}

/**
 * @brief ── per-type helpers ──────────────────────────────────────────────────────────
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: hasCollection(), Ok(), fmt::format(), addCollection().
 */

Result<bool> DDLExecutor::execCreateCollection(const SchemaDDL& ddl) {
    if (registry_.hasCollection(ddl.name)) {
        if (ddl.if_exists) {
            // IF NOT EXISTS — idempotent, not an error
            return Ok(true);
        }
        return Err<bool>(
            errors::ErrorCode::ERR_DOC_ALREADY_EXISTS,
            fmt::format("Collection '{}' already exists", ddl.name));
    }
    registry_.addCollection(ddl.name, ddl.options);
    return Ok(true);
}

/**
 * @brief Exec Drop Collection.
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: hasCollection(), Ok(), fmt::format(), dropCollection().
 */
Result<bool> DDLExecutor::execDropCollection(const SchemaDDL& ddl) {
    if (!registry_.hasCollection(ddl.name)) {
        if (ddl.if_exists) {
            // IF EXISTS — idempotent, not an error
            return Ok(true);
        }
        return Err<bool>(
            errors::ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
            fmt::format("Collection '{}' does not exist", ddl.name));
    }
    registry_.dropCollection(ddl.name);
    return Ok(true);
}

/**
 * @brief Exec Create Index.
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: hasCollection(), fmt::format(), hasIndex(), Ok(), addIndex().
 */
Result<bool> DDLExecutor::execCreateIndex(const SchemaDDL& ddl) {
    // The target collection must already exist
    if (!registry_.hasCollection(ddl.collection)) {
        return Err<bool>(
            errors::ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
            fmt::format("Cannot create index '{}': collection '{}' does not exist",
                        ddl.name, ddl.collection));
    }

    if (registry_.hasIndex(ddl.collection, ddl.name)) {
        if (ddl.if_exists) {
            return Ok(true);
        }
        return Err<bool>(
            errors::ErrorCode::ERR_DOC_ALREADY_EXISTS,
            fmt::format("Index '{}' on collection '{}' already exists",
                        ddl.name, ddl.collection));
    }

    registry_.addIndex(ddl.collection, ddl.index_def);
    return Ok(true);
}

/**
 * @brief Exec Drop Index.
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: hasCollection(), Ok(), fmt::format(), hasIndex(), dropIndex().
 */
Result<bool> DDLExecutor::execDropIndex(const SchemaDDL& ddl) {
    if (!registry_.hasCollection(ddl.collection)) {
        if (ddl.if_exists) {
          return Ok(true);
        }
        return Err<bool>(
            errors::ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
            fmt::format("Cannot drop index '{}': collection '{}' does not exist",
                        ddl.name, ddl.collection));
    }

    if (!registry_.hasIndex(ddl.collection, ddl.name)) {
        if (ddl.if_exists) {
          return Ok(true);
        }
        return Err<bool>(
            errors::ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
            fmt::format("Index '{}' on collection '{}' does not exist",
                        ddl.name, ddl.collection));
    }

    registry_.dropIndex(ddl.collection, ddl.name);
    return Ok(true);
}

/**
 * @brief Exec Create View.
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: hasView(), Ok(), fmt::format(), addView().
 */
Result<bool> DDLExecutor::execCreateView(const SchemaDDL& ddl) {
    if (registry_.hasView(ddl.name)) {
        if (ddl.if_exists) {
          return Ok(true);
        }
        return Err<bool>(
            errors::ErrorCode::ERR_DOC_ALREADY_EXISTS,
            fmt::format("View '{}' already exists", ddl.name));
    }
    registry_.addView(ddl.name, ddl.view_body);
    return Ok(true);
}

/**
 * @brief Exec Drop View.
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: hasView(), Ok(), fmt::format(), dropView().
 */
Result<bool> DDLExecutor::execDropView(const SchemaDDL& ddl) {
    if (!registry_.hasView(ddl.name)) {
        if (ddl.if_exists) {
          return Ok(true);
        }
        return Err<bool>(
            errors::ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
            fmt::format("View '{}' does not exist", ddl.name));
    }
    registry_.dropView(ddl.name);
    return Ok(true);
}

/**
 * @brief Exec Alter Collection.
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: hasCollection(), fmt::format(), alterCollection(), Ok().
 */
Result<bool> DDLExecutor::execAlterCollection(const SchemaDDL& ddl) {
    if (!registry_.hasCollection(ddl.name)) {
        return Err<bool>(
            errors::ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
            fmt::format("Cannot alter collection '{}': it does not exist", ddl.name));
    }
    registry_.alterCollection(ddl.name, ddl.options);
    return Ok(true);
}

/**
 * @brief ── execute — main dispatch ───────────────────────────────────────────────────
 * @param[in] ddl Input parameter.
 * @return Return value.
 * @details Calls: execCreateCollection(), execDropCollection(), execCreateIndex(), execDropIndex(), execCreateView(), execDropView(), execAlterCollection(), fmt::format().
 */

Result<bool> DDLExecutor::execute(const SchemaDDL& ddl) {
    switch (ddl.ddl_type) {
        /**
         * @brief Exec Create Collection.
         * @param[in] ddl Input parameter.
         * @return Return value.
         */
        case SchemaDDLType::CREATE_COLLECTION: return execCreateCollection(ddl);
        /**
         * @brief Exec Drop Collection.
         * @param[in] ddl Input parameter.
         * @return Return value.
         */
        case SchemaDDLType::DROP_COLLECTION:   return execDropCollection(ddl);
        /**
         * @brief Exec Create Index.
         * @param[in] ddl Input parameter.
         * @return Return value.
         */
        case SchemaDDLType::CREATE_INDEX:      return execCreateIndex(ddl);
        /**
         * @brief Exec Drop Index.
         * @param[in] ddl Input parameter.
         * @return Return value.
         */
        case SchemaDDLType::DROP_INDEX:        return execDropIndex(ddl);
        /**
         * @brief Exec Create View.
         * @param[in] ddl Input parameter.
         * @return Return value.
         */
        case SchemaDDLType::CREATE_VIEW:       return execCreateView(ddl);
        /**
         * @brief Exec Drop View.
         * @param[in] ddl Input parameter.
         * @return Return value.
         */
        case SchemaDDLType::DROP_VIEW:         return execDropView(ddl);
        /**
         * @brief Exec Alter Collection.
         * @param[in] ddl Input parameter.
         * @return Return value.
         */
        case SchemaDDLType::ALTER_COLLECTION:  return execAlterCollection(ddl);
    }
    // Unreachable — enum is exhaustive, but keeps -Wreturn-type happy.
    return Err<bool>(
        errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
        fmt::format("Unknown SchemaDDLType value ({})",
                    static_cast<int>(ddl.ddl_type)));
}

}  // namespace query
}  // namespace themis
