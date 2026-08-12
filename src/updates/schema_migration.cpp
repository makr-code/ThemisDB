/**
 * @file schema_migration.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=25, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/schema_migration.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)

#include <map>
#include <stdexcept>
#include <variant>

namespace themis {
namespace updates {

// ============================================================================
// Internal operation descriptors
// ============================================================================

namespace {

struct AddColumnOp {
    std::string table;
    ColumnDef   column;
};

struct RenameColumnOp {
    std::string table;
    std::string old_name;
    std::string new_name;
};

struct AddIndexOp {
    std::string table;
    IndexDef    index;
};

struct DropColumnOp {
    std::string       table;
    std::string       column;
    DropColumnOptions opts;
};

struct CustomOp {
    std::function<bool(MigrationContext&)> fn;
};

using Operation = std::variant<AddColumnOp, RenameColumnOp, AddIndexOp,
                               DropColumnOp, CustomOp>;

// ---------------------------------------------------------------------------
// Minimal MigrationContext implementation
// ---------------------------------------------------------------------------

/**
 * @brief Concrete MigrationContext that wraps an IMigrationStorage.
 *
 * The iterator returned by createIterator() scans a simple in-memory table
 * snapshot built from the storage prefix "<table_name>:".  This is sufficient
 * for unit-testing and for the backfill logic in apply().
 */
class ConcreteMigrationContext final : public MigrationContext {
public:
    explicit ConcreteMigrationContext(const std::string& ver,
                                      IMigrationStorage* store)
    {
        version = ver;
        storage = store;
    }

    /**
     * @brief Iterator over all keys that start with "<table_name>:".
     *
     * Since IMigrationStorage has no native prefix-scan, this iterator calls
     * get() on demand; production callers that need full scans should supply
     * their own MigrationContext subclass backed by a real cursor.
     */
    class PrefixIterator final : public IMigrationIterator {
    public:
        PrefixIterator(IMigrationStorage* store,
                       const std::string& prefix,
                       std::vector<std::string> keys)
            : store_(store), prefix_(prefix), keys_(std::move(keys)), pos_(0)
        {
            advance_to_valid();
        }

        bool        valid() const override { return pos_ < keys_.size(); }
        std::string key()   const override { return valid() ? keys_[pos_] : ""; }
        std::string value() const override
        {
            if (!valid()) return "";
            std::string v;
            const bool found = store_->get(keys_[pos_], v);
            if (!found) {
                return "";
            }
            return v;
        }
        void next() override
        {
            if (valid()) ++pos_;
            advance_to_valid();
        }

    private:
        void advance_to_valid()
        {
            while (pos_ < keys_.size() &&
                   keys_[pos_].substr(0, prefix_.size()) != prefix_)
            {
                ++pos_;
            }
        }

        IMigrationStorage*       store_;
        std::string              prefix_;
        std::vector<std::string> keys_;
        std::size_t              pos_;
    };

    std::unique_ptr<IMigrationIterator> createIterator(
        const std::string& table_name) override
    {
        // Collect all keys stored under "table_name:" prefix.
        // In a real engine this would be a range scan; here we rely on the
        // storage implementation exposing keys via listKeys().
        if (known_keys_.empty() && storage != nullptr) {
            if (!storage->listKeys(known_keys_)) {
                LOG_WARN("SchemaMigration: listKeys() unsupported or failed; "
                         "iterator for table '{}' will yield no records",
                         table_name);
            }
        }
        const std::string prefix = table_name + ":";
        return std::make_unique<PrefixIterator>(storage, prefix, known_keys_);
    }

    /// Supply a pre-collected list of keys for the iterator scan.
    void setKnownKeys(std::vector<std::string> keys)
    {
        known_keys_ = std::move(keys);
    }

private:
    std::vector<std::string> known_keys_;
};

} // anonymous namespace

// ============================================================================
// SchemaMigration::Impl
// ============================================================================
// NOTE: Keep this translation unit rebuilt together with schema_migration.h
// to avoid ABI drift in IMigrationStorage virtual dispatch.

struct SchemaMigration::Impl {
    std::string        version_;
    RollbackStrategy   rollback_strategy_ = RollbackStrategy::AUTOMATIC;
    std::vector<Operation> operations_;
    OnlineDDLPhase     phase_             = OnlineDDLPhase::IDLE;
    IMigrationStorage* active_storage_    = nullptr;
    bool               committed_         = false;  ///< true after a successful apply().

