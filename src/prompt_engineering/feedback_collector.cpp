/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feedback_collector.cpp                             ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     611                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file feedback_collector.cpp
 * @brief Implementation of feedback collection system
 */

#include "prompt_engineering/feedback_collector.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <unordered_set>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// FeedbackType Utilities
// ============================================================================

std::string feedbackTypeToString(FeedbackType type) {
    switch (type) {
        case FeedbackType::USER_POSITIVE: return "USER_POSITIVE";
        case FeedbackType::USER_NEGATIVE: return "USER_NEGATIVE";
        case FeedbackType::HALLUCINATION_DETECTED: return "HALLUCINATION_DETECTED";
        case FeedbackType::TIMEOUT: return "TIMEOUT";
        case FeedbackType::PARSE_ERROR: return "PARSE_ERROR";
        case FeedbackType::VALIDATION_FAILED: return "VALIDATION_FAILED";
        case FeedbackType::CONTEXT_MISSING: return "CONTEXT_MISSING";
        case FeedbackType::AMBIGUOUS_OUTPUT: return "AMBIGUOUS_OUTPUT";
        case FeedbackType::SECURITY_ISSUE: return "SECURITY_ISSUE";
        case FeedbackType::PERFORMANCE_ISSUE: return "PERFORMANCE_ISSUE";
        default: return "UNKNOWN";
    }
}

std::optional<FeedbackType> stringToFeedbackType(const std::string& str) {
    static const std::unordered_map<std::string, FeedbackType> map = {
        {"USER_POSITIVE", FeedbackType::USER_POSITIVE},
        {"USER_NEGATIVE", FeedbackType::USER_NEGATIVE},
        {"HALLUCINATION_DETECTED", FeedbackType::HALLUCINATION_DETECTED},
        {"TIMEOUT", FeedbackType::TIMEOUT},
        {"PARSE_ERROR", FeedbackType::PARSE_ERROR},
        {"VALIDATION_FAILED", FeedbackType::VALIDATION_FAILED},
        {"CONTEXT_MISSING", FeedbackType::CONTEXT_MISSING},
        {"AMBIGUOUS_OUTPUT", FeedbackType::AMBIGUOUS_OUTPUT},
        {"SECURITY_ISSUE", FeedbackType::SECURITY_ISSUE},
        {"PERFORMANCE_ISSUE", FeedbackType::PERFORMANCE_ISSUE}
    };
    
    auto it = map.find(str);
    if (it != map.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ============================================================================
// FeedbackEntry Implementation
// ============================================================================

nlohmann::json FeedbackEntry::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["prompt_id"] = prompt_id;
    j["type"] = feedbackTypeToString(type);
    j["query"] = query;
    j["response"] = response;
    j["feedback_text"] = feedback_text;
    j["metadata"] = metadata;
    j["severity"] = severity;
    
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    j["timestamp"] = time;
    
    return j;
}

FeedbackEntry FeedbackEntry::fromJson(const nlohmann::json& j) {
    FeedbackEntry entry;
    entry.id = j.value("id", "");
    entry.prompt_id = j.value("prompt_id", "");
    
    std::string type_str = j.value("type", "");
    auto type_opt = stringToFeedbackType(type_str);
    entry.type = type_opt.value_or(FeedbackType::USER_NEGATIVE);
    
    entry.query = j.value("query", "");
    entry.response = j.value("response", "");
    entry.feedback_text = j.value("feedback_text", "");
    entry.metadata = j.value("metadata", nlohmann::json::object());
    entry.severity = j.value("severity", 0.5);
    
    if (j.contains("timestamp")) {
        auto time_val = j["timestamp"].get<std::time_t>();
        entry.timestamp = std::chrono::system_clock::from_time_t(time_val);
    }
    
    return entry;
}

// ============================================================================
// FeedbackStats Implementation
// ============================================================================

nlohmann::json FeedbackStats::toJson() const {
    nlohmann::json j;
    j["prompt_id"] = prompt_id;
    j["total_feedback"] = total_feedback;
    j["positive_ratio"] = positive_ratio;
    j["negative_ratio"] = negative_ratio;
    j["hallucination_count"] = hallucination_count;
    j["error_count"] = error_count;
    j["common_issues"] = common_issues;
    
    nlohmann::json counts;
    for (const auto& [type, count] : counts_by_type) {
        counts[feedbackTypeToString(type)] = count;
    }
    j["counts_by_type"] = counts;
    
    auto time = std::chrono::system_clock::to_time_t(last_feedback);
    j["last_feedback"] = time;
    
    return j;
}

// ============================================================================
// FeedbackCollector Implementation
// ============================================================================

FeedbackCollector::FeedbackCollector() = default;

FeedbackCollector::FeedbackCollector(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {
    if (db_) {
        loadFromDB();
    }
}

std::string FeedbackCollector::recordFeedback(
    const std::string& prompt_id,
    const std::string& query,
    const std::string& response,
    FeedbackType type,
    const std::string& feedback_text,
    double severity,
    const nlohmann::json& metadata
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    FeedbackEntry entry;
    entry.id = generateId();
    entry.prompt_id = prompt_id;
    entry.type = type;
    entry.query = query;
    entry.response = response;
    entry.feedback_text = feedback_text;
    entry.severity = std::max(0.0, std::min(1.0, severity));
    entry.metadata = metadata;
    entry.timestamp = std::chrono::system_clock::now();
    
    // Store in memory
    feedback_[prompt_id].push_back(entry);
    
    // Persist if DB available
    if (db_) {
        persist(entry);
    }
    
    THEMIS_DEBUG("Recorded feedback for prompt '{}': type={}, severity={}",
                 prompt_id, feedbackTypeToString(type), severity);
    
    return entry.id;
}

std::vector<FeedbackEntry> FeedbackCollector::getFeedback(
    const std::string& prompt_id,
    size_t limit,
    std::optional<FeedbackType> type_filter
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return {};
    }
    
    std::vector<FeedbackEntry> result;
    
    for (const auto& entry : it->second) {
        // Apply type filter if specified
        if (type_filter.has_value() && entry.type != type_filter.value()) {
            continue;
        }
        
        result.push_back(entry);
        
        // Apply limit if specified
        if (limit > 0 && result.size() >= limit) {
            break;
        }
    }
    
    return result;
}

FeedbackStats FeedbackCollector::getStats(const std::string& prompt_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        FeedbackStats stats;
        stats.prompt_id = prompt_id;
        return stats;
    }
    
    return calculateStats(it->second);
}

