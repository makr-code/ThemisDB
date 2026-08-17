/**
 * @file branch_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include "transaction/snapshot_manager.h"
#include "transaction/merge_engine.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {

namespace transaction {

using json = nlohmann::json;

/**
 * @brief BranchManager provides Git-like persistent branches for ThemisDB's MVCC system
 * 
 * Enables named, persistent branches for:
 * - Schema migration testing
 * - A/B testing scenarios
 * - What-if analysis
 * - Parallel development workflows
 * 
 * Features:
 * - Named branches with parent tracking
 * - Branch creation from tags/sequences/timestamps
 * - Branch switching and merging
 * - Persistent storage in RocksDB
 * - Integration with SnapshotManager and Changefeed
 * - REST API endpoints
 */
class BranchManager {
public:
    /**
     * @brief Branch metadata
     */
    struct Branch {
        std::string branch_name;        // Unique branch identifier
        std::string parent_branch;      // Parent branch (empty for root)
        uint64_t creation_sequence;     // Changefeed sequence at creation
        int64_t creation_timestamp_ms;  // Unix timestamp in milliseconds
        std::string description;        // Human-readable description
        std::string created_by;         // User/service that created the branch
        bool is_active;                 // Whether branch is currently active
        
        json toJson() const;
        static Branch fromJson(const json& j);
    };
    
    /**
     * @brief Statistics about branches
     */
    struct BranchStats {
        size_t total_branches = 0;
        size_t active_branches = 0;
        int64_t oldest_creation_timestamp_ms = 0;
        int64_t newest_creation_timestamp_ms = 0;
        std::string default_branch;
        
        json toJson() const;
    };
    
    /**
     * @brief Options for branch creation
     */
    struct CreateBranchOptions {
        std::string from_tag;              // Create from named snapshot
        std::optional<uint64_t> from_sequence;  // Create from specific sequence
        std::optional<int64_t> from_timestamp;  // Create from timestamp
        bool set_active = false;           // Make this branch active immediately
    };
    
    /**
     * @brief Options for branch merge
     */
    struct MergeOptions {
        bool fast_forward = true;          // Allow fast-forward merges
        bool abort_on_conflict = true;     // Stop on first conflict
        std::string merge_strategy = "default";  // Merge strategy name
    };
    
    /**
     * @brief Result of merge operation
     */
    struct MergeResult {
        bool success = false;
        std::string message;
        std::vector<std::string> conflicts;  // Keys with conflicts
        uint64_t merged_sequence = 0;        // Sequence after merge
        
        json toJson() const;
    };
    
    /**
     * @brief Construct BranchManager
     * @param db Reference to RocksDB wrapper
     * @param changefeed Reference to Changefeed for sequence numbers
     * @param snapshot_manager Reference to SnapshotManager for tag resolution
     * @param merge_engine Optional reference to MergeEngine for 3-way merges
     */
    explicit BranchManager(
        RocksDBWrapper& db, 
        Changefeed& changefeed,
        SnapshotManager& snapshot_manager,
        MergeEngine* merge_engine = nullptr
    );
    
    ~BranchManager() = default;

    // Disable copy, allow move
    BranchManager(const BranchManager&) = delete;
    BranchManager& operator=(const BranchManager&) = delete;
    BranchManager(BranchManager&&) noexcept = default;
    BranchManager& operator=(BranchManager&&) noexcept = default;
    
    /**
     * @brief Set MergeEngine for non-fast-forward merge support
     * @param merge_engine Pointer to MergeEngine instance
     */
    void setMergeEngine(MergeEngine* merge_engine);

    /**
     * @brief Create a new branch
     * @param branch_name Unique branch name (alphanumeric, hyphens, underscores)
     * @param parent_branch Parent branch name (or empty for root)
     * @param description Human-readable description
     * @param created_by Optional user/service identifier
     * @return Branch metadata if successful, nullopt on error
     * 
     * Error conditions:
     * - Branch name already exists
     * - Invalid branch name format
     * - Parent branch does not exist
     * - Database write failure
     */
    std::optional<Branch> createBranch(
        const std::string& branch_name,
        const std::string& parent_branch,
        const std::string& description,
        const std::string& created_by = "system"
    );
    
    /**
     * @brief Create a new branch with custom options
     * @param branch_name Unique branch name (alphanumeric, hyphens, underscores)
     * @param parent_branch Parent branch name (or empty for root)
     * @param description Human-readable description
     * @param created_by Optional user/service identifier
     * @param options Additional creation options
     * @return Branch metadata if successful, nullopt on error
     */
    std::optional<Branch> createBranch(
        const std::string& branch_name,
        const std::string& parent_branch,
        const std::string& description,
        const std::string& created_by,
        const CreateBranchOptions& options
    );
    
