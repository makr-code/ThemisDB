/**
 * @file cross_shard_fk_validator.cpp
 * @brief Cross-shard foreign-key constraint validator implementation.
 *
 * Implements distributed foreign-key validation that spans shard
 * boundaries, issuing parallel lookups and merging results.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/cross_shard_fk_validator.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themisdb {
namespace sharding {

namespace {

/**
 * @brief Attempt to parse @p op_str as a JSON operation object.
 *
 * Returns an empty optional when the string is not valid JSON or does not
 * contain the mandatory "op" and "table" fields.  Parsing failures are
 * intentionally silent (trace-level only) so that non-JSON operation strings
 * from other protocols are ignored without aborting validation.
 */
std::optional<nlohmann::json> tryParseOperation(const std::string& op_str) {
    try {
        auto j = nlohmann::json::parse(op_str);
        if (!j.contains("op") || !j.contains("table")) {
            return std::nullopt;
        }
        return j;
    } catch (const nlohmann::json::parse_error&) {
        // Not a JSON operation string — silently skip.
        return std::nullopt;
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// FKViolation
// ---------------------------------------------------------------------------

std::string FKViolation::toJson() const {
    nlohmann::json j = {
        {"constraint_name", constraint_name},
        {"table",           table},
        {"column",          column},
        {"key_value",       key_value},
        {"message",         message},
        {"deferrable",      deferrable}
    };
    return j.dump();
}

// ---------------------------------------------------------------------------
// CrossShardForeignKeyValidator – public API
// ---------------------------------------------------------------------------

void CrossShardForeignKeyValidator::registerConstraint(
    const CrossShardFKConstraint& constraint)
{
    std::lock_guard<std::mutex> lk(mutex_);
    // Replace if a constraint with the same name already exists.
    auto it = std::find_if(
        constraints_.begin(), constraints_.end(),
        [&](const CrossShardFKConstraint& c) { return c.name == constraint.name; });
    if (it != constraints_.end()) {
        *it = constraint;
        spdlog::debug("CrossShardFKValidator: replaced constraint '{}'", constraint.name);
    } else {
        constraints_.push_back(constraint);
        spdlog::debug("CrossShardFKValidator: registered constraint '{}'", constraint.name);
    }
}

void CrossShardForeignKeyValidator::removeConstraint(
    const std::string& constraint_name)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto before = constraints_.size();
    constraints_.erase(
        std::remove_if(constraints_.begin(), constraints_.end(),
            [&](const CrossShardFKConstraint& c) { return c.name == constraint_name; }),
        constraints_.end());
    if (constraints_.size() < before) {
        spdlog::debug("CrossShardFKValidator: removed constraint '{}'", constraint_name);
    }
}

void CrossShardForeignKeyValidator::setKeyExistsCallback([[maybe_unused]] KeyExistsCallback cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    key_exists_cb_ = std::move(cb);
}

void CrossShardForeignKeyValidator::setChildExistsCallback([[maybe_unused]] ChildExistsCallback cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    child_exists_cb_ = std::move(cb);
}

void CrossShardForeignKeyValidator::setAllShardIds(std::vector<std::string> shard_ids) {
    std::lock_guard<std::mutex> lk(mutex_);
    all_shard_ids_ = std::move(shard_ids);
}

std::size_t CrossShardForeignKeyValidator::constraintCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return constraints_.size();
}

// ---------------------------------------------------------------------------
// validate()
// ---------------------------------------------------------------------------