std::vector<std::string> FeedbackCollector::getPromptsWithNegativeFeedback(
    double threshold,
    size_t min_feedback
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    
    for (const auto& [prompt_id, entries] : feedback_) {
        if (entries.size() < min_feedback) {
            continue;
        }
        
        auto stats = calculateStats(entries);
        if (stats.negative_ratio >= threshold) {
            result.push_back(prompt_id);
        }
    }
    
    THEMIS_INFO("Found {} prompts with negative feedback >= {:.1%}",
                result.size(), threshold);
    
    return result;
}

std::vector<std::tuple<std::string, std::string, FeedbackType>> 
FeedbackCollector::getFailedQueries(
    const std::string& prompt_id,
    size_t limit,
    std::optional<FeedbackType> type_filter
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return {};
    }
    
    std::vector<std::tuple<std::string, std::string, FeedbackType>> result;
    
    for (const auto& entry : it->second) {
        // Only include negative feedback types
        bool is_failure = (entry.type != FeedbackType::USER_POSITIVE);
        
        if (!is_failure) {
            continue;
        }
        
        // Apply type filter if specified
        if (type_filter.has_value() && entry.type != type_filter.value()) {
            continue;
        }
        
        result.emplace_back(entry.query, entry.response, entry.type);
        
        // Apply limit
        if (result.size() >= limit) {
            break;
        }
    }
    
    return result;
}

std::vector<FailedQueryPattern> FeedbackCollector::analyzeFailurePatterns(
    const std::string& prompt_id,
    size_t min_occurrences
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return {};
    }
    
    // Filter to failure entries
    std::vector<FeedbackEntry> failures;
    for (const auto& entry : it->second) {
        if (entry.type != FeedbackType::USER_POSITIVE) {
            failures.push_back(entry);
        }
    }
    
    return extractPatterns(failures, min_occurrences);
}

