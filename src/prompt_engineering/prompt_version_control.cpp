/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_version_control.cpp                         ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:01:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     856                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 235d2ca7f  2026-02-10  Refactor tests and update dependencies   ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_version_control.cpp
 * @brief Implementation of version control system for prompts
 */

#include "prompt_engineering/prompt_version_control.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <unordered_set>
#include <openssl/sha.h>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// PromptVersion Implementation
// ============================================================================

nlohmann::json PromptVersion::toJson() const {
    nlohmann::json j;
    j["version_id"] = version_id;
    j["prompt_id"] = prompt_id;
    j["branch"] = branch;
    j["content"] = content;
    j["commit_message"] = commit_message;
    j["author"] = author;
    j["parent_version"] = parent_version;
    j["metadata"] = metadata;
    j["performance_score"] = performance_score;
    
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    j["timestamp"] = time;
    
    return j;
}

PromptVersion PromptVersion::fromJson(const nlohmann::json& j) {
    PromptVersion v;
    v.version_id = j.value("version_id", "");
    v.prompt_id = j.value("prompt_id", "");
    v.branch = j.value("branch", "main");
    v.content = j.value("content", "");
    v.commit_message = j.value("commit_message", "");
    v.author = j.value("author", "system");
    v.parent_version = j.value("parent_version", "");
    v.metadata = j.value("metadata", nlohmann::json::object());
    v.performance_score = j.value("performance_score", 0.0);
    
    if (j.contains("timestamp")) {
        auto time_val = j["timestamp"].get<std::time_t>();
        v.timestamp = std::chrono::system_clock::from_time_t(time_val);
    }
    
    return v;
}

// ============================================================================
// PromptDiff Implementation
// ============================================================================

nlohmann::json PromptDiff::toJson() const {
    nlohmann::json j;
    j["version_a"] = version_a;
    j["version_b"] = version_b;
    j["added_lines"] = added_lines;
    j["removed_lines"] = removed_lines;
    j["modified_lines"] = modified_lines;
    j["unified_diff"] = unified_diff;
    j["additions"] = additions;
    j["deletions"] = deletions;
    return j;
}

// ============================================================================
// BranchInfo Implementation
// ============================================================================

nlohmann::json BranchInfo::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["head_version"] = head_version;
    j["base_version"] = base_version;
    j["commit_count"] = commit_count;
    j["is_merged"] = is_merged;
    
    auto time = std::chrono::system_clock::to_time_t(created_at);
    j["created_at"] = time;
    
    return j;
}

// ============================================================================
// MergeResult Implementation
// ============================================================================

nlohmann::json MergeResult::toJson() const {
    nlohmann::json j;
    j["success"] = success;
    j["merged_version_id"] = merged_version_id;
    j["conflicts"] = conflicts;
    j["merged_content"] = merged_content;
    j["strategy_used"] = strategy_used;
    return j;
}

// ============================================================================
// PromptVersionControl Implementation
// ============================================================================

PromptVersionControl::PromptVersionControl() = default;

