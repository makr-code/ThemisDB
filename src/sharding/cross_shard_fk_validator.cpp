// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/*
 * ThemisDB | File: cross_shard_fk_validator.cpp
 * Cross-Shard Foreign Key Referential Integrity Validator
 * Issue #5390 — Distributed FK validation in 2PC prepare phase
 */

#include "sharding/cross_shard_fk_validator.h"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace themisdb {
namespace sharding {

// ============================================================================
// CrossShardFKViolation
// ============================================================================

nlohmann::json CrossShardFKViolation::toJSON() const {
    return {
        {"constraint_name", constraint_name},
        {"child_table",     child_table},
        {"child_column",    child_column},
        {"parent_table",    parent_table},
        {"parent_column",   parent_column},
        {"fk_value",        fk_value},
        {"parent_shard_id", parent_shard_id},
        {"message",         message}
    };
}

// ============================================================================
// CrossShardForeignKeyValidator — constraint registration
// ============================================================================

void CrossShardForeignKeyValidator::registerConstraint(CrossShardFKConstraint constraint) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Replace any existing constraint with the same name.
    auto it = std::find_if(constraints_.begin(), constraints_.end(),
        [&constraint](const CrossShardFKConstraint& c) {
            return c.constraint_name == constraint.constraint_name;
        });
    if (it != constraints_.end()) {
        *it = std::move(constraint);
    } else {
        constraints_.push_back(std::move(constraint));
    }
}

void CrossShardForeignKeyValidator::removeConstraint(const std::string& constraint_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    constraints_.erase(
        std::remove_if(constraints_.begin(), constraints_.end(),
            [&constraint_name](const CrossShardFKConstraint& c) {
                return c.constraint_name == constraint_name;
            }),
        constraints_.end());
}

std::vector<CrossShardFKConstraint> CrossShardForeignKeyValidator::getConstraints() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return constraints_;
}

// ============================================================================
// Lookup callback injection
// ============================================================================

void CrossShardForeignKeyValidator::setParentKeyLookup(ParentKeyLookupFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    lookup_ = std::move(fn);
}

// ============================================================================
// Validation
// ============================================================================

std::optional<CrossShardFKViolation> CrossShardForeignKeyValidator::checkSingleConstraint(
    const CrossShardFKConstraint& constraint,
    const std::string& fk_value) const
{
    // Take a snapshot of the callback under the lock, then call it
    // *outside* the lock to avoid holding the mutex during network I/O.
    ParentKeyLookupFn cb;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cb = lookup_;
    }

    bool parent_exists = false;
    if (!cb) {
        // No lookup callback configured — fail-closed: treat as violation.
        spdlog::error(
            "CrossShardForeignKeyValidator: no ParentKeyLookupFn set; "
            "treating FK '{}' as violated (fail-closed)",
            constraint.constraint_name);
    } else {
        try {
            parent_exists = cb(constraint.parent_shard_id,
                               constraint.parent_table,
                               constraint.parent_column,
                               fk_value);
        } catch (const std::exception& ex) {
            spdlog::error(
                "CrossShardForeignKeyValidator: ParentKeyLookupFn threw for "
                "constraint '{}', shard '{}', value '{}': {} — treating as violation (fail-closed)",
                constraint.constraint_name, constraint.parent_shard_id,
                fk_value, ex.what());
            parent_exists = false;
        } catch (...) {
            spdlog::error(
                "CrossShardForeignKeyValidator: ParentKeyLookupFn threw unknown "
                "exception for constraint '{}' — treating as violation (fail-closed)",
                constraint.constraint_name);
            parent_exists = false;
        }
    }

    if (parent_exists) {
        return std::nullopt;  // Satisfied.
    }

    CrossShardFKViolation violation;
    violation.constraint_name  = constraint.constraint_name;
    violation.child_table      = constraint.child_table;
    violation.child_column     = constraint.child_column;
    violation.parent_table     = constraint.parent_table;
    violation.parent_column    = constraint.parent_column;
    violation.fk_value         = fk_value;
    violation.parent_shard_id  = constraint.parent_shard_id;
    violation.message =
        "Cross-shard FK violation: " + constraint.child_table + "." +
        constraint.child_column + " = '" + fk_value +
        "' has no matching row in " + constraint.parent_shard_id + "/" +
        constraint.parent_table + "." + constraint.parent_column;

    return violation;
}

std::vector<CrossShardFKViolation> CrossShardForeignKeyValidator::validateTransaction(
    const nlohmann::json& operations) const
{
    // Snapshot constraints under lock; validation itself runs outside.
    std::vector<CrossShardFKConstraint> local_constraints;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local_constraints = constraints_;
    }

    if (local_constraints.empty()) {
        return {};  // Nothing to check.
    }

    if (!operations.is_array()) {
        spdlog::warn("CrossShardForeignKeyValidator::validateTransaction: "
                     "operations is not a JSON array; skipping FK check");
        return {};
    }

    std::vector<CrossShardFKViolation> violations;

    for (const auto& op : operations) {
        // Only INSERT and UPDATE can introduce child-side FK values.
        if (!op.is_object()) continue;
        const std::string op_type =
            op.value("type", std::string{});
        if (op_type != "INSERT" && op_type != "UPDATE") continue;

        const std::string table_name = op.value("table", std::string{});
        if (table_name.empty()) continue;

        const auto& data = op.contains("data") ? op.at("data") : nlohmann::json{};
        if (!data.is_object()) continue;

        // Check each registered constraint against this operation.
        for (const auto& constraint : local_constraints) {
            if (constraint.child_table != table_name) continue;
            if (!data.contains(constraint.child_column)) continue;

            // Extract the FK value (support string and numeric).
            std::string fk_value;
            const auto& raw = data.at(constraint.child_column);
            if (raw.is_string()) {
                fk_value = raw.get<std::string>();
            } else if (raw.is_number_integer()) {
                fk_value = std::to_string(raw.get<int64_t>());
            } else if (raw.is_number()) {
                fk_value = std::to_string(raw.get<double>());
            } else if (raw.is_null()) {
                // NULL FK value — no referential check needed (nullable FK semantics).
                continue;
            } else {
                fk_value = raw.dump();
            }

            spdlog::debug(
                "CrossShardForeignKeyValidator: checking constraint '{}': "
                "{}.{} = '{}' against {}/{}:{}",
                constraint.constraint_name,
                constraint.child_table, constraint.child_column, fk_value,
                constraint.parent_shard_id, constraint.parent_table,
                constraint.parent_column);

            auto violation = checkSingleConstraint(constraint, fk_value);
            if (violation.has_value()) {
                spdlog::warn(
                    "CrossShardForeignKeyValidator: FK violation detected: {}",
                    violation->message);
                violations.push_back(std::move(*violation));
            }
        }
    }

    return violations;
}

} // namespace sharding
} // namespace themisdb
