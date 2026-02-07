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
        auto current_seq = changefeed_.getCurrentSequence();
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
        sequence = changefeed_.getCurrentSequence();
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
    auto it = db_.newIterator();
    for (it->seek(BRANCH_PREFIX); it->valid(); it->next()) {
        std::string key = it->key();
        
        // Skip if not a branch key or is active branch key
        if (key.find(BRANCH_PREFIX) != 0 || key == ACTIVE_BRANCH_KEY) {
            continue;
        }
        
        auto branch = deserialize(it->value());
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
    
    // Update active branch
    active_branch_ = branch_name;
    
    // Persist active branch
    return saveActiveBranch(branch_name);
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
    return db_.remove(makeKey(branch_name));
}

// Merge branches
BranchManager::MergeResult BranchManager::mergeBranches(
    const std::string& source_branch,
    const std::string& target_branch,
    const MergeOptions& options
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
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
    
    // Determine common ancestor sequence (simpl: use the earlier of the two)
    uint64_t base_seq = std::min(source_seq, target_seq);
    
    // Check if fast-forward is possible
    if (options.fast_forward && source_seq >= target_seq) {
        // Fast-forward merge: just update target sequence
        result.success = true;
        result.message = "Fast-forward merge completed";
        result.merged_sequence = source_seq;
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
            
            return result;
        } catch (const std::exception& e) {
            result.success = false;
            result.message = fmt::format("Merge failed: {}", e.what());
            return result;
        }
    }
    
    // Fallback: MergeEngine not available
    result.success = false;
    result.message = "Non-fast-forward merge not yet implemented. "
                     "Use force merge or rebase source branch. "
                     "(MergeEngine not initialized)";
    
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
    
    auto it = db_.newIterator();
    for (it->seek(BRANCH_PREFIX); it->valid(); it->next()) {
        std::string key = it->key();
        
        if (key.find(BRANCH_PREFIX) != 0 || key == ACTIVE_BRANCH_KEY) {
            continue;
        }
        
        auto branch = deserialize(it->value());
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
    } catch (const std::exception&) {
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
    // Simplified: for now, assume branches are not merged unless explicitly tracked
    // A full implementation would check if all changes in branch_name
    // are also in target_branch via changefeed diff
    return false;
}

} // namespace transaction
} // namespace themis
