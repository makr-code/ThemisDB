/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_version_control.h                           ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     394                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_version_control.h
 * @brief Git-like version control system for prompt templates
 * 
 * Provides comprehensive version management for prompts:
 * - Commit versions with messages
 * - Branch and merge support
 * - History tracking and genealogy
 * - Rollback to any version
 * - Diff visualization
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include &lt;optional&gt;
#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>

namespace rocksdb { class ColumnFamilyHandle; }

namespace themis {
class RocksDBWrapper;

namespace prompt_engineering {

/**
 * @brief A single prompt version
 */
struct PromptVersion {
    std::string version_id;              ///< Unique version identifier (SHA-like hash)
    std::string prompt_id;               ///< Associated prompt template ID
    std::string branch;                  ///< Branch name (e.g., "main", "experiment")
    std::string content;                 ///< Prompt content at this version
    std::string commit_message;          ///< Description of changes
    std::string author;                  ///< Who created this version
    std::string parent_version;          ///< Parent version ID (empty for initial)
    std::chrono::system_clock::time_point timestamp;  ///< When created
    nlohmann::json metadata;             ///< Additional version metadata
    double performance_score = 0.0;      ///< Performance metrics at this version
    
    /**
     * @brief Convert to JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Parse from JSON
     */
    static PromptVersion fromJson(const nlohmann::json& j);
};

/**
 * @brief Difference between two versions
 */
struct PromptDiff {
    std::string version_a;               ///< First version ID
    std::string version_b;               ///< Second version ID
    std::vector<std::string> added_lines;     ///< Lines added in B
    std::vector<std::string> removed_lines;   ///< Lines removed from A
    std::vector<std::string> modified_lines;  ///< Lines changed
    std::string unified_diff;            ///< Unified diff format
    size_t additions = 0;                ///< Total additions
    size_t deletions = 0;                ///< Total deletions
    
    /**
     * @brief Convert to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Branch information
 */
struct BranchInfo {
    std::string name;                    ///< Branch name
    std::string head_version;            ///< Current HEAD version
    std::string base_version;            ///< Base version (where branched from)
    std::chrono::system_clock::time_point created_at;  ///< When created
    size_t commit_count = 0;             ///< Number of commits
    bool is_merged = false;              ///< Whether merged back
    
    /**
     * @brief Convert to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Merge result
 */
struct MergeResult {
    bool success = false;                ///< Whether merge succeeded
    std::string merged_version_id;       ///< New version ID if successful
    std::vector<std::string> conflicts;  ///< Conflict descriptions
    std::string merged_content;          ///< Merged content
    std::string strategy_used;           ///< Merge strategy ("ours", "theirs", "auto")
    
    /**
     * @brief Convert to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Version control system for prompts
 * 
 * Git-like version control:
 * - Commit versions with descriptive messages
 * - Branch management (create, switch, merge)
 * - Complete history tracking
 * - Rollback to any version
 * - Diff between versions
 * - Merge branches with conflict detection
 * 
 * Thread-safe for concurrent operations.
 */
class PromptVersionControl {
public:
    /**
     * @brief Constructor for in-memory version control
     */
    PromptVersionControl();
    
    /**
     * @brief Constructor with RocksDB persistence
     * @param db RocksDB wrapper (not owned)
     * @param cf Column family handle (optional, uses default if null)
     */
    PromptVersionControl(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf = nullptr);
    
    /**
     * @brief Commit a new version
     * @param prompt_id Prompt template ID
     * @param content Prompt content
     * @param message Commit message describing changes
     * @param author Author of this version (default: "system")
     * @param branch Branch name (default: "main")
     * @param metadata Additional metadata
     * @return Version ID of the new commit
     */
    std::string commit(
        const std::string& prompt_id,
        const std::string& content,
        const std::string& message,
        const std::string& author = "system",
        const std::string& branch = "main",
        const nlohmann::json& metadata = {}
    );
    
    /**
     * @brief Get a specific version
     * @param version_id Version identifier
     * @return Version details, or nullopt if not found
     */
    std::optional<PromptVersion> getVersion(const std::string& version_id) const;
    
    /**
     * @brief Get version history for a prompt
     * @param prompt_id Prompt template ID
     * @param branch Branch name (empty = all branches)
     * @param limit Maximum number of versions (0 = all)
     * @return Vector of versions in reverse chronological order
     */
    std::vector<PromptVersion> getHistory(
        const std::string& prompt_id,
        const std::string& branch = "",
        size_t limit = 0
    ) const;
    
    /**
     * @brief Get the latest version for a prompt
     * @param prompt_id Prompt template ID
     * @param branch Branch name (default: "main")
     * @return Latest version, or nullopt if none exists
     */
    std::optional<PromptVersion> getLatest(
        const std::string& prompt_id,
        const std::string& branch = "main"
    ) const;
    
    /**
     * @brief Rollback to a specific version
     * @param prompt_id Prompt template ID
     * @param version_id Version to rollback to
     * @param message Commit message for rollback
     * @return New version ID after rollback
     */
    std::string rollback(
        const std::string& prompt_id,
        const std::string& version_id,
        const std::string& message = "Rollback to previous version"
    );
    
