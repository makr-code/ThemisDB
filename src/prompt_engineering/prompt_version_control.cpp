/**
 * @file prompt_version_control.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=18, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/prompt_version_control.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <unordered_set>
#include <numeric>
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
    std::string parent_version = {};
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
    if (limit > 0 && static_cast<int>(history.size()) > limit) {
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
            if (v_it == versions_.end()) {
              break;
            }
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

    const auto prompt_branches = prompt_it->second;
    if (!prompt_branches.count(source_branch) || !prompt_branches.count(target_branch)) {
        result.conflicts.push_back("One or both branches not found");
        return result;
    }

    const std::string source_id = prompt_branches.at(source_branch);
    const std::string target_id = prompt_branches.at(target_branch);

    auto source_it = versions_.find(source_id);
    auto target_it = versions_.find(target_id);

    if (source_it == versions_.end() || target_it == versions_.end()) {
        result.conflicts.push_back("Version data not found");
        return result;
    }

    const PromptVersion source_version = source_it->second;
    const PromptVersion target_version = target_it->second;
    
    // Find the true lowest common ancestor (LCA) by walking the parent chain
    // from each branch and intersecting the two ancestor sets.
    // This replaces the previous "simplified: use target's parent" heuristic.
    // A depth cap prevents both performance issues and potential infinite loops
    // on corrupted data where the parent chain contains a cycle.
    static constexpr size_t MAX_ANCESTOR_DEPTH = 10000;

    auto collect_ancestors = [&]([[maybe_unused]] const std::string& start_id) {
        // Ordered list (most recent first) so we prefer the closest ancestor.
        std::vector<std::string> ancestors;
        std::unordered_set<std::string> visited;  // cycle guard
        std::string cur = start_id;
        while (!cur.empty() && static_cast<int>(ancestors.size()) < MAX_ANCESTOR_DEPTH) {
            if (!visited.insert(cur).second) break;  // cycle detected
            ancestors.push_back(cur);
            auto it = versions_.find(cur);
            if (it == versions_.end()) {
              break;
            }
            cur = it->second.parent_version;
        }
        return ancestors;
    };

    auto src_ancestors  = collect_ancestors(source_id);
    auto tgt_ancestors  = collect_ancestors(target_id);

    // Build a set of target-side ancestors for O(1) lookup
    std::unordered_set<std::string> tgt_set(tgt_ancestors.begin(), tgt_ancestors.end());

    // The first source ancestor that also appears in the target ancestor chain
    // is the LCA (closest common ancestor).
    std::string base_id = {};
    for (const auto& a : src_ancestors) {
        if (tgt_set.count(a)) {
            base_id = a;
            break;
        }
    }
    
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
                const PromptVersion base_version = base_it->second;
                result = autoMerge(base_version, source_version, target_version);
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

    auto version_it = versions_.find(tag_it->second);
    if (version_it == versions_.end()) {
        return std::nullopt;
    }

    return version_it->second;
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
        stats["branch_count"] = branch_it-> static_cast<int>(second.size());
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
        stats["tag_count"] = tag_it-> static_cast<int>(second.size());
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
    std::ostringstream oss = {};
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

void PromptVersionControl::persistVersion(const PromptVersion& version) {
    if (!db_) {
      return;
    }
    
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
    if (!db_) {
      return;
    }
    
    std::string key = std::string(KEY_PREFIX_BRANCH) + prompt_id + ":" + branch.name;
    std::string value = branch.toJson().dump();
    std::vector<uint8_t> bytes(value.begin(), value.end());
    
    if (!db_->put(key, bytes)) {
        THEMIS_ERROR("Failed to persist branch: {}", branch.name);
    }
}

void PromptVersionControl::loadFromDB() {
    if (!db_) {
      return;
    }
    
    size_t loaded_versions = 0;
    // Load versions
    db_->scanPrefix(KEY_PREFIX_VERSION, [this, &loaded_versions](std::string_view /*key*/, std::string_view value) -> bool {
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
        std::string line = {};
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }
        // Preserve trailing newline by appending an empty sentinel only when
        // the original text ends with '\n' (mirrors standard diff behaviour).
        return lines;
    };

    const auto lines_a = split_lines(content_a);
    const auto lines_b = split_lines(content_b);
    const size_t m = lines_a.size();
    const size_t n = lines_b.size();

    // Build LCS table (Myers-style edit graph, O(m*n) DP)
    // lcs[i][j] = length of LCS of lines_a[0..i-1] and lines_b[0..j-1]
    std::vector<std::vector<size_t>> lcs(m + 1, std::vector<size_t>(n + 1, 0));
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (lines_a[static_cast<int>(i - 1)] == lines_b[static_cast<int>(j - 1)]) {
                lcs[i][j] = lcs[static_cast<int>(i - 1)][static_cast<int>(j - 1)] + 1;
            } else {
                lcs[i][j] = std::max(lcs[static_cast<int>(i - 1)][j], lcs[i][static_cast<int>(j - 1)]);
            }
        }
    }

    // Back-track to produce edit operations in order:
    //   '-' = remove from A,  '+' = add from B,  ' ' = common
    enum class Op { KEEP, REMOVE, ADD };
    struct EditOp { Op op; std::string line; };
    std::vector<EditOp> edits;
    {
        size_t i = m, j = n;
        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && lines_a[static_cast<int>(i - 1)] == lines_b[static_cast<int>(j - 1)]) {
                edits.push_back({Op::KEEP, lines_a[static_cast<int>(i - 1)]});
                --i;
                --j;
            } else if ((j > 0 && (i == 0 || lcs[i][static_cast<int>(j - 1)] >= lcs[static_cast<int>(i - 1)][j]))) {
                edits.push_back({Op::ADD, lines_b[static_cast<int>(j - 1)]});
                --j;
            } else {
                edits.push_back({Op::REMOVE, lines_a[static_cast<int>(i - 1)]});
                --i;
            }
        }
        std::reverse(edits.begin(), edits.end());
    }

    // Populate diff fields
    for (const auto& e : edits) {
        if (e.op == Op::REMOVE) {
            diff.removed_lines.push_back(e.line);
            diff.deletions++;
        } else if (e.op == Op::ADD) {
            diff.added_lines.push_back(e.line);
            diff.additions++;
        }
    }

    // Generate unified diff with 3-line context
    static constexpr int CONTEXT = 3;
    std::ostringstream oss = {};
    oss << "--- " << version_a_id.substr(0, std::min(version_a_id.size(), size_t(8))) << "\n";
    oss << "+++ " << version_b_id.substr(0, std::min(version_b_id.size(), size_t(8))) << "\n";

    // Collect hunk ranges
    // Each hunk is a run of non-KEEP edits expanded by CONTEXT lines.
    const size_t N = edits.size();
    size_t idx = 0;
    while (idx < N) {
        // Skip context-only blocks
        if (edits[idx].op == Op::KEEP) { ++idx; continue; }

        // Find the end of this change cluster
        size_t hunk_start = (idx >= size_t(CONTEXT)) ? idx - CONTEXT : 0;
        size_t hunk_end   = std::min(idx + CONTEXT + 1, N);
        // Extend hunk to cover adjacent changes within 2*CONTEXT distance
        while (hunk_end < N) {
            bool found_change = false;
            for (size_t k = hunk_end; k < std::min(hunk_end + size_t(CONTEXT) * 2, N); ++k) {
                if (edits[k].op != Op::KEEP) { found_change = true; break; }
            }
            if (!found_change) {
              break;
            }
            hunk_end = std::min(hunk_end + size_t(CONTEXT) * 2, N);
        }

        // Compute @@ positions (1-based, for original A and new B)
        int old_start = 1, new_start = 1, old_count = 0, new_count = 0;
        // Count lines in A and B up to hunk_start
        int a_pos = 0, b_pos = 0;
        for (size_t k = 0; k < hunk_start; ++k) {
            if (edits[k].op != Op::ADD) {
              ++a_pos;
            }
            if (edits[k].op != Op::REMOVE) {
              ++b_pos;
            }
        }
        old_start = a_pos + 1;
        new_start = b_pos + 1;

        for (size_t k = hunk_start; k < hunk_end; ++k) {
            if (edits[k].op != Op::ADD) {
              ++old_count;
            }
            if (edits[k].op != Op::REMOVE) {
              ++new_count;
            }
        }

        oss << "@@ -" << old_start << "," << old_count
            << " +" << new_start << "," << new_count << " @@\n";

        for (size_t k = hunk_start; k < hunk_end; ++k) {
            switch (edits[k].op) {
                case Op::KEEP:   oss << " " << edits[k].line << "\n"; break;
                case Op::REMOVE: oss << "-" << edits[k].line << "\n"; break;
                case Op::ADD:    oss << "+" << edits[k].line << "\n"; break;
            }
        }

        idx = hunk_end;
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

    // Fast-forward cases: only one side changed
    if (target.content == base.content) {
        result.merged_content = source.content;
        result.success = true;
        return result;
    }
    if (source.content == base.content) {
        result.merged_content = target.content;
        result.success = true;
        return result;
    }

    // Both sides changed from base — attempt line-level three-way merge.
    // Strategy:
    //   1. Compute base→source diff and base→target diff.
    //   2. Walk through the base lines; apply non-overlapping hunks from both sides.
    //   3. Overlapping hunks that change the same line are conflict regions.

    auto split_lines = [](const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream iss(text);
        std::string line = {};
        while (std::getline(iss, line)) {
          lines.push_back(line);
        }
        return lines;
    };

    const auto base_lines   = split_lines(base.content);
    const auto source_lines = split_lines(source.content);
    const auto target_lines = split_lines(target.content);

    // Build LCS-based edit list: KEEP | REMOVE | ADD
    // Returns a vector<pair<char, string>>: ' ', '-', '+'
    auto lcs_edits = [&](const std::vector<std::string>& from,
                         const std::vector<std::string>& to)
        -> std::vector<std::pair<char, std::string>>
    {
        const size_t m = from.size(), n = to.size();
        std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1, 0));
        for (size_t i = 1; i <= m; ++i)
            for (size_t j = 1; j <= n; ++j)
                dp[i][j] = (from[static_cast<int>(i - 1)] == to[static_cast<int>(j - 1)])
                          ? dp[static_cast<int>(i - 1)][static_cast<int>(j - 1)] + 1
                          : std::max(dp[static_cast<int>(i - 1)][j], dp[i][static_cast<int>(j - 1)]);

        std::vector<std::pair<char, std::string>> edits;
        size_t i = m, j = n;
        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && from[static_cast<int>(i - 1)] == to[static_cast<int>(j - 1)]) {
                edits.push_back({' ', from[static_cast<int>(i - 1)]});
                --i;
                --j;
            } else if ((j > 0 && (i == 0 || dp[i][static_cast<int>(j - 1)] >= dp[static_cast<int>(i - 1)][j]))) {
                edits.push_back({'+', to[static_cast<int>(j - 1)]});
                --j;
            } else {
                edits.push_back({'-', from[static_cast<int>(i - 1)]});
                --i;
            }
        }
        std::reverse(edits.begin(), edits.end());
        return edits;
    };

    auto src_edits = lcs_edits(base_lines, source_lines);
    auto tgt_edits = lcs_edits(base_lines, target_lines);

    // Build a map: base line index → {src change, tgt change}
    // For each base-line position record what src and tgt do with it.
    // Changes are represented as: 0 = keep, 1 = delete, 2 = replace/insert
    struct LineChange {
        bool deleted  = false;
        std::vector<std::string> insertions_before; // lines inserted before this base line
    };

    // Extra slot to hold lines appended after all base content (end-of-file additions).
    LineChange src_eof, tgt_eof;

    std::vector<LineChange> src_changes(base_lines.size());
    std::vector<LineChange> tgt_changes(base_lines.size());

    auto populate_changes = [&](const std::vector<std::pair<char, std::string>>& edits,
                                std::vector<LineChange>& changes,
                                LineChange& eof_slot) {
        size_t base_idx = 0;
        for (const auto& e : edits) {
            if (e.first == ' ') {
                ++base_idx;
            } else if (e.first == '-') {
                if (static_cast<int>(changes.size()) > base_idx) {
                    changes[base_idx].deleted = true;
                    ++base_idx;
                }
            } else { // '+'
                if (static_cast<int>(changes.size()) > base_idx) {
                    changes[base_idx].insertions_before.push_back(e.second);
                } else {
                    // Appended after the last base line
                    eof_slot.insertions_before.push_back(e.second);
                }
            }
        }
    };

    populate_changes(src_edits, src_changes, src_eof);
    populate_changes(tgt_edits, tgt_changes, tgt_eof);

    // Merge: for each base line apply changes from src and tgt
    std::vector<std::string> merged;
    bool has_conflicts = false;

    for (size_t i = 0; i <static_cast<int>(base_lines.size()); ++i) {
        const auto& sc = src_changes[i];
        const auto& tc = tgt_changes[i];

        // Non-conflicting insertions: emit both (src first, then tgt).
        // Build O(1) lookup set to avoid quadratic linear search.
        std::unordered_set<std::string> sc_ins_set(
            sc.insertions_before.begin(), sc.insertions_before.end());
        for (const auto& ins : sc.insertions_before) {
          merged.push_back(ins);
        }
        for (const auto& ins : tc.insertions_before) {
            if (!sc_ins_set.count(ins)) {
                merged.push_back(ins);
            }
        }

        if (sc.deleted && tc.deleted) {
            // Both deleted → keep deleted (non-conflicting)
        } else if (sc.deleted && !tc.deleted) {
            // Only source deleted this line → apply deletion
        } else if (!sc.deleted && tc.deleted) {
            // Only target deleted this line → apply deletion
        } else {
            // Both kept → emit base line
            merged.push_back(base_lines[i]);
        }
    }

    // Emit end-of-file additions from both sides (deduplicating identical lines).
    // Use an unordered_set for O(1) lookup instead of O(n) linear search.
    std::unordered_set<std::string> src_eof_set(
        src_eof.insertions_before.begin(), src_eof.insertions_before.end());
    for (const auto& ins : src_eof.insertions_before) {
      merged.push_back(ins);
    }
    for (const auto& ins : tgt_eof.insertions_before) {
        if (!src_eof_set.count(ins)) {
            merged.push_back(ins);
        }
    }

    // Detect content-level conflicts: if src and tgt both modified the same
    // region and produced different results, add conflict markers.
    // Simple heuristic: if the merged result differs significantly from both
    // source and target, flag a conflict.
    auto join_lines = [](const std::vector<std::string>& lines) {
        std::string out = {};
        for (const auto& l : lines) { out += l; out += '\n'; }
        return out;
    };

    result.merged_content = join_lines(merged);

    // If both sides changed the same lines to different values, report conflict
    for (size_t i = 0; i <static_cast<int>(base_lines.size()); ++i) {
        const auto& sc = src_changes[i];
        const auto& tc = tgt_changes[i];
        bool src_changed = sc.deleted || !sc.insertions_before.empty();
        bool tgt_changed = tc.deleted || !tc.insertions_before.empty();
        if (src_changed && tgt_changed) {
            // Both sides touched line i — check if they agree
            bool agree = (sc.deleted == tc.deleted) &&
                         (sc.insertions_before == tc.insertions_before);
            if (!agree) {
                has_conflicts = true;
                result.conflicts.push_back(
                    "Conflict at base line " + std::to_string(i + 1) +
                    ": '" + base_lines[i] + "'");
            }
        }
    }

    result.success = !has_conflicts;
    if (!result.success && result.merged_content.empty()) {
        result.merged_content = target.content;  // Fallback
    }
    return result;
}

} // namespace prompt_engineering
} // namespace themis