    /**
     * @brief Get branch metadata by name
     * @param branch_name Branch to retrieve
     * @return Branch metadata if found, nullopt otherwise
     */
    std::optional<Branch> getBranch(const std::string& branch_name) const;
    
    /**
     * @brief List all branches
     * @param limit Maximum number of branches to return (0 = all)
     * @param sort_by Sort order: "name" (default), "timestamp", "active"
     * @param ascending Sort direction (default: false = newest first)
     * @return Vector of branch metadata
     */
    std::vector<Branch> listBranches(
        size_t limit = 0,
        const std::string& sort_by = "name",
        bool ascending = true
    ) const;
    
    /**
     * @brief Switch to a different branch
     * @param branch_name Branch to switch to
     * @return true if switched successfully, false on error
     * 
     * This marks the branch as active and updates the current branch context
     */
    bool switchBranch(const std::string& branch_name);
    
    /**
     * @brief Get the currently active branch name
     * @return Active branch name, or "main" if none set
     */
    std::string getActiveBranch() const;
    
    /**
     * @brief Delete a branch
     * @param branch_name Branch to delete
     * @param force Force deletion even if not fully merged
     * @return true if deleted, false if not found or error
     * 
     * Error conditions:
     * - Branch is currently active
     * - Branch is not fully merged (unless force = true)
     * - Branch does not exist
     */
    bool deleteBranch(const std::string& branch_name, bool force = false);
    
    /**
     * @brief Merge source branch into target branch
     * @param source_branch Branch to merge from
     * @param target_branch Branch to merge into
     * @return Merge result with success status and conflicts
     */
    MergeResult mergeBranches(
        const std::string& source_branch,
        const std::string& target_branch
    );
    
    /**
     * @brief Merge source branch into target branch with custom options
     * @param source_branch Branch to merge from
     * @param target_branch Branch to merge into
     * @param options Merge options
     * @return Merge result with success status and conflicts
     */
    MergeResult mergeBranches(
        const std::string& source_branch,
        const std::string& target_branch,
        const MergeOptions& options
    );
    
    /**
     * @brief Preview a branch merge (dry-run) with full conflict details
     * @param source_branch Branch to merge from
     * @param target_branch Branch to merge into
     * @param base_branch   Optional common ancestor branch name.
     *                      When empty, base = min(source_seq, target_seq).
     * @return MergeEngine::MergeResult with full conflict details (base/source/target values)
     * 
     * Does not apply any changes. Returns conflict detail sufficient for a
     * conflict resolution UI to present per-key choices to the user.
     */
    MergeEngine::MergeResult previewBranchMerge(
        const std::string& source_branch,
        const std::string& target_branch,
        const std::string& base_branch = ""
    ) const;

    /**
     * @brief Resolve conflicts and complete a branch merge
     * @param source_branch Branch to merge from
     * @param target_branch Branch to merge into
     * @param resolutions   Per-key conflict resolutions from the user
     * @param base_branch   Optional common ancestor branch name.
     *                      When empty, base = min(source_seq, target_seq).
     * @return MergeEngine::MergeResult with success status and applied changes
     */
    MergeEngine::MergeResult resolveAndMergeBranches(
        const std::string& source_branch,
        const std::string& target_branch,
        const std::vector<MergeEngine::ConflictResolution>& resolutions,
        const std::string& base_branch = ""
    );

    /**
     * @brief Check if a branch exists
     * @param branch_name Branch to check
     * @return true if exists, false otherwise
     */
    bool branchExists(const std::string& branch_name) const;
    
    /**
     * @brief Get statistics about all branches
     * @return Branch statistics
     */
    BranchStats getStats() const;
    
    /**
     * @brief Get sequence number for a branch
     * @param branch_name Branch to query
     * @return Sequence number if found, nullopt otherwise
     */
    std::optional<uint64_t> getSequenceForBranch(const std::string& branch_name) const;
    
    /**
     * @brief Get timestamp for a branch
     * @param branch_name Branch to query
     * @return Timestamp in milliseconds if found, nullopt otherwise
     */
    std::optional<int64_t> getTimestampForBranch(const std::string& branch_name) const;
    
    /**
     * @brief Validate branch name format
     * @param branch_name Branch to validate
     * @return true if valid, false otherwise
     * 
     * Valid format: alphanumeric, hyphens, underscores, forward slashes
     * Length: 1-128 characters
     * Reserved names: HEAD, FETCH_HEAD, ORIG_HEAD
     */
    static bool isValidBranchName(const std::string& branch_name);
    