    /**
     * @brief Rollback N versions
     * @param prompt_id Prompt template ID
     * @param versions Number of versions to rollback (default: 1)
     * @param branch Branch name (default: "main")
     * @return New version ID after rollback, or empty if failed
     */
    std::string rollbackN(
        const std::string& prompt_id,
        size_t versions = 1,
        const std::string& branch = "main"
    );
    
    /**
     * @brief Create a new branch
     * @param prompt_id Prompt template ID
     * @param branch_name New branch name
     * @param from_version Version to branch from (empty = latest on main)
     * @return True if successful
     */
    bool createBranch(
        const std::string& prompt_id,
        const std::string& branch_name,
        const std::string& from_version = ""
    );
    
    /**
     * @brief List all branches for a prompt
     * @param prompt_id Prompt template ID
     * @return Vector of branch information
     */
    std::vector<BranchInfo> listBranches(const std::string& prompt_id) const;
    
    /**
     * @brief Get diff between two versions
     * @param version_a First version ID
     * @param version_b Second version ID
     * @return Diff information
     */
    PromptDiff diff(const std::string& version_a, const std::string& version_b) const;
    
    /**
     * @brief Merge a branch into another
     * @param prompt_id Prompt template ID
     * @param source_branch Branch to merge from
     * @param target_branch Branch to merge into (default: "main")
     * @param strategy Merge strategy ("auto", "ours", "theirs")
     * @param message Commit message for merge
     * @return Merge result
     */
    MergeResult merge(
        const std::string& prompt_id,
        const std::string& source_branch,
        const std::string& target_branch = "main",
        const std::string& strategy = "auto",
        const std::string& message = "Merge branches"
    );
    
    /**
     * @brief Get version genealogy (parent-child relationships)
     * @param prompt_id Prompt template ID
     * @return Map of version_id -> parent_version_id
     */
    std::unordered_map<std::string, std::string> getGenealogy(
        const std::string& prompt_id
    ) const;
    
    /**
     * @brief Tag a version
     * @param version_id Version to tag
     * @param tag_name Tag name (e.g., "v1.0", "production")
     * @return True if successful
     */
    bool tag(const std::string& version_id, const std::string& tag_name);
    
    /**
     * @brief Get version by tag
     * @param prompt_id Prompt template ID
     * @param tag_name Tag name
     * @return Version with this tag, or nullopt if not found
     */
    std::optional<PromptVersion> getByTag(
        const std::string& prompt_id,
        const std::string& tag_name
    ) const;
    
    /**
     * @brief List all tags for a prompt
     * @param prompt_id Prompt template ID
     * @return Map of tag_name -> version_id
     */
    std::unordered_map<std::string, std::string> listTags(
        const std::string& prompt_id
    ) const;
    
    /**
     * @brief Update performance score for a version
     * @param version_id Version identifier
     * @param score Performance score (0.0-1.0)
     */
    void updatePerformanceScore(const std::string& version_id, double score);
    
    /**
     * @brief Get statistics about version history
     * @param prompt_id Prompt template ID
     * @return JSON with statistics
     */
    nlohmann::json getStats(const std::string& prompt_id) const;

private:
    mutable std::mutex mutex_;
    
    // In-memory storage: version_id -> version
    std::unordered_map<std::string, PromptVersion> versions_;
    
    // Branch tracking: prompt_id -> branch_name -> head_version_id
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> branches_;
    
    // Tag tracking: prompt_id -> tag_name -> version_id
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> tags_;
    
    // Optional persistence
    RocksDBWrapper* db_ = nullptr;
    rocksdb::ColumnFamilyHandle* cf_ = nullptr;
    
    static constexpr const char* KEY_PREFIX_VERSION = "version:";
    static constexpr const char* KEY_PREFIX_BRANCH = "branch:";
    static constexpr const char* KEY_PREFIX_TAG = "tag:";
    
    /**
     * @brief Generate unique version ID (SHA-like hash)
     */
    std::string generateVersionId(
        const std::string& prompt_id,
        const std::string& content,
        const std::string& parent
    ) const;
    
    /**
     * @brief Persist version to RocksDB
     */
    void persistVersion(const PromptVersion& version);
    
    /**
     * @brief Persist branch info to RocksDB
     */
    void persistBranch(const std::string& prompt_id, const BranchInfo& branch);
    
    /**
     * @brief Load versions from RocksDB
     */
    void loadFromDB();
    
    /**
     * @brief Compute diff between two strings
     */
    PromptDiff computeDiff(
        const std::string& version_a_id,
        const std::string& content_a,
        const std::string& version_b_id,
        const std::string& content_b
    ) const;
    
    /**
     * @brief Attempt automatic merge
     */
    MergeResult autoMerge(
        const PromptVersion& base,
        const PromptVersion& source,
        const PromptVersion& target
    ) const;
};

} // namespace prompt_engineering
} // namespace themis
