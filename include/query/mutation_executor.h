/**
 * @file mutation_executor.h
 * @brief MutationExecutor — executes MutationExecutionPlans against storage.
 * @version 1.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 4 implementation (EPIC-004) — StorageContext::get() added for rollback support.
 */

#pragma once

#include "query/mutation_execution_plan.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace query {

/**
 * @brief Executes a MutationExecutionPlan against the storage layer.
 *
 * Phase 3 implementation: provides the execution framework with lock
 * management, WAL integration, and step dispatch.  Actual key-value store
 * operations are dispatched via the nested @c StorageContext interface,
 * allowing unit-testable execution without a real RocksDB instance.
 *
 * ### Execution contract
 * - Steps are executed in the order they appear in @c MutationExecutionPlan::steps.
 * - If any step fails (returns @c false from the context), execution stops
 *   immediately and @c MutationResult::Failure is returned.
 * - @c AcquireLock and @c ReleaseLock steps are no-ops at this level; the
 *   context implementation is responsible for actual lock management.
 * - @c UpdateIndexes is logged but deferred (no-op in the base executor).
 *
 * ### Thread safety
 * The executor itself is stateless.  Thread-safety guarantees depend on the
 * @c StorageContext implementation provided by the caller.
 */
class MutationExecutor {
public:
    // -----------------------------------------------------------------------
    // StorageContext
    // -----------------------------------------------------------------------

    /**
     * @brief Abstraction over the key-value storage layer.
     *
     * All storage operations in a mutation are dispatched through this
     * interface.  The production implementation wraps RocksDB; the test
     * implementation (MockStorageContext) records calls for assertion.
     *
     * Implementors must be thread-safe when shared across executors.
     */
    struct StorageContext {
        virtual ~StorageContext() = default;

        /**
         * @brief Write a key-value pair to the storage layer.
         * @param collection  Target collection name.
         * @param key         Document key.
         * @param value       Serialised document value.
         * @return @c true on success.
         */
        virtual bool put(std::string_view collection,
                         std::string_view key,
                         std::string_view value) = 0;

        /**
         * @brief Delete a key-value pair from the storage layer.
         * @param collection  Target collection name.
         * @param key         Document key to delete.
         * @return @c true on success.
         */
        virtual bool remove(std::string_view collection,
                            std::string_view key) = 0;

        /**
         * @brief Check whether a document key exists (used by UPSERT).
         * @param collection  Collection to inspect.
         * @param key         Document key to test.
         * @return @c true if the key is present.
         */
        virtual bool exists(std::string_view collection,
                            std::string_view key) = 0;

        /**
         * @brief Generate a unique document key for a collection.
         * @param collection  Collection for which to generate the key.
         * @return A non-empty unique key string.
         */
        virtual std::string generateKey(std::string_view collection) = 0;

        /**
         * @brief Write a mutation record to the write-ahead log.
         * @param collection  Collection being mutated.
         * @param entry       JSON representation of the mutation entry.
         * @return @c true on success.
         */
        virtual bool writeWAL(std::string_view        collection,
                               const nlohmann::json&  entry) = 0;

        /**
         * @brief Read the current value of a document by key.
         *
         * Used by MutationTransactionContext to capture pre-mutation state for
         * rollback.  The default implementation returns std::nullopt, which
         * means rollback can undo freshly-inserted documents (by deleting them)
         * but cannot restore original values of overwritten or deleted documents.
         *
         * Production RocksDB implementations should override this method.
         *
         * @param collection  Collection to read from.
         * @param key         Document key.
         * @return Serialised document value if the key exists, std::nullopt otherwise.
         */
        virtual std::optional<std::string> get(std::string_view /*collection*/,
                                               std::string_view /*key*/) {
            return std::nullopt;
        }
    };

    // -----------------------------------------------------------------------
    // Constructor / Destructor
    // -----------------------------------------------------------------------

    explicit MutationExecutor() = default;
    ~MutationExecutor()         = default;

    // Non-copyable (stateless but there is no reason to copy)
    MutationExecutor(const MutationExecutor&)            = delete;
    MutationExecutor& operator=(const MutationExecutor&) = delete;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * @brief Execute a mutation plan against a storage context.
     *
     * Iterates over @p plan.steps in order, dispatching each step to the
     * appropriate @p ctx method.  Stops on the first failure.
     *
     * @param plan  The execution plan from AqlMutationTranslator.
     * @param ctx   Storage context implementation (must not be null).
     * @return MutationResult describing success/failure and affected documents.
     */
    [[nodiscard]] MutationResult execute(const MutationExecutionPlan& plan,
                                          StorageContext&              ctx) const;

private:
    /// @brief Execute the INSERT branch of a plan.
    [[nodiscard]] MutationResult executeInsert(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;

    /// @brief Execute the UPDATE branch of a plan.
    [[nodiscard]] MutationResult executeUpdate(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;

    /// @brief Execute the REMOVE branch of a plan.
    [[nodiscard]] MutationResult executeRemove(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;

    /// @brief Execute the REPLACE branch of a plan.
    [[nodiscard]] MutationResult executeReplace(const MutationExecutionPlan& plan,
                                                 StorageContext&              ctx) const;

    /// @brief Execute the UPSERT branch of a plan.
    [[nodiscard]] MutationResult executeUpsert(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;
};

} // namespace query
} // namespace themis