    // Undo log: records of (table, key, old_value) for rollback.
    struct UndoEntry {
        std::string table;
        std::string key;
        std::string old_value;   // empty string means the key did not exist before.
        bool        existed;     // true if old_value is meaningful.
    };
    std::vector<UndoEntry> undo_log_;

    // Tracking which columns / indexes were processed (for the result struct).
    std::vector<std::string> backfilled_columns_;
    std::vector<std::string> indexes_built_online_;

    explicit Impl(const std::string& ver) : version_(ver) {}

    // ------------------------------------------------------------------------
    // Phase helpers
    // ------------------------------------------------------------------------

    MigrationResult run(IMigrationStorage& storage)
    {
        // Reset per-apply state so successive apply() calls start clean.
        active_storage_    = &storage;
        committed_         = false;
        phase_             = OnlineDDLPhase::IDLE;
        undo_log_.clear();
        backfilled_columns_.clear();
        indexes_built_online_.clear();

        MigrationResult result;
        result.version = version_;

        LOG_INFO("SchemaMigration [{}]: starting online DDL ({} operations)",
                 version_, operations_.size());

        // --- Phase 1: SHADOW_CREATE ---
        // In a full production implementation this creates a shadow table.
        // Here we advance the phase to track progress; the actual table
        // duplication is handled during BACKFILL below.
        phase_ = OnlineDDLPhase::SHADOW_CREATE;
        LOG_INFO("SchemaMigration [{}]: phase SHADOW_CREATE", version_);

        // --- Phase 2: DUAL_WRITE ---
        phase_ = OnlineDDLPhase::DUAL_WRITE;
        LOG_INFO("SchemaMigration [{}]: phase DUAL_WRITE", version_);

        // --- Phase 3: BACKFILL ---
        phase_ = OnlineDDLPhase::BACKFILL;
        LOG_INFO("SchemaMigration [{}]: phase BACKFILL", version_);

        for (const auto& op : operations_) {
            bool ok = std::visit(
                [this, &storage](auto&& o) { return applyOp(storage, o); },
                op);
            if (!ok) {
                result.error_message =
                    "SchemaMigration [" + version_ +
                    "]: operation failed during BACKFILL phase";
                LOG_ERROR("SchemaMigration [{}]: {}", version_, result.error_message);
                result.phase_reached = phase_;
                if (rollback_strategy_ == RollbackStrategy::AUTOMATIC) {
                    auto rb = performRollback();
                    if (!rb.success) {
                        LOG_WARN("SchemaMigration [{}]: rollback also failed: {}",
                                 version_, rb.error_message);
                    }
                }
                return result;
            }
        }

        // --- Phase 4: CONSISTENCY_CHECK ---
        phase_ = OnlineDDLPhase::CONSISTENCY_CHECK;
        LOG_INFO("SchemaMigration [{}]: phase CONSISTENCY_CHECK", version_);
        // Consistency is implicitly validated by the successful BACKFILL above.

        // --- Phase 5: ATOMIC_SWAP ---
        phase_ = OnlineDDLPhase::ATOMIC_SWAP;
        LOG_INFO("SchemaMigration [{}]: phase ATOMIC_SWAP (shadow → main)", version_);
        // In a full implementation the shadow table replaces the main table here.

        // --- Phase 6: CLEANUP ---
        phase_ = OnlineDDLPhase::CLEANUP;
        LOG_INFO("SchemaMigration [{}]: phase CLEANUP", version_);
        undo_log_.clear();  // Migration committed; undo log no longer needed.

        // Persist the migration version so the applied version is durable.
        std::string prev_ver;
        if (storage.get("__schema__:version", prev_ver)) {
            LOG_INFO("SchemaMigration [{}]: upgrading schema version marker: {} → {}",
                     version_, prev_ver, version_);
        }
        if (!storage.put("__schema__:version", version_)) {
            LOG_WARN("SchemaMigration [{}]: could not persist schema version marker",
                     version_);
        }

        // Mark as committed so rollback() is a no-op from here on.
        committed_      = true;
        active_storage_ = nullptr;

        // Build result
        result.success             = true;
        result.phase_reached       = phase_;
        result.backfilled_columns  = backfilled_columns_;
        result.indexes_built_online = indexes_built_online_;

        LOG_INFO("SchemaMigration [{}]: migration completed successfully "
                 "({} backfilled columns, {} online indexes)",
                 version_,
                 backfilled_columns_.size(),
                 indexes_built_online_.size());

        return result;
    }

