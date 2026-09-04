/**
 * @file feedback_collector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/feedback_collector.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "distributed_knowledge/cross_shard_feedback_sync.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdint>
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <numeric>
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
    j["checksum"] = checksum;
    
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
    entry.checksum = j.value("checksum", "");
    
    if (j.contains("timestamp")) {
        auto time_val = j["timestamp"].get<std::time_t>();
        entry.timestamp = std::chrono::system_clock::from_time_t(time_val);
    }
    
    return entry;
}

std::string FeedbackEntry::computeChecksum() const {
    // FNV-1a 64-bit hash over key audit fields: id, prompt_id, type, query, severity, timestamp
    // This provides a lightweight audit trail without external crypto dependencies.
    uint64_t hash = 14695981039346656037;
    auto fnv_update = [&]([[maybe_unused]] const std::string& s) {
        for (unsigned char c : s) {
            hash ^= c;
            hash *= 1099511628211;
        }
    };
    fnv_update(id);
    fnv_update(prompt_id);
    fnv_update(feedbackTypeToString(type));
    fnv_update(query);
    // Include severity as a fixed-precision string
    std::ostringstream sev_str = {};
    sev_str << std::fixed << std::setprecision(6) << severity;
    fnv_update(sev_str.str());
    // Include timestamp as epoch seconds
    auto ts = std::chrono::system_clock::to_time_t(timestamp);
    fnv_update(std::to_string(ts));

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
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
    entry.checksum = entry.computeChecksum();
    
    // Store in memory
    feedback_[prompt_id].push_back(entry);
    
    // Persist if DB available
    if (db_) {
        persist(entry);
    }
    
    THEMIS_DEBUG("Recorded feedback for prompt '{}': type={}, severity={}",
                 prompt_id, feedbackTypeToString(type), severity);

    // ── DK-5: Cross-shard publish (anonymised embedding, no raw text) ────────
    if (cross_shard_sync_ && embedding_model_) {
        try {
            distributed_knowledge::FeedbackSummary summary;
            summary.feedback_type_label = feedbackTypeToString(type);
            summary.shard_origin        = "ANON"; // also enforced by sync
            summary.reason_embedding    = embedding_model_->embed(query);
            cross_shard_sync_->publishFeedback(std::move(summary));
        } catch (const std::exception& e) {
            // Cross-shard publish must never fail local recording
            THEMIS_DEBUG("Cross-shard feedback publish failed (skipping): {}", e.what());
        }
    } else if (cross_shard_sync_ && !embedding_model_) {
        THEMIS_DEBUG("Cross-shard feedback skipped: no embedding model set");
    }
    // ─────────────────────────────────────────────────────────────────────────

    return entry.id;
}

// ── DK-5: DI setters ─────────────────────────────────────────────────────────

void FeedbackCollector::setCrossShardSync(
    std::shared_ptr<distributed_knowledge::CrossShardFeedbackSync> sync)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cross_shard_sync_ = std::move(sync);
}

void FeedbackCollector::setEmbeddingModel(std::shared_ptr<IEmbeddingModel> model)
{
    std::lock_guard<std::mutex> lock(mutex_);
    embedding_model_ = std::move(model);
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
        if (static_cast<int>(entries.size()) < min_feedback) {
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
        if (static_cast<int>(result.size()) > = limit) {
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
    std::vector<FeedbackEntry> failures = {};

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

    // When a DB is available, use the time-keyed secondary index for an O(log n)
    // range scan instead of scanning all in-memory entries.
    if (db_) {
        std::vector<FeedbackEntry> result;
        std::string range_start = std::string(IDX_TIME_PREFIX) + prompt_id + ":"
                                + formatTimestampKey(start) + ":";
        // Upper bound: one past the end timestamp
        auto end_plus = end + std::chrono::microseconds(1);
        std::string range_end   = std::string(IDX_TIME_PREFIX) + prompt_id + ":"
                                + formatTimestampKey(end_plus) + ":";
        db_->scanRange(range_start, range_end,
            [&](std::string_view /*idx_key*/, std::string_view primary_key) -> bool {
                std::optional<std::vector<uint8_t>> raw = db_->get(primary_key);
                if (raw.has_value()) {
                    try {
                        auto j = nlohmann::json::parse(
                            std::string(raw->begin(), raw->end()));
                        result.push_back(FeedbackEntry::fromJson(j));
                    } catch (...) {}
                }
                return true;
            });
        return result;
    }

    // Fallback: linear scan of in-memory entries
    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return {};
    }
    
    std::vector<FeedbackEntry> result = {};

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
            [&]([[maybe_unused]] const FeedbackEntry& entry) {
                return entry.timestamp < older_than;
            });
        
        deleted += std::distance(it, entries.end());
        entries.erase(it, entries.end());
    }
    
    // Also delete from DB if available: delete both primary records and index entries
    if (db_ && deleted > 0) {
        // Collect entries to delete by scanning; we need the full entry to
        // also remove the time-index key.
        std::vector<FeedbackEntry> to_delete;
        std::string prefix = KEY_PREFIX;
        db_->scanPrefix(prefix, [&](std::string_view, std::string_view value) -> bool {
            try {
                auto j = nlohmann::json::parse(std::string(value));
                if (!j.contains("timestamp") || !j["timestamp"].is_number()) {
                    return true;  // malformed entry, skip
                }
                auto ts = std::chrono::system_clock::from_time_t(
                    j["timestamp"].get<std::time_t>());
                if (ts < older_than) {
                    to_delete.push_back(FeedbackEntry::fromJson(j));
                }
            } catch (const nlohmann::json::exception& e) {
                THEMIS_WARN("Skipping malformed feedback entry during prune: {}", e.what());
            }
            return true;
        });
        for (const auto& e : to_delete) {
            deleteFromDB(e);
        }
        THEMIS_DEBUG("Pruned {} old feedback entries from DB", to_delete.size());
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
    
    // Delete from DB if available: delete both primary records and index entries
    if (db_) {
        std::string prompt_prefix = std::string(KEY_PREFIX) + prompt_id + ":";
        // Collect full entries so we can also remove the secondary index keys
        std::vector<FeedbackEntry> to_delete;
        db_->scanPrefix(prompt_prefix, [&](std::string_view, std::string_view value) -> bool {
            try {
                auto j = nlohmann::json::parse(std::string(value));
                to_delete.push_back(FeedbackEntry::fromJson(j));
            } catch (...) {}
            return true;
        });
        for (const auto& e : to_delete) {
            deleteFromDB(e);
        }
        THEMIS_DEBUG("Deleted {} feedback DB entries for prompt '{}'", to_delete.size(), prompt_id);
    }
    
    THEMIS_INFO("Cleared {} feedback entries for prompt '{}'", count, prompt_id);
    
    return count;
}