    /**
     * @brief Get the default branch name
     * @return Default branch name (typically "main")
     */
    static std::string getDefaultBranch();

    // ---- Phase 5: Branch History ----

    /**
     * @brief A single entry in a branch's history log.
     */
    struct BranchHistoryEntry {
        std::string event_type;   ///< "created", "switched_to", "merged_from", "deleted"
        std::string branch_name;  ///< Branch this event concerns
        std::string details;      ///< Human-readable detail string
        std::string performed_by; ///< Actor that triggered the event
        int64_t     timestamp_ms{0};
        uint64_t    sequence{0};  ///< Changefeed sequence at event time

        json toJson() const;
        static BranchHistoryEntry fromJson(const json& j);
    };

    /**
     * @brief Get the history log for @p branch_name.
     *
     * Returns events recorded for the branch in chronological order
     * (oldest first).
     *
     * @param branch_name Branch to query.
     * @param limit       Maximum entries (0 = all).
     * @return Vector of history entries.
     */
    std::vector<BranchHistoryEntry> getBranchHistory(
        const std::string& branch_name, size_t limit = 0) const;

    // ---- Phase 5: Branch GC ----

    /**
     * @brief Cleanup policy for pruning stale merged branches.
     */
    struct BranchGCPolicy {
        int64_t max_age_ms{0};       ///< 0 = no age limit; prune branches older than this
        bool    only_merged{true};   ///< Only prune branches that have been merged
        bool    protect_default{true}; ///< Never prune the default branch
    };

    /**
     * @brief Set the GC policy used by pruneMergedBranches().
     */
    void setBranchGCPolicy(const BranchGCPolicy& policy);

    /**
     * @brief Delete branches that satisfy the GC policy.
     * @return Number of branches deleted.
     */
    size_t pruneMergedBranches();

private:
    RocksDBWrapper& db_;
    Changefeed& changefeed_;
    SnapshotManager& snapshot_manager_;
    MergeEngine* merge_engine_;  // Optional pointer for 3-way merge support
    
    mutable std::mutex mutex_;
    std::string active_branch_;
    BranchGCPolicy gc_policy_;

    // Key prefixes for branch storage in RocksDB
    static constexpr const char* BRANCH_PREFIX        = "branch:";
    static constexpr const char* BRANCH_HIST_PREFIX   = "branch_hist:";
    static constexpr const char* BRANCH_MERGED_PREFIX = "branch_merged:";
    static constexpr const char* ACTIVE_BRANCH_KEY    = "branch:_active";
    static constexpr const char* DEFAULT_BRANCH       = "main";
    
    /**
     * @brief Make RocksDB key for a branch
     */
    std::string makeKey(const std::string& branch_name) const;
    
    /**
     * @brief Extract branch name from RocksDB key
     */
    std::string extractBranchName(const std::string& key) const;
    
    /**
     * @brief Serialize branch to bytes
     */
    std::vector<uint8_t> serialize(const Branch& branch) const;
    
    /**
     * @brief Deserialize branch from bytes
     */
    std::optional<Branch> deserialize(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief Load active branch from storage
     */
    void loadActiveBranch();
    
    /**
     * @brief Save active branch to storage
     */
    bool saveActiveBranch(const std::string& branch_name);
    
    /**
     * @brief Resolve sequence from options (tag, sequence, or timestamp)
     */
    std::optional<uint64_t> resolveSequence(const CreateBranchOptions& options) const;
    
    /**
     * @brief Check if branch is fully merged into another branch
     */
    bool isBranchMerged(const std::string& branch_name, const std::string& target_branch) const;

    /**
     * @brief Persist a marker indicating source_branch was merged into target_branch.
     * Must be called after a successful merge to enable isBranchMerged() checks.
     */
    void recordMergeStatus(const std::string& source_branch, const std::string& target_branch);

    /**
     * @brief Append a history entry for @p branch_name.
     * May be called with or without mutex_ held; it does NOT acquire mutex_
     * itself (uses the underlying db_ which is thread-safe independently).
     */
    void appendHistory(const BranchHistoryEntry& entry);

    /**
     * @brief Serialize a history entry to bytes.
     */
    std::vector<uint8_t> serializeHistory(const BranchHistoryEntry& entry) const;

    /**
     * @brief Deserialize a history entry from bytes.
     */
    std::optional<BranchHistoryEntry> deserializeHistory(
        const std::vector<uint8_t>& data) const;
};

} // namespace transaction
} // namespace themis