PromptVersionControl::PromptVersionControl(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {
    if (db_) {
        loadFromDB();
    }
}

std::string PromptVersionControl::commit(
    const std::string& prompt_id,
    const std::string& content,
    const std::string& message,
    const std::string& author,
    const std::string& branch,
    const nlohmann::json& metadata
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get parent version (current HEAD of branch)
    std::string parent_version;
    if (branches_[prompt_id].count(branch)) {
        parent_version = branches_[prompt_id][branch];
    }
    
    // Generate version ID
    std::string version_id = generateVersionId(prompt_id, content, parent_version);
    
    // Create version
    PromptVersion version;
    version.version_id = version_id;
    version.prompt_id = prompt_id;
    version.branch = branch;
    version.content = content;
    version.commit_message = message;
    version.author = author;
    version.parent_version = parent_version;
    version.timestamp = std::chrono::system_clock::now();
    version.metadata = metadata;
    
    // Store version
    versions_[version_id] = version;
    
    // Update branch HEAD
    branches_[prompt_id][branch] = version_id;
    
    // Persist if DB available
    if (db_) {
        persistVersion(version);
        
        BranchInfo branch_info;
        branch_info.name = branch;
        branch_info.head_version = version_id;
        branch_info.created_at = version.timestamp;
        persistBranch(prompt_id, branch_info);
    }
    
    THEMIS_DEBUG("Committed version {} for prompt '{}' on branch '{}'",
                 version_id.substr(0, 8), prompt_id, branch);
    
    return version_id;
}

std::optional<PromptVersion> PromptVersionControl::getVersion(
    const std::string& version_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = versions_.find(version_id);
    if (it != versions_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<PromptVersion> PromptVersionControl::getHistory(
    const std::string& prompt_id,
    const std::string& branch,
    size_t limit
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<PromptVersion> history;
    
    // Collect all versions for this prompt
    for (const auto& [version_id, version] : versions_) {
        if (version.prompt_id == prompt_id) {
            // Filter by branch if specified
            if (!branch.empty() && version.branch != branch) {
                continue;
            }
            history.push_back(version);
        }
    }
    
    // Sort by timestamp (descending)
    std::sort(history.begin(), history.end(),
              [](const auto& a, const auto& b) {
                  return a.timestamp > b.timestamp;
              });
    
    // Apply limit if specified
    if (limit > 0 && history.size() > limit) {
        history.resize(limit);
    }
    
    return history;
}

std::optional<PromptVersion> PromptVersionControl::getLatest(
    const std::string& prompt_id,
    const std::string& branch
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto prompt_it = branches_.find(prompt_id);
    if (prompt_it == branches_.end()) {
        return std::nullopt;
    }
    
    auto branch_it = prompt_it->second.find(branch);
    if (branch_it == prompt_it->second.end()) {
        return std::nullopt;
    }
    
    std::string version_id = branch_it->second;
    auto version_it = versions_.find(version_id);
    if (version_it != versions_.end()) {
        return version_it->second;
    }
    
    return std::nullopt;
}

std::string PromptVersionControl::rollback(
    const std::string& prompt_id,
    const std::string& version_id,
    const std::string& message
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get the target version
    auto it = versions_.find(version_id);
    if (it == versions_.end()) {
        THEMIS_ERROR("Version {} not found for rollback", version_id);
        return "";
    }
    
    const auto& target_version = it->second;
    
    // Create new version with rolled-back content
    std::string new_version_id = generateVersionId(
        prompt_id,
        target_version.content,
        branches_[prompt_id][target_version.branch]
    );
    
    PromptVersion new_version;
    new_version.version_id = new_version_id;
    new_version.prompt_id = prompt_id;
    new_version.branch = target_version.branch;
    new_version.content = target_version.content;
    new_version.commit_message = message + " (rollback to " + version_id.substr(0, 8) + ")";
    new_version.author = "system";
    new_version.parent_version = branches_[prompt_id][target_version.branch];
    new_version.timestamp = std::chrono::system_clock::now();
    new_version.metadata = {{"rollback_to", version_id}};
    
    // Store and update branch
    versions_[new_version_id] = new_version;
    branches_[prompt_id][target_version.branch] = new_version_id;
    
    if (db_) {
        persistVersion(new_version);
    }
    
    THEMIS_INFO("Rolled back prompt '{}' to version {}",
                prompt_id, version_id.substr(0, 8));
    
    return new_version_id;
}

std::string PromptVersionControl::rollbackN(
    const std::string& prompt_id,
    size_t versions,
    const std::string& branch
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get current HEAD
    auto prompt_it = branches_.find(prompt_id);
    if (prompt_it == branches_.end() || !prompt_it->second.count(branch)) {
        THEMIS_ERROR("Branch '{}' not found for prompt '{}'", branch, prompt_id);
        return "";
    }
    
    std::string current_id = prompt_it->second[branch];
    
    // Walk back N versions
    for (size_t i = 0; i < versions; ++i) {
        auto it = versions_.find(current_id);
        if (it == versions_.end() || it->second.parent_version.empty()) {
            THEMIS_WARN("Cannot rollback {} versions, only {} available", versions, i);
            break;
        }
        current_id = it->second.parent_version;
    }
    
    // Unlock for recursive call
    mutex_.unlock();
    
    std::string message = "Rollback " + std::to_string(versions) + " version(s)";
    return rollback(prompt_id, current_id, message);
}

bool PromptVersionControl::createBranch(
    const std::string& prompt_id,
    const std::string& branch_name,
    const std::string& from_version
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if branch already exists
    if (branches_[prompt_id].count(branch_name)) {
        THEMIS_WARN("Branch '{}' already exists for prompt '{}'", branch_name, prompt_id);
        return false;
    }
    
    // Determine base version
    std::string base_version = from_version;
    if (base_version.empty()) {
        // Use latest on main branch
        if (branches_[prompt_id].count("main")) {
            base_version = branches_[prompt_id]["main"];
        } else {
            THEMIS_ERROR("No base version available for branching");
            return false;
        }
    }
    
    // Verify base version exists
    if (!versions_.count(base_version)) {
        THEMIS_ERROR("Base version {} not found", base_version);
        return false;
    }
    
    // Create branch pointing to base version
    branches_[prompt_id][branch_name] = base_version;
    
    if (db_) {
        BranchInfo branch_info;
        branch_info.name = branch_name;
        branch_info.head_version = base_version;
        branch_info.base_version = base_version;
        branch_info.created_at = std::chrono::system_clock::now();
        persistBranch(prompt_id, branch_info);
    }
    
    THEMIS_INFO("Created branch '{}' for prompt '{}' from version {}",
                branch_name, prompt_id, base_version.substr(0, 8));
    
    return true;
}

std::vector<BranchInfo> PromptVersionControl::listBranches(
    const std::string& prompt_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<BranchInfo> result;
    
    auto it = branches_.find(prompt_id);
    if (it == branches_.end()) {
        return result;
    }
    
    for (const auto& [branch_name, head_version] : it->second) {
        BranchInfo info;
        info.name = branch_name;
        info.head_version = head_version;
        
        // Count commits on this branch
        std::string current = head_version;
        while (!current.empty()) {
            info.commit_count++;
            auto v_it = versions_.find(current);
            if (v_it == versions_.end()) break;
            current = v_it->second.parent_version;
        }
        
        // Get creation time (from first version)
        auto v_it = versions_.find(head_version);
        if (v_it != versions_.end()) {
            info.created_at = v_it->second.timestamp;
        }
        
        result.push_back(info);
    }
    
    // Sort by name
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.name < b.name; });
    
    return result;
}

PromptDiff PromptVersionControl::diff(
    const std::string& version_a,
    const std::string& version_b
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it_a = versions_.find(version_a);
    auto it_b = versions_.find(version_b);
    
    if (it_a == versions_.end() || it_b == versions_.end()) {
        THEMIS_ERROR("One or both versions not found for diff");
        return {};
    }
    
    return computeDiff(version_a, it_a->second.content,
                      version_b, it_b->second.content);
}

MergeResult PromptVersionControl::merge(
    const std::string& prompt_id,
    const std::string& source_branch,
    const std::string& target_branch,
    const std::string& strategy,
    const std::string& message
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    MergeResult result;
    result.strategy_used = strategy;
    
    // Get branch HEADs
    auto prompt_it = branches_.find(prompt_id);
    if (prompt_it == branches_.end()) {
        result.conflicts.push_back("Prompt not found");
        return result;
    }
    
    if (!prompt_it->second.count(source_branch) || !prompt_it->second.count(target_branch)) {
        result.conflicts.push_back("One or both branches not found");
        return result;
    }
    
    std::string source_id = prompt_it->second[source_branch];
    std::string target_id = prompt_it->second[target_branch];
    
    auto source_it = versions_.find(source_id);
    auto target_it = versions_.find(target_id);
    
    if (source_it == versions_.end() || target_it == versions_.end()) {
        result.conflicts.push_back("Version data not found");
        return result;
    }
    
    const auto& source_version = source_it->second;
    const auto& target_version = target_it->second;
    
    // Find common ancestor (simplified: use target's parent)
    std::string base_id = target_version.parent_version;
    
    // Apply merge strategy
    if (strategy == "ours") {
        result.merged_content = target_version.content;
        result.success = true;
    } else if (strategy == "theirs") {
        result.merged_content = source_version.content;
        result.success = true;
    } else if (strategy == "auto") {
        // Simple auto-merge: if no common ancestor, use source
        if (base_id.empty()) {
            result.merged_content = source_version.content;
            result.success = true;
        } else {
            auto base_it = versions_.find(base_id);
            if (base_it != versions_.end()) {
                result = autoMerge(base_it->second, source_version, target_version);
            } else {
                result.merged_content = source_version.content;
                result.success = true;
            }
        }
    }
    
    // If successful, create merge commit
    if (result.success) {
        std::string merge_message = message + 
            " (merge " + source_branch + " into " + target_branch + ")";
        
        std::string merge_version_id = generateVersionId(
            prompt_id,
            result.merged_content,
            target_id
        );
        
        PromptVersion merge_version;
        merge_version.version_id = merge_version_id;
        merge_version.prompt_id = prompt_id;
        merge_version.branch = target_branch;
        merge_version.content = result.merged_content;
        merge_version.commit_message = merge_message;
        merge_version.author = "system";
        merge_version.parent_version = target_id;
        merge_version.timestamp = std::chrono::system_clock::now();
        merge_version.metadata = {
            {"merge", true},
            {"source_branch", source_branch},
            {"source_version", source_id}
        };
        
        versions_[merge_version_id] = merge_version;
        branches_[prompt_id][target_branch] = merge_version_id;
        
        if (db_) {
            persistVersion(merge_version);
        }
        
        result.merged_version_id = merge_version_id;
        
        THEMIS_INFO("Merged branch '{}' into '{}' for prompt '{}'",
                    source_branch, target_branch, prompt_id);
    }
    
    return result;
}

std::unordered_map<std::string, std::string> PromptVersionControl::getGenealogy(
    const std::string& prompt_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<std::string, std::string> genealogy;
    
    for (const auto& [version_id, version] : versions_) {
        if (version.prompt_id == prompt_id) {
            genealogy[version_id] = version.parent_version;
        }
    }
    
    return genealogy;
}

bool PromptVersionControl::tag(const std::string& version_id, const std::string& tag_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = versions_.find(version_id);
    if (it == versions_.end()) {
        THEMIS_ERROR("Version {} not found for tagging", version_id);
        return false;
    }
    
    const std::string& prompt_id = it->second.prompt_id;
    tags_[prompt_id][tag_name] = version_id;
    
    THEMIS_INFO("Tagged version {} as '{}'", version_id.substr(0, 8), tag_name);
    
    return true;
}

std::optional<PromptVersion> PromptVersionControl::getByTag(
    const std::string& prompt_id,
    const std::string& tag_name
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto prompt_it = tags_.find(prompt_id);
    if (prompt_it == tags_.end()) {
        return std::nullopt;
    }
    
    auto tag_it = prompt_it->second.find(tag_name);
    if (tag_it == prompt_it->second.end()) {
        return std::nullopt;
    }
    
    return getVersion(tag_it->second);
}

std::unordered_map<std::string, std::string> PromptVersionControl::listTags(
    const std::string& prompt_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tags_.find(prompt_id);
    if (it != tags_.end()) {
        return it->second;
    }
    
    return {};
}

void PromptVersionControl::updatePerformanceScore(
    const std::string& version_id,
    double score
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = versions_.find(version_id);
    if (it != versions_.end()) {
        it->second.performance_score = std::max(0.0, std::min(1.0, score));
        
        if (db_) {
            persistVersion(it->second);
        }
    }
}

nlohmann::json PromptVersionControl::getStats(const std::string& prompt_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json stats;
    stats["prompt_id"] = prompt_id;
    
    // Count versions
    size_t total_versions = 0;
    for (const auto& [_, version] : versions_) {
        if (version.prompt_id == prompt_id) {
            total_versions++;
        }
    }
    stats["total_versions"] = total_versions;
    
    // Count branches
    auto branch_it = branches_.find(prompt_id);
    if (branch_it != branches_.end()) {
        stats["branch_count"] = branch_it->second.size();
        stats["branches"] = nlohmann::json::array();
        for (const auto& [name, _] : branch_it->second) {
            stats["branches"].push_back(name);
        }
    } else {
        stats["branch_count"] = 0;
    }
    
    // Count tags
    auto tag_it = tags_.find(prompt_id);
    if (tag_it != tags_.end()) {
        stats["tag_count"] = tag_it->second.size();
    } else {
        stats["tag_count"] = 0;
    }
    
    return stats;
}

// ============================================================================
// Private Methods
// ============================================================================

std::string PromptVersionControl::generateVersionId(
    const std::string& prompt_id,
    const std::string& content,
    const std::string& parent
) const {
    // Create SHA256 hash
    std::string input = prompt_id + content + parent + 
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    
    // Convert to hex string (first 16 bytes = 32 hex chars, like git)
    std::ostringstream oss;
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

void PromptVersionControl::persistVersion(const PromptVersion& version) {
    if (!db_) return;
    
    std::string key = std::string(KEY_PREFIX_VERSION) + version.version_id;
    std::string value = version.toJson().dump();
    std::vector<uint8_t> bytes(value.begin(), value.end());
    
    if (!db_->put(key, bytes)) {
        THEMIS_ERROR("Failed to persist version: {}", version.version_id);
    }
}

void PromptVersionControl::persistBranch(
    const std::string& prompt_id,
    const BranchInfo& branch
) {
    if (!db_) return;
    
    std::string key = std::string(KEY_PREFIX_BRANCH) + prompt_id + ":" + branch.name;
    std::string value = branch.toJson().dump();
    std::vector<uint8_t> bytes(value.begin(), value.end());
    
    if (!db_->put(key, bytes)) {
        THEMIS_ERROR("Failed to persist branch: {}", branch.name);
    }
}

void PromptVersionControl::loadFromDB() {
    if (!db_) return;
    
    size_t loaded_versions = 0;
    size_t loaded_branches = 0;
    
    // Load versions
    db_->scanPrefix(KEY_PREFIX_VERSION, [this, &loaded_versions](std::string_view key, std::string_view value) -> bool {
        try {
            auto j = nlohmann::json::parse(std::string(value));
            auto version = PromptVersion::fromJson(j);
            versions_[version.version_id] = version;
            
            // Update branch tracking
            branches_[version.prompt_id][version.branch] = version.version_id;
            
            loaded_versions++;
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse version from DB: {}", e.what());
        }
        return true;
    });
    
    THEMIS_INFO("Loaded {} versions from DB", loaded_versions);
}

PromptDiff PromptVersionControl::computeDiff(
    const std::string& version_a_id,
    const std::string& content_a,
    const std::string& version_b_id,
    const std::string& content_b
) const {
    PromptDiff diff;
    diff.version_a = version_a_id;
    diff.version_b = version_b_id;
    
    // Split into lines
    auto split_lines = [](const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }
        return lines;
    };
    
    auto lines_a = split_lines(content_a);
    auto lines_b = split_lines(content_b);
    
    // Simple line-by-line diff
    std::unordered_set<std::string> set_a(lines_a.begin(), lines_a.end());
    std::unordered_set<std::string> set_b(lines_b.begin(), lines_b.end());
    
    // Find removed lines (in A but not in B)
    for (const auto& line : lines_a) {
        if (set_b.find(line) == set_b.end()) {
            diff.removed_lines.push_back(line);
            diff.deletions++;
        }
    }
    
    // Find added lines (in B but not in A)
    for (const auto& line : lines_b) {
        if (set_a.find(line) == set_a.end()) {
            diff.added_lines.push_back(line);
            diff.additions++;
        }
    }
    
    // Generate unified diff format
    std::ostringstream oss;
    oss << "--- " << version_a_id.substr(0, 8) << "\n";
    oss << "+++ " << version_b_id.substr(0, 8) << "\n";
    oss << "@@ -1," << lines_a.size() << " +1," << lines_b.size() << " @@\n";
    
    for (const auto& line : diff.removed_lines) {
        oss << "-" << line << "\n";
    }
    for (const auto& line : diff.added_lines) {
        oss << "+" << line << "\n";
    }
    
    diff.unified_diff = oss.str();
    
    return diff;
}

MergeResult PromptVersionControl::autoMerge(
    const PromptVersion& base,
    const PromptVersion& source,
    const PromptVersion& target
) const {
    MergeResult result;
    result.strategy_used = "auto";
    
    // For now, implement simple strategy: if target hasn't changed from base, use source
    if (target.content == base.content) {
        result.merged_content = source.content;
        result.success = true;
    } else if (source.content == base.content) {
        result.merged_content = target.content;
        result.success = true;
    } else {
        // Both changed - conflict
        result.success = false;
        result.conflicts.push_back("Both branches modified from base - manual resolution required");
        result.merged_content = target.content;  // Default to target
    }
    
    return result;
}

} // namespace prompt_engineering
} // namespace themis
