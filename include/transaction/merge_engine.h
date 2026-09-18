/**
 * @file merge_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class MergeEngine {
public:
    enum class ConflictType {
        MODIFY_MODIFY,  // Both sides modified the same key
        DELETE_MODIFY,  // One side deleted, other modified
        MODIFY_DELETE,  // One side modified, other deleted
        DELETE_DELETE   // Both sides deleted (not a real conflict, auto-resolve)
    };

    enum class MergeStrategy {
        OURS,          // Prefer changes from target branch
        THEIRS,        // Prefer changes from source branch
        MANUAL,        // Require manual conflict resolution
        FAST_FORWARD   // Only merge if no conflicts (fail on conflict)
    };

    struct Conflict {
        ConflictType type;
        std::string key;                         // Conflicting key
        std::optional<std::string> base_value;   // Value at common ancestor
        std::optional<std::string> source_value; // Value in source branch
        std::optional<std::string> target_value; // Value in target branch
        uint64_t source_sequence;                // Source change sequence
        uint64_t target_sequence;                // Target change sequence
        
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
        static Conflict fromJson(const json& j);
    };

    struct ConflictResolution {
        std::string key;                         // Key to resolve
        std::optional<std::string> resolved_value; // Chosen value (nullopt = delete)
        
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
        static ConflictResolution fromJson(const json& j);
    };

    struct MergeOptions {
        MergeStrategy strategy = MergeStrategy::MANUAL;
        bool dry_run = false;                    // Preview mode, don't apply changes
        bool fail_on_conflict = false;           // Abort if conflicts detected
        std::vector<ConflictResolution> manual_resolutions; // Provided resolutions
        
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
        static MergeOptions fromJson(const json& j);
    };

    struct MergeStats {
        size_t changes_applied = 0;
        size_t conflicts_detected = 0;
        size_t conflicts_auto_resolved = 0;
        size_t conflicts_manual = 0;
        bool has_conflicts = false;
        bool is_fast_forward = false;
        
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
        static MergeStats fromJson(const json& j);
    };

    struct MergeResult {
        bool success = 0;
        std::string message;
        MergeStats stats;
        std::vector<Conflict> conflicts;         // Unresolved conflicts
        std::vector<analytics::DiffEngine::Change> changes_applied; // Applied changes
        
        uint64_t base_sequence;
        uint64_t source_sequence;
        uint64_t target_sequence;
        uint64_t result_sequence;                // Sequence after merge (if applied)
        
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
        static MergeResult fromJson(const json& j);
    };

    /**
     * @brief Merge Engine.
     * @param[in,out] diff_engine Input/output parameter.
     * @param[in,out] snapshot_manager Input/output parameter.
     * @param[in,out] changefeed Input/output parameter.
     * @return Return value.
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
     * @brief Merge.
     * @param[in] base_sequence Input parameter.
     * @param[in] source_sequence Input parameter.
     * @param[in] target_sequence Input parameter.
     * @return Return value.
     */
    MergeResult merge(
        uint64_t base_sequence,
        uint64_t source_sequence,
        uint64_t target_sequence
    );
    
    /**
     * @brief Merge.
     * @param[in] base_sequence Input parameter.
     * @param[in] source_sequence Input parameter.
     * @param[in] target_sequence Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    MergeResult merge(
        uint64_t base_sequence,
        uint64_t source_sequence,
        uint64_t target_sequence,
        const MergeOptions& options
    );

    /**
     * @brief Merge By Tag.
     * @param[in] base_tag Input parameter.
     * @param[in] source_tag Input parameter.
     * @param[in] target_tag Input parameter.
     * @return Return value.
     */
    MergeResult mergeByTag(
        const std::string& base_tag,
        const std::string& source_tag,
        const std::string& target_tag
    );
    
    /**
     * @brief Merge By Tag.
     * @param[in] base_tag Input parameter.
     * @param[in] source_tag Input parameter.
     * @param[in] target_tag Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    MergeResult mergeByTag(
        const std::string& base_tag,
        const std::string& source_tag,
        const std::string& target_tag,
        const MergeOptions& options
    );

    /**
     * @brief Preview Merge.
     * @param[in] base_sequence Input parameter.
     * @param[in] source_sequence Input parameter.
     * @param[in] target_sequence Input parameter.
     * @return Return value.
     */
    MergeResult previewMerge(
        uint64_t base_sequence,
        uint64_t source_sequence,
        uint64_t target_sequence
    );

    /**
     * @brief Can Fast Forward.
     * @param[in] base_sequence Input parameter.
     * @param[in] source_sequence Input parameter.
     * @param[in] target_sequence Input parameter.
     * @return True when the operation succeeds.
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
     * @brief Detect Conflicts.
     * @param[in] source_diff Input parameter.
     * @param[in] target_diff Input parameter.
     * @param[in] base_sequence Input parameter.
     * @return Return value.
     */
    std::vector<Conflict> detectConflicts(
        const analytics::DiffEngine::DiffResult& source_diff,
        const analytics::DiffEngine::DiffResult& target_diff,
        uint64_t base_sequence
    );

    /**
     * @brief Resolve Conflicts.
     * @param[in] conflicts Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<analytics::DiffEngine::Change> resolveConflicts(
        const std::vector<Conflict>& conflicts,
        const MergeOptions& options
    );

    /**
     * @brief Apply Changes.
     * @param[in] changes Input parameter.
     * @return Return value.
     */
    uint64_t applyChanges(
        const std::vector<analytics::DiffEngine::Change>& changes
    );

    /**
     * @brief Get Value At Sequence.
     * @param[in] key Input parameter.
     * @param[in] sequence Input parameter.
     * @return Return value.
     */
    std::optional<std::string> getValueAtSequence(
        const std::string& key,
        uint64_t sequence
    );

    /**
     * @brief Is Auto Resolvable.
     * @param[in] conflict Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAutoResolvable(const Conflict& conflict) const;

    /**
     * @brief Auto Resolve.
     * @param[in] conflict Input parameter.
     * @return Return value.
     */
    std::optional<analytics::DiffEngine::Change> autoResolve(
        const Conflict& conflict
    ) const;
};

} // namespace transaction
} // namespace themis
