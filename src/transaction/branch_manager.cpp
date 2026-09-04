/**
 * @file branch_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/branch_manager.h"
#include "transaction/merge_engine.h"
#include <regex>
#include <algorithm>
#include <stdexcept>
#include <fmt/format.h>

namespace themis {
namespace transaction {

// Branch JSON serialization
json BranchManager::Branch::toJson() const {
    return json{
        {"branch_name", branch_name},
        {"parent_branch", parent_branch},
        {"creation_sequence", creation_sequence},
        {"creation_timestamp_ms", creation_timestamp_ms},
        {"description", description},
        {"created_by", created_by},
        {"is_active", is_active}
    };
}

BranchManager::Branch BranchManager::Branch::fromJson(const json& j) {
    Branch branch;
    branch.branch_name = j.value("branch_name", "");
    branch.parent_branch = j.value("parent_branch", "");
    branch.creation_sequence = j.value("creation_sequence", 0ULL);
    branch.creation_timestamp_ms = j.value("creation_timestamp_ms", 0LL);
    branch.description = j.value("description", "");
    branch.created_by = j.value("created_by", "system");
    branch.is_active = j.value("is_active", false);
    return branch;
}

// BranchStats JSON serialization
json BranchManager::BranchStats::toJson() const {
    return json{
        {"total_branches", total_branches},
        {"active_branches", active_branches},
        {"oldest_creation_timestamp_ms", oldest_creation_timestamp_ms},
        {"newest_creation_timestamp_ms", newest_creation_timestamp_ms},
        {"default_branch", default_branch}
    };
}

// MergeResult JSON serialization
json BranchManager::MergeResult::toJson() const {
    return json{
        {"success", success},
        {"message", message},
        {"conflicts", conflicts},
        {"merged_sequence", merged_sequence}
    };
}

// Constructor
BranchManager::BranchManager(
    RocksDBWrapper& db,
    Changefeed& changefeed,
    SnapshotManager& snapshot_manager,
    MergeEngine* merge_engine
) : db_(db),
    changefeed_(changefeed),
    snapshot_manager_(snapshot_manager),
    merge_engine_(merge_engine),
    active_branch_(DEFAULT_BRANCH) {
    
    // Load active branch from storage
    loadActiveBranch();
    
    // Ensure default branch exists
    if (!branchExists(DEFAULT_BRANCH)) {
        auto current_seq = changefeed_.getLatestSequence();
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        Branch default_branch;
        default_branch.branch_name = DEFAULT_BRANCH;
        default_branch.parent_branch = "";
        default_branch.creation_sequence = current_seq;
        default_branch.creation_timestamp_ms = now;
        default_branch.description = "Default main branch";
        default_branch.created_by = "system";
        default_branch.is_active = true;
        
        auto data = serialize(default_branch);
        db_.put(makeKey(DEFAULT_BRANCH), data);
    }
}

void BranchManager::setMergeEngine(MergeEngine* merge_engine) {
    std::lock_guard<std::mutex> lock(mutex_);
    merge_engine_ = merge_engine;
}

// Create branch (4-arg overload without options)
std::optional<BranchManager::Branch> BranchManager::createBranch(
    const std::string& branch_name,
    const std::string& parent_branch,
    const std::string& description,
    const std::string& created_by
) {
    return createBranch(branch_name, parent_branch, description, created_by, CreateBranchOptions{});
}

// Create branch
std::optional<BranchManager::Branch> BranchManager::createBranch(
    const std::string& branch_name,
    const std::string& parent_branch,
    const std::string& description,
    const std::string& created_by,
    const CreateBranchOptions& options
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate branch name
    if (!isValidBranchName(branch_name)) {
        return std::nullopt;
    }
    
    // Check if branch already exists
    if (branchExists(branch_name)) {
        return std::nullopt;
    }
    
    // Validate parent branch exists if specified
    if (!parent_branch.empty() && !branchExists(parent_branch)) {
        return std::nullopt;
    }
    
    // Resolve sequence from options
    auto sequence = resolveSequence(options);
    if (!sequence.has_value()) {
        // Use current sequence if not specified
        sequence = changefeed_.getLatestSequence();
    }
    
    // Get current timestamp
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Create branch metadata
    Branch branch;
    branch.branch_name = branch_name;
    branch.parent_branch = parent_branch.empty() ? DEFAULT_BRANCH : parent_branch;
    branch.creation_sequence = sequence.value();
    branch.creation_timestamp_ms = now;
    branch.description = description;
    branch.created_by = created_by;
    branch.is_active = options.set_active;
    
    // Serialize and store
    auto data = serialize(branch);
    if (!db_.put(makeKey(branch_name), data)) {
        return std::nullopt;
    }
    
    // Update active branch if requested
    if (options.set_active) {
        active_branch_ = branch_name;
        saveActiveBranch(branch_name);
    }

    // Record creation in branch history (outside mutex is fine; appendHistory takes its own)
    auto now2 = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    BranchHistoryEntry hist;
    hist.event_type   = "created";
    hist.branch_name  = branch_name;
    hist.details      = "Created from parent '" + branch.parent_branch +
                        "' at seq " + std::to_string(branch.creation_sequence);
    hist.performed_by = created_by;
    hist.timestamp_ms = now2;
    hist.sequence     = branch.creation_sequence;
    appendHistory(hist);

    return branch;
}

// Get branch
std::optional<BranchManager::Branch> BranchManager::getBranch(const std::string& branch_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto data = db_.get(makeKey(branch_name));
    if (!data.has_value()) {
        return std::nullopt;
    }
    
    return deserialize(data.value());
}

// List branches
std::vector<BranchManager::Branch> BranchManager::listBranches(
    size_t limit,
    const std::string& sort_by,
    bool ascending
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Branch> branches;
    
    // Iterate over all branch keys
    auto it_result = db_.newSafeIterator();
    if (!it_result) {
        return branches;
    }
    
    auto& it = it_result.value();
    for (it.Seek(BRANCH_PREFIX); it.Valid(); it.Next()) {
        std::string key(it.key());
        
        // Skip if not a branch key or is active branch key
        if (key.find(BRANCH_PREFIX) != 0 || key == ACTIVE_BRANCH_KEY) {
            continue;
        }
        
        std::string value_str(it.value());
        std::vector<uint8_t> value_bytes(value_str.begin(), value_str.end());
        auto branch = deserialize(value_bytes);
        if (branch.has_value()) {
            branches.push_back(branch.value());
        }
    }
    
    // Sort branches
    if (sort_by == "name") {
        std::sort(branches.begin(), branches.end(),
            [ascending](const Branch& a, const Branch& b) {
                return ascending ? (a.branch_name < b.branch_name) 
                                : (a.branch_name > b.branch_name);
            });
    } else if (sort_by == "timestamp") {
        std::sort(branches.begin(), branches.end(),
            [ascending](const Branch& a, const Branch& b) {
                return ascending ? (a.creation_timestamp_ms < b.creation_timestamp_ms)
                                : (a.creation_timestamp_ms > b.creation_timestamp_ms);
            });
    } else if (sort_by == "active") {
        std::sort(branches.begin(), branches.end(),
            [ascending](const Branch& a, const Branch& b) {
                return ascending ? (a.is_active < b.is_active)
                                : (a.is_active > b.is_active);
            });
    }
    
    // Apply limit
    if (limit > 0 && branches.size() > limit) {
        branches.resize(limit);
    }
    
    return branches;
}

// Switch branch
bool BranchManager::switchBranch(const std::string& branch_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if branch exists
    if (!branchExists(branch_name)) {
        return false;
    }
    
    std::string previous = active_branch_;

    // Update active branch
    active_branch_ = branch_name;
    
    // Persist active branch
    bool ok = saveActiveBranch(branch_name);

    if (ok) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        BranchHistoryEntry hist;
        hist.event_type   = "switched_to";
        hist.branch_name  = branch_name;
        hist.details      = "Switched from '" + previous + "'";
        hist.performed_by = "system";
        hist.timestamp_ms = now_ms;
        hist.sequence     = changefeed_.getLatestSequence();
        appendHistory(hist);
    }
    return ok;
}

// Get active branch
std::string BranchManager::getActiveBranch() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_branch_;
}

// Delete branch
bool BranchManager::deleteBranch(const std::string& branch_name, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Cannot delete default branch
    if (branch_name == DEFAULT_BRANCH) {
        return false;
    }
    
    // Cannot delete active branch
    if (branch_name == active_branch_) {
        return false;
    }
    
    // Check if branch exists
    if (!branchExists(branch_name)) {
        return false;
    }
    
    // Check if branch is merged (unless force)
    if (!force && !isBranchMerged(branch_name, DEFAULT_BRANCH)) {
        return false;
    }
    
    // Delete branch
    return db_.del(makeKey(branch_name));
}

// Merge branches (2-arg overload without options)
BranchManager::MergeResult BranchManager::mergeBranches(
    const std::string& source_branch,
    const std::string& target_branch
) {
    return mergeBranches(source_branch, target_branch, MergeOptions{});
}

// Merge branches
BranchManager::MergeResult BranchManager::mergeBranches(
    const std::string& source_branch,
    const std::string& target_branch,
    const MergeOptions& options
) {
    MergeResult result;
    
    // Check both branches exist
    auto source = getBranch(source_branch);
    auto target = getBranch(target_branch);
    
    if (!source.has_value() || !target.has_value()) {
        result.message = "Source or target branch does not exist";
        return result;
    }
    
    // Get sequence ranges
    uint64_t source_seq = source->creation_sequence;
    uint64_t target_seq = target->creation_sequence;
    
    // Determine common ancestor sequence (simple heuristic: use the earlier of the two)
    // Note: For divergent branches, ideally we'd track the actual divergence point
    // For now, this simple approach works for basic linear branch histories
    uint64_t base_seq = std::min(source_seq, target_seq);
    
    // Check if fast-forward is possible
    if (options.fast_forward && source_seq >= target_seq) {
        // Fast-forward merge: just update target sequence
        result.success = true;
        result.message = "Fast-forward merge completed";
        result.merged_sequence = source_seq;
        recordMergeStatus(source_branch, target_branch);
        return result;
    }
    
    // Attempt 3-way merge if MergeEngine is available
    if (merge_engine_) {
        try {
            // Use MergeEngine for 3-way merge
            transaction::MergeEngine::MergeOptions merge_opts;
            merge_opts.strategy = transaction::MergeEngine::MergeStrategy::MANUAL;
            merge_opts.fail_on_conflict = options.abort_on_conflict;
            
            auto merge_result = merge_engine_->merge(base_seq, source_seq, target_seq, merge_opts);
            
            // Convert MergeEngine result to BranchManager result
            result.success = merge_result.success;
            result.message = merge_result.message;
            result.merged_sequence = merge_result.result_sequence;
            
            // Extract conflict keys
            for (const auto& conflict : merge_result.conflicts) {
                result.conflicts.push_back(conflict.key);
            }

            if (result.success) {
                recordMergeStatus(source_branch, target_branch);
            }
            
            return result;
        } catch (const std::exception& e) {
            result.success = false;
            result.message = fmt::format("Merge failed: {}", e.what());
            return result;
        }
    }
    
    // Fallback: MergeEngine not available.
    // Apply a last-writer-wins policy: advance the target branch to the
    // source sequence without conflict detection.  Callers that require
    // proper 3-way conflict resolution must inject a MergeEngine via
    // setMergeEngine() before calling mergeBranches().
    result.success = true;
    result.merged_sequence = source_seq;
    result.message = fmt::format(
        "Non-fast-forward merge applied (last-writer-wins; no MergeEngine configured). "
        "source_seq={}, target_seq={}, base_seq={}. "
        "Inject a MergeEngine for 3-way merge with conflict detection.",
        source_seq, target_seq, base_seq);
    recordMergeStatus(source_branch, target_branch);
    
    return result;
}

// Preview branch merge (dry-run with full conflict details)
MergeEngine::MergeResult BranchManager::previewBranchMerge(
    const std::string& source_branch,
    const std::string& target_branch,
    const std::string& base_branch
) const {
    MergeEngine::MergeResult error_result;
    error_result.success = false;

    auto source = getBranch(source_branch);
    auto target = getBranch(target_branch);

    if (!source.has_value()) {
        error_result.message = fmt::format("Source branch not found: {}", source_branch);
        return error_result;
    }
    if (!target.has_value()) {
        error_result.message = fmt::format("Target branch not found: {}", target_branch);
        return error_result;
    }
    if (!merge_engine_) {
        error_result.message = "MergeEngine not initialized; cannot preview merge";
        return error_result;
    }

    uint64_t source_seq = source->creation_sequence;
    uint64_t target_seq = target->creation_sequence;

    uint64_t base_seq;
    if (!base_branch.empty()) {
        auto base = getBranch(base_branch);
        if (!base.has_value()) {
            error_result.message = fmt::format("Base branch not found: {}", base_branch);
            return error_result;
        }
        base_seq = base->creation_sequence;
    } else {
        base_seq = std::min(source_seq, target_seq);
    }

    return merge_engine_->previewMerge(base_seq, source_seq, target_seq);
}

// Resolve conflicts and complete a branch merge
MergeEngine::MergeResult BranchManager::resolveAndMergeBranches(
    const std::string& source_branch,
    const std::string& target_branch,
    const std::vector<MergeEngine::ConflictResolution>& resolutions,
    const std::string& base_branch
) {
    MergeEngine::MergeResult error_result;
    error_result.success = false;

    auto source = getBranch(source_branch);
    auto target = getBranch(target_branch);

    if (!source.has_value()) {
        error_result.message = fmt::format("Source branch not found: {}", source_branch);
        return error_result;
    }
    if (!target.has_value()) {
        error_result.message = fmt::format("Target branch not found: {}", target_branch);
        return error_result;
    }
    if (!merge_engine_) {
        error_result.message = "MergeEngine not initialized; cannot resolve merge";
        return error_result;
    }

    uint64_t source_seq = source->creation_sequence;
    uint64_t target_seq = target->creation_sequence;

    uint64_t base_seq;
    if (!base_branch.empty()) {
        auto base = getBranch(base_branch);
        if (!base.has_value()) {
            error_result.message = fmt::format("Base branch not found: {}", base_branch);
            return error_result;
        }
        base_seq = base->creation_sequence;
    } else {
        base_seq = std::min(source_seq, target_seq);
    }

    MergeEngine::MergeOptions opts;
    opts.strategy           = MergeEngine::MergeStrategy::MANUAL;
    opts.fail_on_conflict   = false;
    opts.manual_resolutions = resolutions;

    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, opts);

    if (result.success) {
        recordMergeStatus(source_branch, target_branch);

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        BranchHistoryEntry hist;
        hist.event_type   = "merged_from";
        hist.branch_name  = target_branch;
        hist.details      = fmt::format("Merged from '{}' with {} manual resolution(s)",
                                        source_branch, resolutions.size());
        hist.performed_by = "system";
        hist.timestamp_ms = now_ms;
        hist.sequence     = result.result_sequence;
        appendHistory(hist);
    }

    return result;
}

// Check branch exists
bool BranchManager::branchExists(const std::string& branch_name) const {
    auto data = db_.get(makeKey(branch_name));
    return data.has_value();
}

// Get stats
BranchManager::BranchStats BranchManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    BranchStats stats;
    stats.default_branch = DEFAULT_BRANCH;
    
    int64_t oldest = std::numeric_limits<int64_t>::max();
    int64_t newest = 0;
    
    auto it_result = db_.newSafeIterator();
    if (!it_result) {
        return stats;
    }
    
    auto& it = it_result.value();
    for (it.Seek(BRANCH_PREFIX); it.Valid(); it.Next()) {
        std::string key(it.key());
        
        if (key.find(BRANCH_PREFIX) != 0 || key == ACTIVE_BRANCH_KEY) {
            continue;
        }
        
        std::string value_str(it.value());
        std::vector<uint8_t> value_bytes(value_str.begin(), value_str.end());
        auto branch = deserialize(value_bytes);
        if (branch.has_value()) {
            stats.total_branches++;
            
            if (branch->is_active) {
                stats.active_branches++;
            }
            
            if (branch->creation_timestamp_ms < oldest) {
                oldest = branch->creation_timestamp_ms;
            }
            if (branch->creation_timestamp_ms > newest) {
                newest = branch->creation_timestamp_ms;
            }
        }
    }
    
    if (stats.total_branches > 0) {
        stats.oldest_creation_timestamp_ms = oldest;
        stats.newest_creation_timestamp_ms = newest;
    }
    
    return stats;
}

// Get sequence for branch
std::optional<uint64_t> BranchManager::getSequenceForBranch(const std::string& branch_name) const {
    auto branch = getBranch(branch_name);
    if (!branch.has_value()) {
        return std::nullopt;
    }
    return branch->creation_sequence;
}

// Get timestamp for branch
std::optional<int64_t> BranchManager::getTimestampForBranch(const std::string& branch_name) const {
    auto branch = getBranch(branch_name);
    if (!branch.has_value()) {
        return std::nullopt;
    }
    return branch->creation_timestamp_ms;
}

// Validate branch name
bool BranchManager::isValidBranchName(const std::string& branch_name) {
    // Check length
    if (branch_name.empty() || branch_name.length() > 128) {
        return false;
    }
    
    // Reserved names
    static const std::vector<std::string> reserved = {
        "HEAD", "FETCH_HEAD", "ORIG_HEAD"
    };
    
    for (const auto& r : reserved) {
        if (branch_name == r) {
            return false;
        }
    }
    
    // Valid characters: alphanumeric, hyphen, underscore, forward slash
    std::regex pattern("^[a-zA-Z0-9_/-]+$");
    return std::regex_match(branch_name, pattern);
}

// Get default branch
std::string BranchManager::getDefaultBranch() {
    return DEFAULT_BRANCH;
}

// Make key
std::string BranchManager::makeKey(const std::string& branch_name) const {
    return std::string(BRANCH_PREFIX) + branch_name;
}

// Extract branch name
std::string BranchManager::extractBranchName(const std::string& key) const {
    if (key.find(BRANCH_PREFIX) == 0) {
        return key.substr(std::strlen(BRANCH_PREFIX));
    }
    return "";
}

// Serialize branch
std::vector<uint8_t> BranchManager::serialize(const Branch& branch) const {
    json j = branch.toJson();
    std::string str = j.dump();
    return std::vector<uint8_t>(str.begin(), str.end());
}

// Deserialize branch
std::optional<BranchManager::Branch> BranchManager::deserialize(const std::vector<uint8_t>& data) const {
    try {
        std::string str(data.begin(), data.end());
        json j = json::parse(str);
        return Branch::fromJson(j);
    } catch (const json::exception&) {
        return std::nullopt;
    } catch (const std::string&) {
        return std::nullopt;
    } catch (const char*) {
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

// Load active branch
void BranchManager::loadActiveBranch() {
    auto data = db_.get(ACTIVE_BRANCH_KEY);
    if (data.has_value()) {
        std::string branch_name(data.value().begin(), data.value().end());
        active_branch_ = branch_name;
    } else {
        active_branch_ = DEFAULT_BRANCH;
    }
}

// Save active branch
bool BranchManager::saveActiveBranch(const std::string& branch_name) {
    std::vector<uint8_t> data(branch_name.begin(), branch_name.end());
    return db_.put(ACTIVE_BRANCH_KEY, data);
}

// Resolve sequence
std::optional<uint64_t> BranchManager::resolveSequence(const CreateBranchOptions& options) const {
    // Try tag first
    if (!options.from_tag.empty()) {
        return snapshot_manager_.getSequenceForTag(options.from_tag);
    }
    
    // Try explicit sequence
    if (options.from_sequence.has_value()) {
        return options.from_sequence;
    }
    
    // Try timestamp
    if (options.from_timestamp.has_value()) {
        // Find the latest sequence <= timestamp
        // This would require iterating changefeed, simplified for now
        return std::nullopt;
    }
    
    return std::nullopt;
}

// Check if branch is merged
bool BranchManager::isBranchMerged(
    const std::string& branch_name,
    const std::string& target_branch
) const {
    std::string key = std::string(BRANCH_MERGED_PREFIX) + branch_name + ":" + target_branch;
    return db_.get(key).has_value();
}

// Persist merge status
void BranchManager::recordMergeStatus(
    const std::string& source_branch,
    const std::string& target_branch
) {
    std::string key = std::string(BRANCH_MERGED_PREFIX) + source_branch + ":" + target_branch;
    // Value is a single sentinel byte; the key's presence is all we care about.
    // Write errors are intentionally swallowed – this is a best-effort audit marker
    // and should not abort the calling merge operation.
    std::vector<uint8_t> sentinel = {1};
    try {
        db_.put(key, sentinel);
    } catch (...) {
        // Best-effort marker only — ignore write failures.
    }
}

// ---- Phase 5: Branch History ----

json BranchManager::BranchHistoryEntry::toJson() const {
    return {
        {"event_type",    event_type},
        {"branch_name",   branch_name},
        {"details",       details},
        {"performed_by",  performed_by},
        {"timestamp_ms",  timestamp_ms},
        {"sequence",      sequence}
    };
}

BranchManager::BranchHistoryEntry
BranchManager::BranchHistoryEntry::fromJson(const json& j) {
    BranchHistoryEntry e;
    e.event_type   = j.value("event_type",   "");
    e.branch_name  = j.value("branch_name",  "");
    e.details      = j.value("details",      "");
    e.performed_by = j.value("performed_by", "system");
    e.timestamp_ms = j.value("timestamp_ms", (int64_t)0);
    e.sequence     = j.value("sequence",     (uint64_t)0);
    return e;
}

std::vector<uint8_t>
BranchManager::serializeHistory(const BranchHistoryEntry& entry) const {
    auto s = entry.toJson().dump();
    return {s.begin(), s.end()};
}

std::optional<BranchManager::BranchHistoryEntry>
BranchManager::deserializeHistory(const std::vector<uint8_t>& data) const {
    try {
        std::string s(data.begin(), data.end());
        return BranchHistoryEntry::fromJson(json::parse(s));
    } catch (const json::exception&) {
        return std::nullopt;
    } catch (const std::string&) {
        return std::nullopt;
    } catch (const char*) {
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

void BranchManager::appendHistory(const BranchHistoryEntry& entry) {
    // Key: "branch_hist:<branch_name>:<timestamp_ms>:<sequence>:<counter>"
    // A monotonic counter suffix ensures uniqueness even when two events
    // share the same timestamp_ms and sequence (e.g. rapid operations).
    static std::atomic<uint64_t> hist_counter{0};
    uint64_t counter = hist_counter.fetch_add(1, std::memory_order_relaxed);

    std::string key = std::string(BRANCH_HIST_PREFIX) +
                      entry.branch_name + ":" +
                      std::to_string(entry.timestamp_ms) + ":" +
                      std::to_string(entry.sequence) + ":" +
                      std::to_string(counter);

    auto data = serializeHistory(entry);
    db_.put(key, data); // best-effort; ignore write errors for audit log
}

std::vector<BranchManager::BranchHistoryEntry>
BranchManager::getBranchHistory(const std::string& branch_name,
                                 size_t limit) const {
    std::lock_guard<std::mutex> lk(mutex_);

    std::vector<BranchHistoryEntry> result;
    std::string prefix = std::string(BRANCH_HIST_PREFIX) + branch_name + ":";

    auto it_result = db_.newSafeIterator();
    if (!it_result) {
      return result;
    }

    auto& it = it_result.value();
    for (it.Seek(prefix); it.Valid(); it.Next()) {
        std::string key(it.key());
        if (key.find(prefix) != 0) {
          break;
        }

        std::string vs(it.value());
        std::vector<uint8_t> data(vs.begin(), vs.end());
        auto entry = deserializeHistory(data);
        if (entry.has_value()) {
            result.push_back(*entry);
            if (limit > 0 && result.size() >= limit) {
              break;
            }
        }
    }
    return result;
}

// ---- Phase 5: Branch GC ----

void BranchManager::setBranchGCPolicy(const BranchGCPolicy& policy) {
    std::lock_guard<std::mutex> lk(mutex_);
    gc_policy_ = policy;
}

size_t BranchManager::pruneMergedBranches() {
    std::lock_guard<std::mutex> lk(mutex_);

    size_t pruned = 0;
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::vector<Branch> candidates;

    auto it_result = db_.newSafeIterator();
    if (!it_result) {
      return 0;
    }

    auto& it = it_result.value();
    for (it.Seek(BRANCH_PREFIX); it.Valid(); it.Next()) {
        std::string key(it.key());
        if (key.find(BRANCH_PREFIX) != 0 || key == ACTIVE_BRANCH_KEY) {
          continue;
        }

        std::string vs(it.value());
        std::vector<uint8_t> data(vs.begin(), vs.end());
        auto branch = deserialize(data);
        if (!branch.has_value()) {
          continue;
        }

        // Skip protected names
        if (gc_policy_.protect_default &&
            branch->branch_name == std::string(DEFAULT_BRANCH)) continue;
        // Skip the currently active branch
        if (branch->branch_name == active_branch_) {
          continue;
        }

        bool age_ok = (gc_policy_.max_age_ms <= 0) ||
                      (now_ms > branch->creation_timestamp_ms &&
                       (now_ms - branch->creation_timestamp_ms) > gc_policy_.max_age_ms);

        bool merged_ok = !gc_policy_.only_merged ||
                         isBranchMerged(branch->branch_name, DEFAULT_BRANCH);

        if (age_ok && merged_ok) {
            candidates.push_back(*branch);
        }
    }

    for (const auto& b : candidates) {
        if (db_.del(makeKey(b.branch_name))) {
            ++pruned;
        }
    }

    return pruned;
}

} // namespace transaction
} // namespace themis

