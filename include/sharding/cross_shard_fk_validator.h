// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <mutex>

/**
 * @file cross_shard_fk_validator.h
 * @brief Cross-shard foreign-key constraint validation for ThemisDB.
 *
 * This module implements the Two-Phase Check for distributed referential
 * integrity (issue #5392).  During the 2PC prepare phase, the coordinator
 * invokes CrossShardForeignKeyValidator::validate() which:
 *
 *  - For **INSERT / UPDATE** on a child table: confirms the referenced parent
 *    key exists on the designated parent shard.
 *  - For **DELETE / UPDATE** on a parent table: confirms no child shard holds
 *    a referencing row (RESTRICT semantics).
 *
 * Operations are supplied as JSON strings (one per participant shard) using
 * the format:
 * @code
 *   {"op":"INSERT","table":"orders","data":{"user_id":"u1"}}
 *   {"op":"DELETE","table":"users","key":"u1"}
 * @endcode
 *
 * Callers inject shard-lookup callbacks via setKeyExistsCallback() and
 * setChildExistsCallback() so the validator remains transport-agnostic and
 * fully testable without live shards.
 *
 * ### Trade-off notes
 * - Synchronous validation inside prepare() increases commit latency by one
 *   round-trip per FK constraint per shard.  For latency-sensitive workloads
 *   use `deferrable = true` on constraints; violations will be reported
 *   post-commit as eventual-consistency findings only.
 * - Temporary inconsistencies are still possible after shard failover if
 *   the callback cannot reach the parent shard (the validator aborts the
 *   transaction in that case unless `deferrable = true`).
 */

namespace themisdb {
namespace sharding {

// ---------------------------------------------------------------------------
// CrossShardFKConstraint
// ---------------------------------------------------------------------------

/**
 * @brief Defines a single cross-shard foreign-key constraint.
 *
 * A constraint links a *child* column (the FK) to a *parent* column (the
 * referenced key, usually the primary key) and carries the enforcement policy.
 */
struct CrossShardFKConstraint {
    /// Human-readable constraint name used in violation messages.
    std::string name;

    /// Table that owns the foreign-key column.
    std::string child_table = {};

    /// Column in @c child_table that holds the foreign-key value.
    std::string child_column;

    /// Table that is referenced (parent / owner of the key).
    std::string parent_table = {};

    /// Column in @c parent_table that is referenced (typically the PK).
    std::string parent_column;

    /**
     * @brief Optional shard-routing hint for the parent table.
     *
     * When non-empty the validator queries only this shard for parent-key
     * existence checks.  When empty, the validator queries every shard
     * registered via CrossShardForeignKeyValidator::setAllShardIds().
     */
    std::string parent_shard_id;

    /**
     * @brief When true the constraint is checked only in eventual-consistency
     *        mode (post-commit scan) and does **not** block the prepare phase.
     *
     * Use for cross-region or high-latency parent shards where a synchronous
     * round-trip during prepare is unacceptable.  Violations are still
     * reported via CrossShardForeignKeyValidator::validate() but the caller
     * must decide whether to abort or accept eventual consistency.
     */
    bool deferrable = false;
};

// ---------------------------------------------------------------------------
// FKViolation
// ---------------------------------------------------------------------------

/**
 * @brief Describes a single foreign-key constraint violation.
 *
 * Returned by CrossShardForeignKeyValidator::validate() for every constraint
 * that cannot be satisfied.
 */
struct FKViolation {
    /// Name of the violated constraint (from CrossShardFKConstraint::name).
    std::string constraint_name;

    /// Table that issued the violating write or delete.
    std::string table;

    /// Column involved in the violation.
    std::string column;

    /// FK value or parent key that was dangling / still referenced.
    std::string key_value;

    /// Human-readable description of the violation.
    std::string message;

    /**
     * @brief Whether the violated constraint is deferrable.
     *
     * When true the coordinator may choose to accept eventual consistency
     * rather than aborting the transaction.
     */
    bool deferrable = false;

    /// @brief Serialise this violation to a JSON-compatible string.
    [[nodiscard]] std::string toJson() const;
};

// ---------------------------------------------------------------------------
// CrossShardForeignKeyValidator
// ---------------------------------------------------------------------------

/**
 * @brief Validates cross-shard foreign-key constraints during 2PC prepare.
 *
 * ### Lifecycle
 * 1. Register FK constraints with registerConstraint().
 * 2. Inject lookup callbacks via setKeyExistsCallback() and
 *    setChildExistsCallback().
 * 3. Register all shard IDs via setAllShardIds() (used when no parent-shard
 *    hint is given on a constraint).
 * 4. The CrossShardTransactionCoordinator calls validate() from inside
 *    prepare() before dispatching prepare RPCs to participants.
 *
 * ### Thread safety
 * All public methods are thread-safe.  The validator is safe to share across
 * concurrent transaction threads.
 */
class CrossShardForeignKeyValidator {
public:
    /**
     * @brief Callback to test whether a specific key exists in a shard table.
     *
     * @param shard_id   Shard to query.
     * @param table      Table name.
     * @param column     Column to match.
     * @param key_value  Value to look up.
     * @return true  → the row exists (parent present / child not cascadable);
     *         false → the key is absent.
     *
     * The callback must not throw.  If the shard is unreachable the callback
     * should return false; the validator treats an absent parent as a
     * constraint violation.
     */
    using KeyExistsCallback = std::function<bool(
        const std::string& shard_id,
        const std::string& table,
        const std::string& column,
        const std::string& key_value
    )>;

