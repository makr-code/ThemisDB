// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

// ============================================================================
// Cross-Shard Foreign Key Referential Integrity Validator
// Issue #5390 — Closes the gap: FK constraints only enforced locally.
// ============================================================================

/**
 * @brief Describes a cross-shard foreign key constraint.
 *
 * A cross-shard FK links a child column on one shard to a parent column that
 * may reside on a *different* shard.  Because each shard only knows its own
 * rows, the coordinator must perform an explicit parent-key lookup on the
 * remote shard during the 2PC prepare phase to detect violations before any
 * data is durably written.
 *
 * Fields:
 *  - constraint_name   Human-readable constraint identifier.
 *  - child_table       Table that holds the referencing column.
 *  - child_column      Column that carries the foreign-key value.
 *  - parent_table      Referenced table (may be on a different shard).
 *  - parent_column     Referenced column (must be unique/PK on parent side).
 *  - parent_shard_id   Shard that owns @p parent_table; used to route the
 *                      existence check.
 */
struct CrossShardFKConstraint {
    std::string constraint_name;  ///< Unique constraint identifier.
    std::string child_table;      ///< Referencing table.
    std::string child_column;     ///< Referencing column.
    std::string parent_table;     ///< Referenced (parent) table.
    std::string parent_column;    ///< Referenced (parent) column (PK / unique).
    std::string parent_shard_id;  ///< Shard that hosts the parent table.
};

/**
 * @brief Describes a single cross-shard foreign key violation.
 *
 * Returned by CrossShardForeignKeyValidator::validateTransaction() when at
 * least one child row references a non-existent parent row.
 */
struct CrossShardFKViolation {
    std::string constraint_name;  ///< Constraint that was violated.
    std::string child_table;      ///< Referencing table.
    std::string child_column;     ///< Referencing column.
    std::string parent_table;     ///< Referenced table.
    std::string parent_column;    ///< Referenced column.
    std::string fk_value;         ///< The FK value that had no parent row.
    std::string parent_shard_id;  ///< Shard that was queried for the parent.
    std::string message;          ///< Human-readable violation description.

    /**
     * @brief Serialise the violation to JSON for logging and audit trails.
     */
    nlohmann::json toJSON() const;
};

/**
 * @brief Cross-Shard Foreign Key Validator — enforces referential integrity
 *        at 2PC prepare time across shard boundaries.
 *
 * ## Motivation
 *
 * ThemisDB's per-shard schema engine (SchemaConstraints) can only verify FK
 * references within the same shard.  When parent and child rows live on
 * different shards, orphaned child records are possible unless the
 * coordinator validates the constraint before committing.
 *
 * ## Integration point
 *
 * CrossShardTransactionCoordinator::prepare() calls
 * CrossShardForeignKeyValidator::validateTransaction() **before** sending
 * PREPARE RPCs to participants.  If any violation is detected the prepare
 * phase fails immediately, the transaction is aborted, and no participant
 * ever reaches the PREPARED state — preserving ACID invariants.
 *
 * ## Parent-key lookup
 *
 * The validator delegates actual cross-shard row existence checks to an
 * injected ParentKeyLookupFn callback.  This keeps the validator testable
 * without a live network stack and allows callers to supply a real gRPC /
 * HTTP lookup in production.
 *
 * Signature of ParentKeyLookupFn:
 * @code
 *   bool lookup(const std::string& shard_id,
 *               const std::string& parent_table,
 *               const std::string& parent_column,
 *               const std::string& fk_value);
 * @endcode
 * Returns true when the parent row exists, false otherwise.  On network
 * error or timeout, the callback MUST return false (fail-closed).
 *
 * ## Thread-safety
 *
 * All public methods are thread-safe; an internal mutex protects constraint
 * registration and the lookup callback.
 *
 * ## Usage example
 * @code
 *   auto fkv = std::make_shared<CrossShardForeignKeyValidator>();
 *
 *   CrossShardFKConstraint fk;
 *   fk.constraint_name = "fk_orders_user_id";
 *   fk.child_table     = "orders";
 *   fk.child_column    = "user_id";
 *   fk.parent_table    = "users";
 *   fk.parent_column   = "id";
 *   fk.parent_shard_id = "shard_users";
 *   fkv->registerConstraint(std::move(fk));
 *
 *   fkv->setParentKeyLookup([](auto& sid, auto& tbl, auto& col, auto& val) {
 *       return my_rpc_client.rowExists(sid, tbl, col, val);
 *   });
 *
 *   coordinator.setForeignKeyValidator(fkv);
 * @endcode
 */
