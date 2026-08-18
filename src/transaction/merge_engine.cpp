/**
 * @file merge_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=19, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/merge_engine.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <algorithm>
#include <unordered_set>

namespace themis {
namespace transaction {

// Conflict serialization
json MergeEngine::Conflict::toJson() const {
    json j;
    j["type"] = (type == ConflictType::MODIFY_MODIFY ? "modify_modify" :
                 type == ConflictType::DELETE_MODIFY ? "delete_modify" :
                 type == ConflictType::MODIFY_DELETE ? "modify_delete" : "delete_delete");
    j["key"] = key;
    
    if (base_value.has_value()) {
        j["base_value"] = *base_value;
    }
    if (source_value.has_value()) {
        j["source_value"] = *source_value;
    }
    if (target_value.has_value()) {
        j["target_value"] = *target_value;
    }
    
    j["source_sequence"] = source_sequence;
    j["target_sequence"] = target_sequence;
    
    return j;
}

MergeEngine::Conflict MergeEngine::Conflict::fromJson(const json& j) {
    Conflict c{};  // Explicit value-initialization: all members default-initialized
    
    std::string type_str = j["type"];
    if (type_str == "modify_modify") c.type = ConflictType::MODIFY_MODIFY;
    else if (type_str == "delete_modify") c.type = ConflictType::DELETE_MODIFY;
    else if (type_str == "modify_delete") c.type = ConflictType::MODIFY_DELETE;
    else c.type = ConflictType::DELETE_DELETE;
    
    c.key = j["key"];
    
    if (j.contains("base_value")) {
        c.base_value = j["base_value"].get<std::string>();
    }
    if (j.contains("source_value")) {
        c.source_value = j["source_value"].get<std::string>();
    }
    if (j.contains("target_value")) {
        c.target_value = j["target_value"].get<std::string>();
    }
    
    c.source_sequence = j["source_sequence"];
    c.target_sequence = j["target_sequence"];
    
    return c;
}

// ConflictResolution serialization
json MergeEngine::ConflictResolution::toJson() const {
    json j;
    j["key"] = key;
    if (resolved_value.has_value()) {
        j["resolved_value"] = *resolved_value;
    }
    return j;
}

MergeEngine::ConflictResolution MergeEngine::ConflictResolution::fromJson(const json& j) {
    ConflictResolution r{};  // Explicit value-initialization: all members default-initialized
    r.key = j["key"];
    if (j.contains("resolved_value")) {
        r.resolved_value = j["resolved_value"].get<std::string>();
    }
    return r;
}

// MergeOptions serialization
json MergeEngine::MergeOptions::toJson() const {
    json j;
    j["strategy"] = (strategy == MergeStrategy::OURS ? "ours" :
                     strategy == MergeStrategy::THEIRS ? "theirs" :
                     strategy == MergeStrategy::FAST_FORWARD ? "fast_forward" : "manual");
    j["dry_run"] = dry_run;
    j["fail_on_conflict"] = fail_on_conflict;
    
    json resolutions = json::array();
    for (const auto& res : manual_resolutions) {
        resolutions.push_back(res.toJson());
    }
    j["manual_resolutions"] = resolutions;
    
    return j;
}

MergeEngine::MergeOptions MergeEngine::MergeOptions::fromJson(const json& j) {
    MergeOptions opts{};  // Explicit value-initialization: all members default-initialized
    
    std::string strategy_str = j["strategy"];
    if (strategy_str == "ours") opts.strategy = MergeStrategy::OURS;
    else if (strategy_str == "theirs") opts.strategy = MergeStrategy::THEIRS;
    else if (strategy_str == "fast_forward") opts.strategy = MergeStrategy::FAST_FORWARD;
    else opts.strategy = MergeStrategy::MANUAL;
    
    opts.dry_run = j["dry_run"];
    opts.fail_on_conflict = j["fail_on_conflict"];
    
    if (j.contains("manual_resolutions")) {
        for (const auto& res_json : j["manual_resolutions"]) {
            opts.manual_resolutions.push_back(ConflictResolution::fromJson(res_json));
        }
    }
    
    return opts;
}

// MergeStats serialization
json MergeEngine::MergeStats::toJson() const {
    json j;
    j["changes_applied"] = changes_applied;
    j["conflicts_detected"] = conflicts_detected;
    j["conflicts_auto_resolved"] = conflicts_auto_resolved;
    j["conflicts_manual"] = conflicts_manual;
    j["has_conflicts"] = has_conflicts;
    j["is_fast_forward"] = is_fast_forward;
    return j;
}

MergeEngine::MergeStats MergeEngine::MergeStats::fromJson(const json& j) {
    MergeStats stats{};  // Explicit value-initialization: all members default-initialized
    stats.changes_applied         = j.value("changes_applied",         (size_t)0);
    stats.conflicts_detected      = j.value("conflicts_detected",      (size_t)0);
    stats.conflicts_auto_resolved = j.value("conflicts_auto_resolved", (size_t)0);
    stats.conflicts_manual        = j.value("conflicts_manual",        (size_t)0);
    stats.has_conflicts           = j.value("has_conflicts",           false);
    stats.is_fast_forward         = j.value("is_fast_forward",         false);
    return stats;
}

// MergeResult serialization
json MergeEngine::MergeResult::toJson() const {
    json j;
    j["success"] = success;
    j["message"] = message;
    j["stats"] = stats.toJson();
    
    json conflicts_arr = json::array();
    for (const auto& conflict : conflicts) {
        conflicts_arr.push_back(conflict.toJson());
    }
    j["conflicts"] = conflicts_arr;
    
    json changes_arr = json::array();
    for (const auto& change : changes_applied) {
        changes_arr.push_back(change.toJson());
    }
    j["changes_applied"] = changes_arr;
    
    j["base_sequence"] = base_sequence;
    j["source_sequence"] = source_sequence;
    j["target_sequence"] = target_sequence;
    j["result_sequence"] = result_sequence;
    
    return j;
}

MergeEngine::MergeResult MergeEngine::MergeResult::fromJson(const json& j) {
    MergeResult result;
    result.success = j["success"];
    result.message = j["message"];
    if (j.contains("stats")) {
        result.stats = MergeStats::fromJson(j["stats"]);
    }
    
    if (j.contains("conflicts")) {
        for (const auto& conflict_json : j["conflicts"]) {
            result.conflicts.push_back(Conflict::fromJson(conflict_json));
        }
    }
    
    if (j.contains("changes_applied")) {
        for (const auto& change_json : j["changes_applied"]) {
            result.changes_applied.push_back(analytics::DiffEngine::Change::fromJson(change_json));
        }
    }
    
    result.base_sequence = j["base_sequence"];
    result.source_sequence = j["source_sequence"];
    result.target_sequence = j["target_sequence"];
    result.result_sequence = j["result_sequence"];
    
    return result;
}

// Constructor
MergeEngine::MergeEngine(
    analytics::DiffEngine& diff_engine,
    SnapshotManager& snapshot_manager,
    Changefeed& changefeed)
    : diff_engine_(diff_engine),
      snapshot_manager_(snapshot_manager),
      changefeed_(changefeed) {
    spdlog::info("MergeEngine initialized");
}

// Main merge function (no-arg overload)
MergeEngine::MergeResult MergeEngine::merge(
    uint64_t base_sequence,
    uint64_t source_sequence,
    uint64_t target_sequence) {
    return merge(base_sequence, source_sequence, target_sequence, MergeOptions{});
}

// Main merge function
MergeEngine::MergeResult MergeEngine::merge(
    uint64_t base_sequence,
    uint64_t source_sequence,
    uint64_t target_sequence,
    const MergeOptions& options) {
    
    spdlog::info("Starting merge: base={}, source={}, target={}", 
                 base_sequence, source_sequence, target_sequence);
    
    MergeResult result;
    result.base_sequence = base_sequence;
    result.source_sequence = source_sequence;
    result.target_sequence = target_sequence;
    result.result_sequence = target_sequence;
    
    // Compute diffs from base to source and base to target
    analytics::DiffEngine::DiffOptions diff_opts;
    diff_opts.include_values = true;
    
    analytics::DiffEngine::DiffResult source_diff;
    source_diff.from_sequence = base_sequence;
    source_diff.to_sequence = source_sequence;
    if (source_sequence > base_sequence) {
        source_diff = diff_engine_.computeDiff(base_sequence, source_sequence, diff_opts);
    }

    analytics::DiffEngine::DiffResult target_diff;
    target_diff.from_sequence = base_sequence;
    target_diff.to_sequence = target_sequence;
    if (target_sequence > base_sequence) {
        target_diff = diff_engine_.computeDiff(base_sequence, target_sequence, diff_opts);
    }
    
    spdlog::debug("Source diff: {} changes, Target diff: {} changes",
                  source_diff.stats.total_changes, target_diff.stats.total_changes);
    
    // Check for fast-forward possibility
    if (target_diff.stats.total_changes == 0) {
        result.stats.is_fast_forward = true;
        result.success = true;
        result.message = "Fast-forward merge: target has no changes from base";
        spdlog::info("Fast-forward merge detected");
        
        // Apply all source changes
        for (const auto& change : source_diff.added) {
            result.changes_applied.push_back(change);
        }
        for (const auto& change : source_diff.modified) {
            result.changes_applied.push_back(change);
        }
        for (const auto& change : source_diff.deleted) {
            result.changes_applied.push_back(change);
        }
        
        result.stats.changes_applied = result.changes_applied.size();
        
        if (!options.dry_run) {
            result.result_sequence = applyChanges(result.changes_applied);
        }
        
        return result;
    }
    
    // Detect conflicts
    auto conflicts = detectConflicts(source_diff, target_diff, base_sequence);
    result.conflicts = conflicts;
    result.stats.conflicts_detected = conflicts.size();
    result.stats.has_conflicts = !conflicts.empty();
    
    spdlog::debug("Detected {} conflicts", conflicts.size());
    
    // If fail_on_conflict or fast_forward strategy with conflicts
    if ((options.fail_on_conflict || options.strategy == MergeStrategy::FAST_FORWARD) 
        && !conflicts.empty()) {
        result.success = false;
        result.message = fmt::format("Merge aborted: {} conflicts detected", conflicts.size());
        spdlog::warn("Merge aborted due to conflicts");
        return result;
    }
    
    // Resolve conflicts
    std::vector<analytics::DiffEngine::Change> resolved_changes;
    if (!conflicts.empty()) {
        resolved_changes = resolveConflicts(conflicts, options);
        
        // Check if all conflicts were resolved
        if (resolved_changes.size() < conflicts.size() && options.strategy == MergeStrategy::MANUAL) {
            result.success = false;
            result.message = "Merge requires manual conflict resolution";
            spdlog::warn("Unresolved conflicts remain");
            return result;
        }
    }
    
    // Collect non-conflicting changes from source
    std::unordered_set<std::string> conflict_keys;
    for (const auto& conflict : conflicts) {
        conflict_keys.insert(conflict.key);
    }
    
    auto addNonConflictingChanges = [&](const std::vector<analytics::DiffEngine::Change>& changes) {
        for (const auto& change : changes) {
            if (conflict_keys.find(change.key) == conflict_keys.end()) {
                result.changes_applied.push_back(change);
            }
        }
    };
    
    addNonConflictingChanges(source_diff.added);
    addNonConflictingChanges(source_diff.modified);
    addNonConflictingChanges(source_diff.deleted);
    
    // Add resolved conflicts
    result.changes_applied.insert(
        result.changes_applied.end(),
        resolved_changes.begin(),
        resolved_changes.end()
    );
    
    result.stats.changes_applied = result.changes_applied.size();
    result.stats.conflicts_manual = conflicts.size() - result.stats.conflicts_auto_resolved;
    
    // Apply changes if not dry-run
    if (!options.dry_run) {
        try {
            result.result_sequence = applyChanges(result.changes_applied);
            result.success = true;
            result.message = fmt::format("Merge successful: {} changes applied, {} conflicts resolved",
                                        result.stats.changes_applied, result.stats.conflicts_detected);
            spdlog::info("Merge completed successfully");
        } catch (const std::exception& e) {
            result.success = false;
            result.message = fmt::format("Merge failed: {}", e.what());
            spdlog::error("Merge failed: {}", e.what());
        }
    } else {
        result.success = true;
        result.message = "Dry-run: merge preview completed";
        spdlog::info("Dry-run merge completed");
    }
    
    return result;
}

// Merge by tag (no-arg overload)
MergeEngine::MergeResult MergeEngine::mergeByTag(
    const std::string& base_tag,
    const std::string& source_tag,
    const std::string& target_tag) {
    return mergeByTag(base_tag, source_tag, target_tag, MergeOptions{});
}

// Merge by tag
MergeEngine::MergeResult MergeEngine::mergeByTag(
    const std::string& base_tag,
    const std::string& source_tag,
    const std::string& target_tag,
    const MergeOptions& options) {
    
    spdlog::info("Merging by tags: base={}, source={}, target={}",
                 base_tag, source_tag, target_tag);
    
    // Resolve tags to sequences
    auto base_snapshot = snapshot_manager_.getTag(base_tag);
    if (!base_snapshot.has_value()) {
        MergeResult result;
        result.success = false;
        result.message = fmt::format("Base tag not found: {}", base_tag);
        spdlog::error("Base tag not found: {}", base_tag);
        return result;
    }
    
    auto source_snapshot = snapshot_manager_.getTag(source_tag);
    if (!source_snapshot.has_value()) {
        MergeResult result;
        result.success = false;
        result.message = fmt::format("Source tag not found: {}", source_tag);
        spdlog::error("Source tag not found: {}", source_tag);
        return result;
    }
    
    uint64_t target_sequence;
    if (target_tag == "current" || target_tag == "HEAD") {
        target_sequence = changefeed_.getLatestSequence();
    } else {
        auto target_snapshot = snapshot_manager_.getTag(target_tag);
        if (!target_snapshot.has_value()) {
            MergeResult result;
            result.success = false;
            result.message = fmt::format("Target tag not found: {}", target_tag);
            spdlog::error("Target tag not found: {}", target_tag);
            return result;
        }
        target_sequence = target_snapshot->sequence_number;
    }
    
    return merge(base_snapshot->sequence_number,
                 source_snapshot->sequence_number,
                 target_sequence,
                 options);
}

// Preview merge
MergeEngine::MergeResult MergeEngine::previewMerge(
    uint64_t base_sequence,
    uint64_t source_sequence,
    uint64_t target_sequence) {
    
    MergeOptions options;
    options.dry_run = true;
    return merge(base_sequence, source_sequence, target_sequence, options);
}

// Check if fast-forward is possible
bool MergeEngine::canFastForward(
    uint64_t base_sequence,
    [[maybe_unused]] uint64_t source_sequence,
    uint64_t target_sequence) {
    
    analytics::DiffEngine::DiffOptions opts;
    opts.include_values = false; // Don't need values for this check
    
    auto target_diff = diff_engine_.computeDiff(base_sequence, target_sequence, opts);
    return target_diff.stats.total_changes == 0;
}

// Detect conflicts
std::vector<MergeEngine::Conflict> MergeEngine::detectConflicts(
    const analytics::DiffEngine::DiffResult& source_diff,
    const analytics::DiffEngine::DiffResult& target_diff,
    uint64_t base_sequence) {
    
    std::vector<Conflict> conflicts;
    
    // Build maps for quick lookup
    std::unordered_map<std::string, const analytics::DiffEngine::Change*> source_map;
    std::unordered_map<std::string, const analytics::DiffEngine::Change*> target_map;
    
    auto addToMap = [](auto& map, const std::vector<analytics::DiffEngine::Change>& changes) {
        for (const auto& change : changes) {
            map[change.key] = &change;
        }
    };
    
    addToMap(source_map, source_diff.added);
    addToMap(source_map, source_diff.modified);
    addToMap(source_map, source_diff.deleted);
    
    addToMap(target_map, target_diff.added);
    addToMap(target_map, target_diff.modified);
    addToMap(target_map, target_diff.deleted);
    
    // Find overlapping keys
    for (const auto& [key, source_change] : source_map) {
        auto it = target_map.find(key);
        if (it != target_map.end()) {
            const auto* target_change = it->second;
            
            Conflict conflict;
            conflict.key = key;
            conflict.base_value = getValueAtSequence(key, base_sequence);
            conflict.source_value = source_change->new_value;
            conflict.target_value = target_change->new_value;
            conflict.source_sequence = source_change->sequence;
            conflict.target_sequence = target_change->sequence;
            
            // Determine conflict type
            bool source_deleted = source_change->type == analytics::DiffEngine::ChangeType::DELETED;
            bool target_deleted = target_change->type == analytics::DiffEngine::ChangeType::DELETED;
            
            if (source_deleted && target_deleted) {
                conflict.type = ConflictType::DELETE_DELETE;
            } else if (source_deleted && !target_deleted) {
                conflict.type = ConflictType::DELETE_MODIFY;
            } else if (!source_deleted && target_deleted) {
                conflict.type = ConflictType::MODIFY_DELETE;
            } else {
                conflict.type = ConflictType::MODIFY_MODIFY;
            }
            
            conflicts.push_back(conflict);
        }
    }
    
    return conflicts;
}

// Resolve conflicts
std::vector<analytics::DiffEngine::Change> MergeEngine::resolveConflicts(
    const std::vector<Conflict>& conflicts,
    const MergeOptions& options) {
    
    std::vector<analytics::DiffEngine::Change> resolved_changes;
    
    // Build manual resolution map
    std::unordered_map<std::string, const ConflictResolution*> resolution_map;
    for (const auto& res : options.manual_resolutions) {
        resolution_map[res.key] = &res;
    }
    
    for (const auto& conflict : conflicts) {
        // Check for manual resolution first
        auto res_it = resolution_map.find(conflict.key);
        if (res_it != resolution_map.end()) {
            analytics::DiffEngine::Change change;
            change.key = conflict.key;
            change.old_value = conflict.base_value;
            change.new_value = res_it->second->resolved_value;
            change.type = change.new_value.has_value() ? 
                         analytics::DiffEngine::ChangeType::MODIFIED : 
                         analytics::DiffEngine::ChangeType::DELETED;
            change.sequence = conflict.target_sequence + 1;
            resolved_changes.push_back(change);
            continue;
        }
        
        // Try auto-resolve
        if (isAutoResolvable(conflict)) {
            auto auto_resolved = autoResolve(conflict);
            if (auto_resolved.has_value()) {
                resolved_changes.push_back(*auto_resolved);
                continue;
            }
        }
        
        // Apply strategy
        analytics::DiffEngine::Change change;
        change.key = conflict.key;
        change.old_value = conflict.base_value;
        
        switch (options.strategy) {
            case MergeStrategy::OURS:
                change.new_value = conflict.target_value;
                break;
            case MergeStrategy::THEIRS:
                change.new_value = conflict.source_value;
                break;
            case MergeStrategy::MANUAL:
            case MergeStrategy::FAST_FORWARD:
                // Leave unresolved
                continue;
        }
        
        change.type = change.new_value.has_value() ? 
                     analytics::DiffEngine::ChangeType::MODIFIED : 
                     analytics::DiffEngine::ChangeType::DELETED;
        change.sequence = conflict.target_sequence + 1;
        resolved_changes.push_back(change);
    }
    
    return resolved_changes;
}

// Apply changes
uint64_t MergeEngine::applyChanges(
    const std::vector<analytics::DiffEngine::Change>& changes) {
    
    uint64_t result_sequence = changefeed_.getLatestSequence();
    
    for (const auto& change : changes) {
        Changefeed::ChangeEvent event;
        event.key = change.key;
        
        if (change.type == analytics::DiffEngine::ChangeType::DELETED) {
            event.type = Changefeed::ChangeEventType::EVENT_DELETE;
            event.value = std::nullopt;
        } else {
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.value = change.new_value;
        }
        
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        event.metadata = json::object();
        event.metadata["merge"] = true;
        event.metadata["original_sequence"] = change.sequence;
        
        auto recorded = changefeed_.recordEvent(event);
        result_sequence = recorded.sequence;
    }
    
    return result_sequence;
}

// Get value at sequence
std::optional<std::string> MergeEngine::getValueAtSequence(
    const std::string& key,
    uint64_t sequence) {
    
    // Query changefeed for key's value at sequence
    // Using configurable limit defined in header (DEFAULT_HISTORY_LIMIT)
    Changefeed::ListOptions opts;
    opts.from_sequence = 0;
    opts.limit = DEFAULT_HISTORY_LIMIT;
    
    auto events = changefeed_.listEvents(opts);
    
    std::optional<std::string> value;
    for (const auto& event : events) {
        if (event.sequence > sequence) {
            break;
        }
        if (event.key == key) {
            if (event.type == Changefeed::ChangeEventType::EVENT_DELETE) {
                value = std::nullopt;
            } else {
                value = event.value;
            }
        }
    }
    
    return value;
}

// Check if conflict is auto-resolvable
bool MergeEngine::isAutoResolvable(const Conflict& conflict) const {
    // DELETE_DELETE conflicts are auto-resolvable (both sides agree)
    if (conflict.type == ConflictType::DELETE_DELETE) {
        return true;
    }
    
    // If both sides made identical changes, auto-resolve
    if (conflict.source_value == conflict.target_value) {
        return true;
    }
    
    return false;
}

// Auto-resolve conflict
std::optional<analytics::DiffEngine::Change> MergeEngine::autoResolve(
    const Conflict& conflict) const {
    
    if (!isAutoResolvable(conflict)) {
        return std::nullopt;
    }
    
    analytics::DiffEngine::Change change;
    change.key = conflict.key;
    change.old_value = conflict.base_value;
    
    if (conflict.type == ConflictType::DELETE_DELETE) {
        change.new_value = std::nullopt;
        change.type = analytics::DiffEngine::ChangeType::DELETED;
    } else {
        // Both sides made identical changes
        change.new_value = conflict.source_value;
        change.type = analytics::DiffEngine::ChangeType::MODIFIED;
    }
    
    change.sequence = conflict.target_sequence + 1;
    
    return change;
}

} // namespace transaction
} // namespace themis
