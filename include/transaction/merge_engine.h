/**
 * @file merge_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "transaction/snapshot_manager.h"
#include "analytics/diff_engine.h"
#include "cdc/changefeed.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {
namespace transaction {

using json = nlohmann::json;

/**
 * @brief MergeEngine provides three-way merge functionality for ThemisDB's MVCC system
 * 
 * Enables Git-like merging of branches or snapshots by analyzing changes from a common
 * ancestor (base) and detecting/resolving conflicts between two divergent states.
 * 
 * Features:
 * - Three-way merge algorithm (base -> source, base -> target)
 * - Automatic conflict detection for overlapping changes
 * - Multiple conflict resolution strategies (ours, theirs, manual)
 * - Fast-forward merge support
 * - Dry-run mode for merge preview
 * - Integration with Snapshot and Diff infrastructure
 * 
 * Use Cases:
 * - Multi-user schema migrations
 * - Distributed database reconciliation
 * - Branch merging after parallel development
 * - Conflict resolution for concurrent changes
 */
class MergeEngine {
public:
    /**
     * @brief Type of merge conflict
     */
    enum class ConflictType {
        MODIFY_MODIFY,  // Both sides modified the same key
        DELETE_MODIFY,  // One side deleted, other modified
        MODIFY_DELETE,  // One side modified, other deleted
        DELETE_DELETE   // Both sides deleted (not a real conflict, auto-resolve)
    };

    /**
     * @brief Strategy for resolving conflicts
     */
    enum class MergeStrategy {
        OURS,          // Prefer changes from target branch
        THEIRS,        // Prefer changes from source branch
        MANUAL,        // Require manual conflict resolution
        FAST_FORWARD   // Only merge if no conflicts (fail on conflict)
    };

    /**
     * @brief A detected merge conflict
     */
    struct Conflict {
        ConflictType type;
        std::string key;                         // Conflicting key
        std::optional<std::string> base_value;   // Value at common ancestor
        std::optional<std::string> source_value; // Value in source branch
        std::optional<std::string> target_value; // Value in target branch
        uint64_t source_sequence;                // Source change sequence
        uint64_t target_sequence;                // Target change sequence
        
        json toJson() const;
        static Conflict fromJson(const json& j);
    };

    /**
     * @brief Manual resolution for a conflict
     */
    struct ConflictResolution {
        std::string key;                         // Key to resolve
        std::optional<std::string> resolved_value; // Chosen value (nullopt = delete)
        
        json toJson() const;
        static ConflictResolution fromJson(const json& j);
    };

    /**
     * @brief Options for merge operation
     */
    struct MergeOptions {
        MergeStrategy strategy = MergeStrategy::MANUAL;
        bool dry_run = false;                    // Preview mode, don't apply changes
        bool fail_on_conflict = false;           // Abort if conflicts detected
        std::vector<ConflictResolution> manual_resolutions; // Provided resolutions
        
        json toJson() const;
        static MergeOptions fromJson(const json& j);
    };

    /**
     * @brief Statistics about merge operation
     */
    struct MergeStats {
        size_t changes_applied = 0;
        size_t conflicts_detected = 0;
        size_t conflicts_auto_resolved = 0;
        size_t conflicts_manual = 0;
        bool has_conflicts = false;
        bool is_fast_forward = false;
        
        json toJson() const;
        static MergeStats fromJson(const json& j);
    };

    /**
     * @brief Result of a merge operation
     */
    struct MergeResult {
        bool success;
        std::string message;
        MergeStats stats;
        std::vector<Conflict> conflicts;         // Unresolved conflicts
        std::vector<analytics::DiffEngine::Change> changes_applied; // Applied changes
        
        uint64_t base_sequence;
        uint64_t source_sequence;
        uint64_t target_sequence;
        uint64_t result_sequence;                // Sequence after merge (if applied)
        
        json toJson() const;
        static MergeResult fromJson(const json& j);
    };

    /**
     * @brief Construct MergeEngine
     * @param diff_engine Reference to DiffEngine for computing changes
     * @param snapshot_manager Reference to SnapshotManager for tag resolution
     * @param changefeed Reference to Changefeed for applying changes
     */
    explicit MergeEngine(
        analytics::DiffEngine& diff_engine,
        SnapshotManager& snapshot_manager,
        Changefeed& changefeed
    );

    ~MergeEngine() = default;