std::vector<FeedbackEntry> FeedbackCollector::getFeedbackInTimeRange(
    const std::string& prompt_id,
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return {};
    }
    
    std::vector<FeedbackEntry> result;
    
    for (const auto& entry : it->second) {
        if (entry.timestamp >= start && entry.timestamp <= end) {
            result.push_back(entry);
        }
    }
    
    return result;
}

size_t FeedbackCollector::pruneOldFeedback(
    const std::chrono::system_clock::time_point& older_than
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t deleted = 0;
    
    for (auto& [prompt_id, entries] : feedback_) {
        auto it = std::remove_if(entries.begin(), entries.end(),
            [&](const FeedbackEntry& entry) {
                return entry.timestamp < older_than;
            });
        
        deleted += std::distance(it, entries.end());
        entries.erase(it, entries.end());
    }
    
    // Also delete from DB if available
    if (db_ && deleted > 0) {
        // In production, implement DB cleanup
        THEMIS_DEBUG("Pruned {} old feedback entries (DB cleanup not implemented)", deleted);
    }
    
    THEMIS_INFO("Pruned {} old feedback entries", deleted);
    
    return deleted;
}

size_t FeedbackCollector::clearFeedback(const std::string& prompt_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return 0;
    }
    
    size_t count = it->second.size();
    feedback_.erase(it);
    
    // Delete from DB if available
    if (db_) {
        // In production, implement DB cleanup
        THEMIS_DEBUG("Cleared {} feedback entries for prompt '{}' (DB cleanup not implemented)",
                     count, prompt_id);
    }
    
    THEMIS_INFO("Cleared {} feedback entries for prompt '{}'", count, prompt_id);
    
    return count;
}

nlohmann::json FeedbackCollector::getSummary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t total_feedback = 0;
    size_t total_positive = 0;
    size_t total_negative = 0;
    size_t total_hallucinations = 0;
    size_t total_errors = 0;
    
    for (const auto& [prompt_id, entries] : feedback_) {
        total_feedback += entries.size();
        
        for (const auto& entry : entries) {
            if (entry.type == FeedbackType::USER_POSITIVE) {
                total_positive++;
            } else if (entry.type == FeedbackType::USER_NEGATIVE) {
                total_negative++;
            } else if (entry.type == FeedbackType::HALLUCINATION_DETECTED) {
                total_hallucinations++;
            } else {
                total_errors++;
            }
        }
    }
    
    nlohmann::json summary;
    summary["total_prompts_tracked"] = feedback_.size();
    summary["total_feedback"] = total_feedback;
    summary["positive_feedback"] = total_positive;
    summary["negative_feedback"] = total_negative;
    summary["hallucinations"] = total_hallucinations;
    summary["errors"] = total_errors;
    
    if (total_feedback > 0) {
        summary["positive_ratio"] = static_cast<double>(total_positive) / total_feedback;
        summary["negative_ratio"] = static_cast<double>(total_negative) / total_feedback;
    }
    
    return summary;
}

// ============================================================================
// Private Methods
// ============================================================================

std::string FeedbackCollector::generateId() const {
    static thread_local std::mt19937_64 gen((std::random_device())());
    static std::uniform_int_distribution<uint64_t> dis;
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    
    std::ostringstream oss;
    oss << "feedback_" << std::hex << std::setfill('0')
        << std::setw(12) << ms
        << "_"
        << std::setw(8) << dis(gen);
    return oss.str();
}

void FeedbackCollector::persist(const FeedbackEntry& entry) {
    if (!db_) return;
    
    std::string key = std::string(KEY_PREFIX) + entry.prompt_id + ":" + entry.id;
    std::string value = entry.toJson().dump();
    std::vector<uint8_t> bytes(value.begin(), value.end());
    
    if (!db_->put(key, bytes)) {
        THEMIS_ERROR("Failed to persist feedback entry: {}", entry.id);
    }
}