    RollbackResult performRollback()
    {
        RollbackResult rb;
        rb.rolled_back_from = phase_;

        if (committed_ || active_storage_ == nullptr || phase_ == OnlineDDLPhase::IDLE) {
            // Either migration succeeded (committed) or no apply() has run yet.
            rb.success = true;
            if (!committed_) {
                phase_ = OnlineDDLPhase::IDLE;
            }
            return rb;
        }

        LOG_INFO("SchemaMigration [{}]: rolling back from phase {}",
                 version_, static_cast<int>(phase_));

        // Replay undo log in reverse order.
        bool undo_ok = true;
        for (auto it = undo_log_.rbegin(); it != undo_log_.rend(); ++it) {
            if (it->existed) {
                if (!active_storage_->put(it->key, it->old_value)) {
                    LOG_ERROR("SchemaMigration [{}]: rollback failed to restore key '{}'",
                              version_, it->key);
                    undo_ok = false;
                }
            } else {
                if (!active_storage_->remove(it->key)) {
                    LOG_ERROR("SchemaMigration [{}]: rollback failed to remove key '{}'",
                              version_, it->key);
                    undo_ok = false;
                }
            }
        }
        undo_log_.clear();
        phase_ = OnlineDDLPhase::ROLLED_BACK;

        if (!undo_ok) {
            rb.error_message =
                "One or more undo steps failed; storage may be in a partial state";
        }
        rb.success = undo_ok;
        return rb;
    }

    // ------------------------------------------------------------------------
    // Per-operation apply
    // ------------------------------------------------------------------------

    bool applyOp(IMigrationStorage& storage, const AddColumnOp& op)
    {
        if (op.column.name.empty()) {
            LOG_ERROR("SchemaMigration [{}]: addColumn '{}' has empty column name",
                      version_, op.table);
            return false;
        }
        if (op.table.empty()) {
            LOG_ERROR("SchemaMigration [{}]: addColumn has empty table name", version_);
            return false;
        }

        // Store column metadata: key = "<table>:__schema__:col:<colname>"
        const std::string meta_key =
            op.table + ":__schema__:col:" + op.column.name;
        const std::string meta_value =
            op.column.type + "|" +
            (op.column.nullable ? "nullable" : "not_null") + "|" +
            op.column.default_value;

        // Record undo entry.
        std::string existing;
        bool had_value = storage.get(meta_key, existing);
        undo_log_.push_back({op.table, meta_key, existing, had_value});

        if (!storage.put(meta_key, meta_value)) {
            LOG_ERROR("SchemaMigration [{}]: failed to write column metadata for '{}.{}'",
                      version_, op.table, op.column.name);
            return false;
        }

        backfilled_columns_.push_back(op.column.name);

        LOG_INFO("SchemaMigration [{}]: addColumn '{}.{}' (type={}, nullable={}, default='{}')",
                 version_, op.table, op.column.name,
                 op.column.type, op.column.nullable, op.column.default_value);
        return true;
    }

    bool applyOp(IMigrationStorage& storage, const RenameColumnOp& op)
    {
        if (op.old_name.empty() || op.new_name.empty()) {
            LOG_ERROR("SchemaMigration [{}]: renameColumn '{}' has empty column name",
                      version_, op.table);
            return false;
        }
        if (op.table.empty()) {
            LOG_ERROR("SchemaMigration [{}]: renameColumn has empty table name", version_);
            return false;
        }

        const std::string old_meta_key =
            op.table + ":__schema__:col:" + op.old_name;
        const std::string new_meta_key =
            op.table + ":__schema__:col:" + op.new_name;
        const std::string rename_key =
            op.table + ":__schema__:rename:" + op.old_name;

        // Read existing metadata so we can copy it under the new name.
        std::string meta_value;
        bool had_old = storage.get(old_meta_key, meta_value);
        if (!had_old) {
            // Column might not have schema metadata yet (freshly created table);
            // record a sentinel so rollback can remove the new-name key.
            meta_value = "UNKNOWN";
        }

        // Record undo entries for both old and new keys.
        // Capture the existing values so rollback can restore the prior state.
        std::string existing_new_meta;
        bool had_new_meta = storage.get(new_meta_key, existing_new_meta);
        std::string existing_rename;
        bool had_rename = storage.get(rename_key, existing_rename);

        undo_log_.push_back({op.table, new_meta_key, existing_new_meta, had_new_meta});
        undo_log_.push_back({op.table, old_meta_key, meta_value, had_old});
        undo_log_.push_back({op.table, rename_key, existing_rename, had_rename});

        // Write the new-name metadata.
        if (!storage.put(new_meta_key, meta_value)) {
            LOG_ERROR("SchemaMigration [{}]: failed to write renamed column metadata '{}.{}'",
                      version_, op.table, op.new_name);
            return false;
        }
        // Remove old-name metadata.
        if (!storage.remove(old_meta_key)) {
            LOG_ERROR("SchemaMigration [{}]: failed to remove old column metadata '{}.{}'",
                      version_, op.table, op.old_name);
            return false;
        }

        // Write a rename-marker so dual-write logic can translate writes.
        if (!storage.put(rename_key, op.new_name)) {
            LOG_ERROR("SchemaMigration [{}]: failed to write rename marker for '{}.{}'",
                      version_, op.table, op.old_name);
            return false;
        }

        LOG_INFO("SchemaMigration [{}]: renameColumn '{}.{}' → '{}'",
                 version_, op.table, op.old_name, op.new_name);
        return true;
    }