    // Disable copy, allow move
    MergeEngine(const MergeEngine&) = delete;
    MergeEngine& operator=(const MergeEngine&) = delete;
    MergeEngine(MergeEngine&&) noexcept = default;
    MergeEngine& operator=(MergeEngine&&) noexcept = default;

    /**
     * @brief Perform three-way merge between two sequences
     * @param base_sequence Common ancestor sequence
     * @param source_sequence Source branch sequence (to merge from)
     * @param target_sequence Target branch sequence (to merge into)
     * @return MergeResult with success status, conflicts, and applied changes
     */
    MergeResult merge(
        uint64_t base_sequence,
        uint64_t source_sequence,
        uint64_t target_sequence
    );
    
    /**
     * @brief Perform three-way merge with custom options
     * @param base_sequence Common ancestor sequence
     * @param source_sequence Source branch sequence (to merge from)
     * @param target_sequence Target branch sequence (to merge into)
     * @param options Merge options including strategy and resolutions
     * @return MergeResult with success status, conflicts, and applied changes
     */
    MergeResult merge(
        uint64_t base_sequence,
        uint64_t source_sequence,
        uint64_t target_sequence,
        const MergeOptions& options
    );

    /**
     * @brief Perform three-way merge using snapshot tags
     * @param base_tag Tag name for common ancestor
     * @param source_tag Tag name for source branch
     * @param target_tag Tag name for target branch (or "current" for HEAD)
     * @return MergeResult with success status, conflicts, and applied changes
     */
    MergeResult mergeByTag(
        const std::string& base_tag,
        const std::string& source_tag,
        const std::string& target_tag
    );
    
    /**
     * @brief Perform three-way merge using snapshot tags with custom options
     * @param base_tag Tag name for common ancestor
     * @param source_tag Tag name for source branch
     * @param target_tag Tag name for target branch (or "current" for HEAD)
     * @param options Merge options including strategy and resolutions
     * @return MergeResult with success status, conflicts, and applied changes
     */
    MergeResult mergeByTag(
        const std::string& base_tag,
        const std::string& source_tag,
        const std::string& target_tag,
        const MergeOptions& options
    );

    /**
     * @brief Preview merge without applying changes (dry-run)
     * @param base_sequence Common ancestor sequence
     * @param source_sequence Source branch sequence
     * @param target_sequence Target branch sequence
     * @return MergeResult with conflicts and planned changes
     */
    MergeResult previewMerge(
        uint64_t base_sequence,
        uint64_t source_sequence,
        uint64_t target_sequence
    );

    /**
     * @brief Check if merge can be fast-forwarded (no conflicts)
     * @param base_sequence Common ancestor sequence
     * @param source_sequence Source branch sequence
     * @param target_sequence Target branch sequence
     * @return true if fast-forward is possible
     */
    bool canFastForward(
        uint64_t base_sequence,
        uint64_t source_sequence,
        uint64_t target_sequence
    );

private:
    analytics::DiffEngine& diff_engine_;
    SnapshotManager& snapshot_manager_;
    Changefeed& changefeed_;
    
    // Configurable limit for history retrieval in getValueAtSequence()
    // Can be adjusted based on expected changefeed size
    static constexpr size_t DEFAULT_HISTORY_LIMIT = 10000;

    /**
     * @brief Detect conflicts between source and target changes
     */
    std::vector<Conflict> detectConflicts(
        const analytics::DiffEngine::DiffResult& source_diff,
        const analytics::DiffEngine::DiffResult& target_diff,
        uint64_t base_sequence
    );

    /**
     * @brief Resolve conflicts using specified strategy
     */
    std::vector<analytics::DiffEngine::Change> resolveConflicts(
        const std::vector<Conflict>& conflicts,
        const MergeOptions& options
    );

    /**
     * @brief Apply changes to database (non-dry-run mode)
     */
    uint64_t applyChanges(
        const std::vector<analytics::DiffEngine::Change>& changes
    );

    /**
     * @brief Get value at specific sequence
     */
    std::optional<std::string> getValueAtSequence(
        const std::string& key,
        uint64_t sequence
    );

    /**
     * @brief Determine if conflict is auto-resolvable
     */
    bool isAutoResolvable(const Conflict& conflict) const;

    /**
     * @brief Auto-resolve conflict if possible
     */
    std::optional<analytics::DiffEngine::Change> autoResolve(
        const Conflict& conflict
    ) const;
};

} // namespace transaction
} // namespace themis