void FeedbackCollector::loadFromDB() {
    if (!db_) return;
    
    std::string prefix = KEY_PREFIX;
    size_t loaded = 0;
    
    db_->scanPrefix(prefix, [this, &loaded](std::string_view key, std::string_view value) -> bool {
        try {
            auto j = nlohmann::json::parse(std::string(value));
            auto entry = FeedbackEntry::fromJson(j);
            feedback_[entry.prompt_id].push_back(entry);
            loaded++;
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse feedback entry from DB: {}", e.what());
        }
        return true; // continue scanning
    });
    
    THEMIS_INFO("Loaded {} feedback entries from DB", loaded);
}

FeedbackStats FeedbackCollector::calculateStats(
    const std::vector<FeedbackEntry>& entries
) const {
    FeedbackStats stats;
    
    if (entries.empty()) {
        return stats;
    }
    
    stats.prompt_id = entries[0].prompt_id;
    stats.total_feedback = entries.size();
    
    size_t positive_count = 0;
    size_t negative_count = 0;
    
    // Count by type
    for (const auto& entry : entries) {
        stats.counts_by_type[entry.type]++;
        
        if (entry.type == FeedbackType::USER_POSITIVE) {
            positive_count++;
        } else if (entry.type == FeedbackType::USER_NEGATIVE) {
            negative_count++;
        } else if (entry.type == FeedbackType::HALLUCINATION_DETECTED) {
            stats.hallucination_count++;
        } else {
            stats.error_count++;
        }
        
        // Track latest feedback
        if (entry.timestamp > stats.last_feedback) {
            stats.last_feedback = entry.timestamp;
        }
    }
    
    // Calculate ratios
    if (stats.total_feedback > 0) {
        stats.positive_ratio = static_cast<double>(positive_count) / stats.total_feedback;
        stats.negative_ratio = static_cast<double>(negative_count) / stats.total_feedback;
    }
    
    // Identify common issues
    std::vector<std::pair<FeedbackType, size_t>> sorted_types;
    for (const auto& [type, count] : stats.counts_by_type) {
        if (type != FeedbackType::USER_POSITIVE) {
            sorted_types.emplace_back(type, count);
        }
    }
    
    std::sort(sorted_types.begin(), sorted_types.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(size_t(3), sorted_types.size()); ++i) {
        stats.common_issues.push_back(feedbackTypeToString(sorted_types[i].first));
    }
    
    return stats;
}

std::vector<FailedQueryPattern> FeedbackCollector::extractPatterns(
    const std::vector<FeedbackEntry>& entries,
    size_t min_occurrences
) const {
    // Simple pattern extraction based on query keywords
    // In production, use more sophisticated NLP techniques
    
    std::unordered_map<std::string, FailedQueryPattern> patterns;
    
    for (const auto& entry : entries) {
        // Extract first few words as a simple pattern
        std::string pattern;
        std::istringstream iss(entry.query);
        std::string word;
        int word_count = 0;
        while (iss >> word && word_count < 3) {
            if (word_count > 0) pattern += " ";
            pattern += word;
            word_count++;
        }
        
        if (pattern.empty()) {
            pattern = "[empty]";
        }
        
        auto& p = patterns[pattern];
        p.pattern = pattern;
        p.occurrences++;
        
        if (p.examples.size() < 5) {
            p.examples.push_back(entry.query);
        }
        
        // Track primary type and severity
        if (p.occurrences == 1) {
            p.primary_type = entry.type;
            p.avg_severity = entry.severity;
        } else {
            p.avg_severity = (p.avg_severity * (p.occurrences - 1) + entry.severity) / p.occurrences;
        }
    }
    
    // Filter by minimum occurrences
    std::vector<FailedQueryPattern> result;
    for (const auto& [pattern_str, pattern] : patterns) {
        if (pattern.occurrences >= min_occurrences) {
            result.push_back(pattern);
        }
    }
    
    // Sort by occurrences (descending)
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.occurrences > b.occurrences; });
    
    return result;
}

} // namespace prompt_engineering
} // namespace themis