std::vector<FKViolation> CrossShardForeignKeyValidator::validate(
    const std::string& transaction_id,
    const std::map<std::string, std::vector<std::string>>& shard_operations
) const {
    // Snapshot callbacks and constraints under the lock so we release it
    // before any potentially blocking shard I/O.
    std::vector<CrossShardFKConstraint> local_constraints;
    KeyExistsCallback  key_cb;
    ChildExistsCallback child_cb;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        local_constraints = constraints_;
        key_cb   = key_exists_cb_;
        child_cb = child_exists_cb_;
    }

    if (local_constraints.empty()) {
        return {};
    }

    std::vector<FKViolation> violations;

    // Build an index: child_table -> [constraints checking INSERT/UPDATE]
    //                 parent_table -> [constraints checking DELETE]
    // so we scan each operation only once.

    for (const auto& [shard_id, ops] : shard_operations) {
        for (const auto& op_str : ops) {
            auto maybe_j = tryParseOperation(op_str);
            if (!maybe_j) {
                continue;
            }
            const auto& j = *maybe_j;

            const std::string op    = j["op"].get<std::string>();
            const std::string table = j["table"].get<std::string>();

            if (op == "INSERT" || op == "UPDATE") {
                // Check every FK constraint whose child_table matches.
                if (!j.contains("data") || !j["data"].is_object()) {
                    continue;
                }
                const auto& data = j["data"];

                for (const auto& c : local_constraints) {
                    if (c.child_table != table) {
                        continue;
                    }
                    if (!data.contains(c.child_column)) {
                        continue; // column not in this operation
                    }

                    // Null / JSON-null FK values are treated as no constraint
                    // (NULL semantics: a NULL FK does not require a parent row).
                    if (data[c.child_column].is_null()) {
                        continue;
                    }

                    const std::string fk_value =
                        data[c.child_column].get<std::string>();

                    if (fk_value.empty()) {
                        continue;
                    }

                    auto violation = checkParentExists(c, fk_value, transaction_id);
                    if (violation) {
                        violations.push_back(std::move(*violation));
                    }
                }

            } else if (op == "DELETE") {
                // Check every FK constraint whose parent_table matches.
                // We need the parent key being deleted to fan out child checks.
                if (!j.contains("key")) {
                    continue;
                }
                const std::string parent_value = j["key"].get<std::string>();
                if (parent_value.empty()) {
                    continue;
                }

                for (const auto& c : local_constraints) {
                    if (c.parent_table != table) {
                        continue;
                    }

                    // Skip if the key column is specified and doesn't match
                    // the parent_column of this constraint.
                    if (j.contains("key_column")) {
                        const std::string key_col = j["key_column"].get<std::string>();
                        if (!key_col.empty() && key_col != c.parent_column) {
                            continue;
                        }
                    }

                    auto violation = checkNoChildExists(c, parent_value, transaction_id);
                    if (violation) {
                        violations.push_back(std::move(*violation));
                    }
                }
            }
            // UPDATE on parent (key change) could also be handled here in a
            // future extension; currently treated as an ordinary UPDATE.
        }
    }

    if (!violations.empty()) {
        std::size_t non_deferrable = 0;
        for (const auto& v : violations) {
            if (!v.deferrable) {
                ++non_deferrable;
            }
        }
        spdlog::warn(
            "CrossShardFKValidator: txn {} has {} FK violation(s) "
            "({} non-deferrable)",
            transaction_id, violations.size(), non_deferrable);
    }

    return violations;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::optional<FKViolation> CrossShardForeignKeyValidator::checkParentExists(
    const CrossShardFKConstraint& constraint,
    const std::string& fk_value,
    const std::string& txn_id
) const {
    // Snapshot state needed for the check (already called without mutex).
    KeyExistsCallback  key_cb;
    std::vector<std::string> shards;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        key_cb = key_exists_cb_;
        if (!constraint.parent_shard_id.empty()) {
            shards = {constraint.parent_shard_id};
        } else {
            shards = all_shard_ids_;
        }
    }

    if (!key_cb) {
        // No callback injected — cannot perform the lookup.
        // Fail-closed: report a violation.
        spdlog::error(
            "CrossShardFKValidator: no KeyExistsCallback set; "
            "treating '{}' FK value '{}' as missing parent (txn {})",
            constraint.name, fk_value, txn_id);
        FKViolation v;
        v.constraint_name = constraint.name;
        v.table           = constraint.child_table;
        v.column          = constraint.child_column;
        v.key_value       = fk_value;
        v.deferrable      = constraint.deferrable;
        v.message         = "No KeyExistsCallback configured; cannot verify parent key '" +
                            fk_value + "' in " + constraint.parent_table +
                            "." + constraint.parent_column;
        return v;
    }

    // Fan out to all relevant shards — parent key is present if any shard
    // returns true.
    bool parent_found = false;
    for (const auto& shard_id : shards) {
        try {
            if (key_cb(shard_id, constraint.parent_table,
                       constraint.parent_column, fk_value)) {
                parent_found = true;
                break;
            }
        } catch (const std::exception& ex) {
            spdlog::error(
                "CrossShardFKValidator: KeyExistsCallback threw for shard '{}' "
                "constraint '{}' value '{}' (txn {}): {}",
                shard_id, constraint.name, fk_value, txn_id, ex.what());
            // Treat callback exception as shard-unreachable → fail-closed.
        }
    }

    if (parent_found) {
        return std::nullopt; // constraint satisfied
    }

    FKViolation v;
    v.constraint_name = constraint.name;
    v.table           = constraint.child_table;
    v.column          = constraint.child_column;
    v.key_value       = fk_value;
    v.deferrable      = constraint.deferrable;
    v.message         = "FK constraint '" + constraint.name +
                        "': parent key '" + fk_value +
                        "' not found in " + constraint.parent_table +
                        "." + constraint.parent_column +
                        " on any queried shard";
    spdlog::warn(
        "CrossShardFKValidator: violation — {} (txn {})",
        v.message, txn_id);
    return v;
}

