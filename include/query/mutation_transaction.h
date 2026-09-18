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

struct MutationUndoEntry {
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

class MutationTransactionContext : public MutationExecutor::StorageContext {
public:
    /**
     * @brief Mutation Transaction Context.
     * @param[in,out] underlying Input/output parameter.
     * @return Return value.
     */
    explicit MutationTransactionContext(MutationExecutor::StorageContext& underlying)
        : underlying_(underlying) {}

    MutationTransactionContext(const MutationTransactionContext&)            = delete;
    MutationTransactionContext& operator=(const MutationTransactionContext&) = delete;

    // -----------------------------------------------------------------------
    // StorageContext — forwarding + undo-log intercept
    // -----------------------------------------------------------------------

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

    bool exists(std::string_view collection, std::string_view key) override {
        return underlying_.exists(collection, key);
    }

    std::string generateKey(std::string_view collection) override {
        return underlying_.generateKey(collection);
    }

    bool writeWAL(std::string_view collection, const nlohmann::json& entry) override {
        return underlying_.writeWAL(collection, entry);
    }

    std::optional<std::string> get(std::string_view collection,
                                   std::string_view key) override {
        return underlying_.get(collection, key);
    }

    // -----------------------------------------------------------------------
    // Transaction control
    // -----------------------------------------------------------------------

    /**
     * @brief Rollback.
     * @details Calls: rbegin(), rend(), remove(), put(), clear().
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

    [[nodiscard]] bool empty() const noexcept { return undo_log_.empty(); }

    [[nodiscard]] std::size_t size() const noexcept { return undo_log_.size(); }

private:
    MutationExecutor::StorageContext& underlying_;
    std::vector<MutationUndoEntry>    undo_log_;
};

}  // namespace query
}  // namespace themis