    /**
     * @brief Callback to test whether any child row references a parent key.
     *
     * Used during parent-table DELETE checks (RESTRICT semantics).
     *
     * @param shard_id     Shard to query.
     * @param child_table  Table with the FK column.
     * @param child_column FK column.
     * @param parent_value Parent key value being deleted.
     * @return true  → at least one child row references @p parent_value;
     *         false → no referencing child rows found.
     *
     * Must not throw.
     */
    using ChildExistsCallback = std::function<bool(
        const std::string& shard_id,
        const std::string& child_table,
        const std::string& child_column,
        const std::string& parent_value
    )>;

    CrossShardForeignKeyValidator() = default;

    // Non-copyable, movable.
    CrossShardForeignKeyValidator(const CrossShardForeignKeyValidator&) = delete;
    CrossShardForeignKeyValidator& operator=(const CrossShardForeignKeyValidator&) = delete;
    CrossShardForeignKeyValidator(CrossShardForeignKeyValidator&&) noexcept = default;
    CrossShardForeignKeyValidator& operator=(CrossShardForeignKeyValidator&&) noexcept = default;

    /**
     * @brief Register a cross-shard FK constraint.
     *
     * Constraints are matched against every transaction operation by
     * (table, column) during validate().  Registering the same constraint
     * name twice replaces the earlier definition.
     *
     * @param constraint  Fully populated constraint definition.
     */
    void registerConstraint(const CrossShardFKConstraint& constraint);

    /**
     * @brief Remove a previously registered constraint by name.
     *
     * No-op when the name is unknown.
     *
     * @param constraint_name  Name of the constraint to remove.
     */
    void removeConstraint(const std::string& constraint_name);

    /**
     * @brief Inject the key-existence lookup callback.
     *
     * Must be set before calling validate() for INSERT/UPDATE checks.
     * Passing nullptr disables parent-existence checks (violations are
     * reported without performing actual lookups).
     *
     * @param cb  Callback implementing cross-shard key lookup.
     */
    void setKeyExistsCallback(KeyExistsCallback cb);

    /**
     * @brief Inject the child-row existence lookup callback.
     *
     * Must be set before calling validate() for DELETE-on-parent checks.
     * Passing nullptr disables child-existence checks.
     *
     * @param cb  Callback implementing cross-shard child-row lookup.
     */
    void setChildExistsCallback(ChildExistsCallback cb);

    /**
     * @brief Provide the full list of shard IDs in the cluster.
     *
     * Used when CrossShardFKConstraint::parent_shard_id is empty so the
     * validator can fan out the existence check to every shard.
     *
     * @param shard_ids  All shard IDs that can own parent rows.
     */
    void setAllShardIds(std::vector<std::string> shard_ids);

    /**
     * @brief Validate FK constraints for a pending transaction.
     *
     * Parses the per-shard operation lists, extracts INSERT/UPDATE/DELETE
     * ops, and evaluates every registered FK constraint.  Non-deferrable
     * violations must be treated as prepare failures by the caller.
     *
     * ### Operation JSON format
     * Each string in @p shard_operations must be a JSON object with at least:
     * @code
     * { "op": "INSERT"|"UPDATE"|"DELETE",
     *   "table": "<table>",
     *   // For INSERT/UPDATE on a child table:
     *   "data": { "<fk_column>": "<value>", ... },
     *   // For DELETE on a parent table:
     *   "key": "<pk_value>"
     * }
     * @endcode
     *
     * Operations in unknown tables or with unrelated columns are silently
     * skipped.
     *
     * @param transaction_id    Global transaction ID (used in log messages).
     * @param shard_operations  Map of shard_id → list of serialised operations.
     * @return                  List of FK violations; empty on full compliance.
     *
     * @note Deferrable-constraint violations are included in the returned
     *       list but flagged with FKViolation::deferrable = true so the
     *       coordinator can decide whether to abort or proceed.
     */
    [[nodiscard]] std::vector<FKViolation> validate(
        const std::string& transaction_id,
        const std::map<std::string, std::vector<std::string>>& shard_operations
    ) const;

    /**
     * @brief Return the number of currently registered constraints.
     */
    [[nodiscard]] std::size_t constraintCount() const;

private:
    /**
     * @brief Check parent-key existence for an INSERT/UPDATE FK column value.
     *
     * @param constraint   The FK constraint being checked.
     * @param fk_value     The FK column value extracted from the operation.
     * @param txn_id       Transaction ID for log context.
     * @return             Optional violation; empty means constraint satisfied.
     */
    [[nodiscard]] std::optional<FKViolation> checkParentExists(
        const CrossShardFKConstraint& constraint,
        const std::string& fk_value,
        const std::string& txn_id
    ) const;

    /**
     * @brief Check that no child row references a parent key being deleted.
     *
     * Implements RESTRICT semantics: if any child shard has a referencing row
     * the delete must be blocked.
     *
     * @param constraint    The FK constraint being checked.
     * @param parent_value  The PK value being deleted.
     * @param txn_id        Transaction ID for log context.
     * @return              Optional violation; empty means no children found.
     */
    [[nodiscard]] std::optional<FKViolation> checkNoChildExists(
        const CrossShardFKConstraint& constraint,
        const std::string& parent_value,
        const std::string& txn_id
    ) const;

    mutable std::mutex mutex_;                    ///< Protects all mutable state.
    std::vector<CrossShardFKConstraint> constraints_; ///< Registered FK constraints.
    KeyExistsCallback key_exists_cb_;             ///< Parent-key lookup callback.
    ChildExistsCallback child_exists_cb_;         ///< Child-row lookup callback.
    std::vector<std::string> all_shard_ids_;      ///< All shard IDs for fan-out.
};

} // namespace sharding
} // namespace themisdb
