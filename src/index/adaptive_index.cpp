/**
 * @file adaptive_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/adaptive_index.h"
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/iterator.h>
#include <algorithm>
#include <set>
#include <cmath>

namespace themis {

// ===== QueryPatternTracker Implementation =====

QueryPatternTracker::QueryPatternTracker() = default;

std::string QueryPatternTracker::makeKey(const std::string& collection,
                                        const std::string& field,
                                        const std::string& operation) const {
    return collection + ":" + field + ":" + operation;
}

int64_t QueryPatternTracker::getCurrentTimeMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void QueryPatternTracker::recordPattern(const std::string& collection,
                                       const std::string& field,
                                       const std::string& operation,
                                       int64_t execution_time_ms,
                                       bool cache_miss,
                                       double cache_miss_penalty_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeKey(collection, field, operation);
    auto& pattern = patterns_[key];
    
    pattern.collection = collection;
    pattern.field = field;
    pattern.operation = operation;
    pattern.count++;
    pattern.total_time_ms += execution_time_ms;
    pattern.last_seen_ms = getCurrentTimeMs();
    
    // Phase 2: Track cache metrics
    if (cache_miss) {
        pattern.cache_misses++;
        pattern.avg_cache_miss_penalty_ms = 
            (pattern.avg_cache_miss_penalty_ms * (pattern.cache_misses - 1) + cache_miss_penalty_ms) 
            / pattern.cache_misses;
    } else {
        pattern.cache_hits++;
    }
}

std::vector<QueryPatternTracker::QueryPattern> 
QueryPatternTracker::getPatterns(const std::string& collection) const {
    // RACE CONDITION FIX: Move sorting outside lock to reduce contention
    std::vector<QueryPattern> result;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        result.reserve(patterns_.size());
        for (const auto& [key, pattern] : patterns_) {
            if (collection.empty() || pattern.collection == collection) {
                result.push_back(pattern);
            }
        }
    }  // Release lock before sorting
    
    // Sort by count (descending) - outside lock (O(n log n) operation)
    std::sort(result.begin(), result.end(), 
             [](const QueryPattern& a, const QueryPattern& b) {
                 return a.count > b.count;
             });
    
    return result;
}

std::vector<QueryPatternTracker::QueryPattern> 
QueryPatternTracker::getTopPatterns([[maybe_unused]] size_t limit) const {
    auto patterns = getPatterns("");
    if (patterns.size() > limit) {
        patterns.resize(limit);
    }
    return patterns;
}

void QueryPatternTracker::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    patterns_.clear();
}

size_t QueryPatternTracker::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return patterns_.size();
}

nlohmann::json QueryPatternTracker::QueryPattern::toJson() const {
    return nlohmann::json{
        {"collection", collection},
        {"field", field},
        {"operation", operation},
        {"count", count},
        {"total_time_ms", total_time_ms},
        {"avg_time_ms", count > 0 ? total_time_ms / count : 0},
        {"last_seen_ms", last_seen_ms},
        {"cache_misses", cache_misses},
        {"cache_hits", cache_hits},
        {"cache_hit_rate", (cache_hits + cache_misses) > 0 ? 
            static_cast<double>(cache_hits) / (cache_hits + cache_misses) : 0.0},
        {"avg_cache_miss_penalty_ms", avg_cache_miss_penalty_ms}
    };
}

QueryPatternTracker::QueryPattern 
QueryPatternTracker::QueryPattern::fromJson(const nlohmann::json& j) {
    QueryPattern p;
    p.collection = j.value("collection", "");
    p.field = j.value("field", "");
    p.operation = j.value("operation", "");
    p.count = j.value("count", 0);
    p.total_time_ms = j.value("total_time_ms", 0);
    p.last_seen_ms = j.value("last_seen_ms", 0);
    p.cache_misses = j.value("cache_misses", 0);
    p.cache_hits = j.value("cache_hits", 0);
    p.avg_cache_miss_penalty_ms = j.value("avg_cache_miss_penalty_ms", 0.0);
    return p;
}

// ===== SelectivityAnalyzer Implementation =====

SelectivityAnalyzer::SelectivityAnalyzer(rocksdb::TransactionDB* db)
    : db_(db) {
    if (!db_) {
        throw std::invalid_argument("SelectivityAnalyzer: db cannot be null");
    }
}

SelectivityAnalyzer::SelectivityStats 
SelectivityAnalyzer::analyze(const std::string& collection,
                            const std::string& field,
                            size_t sample_size) {
    SelectivityStats stats;
    stats.collection = collection;
    stats.field = field;

    if (!db_) {
        return stats;
    }
    
    // Build prefix for collection
    std::string prefix = "d:" + collection + ":";
    
    rocksdb::ReadOptions read_opts;
    // RACE CONDITION FIX: Use unique_ptr for automatic cleanup and safer lifetime management
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_opts));
    if (!it) {
        return stats;
    }
    
    std::set<std::string> unique_values;
    std::map<std::string, int> value_counts;
    int64_t total = 0;
    int64_t sampled = 0;
    
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        if (sample_size > 0 && sampled >= static_cast<int64_t>(sample_size)) {
            break;
        }
        
        try {
            nlohmann::json doc = nlohmann::json::parse(it->value().ToString());
            
            if (doc.contains(field)) {
                std::string value = doc[field].dump();  // Serialize value
                unique_values.insert(value);
                value_counts[value]++;
            } else {
                stats.null_count++;
            }

            total++;
            sampled++;
        } catch ([[maybe_unused]] const std::exception& e) {
            // Skip invalid JSON
            continue;
        }
    }

    if (!it->status().ok()) {
        return stats;
    }
    
    // No need to delete - unique_ptr handles cleanup automatically
    
    stats.total_documents = total;
    stats.unique_values = static_cast<int64_t>(unique_values.size());
    
    if (total > 0) {
        stats.selectivity = static_cast<double>(stats.unique_values) / total;
    }
    
    stats.distribution = determineDistribution(value_counts, total);
    
    return stats;
}

double SelectivityAnalyzer::calculateIndexBenefit(const SelectivityStats& stats) const {
    if (stats.total_documents == 0) {
        return 0.0;
    }
    
    // Factors:
    // 1. Selectivity (higher = better for range/hash index)
    // 2. Null ratio (lower = better)
    // 3. Distribution (uniform > skewed > sparse)
    
    double selectivity_score = stats.selectivity;
    
    double null_ratio = static_cast<double>(stats.null_count) / stats.total_documents;
    double null_score = 1.0 - null_ratio;
    
    double distribution_score = 0.5;
    if (stats.distribution == "uniform") {
        distribution_score = 1.0;
    } else if (stats.distribution == "skewed") {
        distribution_score = 0.7;
    } else if (stats.distribution == "sparse") {
        distribution_score = 0.3;
    }
    
    // Weighted average
    double benefit = (selectivity_score * 0.5) + 
                    (null_score * 0.3) + 
                    (distribution_score * 0.2);
    
    return std::clamp(benefit, 0.0, 1.0);
}

std::string SelectivityAnalyzer::determineDistribution(
    const std::map<std::string, int>& value_counts,
    int64_t total) const {
    
    if (value_counts.empty()) {
        return "sparse";
    }
    
    // Calculate variance
    double mean = static_cast<double>(total) / value_counts.size();
    double variance = 0.0;
    
    for (const auto& [value, count] : value_counts) {
        double diff = count - mean;
        variance += diff * diff;
    }
    variance /= value_counts.size();
    
    double std_dev = std::sqrt(variance);
    double cv = std_dev / mean;  // Coefficient of variation
    
    // Classification based on coefficient of variation
    if (cv < 0.3) {
        return "uniform";
    } else if (cv < 1.0) {
        return "skewed";
    } else {
        return "sparse";
    }
}

nlohmann::json SelectivityAnalyzer::SelectivityStats::toJson() const {
    return nlohmann::json{
        {"collection", collection},
        {"field", field},
        {"total_documents", total_documents},
        {"unique_values", unique_values},
        {"null_count", null_count},
        {"selectivity", selectivity},
        {"distribution", distribution},
        {"estimated_l3_cache_fit_ratio", estimated_l3_cache_fit_ratio},
        {"estimated_cache_miss_rate", estimated_cache_miss_rate}
    };
}

SelectivityAnalyzer::SelectivityStats 
SelectivityAnalyzer::SelectivityStats::fromJson(const nlohmann::json& j) {
    SelectivityStats s;
    s.collection = j.value("collection", "");
    s.field = j.value("field", "");
    s.total_documents = j.value("total_documents", 0);
    s.unique_values = j.value("unique_values", 0);
    s.null_count = j.value("null_count", 0);
    s.selectivity = j.value("selectivity", 0.0);
    s.distribution = j.value("distribution", "");
    s.estimated_l3_cache_fit_ratio = j.value("estimated_l3_cache_fit_ratio", 0.0);
    s.estimated_cache_miss_rate = j.value("estimated_cache_miss_rate", 0.0);
    return s;
}

// Phase 2: Cache-aware selectivity analysis
SelectivityAnalyzer::SelectivityStats 
SelectivityAnalyzer::analyzeCacheAware(const SelectivityStats& stats,
                                      size_t l3_cache_size_mb) const {
    SelectivityStats result = stats;
    
    // Estimate index size in bytes
    // Assume: 8 bytes per unique value (approximate)
    size_t estimated_index_size_bytes = stats.unique_values * 8;
    size_t l3_cache_size_bytes = l3_cache_size_mb * 1024 * 1024;
    
    // Calculate how much of index fits in L3 cache
    if (estimated_index_size_bytes <= l3_cache_size_bytes) {
        result.estimated_l3_cache_fit_ratio = 1.0;  // Entire index fits
    } else {
        result.estimated_l3_cache_fit_ratio = 
            static_cast<double>(l3_cache_size_bytes) / estimated_index_size_bytes;
    }
    
    // Estimate cache miss rate based on working set size
    // Formula: miss_rate = 1 - fit_ratio (simplified model)
    result.estimated_cache_miss_rate = 1.0 - result.estimated_l3_cache_fit_ratio;
    
    // Apply penalty for sparse distribution (worse cache locality)
    if (stats.distribution == "sparse") {
        result.estimated_cache_miss_rate *= 1.5;  // 50% penalty
    } else if (stats.distribution == "skewed") {
        result.estimated_cache_miss_rate *= 1.2;  // 20% penalty
    }
    
    // Clamp to [0.0, 1.0]
    result.estimated_cache_miss_rate = std::clamp(result.estimated_cache_miss_rate, 0.0, 1.0);
    
    return result;
}

// ===== IndexSuggestionEngine Implementation =====

IndexSuggestionEngine::IndexSuggestionEngine(QueryPatternTracker* tracker,
                                            SelectivityAnalyzer* analyzer)
    : tracker_(tracker), analyzer_(analyzer) {
    if (!tracker_) {
        throw std::invalid_argument("IndexSuggestionEngine: tracker cannot be null");
    }
    if (!analyzer_) {
        throw std::invalid_argument("IndexSuggestionEngine: analyzer cannot be null");
    }
}

std::vector<IndexSuggestionEngine::IndexSuggestion> 
IndexSuggestionEngine::generateSuggestions(const std::string& collection,
                                          double min_score,
                                          size_t limit) {
    auto patterns = tracker_->getPatterns(collection);
    std::vector<IndexSuggestion> suggestions;
    
    for (const auto& pattern : patterns) {
        // Skip if index already exists
        if (indexExists(pattern.collection, pattern.field)) {
            continue;
        }
        
        // Analyze selectivity
        SelectivityAnalyzer::SelectivityStats stats;
        {
            std::lock_guard<std::mutex> analyzerLock(analyzerMutex_);
            stats = analyzer_->analyze(pattern.collection, pattern.field, 1000);
        }
        
        // Calculate score
        double score = calculateScore(pattern, stats);
        
        if (score < min_score) {
            continue;
        }
        
        IndexSuggestion suggestion;
        suggestion.collection = pattern.collection;
        suggestion.field = pattern.field;
        suggestion.index_type = recommendIndexType(pattern, stats);
        suggestion.score = score;
        suggestion.reason = generateReason(pattern, stats, suggestion.index_type);
        suggestion.queries_affected = static_cast<int64_t>(pattern.count);
        suggestion.estimated_speedup_ms = static_cast<int64_t>(pattern.total_time_ms * 0.5);  // Estimate 50% speedup
        
        suggestion.metadata = nlohmann::json{
            {"pattern", pattern.toJson()},
            {"selectivity", stats.toJson()}
        };
        
        suggestions.push_back(suggestion);
    }
    
    // Sort by score (descending)
    std::sort(suggestions.begin(), suggestions.end(),
             [](const IndexSuggestion& a, const IndexSuggestion& b) {
                 return a.score > b.score;
             });
    
    if (suggestions.size() > limit) {
        suggestions.resize(limit);
    }
    
    return suggestions;
}

bool IndexSuggestionEngine::indexExists(const std::string& collection,
                                       const std::string& field) const {
    std::shared_lock<std::shared_mutex> lock(existingIndexesMutex_);
    return existingIndexes_.count(collection + ":" + field) > 0;
}

void IndexSuggestionEngine::registerIndex(const std::string& collection,
                                          const std::string& field) {
    std::unique_lock<std::shared_mutex> lock(existingIndexesMutex_);
    existingIndexes_.insert(collection + ":" + field);
}

void IndexSuggestionEngine::unregisterIndex(const std::string& collection,
                                             const std::string& field) {
    std::unique_lock<std::shared_mutex> lock(existingIndexesMutex_);
    existingIndexes_.erase(collection + ":" + field);
}

double IndexSuggestionEngine::calculateScore(
    const QueryPatternTracker::QueryPattern& pattern,
    const SelectivityAnalyzer::SelectivityStats& stats) const {
    
    // Factors:
    // 1. Query frequency (higher = better)
    // 2. Average query time (higher = more benefit)
    // 3. Selectivity benefit (from analyzer)
    
    // Normalize frequency (log scale to prevent dominance)
    double freq_score = std::log10(pattern.count + 1) / 5.0;  // Cap at ~100k queries
    freq_score = std::clamp(freq_score, 0.0, 1.0);
    
    // Normalize avg time (assume 100ms = high)
    double avg_time = pattern.count > 0 ? 
        static_cast<double>(pattern.total_time_ms) / pattern.count : 0.0;
    double time_score = std::min(avg_time / 100.0, 1.0);
    
    // Selectivity benefit
    double selectivity_score = 0.0;
    {
        std::lock_guard<std::mutex> analyzerLock(analyzerMutex_);
        selectivity_score = analyzer_->calculateIndexBenefit(stats);
    }
    
    // Weighted average
    double score = (freq_score * 0.4) + 
                  (time_score * 0.3) + 
                  (selectivity_score * 0.3);
    
    return std::clamp(score, 0.0, 1.0);
}

std::string IndexSuggestionEngine::recommendIndexType(
    const QueryPatternTracker::QueryPattern& pattern,
    const SelectivityAnalyzer::SelectivityStats& stats) const {
    
    if (pattern.operation == "range") {
        return "range";
    } else if (pattern.operation == "eq") {
        // High selectivity = hash, low selectivity = range
        return stats.selectivity > 0.5 ? "hash" : "range";
    } else if (pattern.operation == "in") {
        return "hash";
    } else if (pattern.operation == "join") {
        return "hash";
    }
    
    return "range";  // Default
}

std::string IndexSuggestionEngine::generateReason(
    const QueryPatternTracker::QueryPattern& pattern,
    const SelectivityAnalyzer::SelectivityStats& stats,
    const std::string& index_type) const {
    
    std::string reason = "Field '" + pattern.field + "' in collection '" + 
                        pattern.collection + "' is frequently used in " + 
                        pattern.operation + " operations (" + 
                        std::to_string(pattern.count) + " queries). ";
    
    if (stats.selectivity > 0.7) {
        reason += "High selectivity (" + 
                 std::to_string(static_cast<int>(stats.selectivity * 100)) + 
                 "%) makes " + index_type + " index very effective.";
    } else if (stats.selectivity > 0.3) {
        reason += "Moderate selectivity suggests " + index_type + " index would help.";
    } else {
        reason += "Low selectivity, but high query frequency justifies index.";
    }
    
    return reason;
}

nlohmann::json IndexSuggestionEngine::IndexSuggestion::toJson() const {
    return nlohmann::json{
        {"collection", collection},
        {"field", field},
        {"index_type", index_type},
        {"score", score},
        {"reason", reason},
        {"queries_affected", queries_affected},
        {"estimated_speedup_ms", estimated_speedup_ms},
        {"metadata", metadata}
    };
}

IndexSuggestionEngine::IndexSuggestion 
IndexSuggestionEngine::IndexSuggestion::fromJson(const nlohmann::json& j) {
    IndexSuggestion s;
    s.collection = j.value("collection", "");
    s.field = j.value("field", "");
    s.index_type = j.value("index_type", "");
    s.score = j.value("score", 0.0);
    s.reason = j.value("reason", "");
    s.queries_affected = j.value("queries_affected", 0);
    s.estimated_speedup_ms = j.value("estimated_speedup_ms", 0);
    s.metadata = j.value("metadata", nlohmann::json::object());
    return s;
}

// Phase 2: Cache-aware index suggestion generation
std::vector<IndexSuggestionEngine::IndexSuggestion> 
IndexSuggestionEngine::generateCacheAwareIndexes(
    const std::string& collection,
    float target_cache_hit_rate,
    double min_score,
    size_t limit) {
    
    auto patterns = tracker_->getPatterns(collection);
    std::vector<IndexSuggestion> suggestions;
    
    for (const auto& pattern : patterns) {
        // Skip if index already exists
        if (indexExists(pattern.collection, pattern.field)) {
            continue;
        }
        
        // Analyze selectivity
        SelectivityAnalyzer::SelectivityStats stats;
        SelectivityAnalyzer::SelectivityStats cache_aware_stats;
        {
            std::lock_guard<std::mutex> analyzerLock(analyzerMutex_);
            stats = analyzer_->analyze(pattern.collection, pattern.field, 1000);
            // Phase 2: Apply cache-aware analysis
            cache_aware_stats = analyzer_->analyzeCacheAware(stats);
        }
        
        // Calculate cache-aware score
        double base_score = calculateScore(pattern, cache_aware_stats);
        
        // Adjust score based on cache behavior
        double cache_hit_rate = static_cast<double>(pattern.cache_hits) / 
                               std::max(static_cast<decltype(pattern.cache_hits)>(1), pattern.cache_hits + pattern.cache_misses);
        
        // Penalty for high cache miss rate
        double cache_penalty = 0.0;
        if (cache_hit_rate < target_cache_hit_rate) {
            cache_penalty = (target_cache_hit_rate - cache_hit_rate) * 0.3;  // Up to 30% penalty
        }
        
        // Bonus for indexes that fit in L3 cache
        double cache_fit_bonus = cache_aware_stats.estimated_l3_cache_fit_ratio * 0.2;  // Up to 20% bonus
        
        // Final score
        double score = base_score - cache_penalty + cache_fit_bonus;
        score = std::clamp(score, 0.0, 1.0);
        
        if (score < min_score) {
            continue;
        }
        
        IndexSuggestion suggestion;
        suggestion.collection = pattern.collection;
        suggestion.field = pattern.field;
        suggestion.index_type = recommendIndexType(pattern, cache_aware_stats);
        suggestion.score = score;
        suggestion.reason = generateReason(pattern, cache_aware_stats, suggestion.index_type);
        
        // Add cache-aware information to reason
        if (cache_aware_stats.estimated_l3_cache_fit_ratio < 1.0) {
            suggestion.reason += " NOTE: Index size exceeds L3 cache (" +
                                std::to_string(static_cast<int>(cache_aware_stats.estimated_l3_cache_fit_ratio * 100)) +
                                "% fits), expect " +
                                std::to_string(static_cast<int>(cache_aware_stats.estimated_cache_miss_rate * 100)) +
                                "% cache miss rate.";
        } else {
            suggestion.reason += " Index fits entirely in L3 cache (optimal performance).";
        }
        
        suggestion.queries_affected = static_cast<int64_t>(pattern.count);
        
        // Estimate speedup considering cache misses
        double cache_miss_penalty = pattern.avg_cache_miss_penalty_ms * pattern.cache_misses;
        suggestion.estimated_speedup_ms = static_cast<int64_t>(
            pattern.total_time_ms * 0.5 + cache_miss_penalty * 0.5  // 50% speedup + 50% cache miss recovery
        );
        
        suggestion.metadata = nlohmann::json{
            {"pattern", pattern.toJson()},
            {"selectivity", cache_aware_stats.toJson()},
            {"cache_aware", true},
            {"target_cache_hit_rate", target_cache_hit_rate},
            {"current_cache_hit_rate", cache_hit_rate},
            {"cache_fit_ratio", cache_aware_stats.estimated_l3_cache_fit_ratio},
            {"cache_miss_rate", cache_aware_stats.estimated_cache_miss_rate}
        };
        
        suggestions.push_back(suggestion);
    }
    
    // Sort by score (descending)
    std::sort(suggestions.begin(), suggestions.end(),
             [](const IndexSuggestion& a, const IndexSuggestion& b) {
                 return a.score > b.score;
             });
    
    if (suggestions.size() > limit) {
        suggestions.resize(limit);
    }
    
    return suggestions;
}

// ===== AdaptiveIndexManager Implementation =====

AdaptiveIndexManager::AdaptiveIndexManager(rocksdb::TransactionDB* db)
    : db_(db), tracker_(), analyzer_(db), engine_(&tracker_, &analyzer_) {
    if (!db_) {
        throw std::invalid_argument("AdaptiveIndexManager: db cannot be null");
    }
}

std::vector<IndexSuggestionEngine::IndexSuggestion> 
AdaptiveIndexManager::getSuggestions(const std::string& collection,
                                    double min_score,
                                    size_t limit) {
    return engine_.generateSuggestions(collection, min_score, limit);
}

std::vector<QueryPatternTracker::QueryPattern> 
AdaptiveIndexManager::getPatterns(const std::string& collection) {
    return tracker_.getPatterns(collection);
}

} // namespace themis