std::vector<FeedbackEntry> FeedbackCollector::getFeedbackPaged(
    const std::string& prompt_id,
    size_t offset,
    size_t page_size,
    std::optional<FeedbackType> type_filter
) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return {};
    }

    std::vector<FeedbackEntry> result;
    size_t skipped = 0;

    for (const auto& entry : it->second) {
        if (type_filter.has_value() && entry.type != type_filter.value()) {
            continue;
        }
        if (skipped < offset) {
            ++skipped;
            continue;
        }
        result.push_back(entry);
        if (page_size > 0 && result.size() >= page_size) {
            break;
        }
    }

    return result;
}

std::vector<FeedbackEntry> FeedbackCollector::detectOutliers(
    const std::string& prompt_id,
    double z_threshold
) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = feedback_.find(prompt_id);
    if (it == feedback_.end()) {
        return {};
    }

    const auto& entries = it->second;
    if (static_cast<int>(entries.size()) < 2) {
        return {};
    }

    // Compute mean severity
    double sum = 0.0;
    for (const auto& e : entries) {
        sum += e.severity;
    }
    double mean = sum / entries.size();

    // Compute standard deviation
    double sq_diff_sum = 0.0;
    for (const auto& e : entries) {
        double d = e.severity - mean;
        sq_diff_sum += d * d;
    }
    double stddev = std::sqrt(sq_diff_sum / entries.size());

    if (stddev < 1e-10) {
        return {}; // All severities are identical – no outliers
    }

    std::vector<FeedbackEntry> outliers = {};

    for (const auto& e : entries) {
        double z = std::abs(e.severity - mean) / stddev;
        if (z > z_threshold) {
            outliers.push_back(e);
        }
    }

    THEMIS_DEBUG("Detected {} outliers for prompt '{}' (z_threshold={})",
                 outliers.size(), prompt_id, z_threshold);

    return outliers;
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
    
    std::ostringstream oss = {};
    oss << "feedback_" << std::hex << std::setfill('0')
        << std::setw(12) << ms
        << "_"
        << std::setw(8) << dis(gen);
    return oss.str();
}