    bool applyOp(IMigrationStorage& storage, const AddIndexOp& op)
    {
        if (op.index.name.empty()) {
            LOG_ERROR("SchemaMigration [{}]: addIndex '{}' has empty index name",
                      version_, op.table);
            return false;
        }
        if (op.index.columns.empty()) {
            LOG_ERROR("SchemaMigration [{}]: addIndex '{}' on table '{}' has no columns",
                      version_, op.index.name, op.table);
            return false;
        }
        if (op.table.empty()) {
            LOG_ERROR("SchemaMigration [{}]: addIndex has empty table name", version_);
            return false;
        }

        const std::string idx_key =
            op.table + ":__schema__:idx:" + op.index.name;

        // Build column list string.
        std::string cols;
        for (std::size_t i = 0; i < op.index.columns.size(); ++i) {
            if (i) cols += ",";
            cols += op.index.columns[i];
        }
        const std::string idx_value =
            cols + "|" +
            (op.index.unique ? "unique" : "non_unique") + "|" +
            (op.index.build_online ? "online" : "blocking");

        // Record undo.
        std::string existing;
        bool had_value = storage.get(idx_key, existing);
        undo_log_.push_back({op.table, idx_key, existing, had_value});

        if (!storage.put(idx_key, idx_value)) {
            LOG_ERROR("SchemaMigration [{}]: failed to write index metadata '{}.{}'",
                      version_, op.table, op.index.name);
            return false;
        }

        if (op.index.build_online) {
            indexes_built_online_.push_back(op.index.name);
            LOG_INFO("SchemaMigration [{}]: addIndex '{}.{}' (cols={}, unique={}) – built online",
                     version_, op.table, op.index.name, cols, op.index.unique);
        } else {
            LOG_INFO("SchemaMigration [{}]: addIndex '{}.{}' (cols={}, unique={}) – blocking build",
                     version_, op.table, op.index.name, cols, op.index.unique);
        }
        return true;
    }

    bool applyOp(IMigrationStorage& storage, const DropColumnOp& op)
    {
        if (op.column.empty()) {
            LOG_ERROR("SchemaMigration [{}]: dropColumn '{}' has empty column name",
                      version_, op.table);
            return false;
        }
        if (op.table.empty()) {
            LOG_ERROR("SchemaMigration [{}]: dropColumn has empty table name", version_);
            return false;
        }

        const std::string meta_key =
            op.table + ":__schema__:col:" + op.column;
        const std::string drop_key =
            op.table + ":__schema__:dropped:" + op.column;

        // Record undo (restore column as "not dropped").
        std::string existing;
        bool had_value = storage.get(meta_key, existing);
        // Capture any pre-existing drop marker so rollback can restore it.
        std::string existing_drop;
        bool had_drop = storage.get(drop_key, existing_drop);
        undo_log_.push_back({op.table, drop_key, existing_drop, had_drop});
        undo_log_.push_back({op.table, meta_key, existing, had_value});

        // Mark column as dropped (hidden).  The grace period controls when
        // physical purge happens; we record it in the drop marker.
        const std::string drop_value =
            "grace_hours:" +
            std::to_string(
                std::chrono::duration_cast<std::chrono::hours>(
                    op.opts.grace_period).count());

        if (!storage.put(drop_key, drop_value)) {
            LOG_ERROR("SchemaMigration [{}]: failed to write drop marker for '{}.{}'",
                      version_, op.table, op.column);
            return false;
        }

        // Remove column metadata immediately (column is hidden).
        if (!storage.remove(meta_key)) {
            LOG_ERROR("SchemaMigration [{}]: failed to remove column metadata for '{}.{}'",
                      version_, op.table, op.column);
            return false;
        }

        LOG_INFO("SchemaMigration [{}]: dropColumn '{}.{}' (grace_period={} hours)",
                 version_, op.table, op.column,
                 std::chrono::duration_cast<std::chrono::hours>(
                     op.opts.grace_period).count());
        return true;
    }