class CrossShardForeignKeyValidator {
public:
    /**
     * @brief Callback type for parent-key existence check.
     *
     * @param shard_id      Shard to query.
     * @param parent_table  Table that should contain the parent row.
     * @param parent_column Column that must match @p fk_value.
     * @param fk_value      The foreign-key value to look up.
     * @return true  if the parent row exists and the constraint is satisfied.
     * @return false if the row is absent, the shard is unreachable, or any
     *               error occurs (fail-closed).
     */
    using ParentKeyLookupFn = std::function<bool(const std::string& /*shard_id*/,
                                                  const std::string& /*parent_table*/,
                                                  const std::string& /*parent_column*/,
                                                  const std::string& /*fk_value*/)>;

    CrossShardForeignKeyValidator() = default;
    ~CrossShardForeignKeyValidator() = default;

    // Non-copyable, movable.
    CrossShardForeignKeyValidator(const CrossShardForeignKeyValidator&) = delete;
    CrossShardForeignKeyValidator& operator=(const CrossShardForeignKeyValidator&) = delete;
    CrossShardForeignKeyValidator(CrossShardForeignKeyValidator&&) = default;
    CrossShardForeignKeyValidator& operator=(CrossShardForeignKeyValidator&&) = default;

    // ========================================================================
    // Constraint registration
    // ========================================================================

    /**
     * @brief Register a cross-shard FK constraint.
     *
     * Constraints are matched against write operations by (child_table,
     * child_column).  Registering a second constraint with the same
     * constraint_name replaces the earlier one.
     *
     * @param constraint  Fully-specified FK constraint descriptor.
     */
    void registerConstraint(CrossShardFKConstraint constraint);

    /**
     * @brief Remove a previously registered constraint by name.
     *
     * A no-op if the constraint does not exist.
     *
     * @param constraint_name  Name of the constraint to remove.
     */
    void removeConstraint(const std::string& constraint_name);

    /**
     * @brief Return all currently registered constraints.
     */
    std::vector<CrossShardFKConstraint> getConstraints() const;

    // ========================================================================
    // Lookup callback
    // ========================================================================

    /**
     * @brief Inject the parent-key existence callback.
     *
     * Must be set before calling validateTransaction().  If not set (or set
     * to nullptr), validateTransaction() returns an error for every
     * registered constraint (fail-closed).
     *
     * @param fn  Callable with ParentKeyLookupFn signature; must not throw.
     */
    void setParentKeyLookup(ParentKeyLookupFn fn);

    // ========================================================================
    // Validation
    // ========================================================================

    /**
     * @brief Validate all registered FK constraints for the given operations.
     *
     * Scans @p operations for INSERT/UPDATE writes that touch a child column
     * covered by a registered FK constraint, then verifies each referenced
     * parent key exists on the designated parent shard.
     *
     * This method is called by CrossShardTransactionCoordinator::prepare()
     * *before* sending PREPARE RPCs to participants.  A non-empty return
     * value causes prepare() to fail immediately (no PREPARE is sent).
     *
     * @param operations  JSON array of operation descriptors.  Each element
     *                    must contain at least:
     *                    - "type"  : "INSERT" | "UPDATE" | "DELETE"
     *                    - "table" : child table name (string)
     *                    - "data"  : object mapping column → value (strings)
     *
     * @return  Vector of violations; empty when all constraints are satisfied.
     *          On lookup callback error, a synthetic violation is returned
     *          (fail-closed).
     */
    std::vector<CrossShardFKViolation> validateTransaction(
        const nlohmann::json& operations) const;

    /**
     * @brief Check a single FK value against the parent shard.
     *
     * Convenience wrapper used by validateTransaction() and directly in tests.
     *
     * @param constraint  FK constraint to evaluate.
     * @param fk_value    The child column value to look up.
     * @return nullopt when the parent row exists (satisfied).
     * @return CrossShardFKViolation describing the violation otherwise.
     */
    std::optional<CrossShardFKViolation> checkSingleConstraint(
        const CrossShardFKConstraint& constraint,
        const std::string& fk_value) const;

private:
    mutable std::mutex mutex_;                         ///< Protects constraints_ and lookup_.
    std::vector<CrossShardFKConstraint> constraints_;  ///< Registered constraints.
    ParentKeyLookupFn lookup_;                         ///< Injected existence callback.
};

} // namespace sharding
} // namespace themisdb