std::optional<FKViolation> CrossShardForeignKeyValidator::checkNoChildExists(
    const CrossShardFKConstraint& constraint,
    const std::string& parent_value,
    const std::string& txn_id
) const {
    ChildExistsCallback child_cb;
    std::vector<std::string> shards;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        child_cb = child_exists_cb_;
        shards   = all_shard_ids_;
    }

    if (!child_cb) {
        // No callback — fail-closed: assume a child might exist.
        spdlog::error(
            "CrossShardFKValidator: no ChildExistsCallback set; "
            "treating delete of '{}' as having dependent children (txn {})",
            parent_value, txn_id);
        FKViolation v;
        v.constraint_name = constraint.name;
        v.table           = constraint.parent_table;
        v.column          = constraint.parent_column;
        v.key_value       = parent_value;
        v.deferrable      = constraint.deferrable;
        v.message         = "No ChildExistsCallback configured; cannot verify "
                            "absence of children in " + constraint.child_table +
                            "." + constraint.child_column +
                            " for parent key '" + parent_value + "'";
        return v;
    }

    // Fan out: if any shard has a child row the DELETE is blocked (RESTRICT).
    for (const auto& shard_id : shards) {
        try {
            if (child_cb(shard_id, constraint.child_table,
                         constraint.child_column, parent_value)) {
                FKViolation v;
                v.constraint_name = constraint.name;
                v.table           = constraint.parent_table;
                v.column          = constraint.parent_column;
                v.key_value       = parent_value;
                v.deferrable      = constraint.deferrable;
                v.message         = "FK constraint '" + constraint.name +
                                    "' (RESTRICT): child rows referencing '" +
                                    parent_value + "' exist in " +
                                    constraint.child_table + "." +
                                    constraint.child_column +
                                    " on shard '" + shard_id + "'";
                spdlog::warn(
                    "CrossShardFKValidator: violation — {} (txn {})",
                    v.message, txn_id);
                return v;
            }
        } catch (const std::exception& ex) {
            spdlog::error(
                "CrossShardFKValidator: ChildExistsCallback threw for shard '{}' "
                "constraint '{}' parent '{}' (txn {}): {}",
                shard_id, constraint.name, parent_value, txn_id, ex.what());
            // Treat exception as potential child-existence — fail-closed.
            FKViolation v;
            v.constraint_name = constraint.name;
            v.table           = constraint.parent_table;
            v.column          = constraint.parent_column;
            v.key_value       = parent_value;
            v.deferrable      = constraint.deferrable;
            v.message         = "FK constraint '" + constraint.name +
                                "': child-existence check failed on shard '" +
                                shard_id + "': " + ex.what();
            return v;
        }
    }

    return std::nullopt; // no children found — constraint satisfied
}

} // namespace sharding
} // namespace themisdb
