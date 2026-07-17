/**
 * @file mutation_transaction.h
 * @brief MutationTransactionContext — transactional StorageContext proxy for Phase 4.
 *
 * Wraps a MutationExecutor::StorageContext and intercepts put()/remove() calls
 * to build an undo log.  rollback() reverses every recorded mutation in LIFO
 * order, restoring the storage to its pre-transaction state.
 *
 * @version 1.0.0
 * @note Status: Phase 4 implementation (EPIC-004)
 */

#pragma once

#include "query/mutation_executor.h"
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace query {

// ============================================================================
// MutationUndoEntry
// ============================================================================

/**
 * @brief Records a single mutation so it can be undone during rollback.
 *
 * Three compensating operations are modelled:
 * - Delete  — undo an INSERT by deleting the newly-created key.
 * - Put     — undo an UPDATE/REPLACE by re-storing the original value.
 * - Insert  — undo a REMOVE by re-inserting the original value.
 */
struct MutationUndoEntry {
    /// @brief The compensating action to apply during rollback.
    enum class Op {
        Delete,  ///< Undo an INSERT: remove the newly-inserted key.
        Put,     ///< Undo an UPDATE/REPLACE: restore the original document.
        Insert,  ///< Undo a REMOVE/DELETE: re-insert the original document.
    };

    Op          op;
    std::string collection;
    std::string key;
    std::string original_value; ///< Non-empty for Put and Insert ops.
};

// ============================================================================
// MutationTransactionContext
// ============================================================================

/**
 * @brief Transactional StorageContext proxy for atomic multi-mutation execution.
 *
 * Wraps an underlying MutationExecutor::StorageContext (which may be a real
 * RocksDB-backed context or a MockStorageContext in tests).  Every put() and
 * remove() call is forwarded to the underlying context AND recorded in an
 * internal undo log.  rollback() replays the log in reverse to restore the
 * original state.
 *
 * @par Atomicity
 * The caller is responsible for executing all mutation steps through this
 * context and calling rollback() on failure.  Successful transactions require
 * no special commit step — changes are applied eagerly to the underlying
 * storage (reflecting the same behaviour as the original non-transactional
 * executor paths).
 *
 * @par Undo Log Fidelity
 * rollback() fidelity depends on the underlying context's get() support:
 * - If get() returns the original value before an overwrite, the original
 *   document is fully restored.
 * - If get() returns std::nullopt (the default for StorageContext), rollback
 *   can still delete keys that were freshly inserted (Op::Delete) but cannot
 *   restore documents that were updated or removed.
 *   Production RocksDB contexts should therefore override get().
 *
 * @par Thread Safety
 * Not thread-safe.  Each concurrent mutation path should use a separate
 * MutationTransactionContext instance.
 */
class MutationTransactionContext : public MutationExecutor::StorageContext {
public:
    /**
     * @brief Construct a context wrapping @p underlying.
     * @param underlying  StorageContext to forward all storage calls to.
     */
    explicit MutationTransactionContext(MutationExecutor::StorageContext& underlying)
        : underlying_(underlying) {}

    MutationTransactionContext(const MutationTransactionContext&)            = delete;
    MutationTransactionContext& operator=(const MutationTransactionContext&) = delete;

    // -----------------------------------------------------------------------
    // StorageContext — forwarding + undo-log intercept
    // -----------------------------------------------------------------------

    /**
     * @brief Forward put() to underlying storage and record an undo entry.
     *
     * Reads the pre-mutation document value via get() so it can be restored
     * on rollback.  If get() returns std::nullopt the key is assumed to be
     * new (INSERT semantics); rollback will delete it.
     *
     * @param collection  Target collection.
     * @param key         Document key.
     * @param value       Serialised document value.
     * @return @c true on success.
     */
    bool put(std::string_view collection,
             std::string_view key,
             std::string_view value) override {
        auto original = underlying_.get(collection, key);
        if (original.has_value()) {
            undo_log_.push_back({MutationUndoEntry::Op::Put,
                                 std::string(collection), std::string(key),
                                 std::move(*original)});
        } else {
            undo_log_.push_back({MutationUndoEntry::Op::Delete,
                                 std::string(collection), std::string(key), {}});
        }
        return underlying_.put(collection, key, value);
    }

    /**
     * @brief Forward remove() to underlying storage and record an undo entry.
     *
     * Reads the pre-removal document value via get() to enable re-insertion
     * during rollback.  If get() returns std::nullopt the document cannot be
     * fully restored (undo entry is omitted for that case).
     *
     * @param collection  Target collection.
     * @param key         Document key.
     * @return @c true on success.
     */
    bool remove(std::string_view collection,
                std::string_view key) override {
        auto original = underlying_.get(collection, key);
        if (original.has_value()) {
            undo_log_.push_back({MutationUndoEntry::Op::Insert,
                                 std::string(collection), std::string(key),
                                 std::move(*original)});
        }
        return underlying_.remove(collection, key);
    }

    /// @brief Forwarded to underlying_.
    bool exists(std::string_view collection, std::string_view key) override {
        return underlying_.exists(collection, key);
    }

    /// @brief Forwarded to underlying_.
    std::string generateKey(std::string_view collection) override {
        return underlying_.generateKey(collection);
    }

    /// @brief Forwarded to underlying_.
    bool writeWAL(std::string_view collection, const nlohmann::json& entry) override {
        return underlying_.writeWAL(collection, entry);
    }

    /**
     * @brief Forwarded to underlying_.
     *
     * Exposes get() so that nested MutationTransactionContext wrapping is
     * possible (though not required in normal usage).
     */
    std::optional<std::string> get(std::string_view collection,
                                   std::string_view key) override {
        return underlying_.get(collection, key);
    }

    // -----------------------------------------------------------------------
    // Transaction control
    // -----------------------------------------------------------------------

    /**
     * @brief Reverse all recorded mutations in LIFO order.
     *
     * After rollback() the undo log is cleared.  Subsequent calls are no-ops
     * unless new mutations are recorded.
     */
    void rollback() {
        for (auto it = undo_log_.rbegin(); it != undo_log_.rend(); ++it) {
            const auto& entry = *it;
            switch (entry.op) {
                case MutationUndoEntry::Op::Delete:
                    underlying_.remove(entry.collection, entry.key);
                    break;
                case MutationUndoEntry::Op::Put:
                    underlying_.put(entry.collection, entry.key, entry.original_value);
                    break;
                case MutationUndoEntry::Op::Insert:
                    underlying_.put(entry.collection, entry.key, entry.original_value);
                    break;
            }
        }
        undo_log_.clear();
    }

    /// @return @c true when no mutations have been recorded.
    [[nodiscard]] bool empty() const noexcept { return undo_log_.empty(); }

    /// @return Number of mutation steps recorded in the undo log.
    [[nodiscard]] std::size_t size() const noexcept { return undo_log_.size(); }

private:
    MutationExecutor::StorageContext& underlying_;
    std::vector<MutationUndoEntry>    undo_log_;
};

}  // namespace query
}  // namespace themis
