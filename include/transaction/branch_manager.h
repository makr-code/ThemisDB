/**
 * @file branch_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class BranchManager {
public:
    struct Branch {
        std::string branch_name;        // Unique branch identifier
        std::string parent_branch;      // Parent branch (empty for root)
        uint64_t creation_sequence;     // Changefeed sequence at creation
        int64_t creation_timestamp_ms;  // Unix timestamp in milliseconds
        std::string description;        // Human-readable description
        std::string created_by;         // User/service that created the branch
        bool is_active;                 // Whether branch is currently active
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static Branch fromJson(const json& j);
    };
    
    struct BranchStats {
        size_t total_branches = 0;
        size_t active_branches = 0;
        int64_t oldest_creation_timestamp_ms = 0;
        int64_t newest_creation_timestamp_ms = 0;
        std::string default_branch;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
    };
    
    struct CreateBranchOptions {
        std::string from_tag;              // Create from named snapshot
        std::optional<uint64_t> from_sequence;  // Create from specific sequence
        std::optional<int64_t> from_timestamp;  // Create from timestamp
        bool set_active = false;           // Make this branch active immediately
    };
    
    struct MergeOptions {
        bool fast_forward = true;          // Allow fast-forward merges
        bool abort_on_conflict = true;     // Stop on first conflict
        std::string merge_strategy = "default";  // Merge strategy name
    };
    
    struct MergeResult {
        bool success = false;
        std::string message;
        std::vector<std::string> conflicts;  // Keys with conflicts
        uint64_t merged_sequence = 0;        // Sequence after merge
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
    };
    
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
     * @brief Set Merge Engine.
     * @param[in,out] merge_engine Input/output parameter.
     */
    void setMergeEngine(MergeEngine* merge_engine);

    std::optional<Branch> createBranch(
        const std::string& branch_name,
        const std::string& parent_branch,
        const std::string& description,
        const std::string& created_by = "system"
    );
    
    /**
     * @brief Create Branch.
     * @param[in] branch_name Name of the branch.
     * @param[in] parent_branch Input parameter.
     * @param[in] description Input parameter.
     * @param[in] created_by Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::optional<Branch> createBranch(
        const std::string& branch_name,
        const std::string& parent_branch,
        const std::string& description,
        const std::string& created_by,
        const CreateBranchOptions& options
    );
    
    /**
     * @brief Get Branch.
     * @param[in] branch_name Name of the branch.
     * @return Return value.
     */
    std::optional<Branch> getBranch(const std::string& branch_name) const;
    
    std::vector<Branch> listBranches(
        size_t limit = 0,
        const std::string& sort_by = "name",
        bool ascending = true
    ) const;
    
    /**
     * @brief Switch Branch.
     * @param[in] branch_name Name of the branch.
     * @return True when the operation succeeds.
     */
    bool switchBranch(const std::string& branch_name);
    
    /**
     * @brief Get Active Branch.
     * @return Return value.
     */
    std::string getActiveBranch() const;
    
    bool deleteBranch(const std::string& branch_name, bool force = false);
    
    /**
     * @brief Merge Branches.
     * @param[in] source_branch Input parameter.
     * @param[in] target_branch Input parameter.
     * @return Return value.
     */
    MergeResult mergeBranches(
        const std::string& source_branch,
        const std::string& target_branch
    );
    
    /**
     * @brief Merge Branches.
     * @param[in] source_branch Input parameter.
     * @param[in] target_branch Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    MergeResult mergeBranches(
        const std::string& source_branch,
        const std::string& target_branch,
        const MergeOptions& options
    );
    
    MergeEngine::MergeResult previewBranchMerge(
        const std::string& source_branch,
        const std::string& target_branch,
        const std::string& base_branch = ""
    ) const;

    MergeEngine::MergeResult resolveAndMergeBranches(
        const std::string& source_branch,
        const std::string& target_branch,
        const std::vector<MergeEngine::ConflictResolution>& resolutions,
        const std::string& base_branch = ""
    );

    /**
     * @brief Branch Exists.
     * @param[in] branch_name Name of the branch.
     * @return True when the operation succeeds.
     */
    bool branchExists(const std::string& branch_name) const;
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    BranchStats getStats() const;
    
    /**
     * @brief Get Sequence For Branch.
     * @param[in] branch_name Name of the branch.
     * @return Return value.
     */
    std::optional<uint64_t> getSequenceForBranch(const std::string& branch_name) const;
    
    /**
     * @brief Get Timestamp For Branch.
     * @param[in] branch_name Name of the branch.
     * @return Return value.
     */
    std::optional<int64_t> getTimestampForBranch(const std::string& branch_name) const;
    
    /**
     * @brief Is Valid Branch Name.
     * @param[in] branch_name Name of the branch.
     * @return True when the operation succeeds.
     */
    static bool isValidBranchName(const std::string& branch_name);
    
    /**
     * @brief Get Default Branch.
     * @return Return value.
     */
    static std::string getDefaultBranch();

    // ---- Phase 5: Branch History ----

    struct BranchHistoryEntry {
        std::string event_type;   ///< "created", "switched_to", "merged_from", "deleted"
        std::string branch_name;  ///< Branch this event concerns
        std::string details;      ///< Human-readable detail string
        std::string performed_by; ///< Actor that triggered the event
        int64_t     timestamp_ms{0};
        uint64_t    sequence{0};  ///< Changefeed sequence at event time

        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static BranchHistoryEntry fromJson(const json& j);
    };

    std::vector<BranchHistoryEntry> getBranchHistory(
        const std::string& branch_name, size_t limit = 0) const;

    // ---- Phase 5: Branch GC ----

    struct BranchGCPolicy {
        int64_t max_age_ms{0};       ///< 0 = no age limit; prune branches older than this
        bool    only_merged{true};   ///< Only prune branches that have been merged
        bool    protect_default{true}; ///< Never prune the default branch
    };

    /**
     * @brief Set Branch GCPolicy.
     * @param[in] policy Input parameter.
     */
    void setBranchGCPolicy(const BranchGCPolicy& policy);

    /**
     * @brief Prune Merged Branches.
     * @return Return value.
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
     * @brief Make Key.
     * @param[in] branch_name Name of the branch.
     * @return Return value.
     */
    std::string makeKey(const std::string& branch_name) const;
    
    /**
     * @brief Extract Branch Name.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::string extractBranchName(const std::string& key) const;
    
    /**
     * @brief Serialize.
     * @param[in] branch Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> serialize(const Branch& branch) const;
    
    /**
     * @brief Deserialize.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::optional<Branch> deserialize(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief Load Active Branch.
     */
    void loadActiveBranch();
    
    /**
     * @brief Save Active Branch.
     * @param[in] branch_name Name of the branch.
     * @return True when the operation succeeds.
     */
    bool saveActiveBranch(const std::string& branch_name);
    
    /**
     * @brief Resolve Sequence.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::optional<uint64_t> resolveSequence(const CreateBranchOptions& options) const;
    
    /**
     * @brief Is Branch Merged.
     * @param[in] branch_name Name of the branch.
     * @param[in] target_branch Input parameter.
     * @return True when the operation succeeds.
     */
    bool isBranchMerged(const std::string& branch_name, const std::string& target_branch) const;

    /**
     * @brief Record Merge Status.
     * @param[in] source_branch Input parameter.
     * @param[in] target_branch Input parameter.
     */
    void recordMergeStatus(const std::string& source_branch, const std::string& target_branch);

    /**
     * @brief Append History.
     * @param[in] entry Input parameter.
     */
    void appendHistory(const BranchHistoryEntry& entry);

    /**
     * @brief Serialize History.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> serializeHistory(const BranchHistoryEntry& entry) const;

    /**
     * @brief Deserialize History.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::optional<BranchHistoryEntry> deserializeHistory(
        const std::vector<uint8_t>& data) const;
};

} // namespace transaction
} // namespace themis