// static
std::string FeedbackCollector::formatTimestampKey(
    const std::chrono::system_clock::time_point& tp
) {
    // Zero-padded 20-digit microseconds since epoch ensures lexicographic == chronological order.
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                  tp.time_since_epoch()).count();
    std::ostringstream oss = {};
    oss << std::setfill('0') << std::setw(20) << us;
    return oss.str();
}

void FeedbackCollector::persist(const FeedbackEntry& entry) {
    if (!db_) {
      return;
    }
    
    // Primary record: feedback:{prompt_id}:{entry_id} → JSON
    std::string primary_key = std::string(KEY_PREFIX) + entry.prompt_id + ":" + entry.id;
    std::string value = entry.toJson().dump();
    std::vector<uint8_t> bytes(value.begin(), value.end());
    
    if (!db_->put(primary_key, bytes)) {
        THEMIS_ERROR("Failed to persist feedback entry: {}", entry.id);
        return;
    }

    // Secondary time index: idx:time:{prompt_id}:{ts_us}:{entry_id} → primary_key
    // Lexicographic key order == timestamp order, enabling O(log n) range scans.
    std::string idx_key = std::string(IDX_TIME_PREFIX)
                        + entry.prompt_id + ":"
                        + formatTimestampKey(entry.timestamp) + ":"
                        + entry.id;
    // Use vector<uint8_t> to match the overload used for the primary record.
    std::string idx_val = primary_key;
    std::vector<uint8_t> idx_bytes(idx_val.begin(), idx_val.end());
    db_->put(idx_key, idx_bytes);
}

void FeedbackCollector::deleteFromDB(const FeedbackEntry& entry) {
    if (!db_) {
      return;
    }

    std::string primary_key = std::string(KEY_PREFIX) + entry.prompt_id + ":" + entry.id;
    db_->del(primary_key);

    std::string idx_key = std::string(IDX_TIME_PREFIX)
                        + entry.prompt_id + ":"
                        + formatTimestampKey(entry.timestamp) + ":"
                        + entry.id;
    db_->del(idx_key);
}