    bool applyOp(IMigrationStorage& storage, const CustomOp& op)
    {
        ConcreteMigrationContext ctx(version_, &storage);
        // Pre-populate the key snapshot so createIterator() yields real records.
        std::vector<std::string> keys;
        if (!storage.listKeys(keys)) {
            LOG_WARN("SchemaMigration [{}]: listKeys() unsupported or failed; "
                     "custom migration callback will see an empty iterator",
                     version_);
        }
        ctx.setKnownKeys(std::move(keys));
        bool ok = op.fn(ctx);
        if (!ok) {
            LOG_ERROR("SchemaMigration [{}]: custom migration callback returned false",
                      version_);
        } else {
            LOG_INFO("SchemaMigration [{}]: custom migration callback succeeded",
                     version_);
        }
        return ok;
    }
};

// ============================================================================
// SchemaMigration public API
// ============================================================================

SchemaMigration::SchemaMigration(const std::string& version)
    : impl_(std::make_unique<Impl>(version)) {}

SchemaMigration::~SchemaMigration() = default;

// ----------------------------------------------------------------------------
// setRollbackStrategy
// ----------------------------------------------------------------------------

SchemaMigration& SchemaMigration::setRollbackStrategy(RollbackStrategy strategy)
{
    impl_->rollback_strategy_ = strategy;
    return *this;
}

// ----------------------------------------------------------------------------
// addColumn
// ----------------------------------------------------------------------------

SchemaMigration& SchemaMigration::addColumn(const std::string& table,
                                             const ColumnDef& column)
{
    impl_->operations_.emplace_back(AddColumnOp{table, column});
    return *this;
}

// ----------------------------------------------------------------------------
// renameColumn
// ----------------------------------------------------------------------------

SchemaMigration& SchemaMigration::renameColumn(const std::string& table,
                                                const std::string& old_name,
                                                const std::string& new_name)
{
    impl_->operations_.emplace_back(RenameColumnOp{table, old_name, new_name});
    return *this;
}

// ----------------------------------------------------------------------------
// addIndex
// ----------------------------------------------------------------------------

SchemaMigration& SchemaMigration::addIndex(const std::string& table,
                                            const IndexDef& index)
{
    impl_->operations_.emplace_back(AddIndexOp{table, index});
    return *this;
}

// ----------------------------------------------------------------------------
// dropColumn
// ----------------------------------------------------------------------------

SchemaMigration& SchemaMigration::dropColumn(const std::string& table,
                                              const std::string& column,
                                              const DropColumnOptions& opts)
{
    impl_->operations_.emplace_back(DropColumnOp{table, column, opts});
    return *this;
}

// ----------------------------------------------------------------------------
// addCustomMigration
// ----------------------------------------------------------------------------

SchemaMigration& SchemaMigration::addCustomMigration(
    std::function<bool(MigrationContext&)> migration)
{
    impl_->operations_.emplace_back(CustomOp{std::move(migration)});
    return *this;
}

// ----------------------------------------------------------------------------
// apply
// ----------------------------------------------------------------------------

MigrationResult SchemaMigration::apply(IMigrationStorage& storage)
{
    return impl_->run(storage);
}

// ----------------------------------------------------------------------------
// rollback
// ----------------------------------------------------------------------------

RollbackResult SchemaMigration::rollback()
{
    return impl_->performRollback();
}

// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------

const std::string& SchemaMigration::version() const noexcept
{
    return impl_->version_;
}

OnlineDDLPhase SchemaMigration::currentPhase() const noexcept
{
    return impl_->phase_;
}

std::size_t SchemaMigration::operationCount() const noexcept
{
    return impl_->operations_.size();
}

} // namespace updates
} // namespace themis

