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
    std::lock_guard<std::mutex> lk(mu_);
    return collections_.count(name) > 0;
}

bool SchemaRegistry::hasIndex(const std::string& collection,
                               const std::string& index_name) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = indexes_.find(collection);
    if (it == indexes_.end()) {
      return false;
    }
    return it->second.count(index_name) > 0;
}

bool SchemaRegistry::hasView(const std::string& name) const {
    std::lock_guard<std::mutex> lk(mu_);
    return views_.count(name) > 0;
}

nlohmann::json SchemaRegistry::collectionOptions(const std::string& name) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = collections_.find(name);
    if (it == collections_.end()) {
      return nlohmann::json::object();
    }
    return it->second;
}

std::vector<std::string> SchemaRegistry::collections() const {
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<std::string> result;
    result.reserve(collections_.size());
    for (const auto& kv : collections_) {
      result.push_back(kv.first);
    }
    return result;
}

std::vector<std::string> SchemaRegistry::views() const {
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<std::string> result;
    result.reserve(views_.size());
    for (const auto& kv : views_) {
      result.push_back(kv.first);
    }
    return result;
}

void SchemaRegistry::addCollection(const std::string& name, const nlohmann::json& options) {
    std::lock_guard<std::mutex> lk(mu_);
    collections_[name] = options;
    // Ensure the index sub-map exists even if empty
    indexes_.emplace(name, std::unordered_map<std::string, IndexDef>{});
}

void SchemaRegistry::dropCollection(const std::string& name) {
    std::lock_guard<std::mutex> lk(mu_);
    collections_.erase(name);
    indexes_.erase(name);
}

void SchemaRegistry::addIndex(const std::string& collection, const IndexDef& def) {
    std::lock_guard<std::mutex> lk(mu_);
    indexes_[collection][def.name] = def;
}

void SchemaRegistry::dropIndex(const std::string& collection,
                                const std::string& index_name) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = indexes_.find(collection);
    if (it != indexes_.end()) {
      it->second.erase(index_name);
    }
}

void SchemaRegistry::addView(const std::string& name, const std::string& body) {
    std::lock_guard<std::mutex> lk(mu_);
    views_[name] = body;
}

void SchemaRegistry::dropView(const std::string& name) {
    std::lock_guard<std::mutex> lk(mu_);
    views_.erase(name);
}

void SchemaRegistry::alterCollection(const std::string& name,
                                      const nlohmann::json& options) {
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

// ── per-type helpers ──────────────────────────────────────────────────────────

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

Result<bool> DDLExecutor::execAlterCollection(const SchemaDDL& ddl) {
    if (!registry_.hasCollection(ddl.name)) {
        return Err<bool>(
            errors::ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
            fmt::format("Cannot alter collection '{}': it does not exist", ddl.name));
    }
    registry_.alterCollection(ddl.name, ddl.options);
    return Ok(true);
}

// ── execute — main dispatch ───────────────────────────────────────────────────

Result<bool> DDLExecutor::execute(const SchemaDDL& ddl) {
    switch (ddl.ddl_type) {
        case SchemaDDLType::CREATE_COLLECTION: return execCreateCollection(ddl);
        case SchemaDDLType::DROP_COLLECTION:   return execDropCollection(ddl);
        case SchemaDDLType::CREATE_INDEX:      return execCreateIndex(ddl);
        case SchemaDDLType::DROP_INDEX:        return execDropIndex(ddl);
        case SchemaDDLType::CREATE_VIEW:       return execCreateView(ddl);
        case SchemaDDLType::DROP_VIEW:         return execDropView(ddl);
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