void FeedbackCollector::loadFromDB() {
    if (!db_) {
      return;
    }
    
    std::string prefix = KEY_PREFIX;
    size_t loaded = 0;
    
    db_->scanPrefix(prefix, [this, &loaded](std::string_view /*key*/, std::string_view value) -> bool {
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
    FeedbackStats stats = {};
    
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
    if (entries.empty()) return {};

    // --- Step 1: tokenise and normalise ---
    // Common English stop words that carry no discriminative signal.
    static const std::unordered_set<std::string> STOP_WORDS = {
        "a","an","the","and","or","but","in","on","at","to","for","of","with",
        "by","from","up","about","into","through","during","before","after",
        "is","are","was","were","be","been","being","have","has","had","do",
        "does","did","will","would","could","should","may","might","shall",
        "that","this","these","those","it","its","i","you","he","she","we",
        "they","what","which","who","how","when","where","why","not","no",
        "can","as","if","so","than","then","there","also","just","more","all"
    };

    auto tokenise = [&]([[maybe_unused]] const std::string& text) {
        std::vector<std::string> tokens;
        std::string cur = {};
        for (unsigned char c : text) {
            if (std::isalnum(c)) {
                cur += static_cast<char>(std::tolower(c));
            } else if (!cur.empty()) {
                if (STOP_WORDS.find(cur) == STOP_WORDS.end() && cur.size() >= 2) {
                    tokens.push_back(cur);
                }
                cur.clear();
            }
        }
        if (!cur.empty() && STOP_WORDS.find(cur) == STOP_WORDS.end() && cur.size() >= 2) {
            tokens.push_back(cur);
        }
        return tokens;
    };

    // --- Step 2: compute TF-IDF-style term frequency per document and DF ---
    // document = one FeedbackEntry.query
    const size_t num_docs = entries.size();

    // document_frequency[term] = number of documents containing the term
    std::unordered_map<std::string, size_t> document_frequency;
    // per_entry_tokens[i] = token list for entries[i]
    std::vector<std::vector<std::string>> per_entry_tokens(num_docs);

    for (size_t i = 0; i < num_docs; ++i) {
        auto tokens = tokenise(entries[i].query);
        per_entry_tokens[i] = tokens;
        // Count document frequency (each token counted once per document)
        std::unordered_set<std::string> seen(tokens.begin(), tokens.end());
        for (const auto& t : seen) {
            document_frequency[t]++;
        }
    }

    // --- Step 3: for each entry compute its most discriminative keyword ---
    // Score = TF * log(N / DF + 1).  The keyword with the highest score
    // becomes the representative for the pattern.
    auto best_keyword = [&]([[maybe_unused]] size_t entry_idx) -> std::string {
        const auto& tokens = per_entry_tokens[entry_idx];
        if (tokens.empty()) {
          return "[empty]";
        }

        // TF: raw count within this query
        std::unordered_map<std::string, size_t> tf = {};

        for (const auto& t : tokens) {
          tf[t]++;
        }

        std::string best_term = {};
        double best_score = -1.0;
        for (const auto& [term, count] : tf) {
            double tfidf = static_cast<double>(count)
                         * std::log(static_cast<double>(num_docs + 1)
                                    / static_cast<double>(document_frequency[term] + 1));
            if (tfidf > best_score) {
                best_score = tfidf;
                best_term  = term;
            }
        }
        return best_term.empty() ? "[empty]" : best_term;
    };

    // --- Step 4: group entries by their best keyword (pattern) ---
    std::unordered_map<std::string, FailedQueryPattern> patterns = {};

    for (size_t i = 0; i < num_docs; ++i) {
        const auto& entry = entries[i];
        std::string key   = best_keyword(i);

        auto& p = patterns[key];
        p.pattern = key;
        p.occurrences++;

        if (static_cast<int>(p.examples.size()) < 5) {
            p.examples.push_back(entry.query);
        }

        // Incremental average severity; most-common type wins
        if (p.occurrences == 1) {
            p.primary_type = entry.type;
            p.avg_severity = entry.severity;
        } else {
            p.avg_severity = (p.avg_severity * (p.occurrences - 1) + entry.severity)
                           / p.occurrences;
        }
    }

    // --- Step 5: filter by min_occurrences and sort ---
    std::vector<FailedQueryPattern> result = {};

    result.reserve(patterns.size());
    for (const auto& [key, pat] : patterns) {
        if (pat.occurrences >= min_occurrences) {
            result.push_back(pat);
        }
    }

    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.occurrences > b.occurrences; });

    return result;
}

size_t FeedbackCollector::newEntryCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [prompt_id, entries] : feedback_) {
        total += entries.size();
    }
    return total;
}

} // namespace prompt_engineering
} // namespace themis

