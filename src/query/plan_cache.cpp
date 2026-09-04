/**
 * @file plan_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Query Plan Cache implementation — v1.7.0

#include "query/plan_cache.h"
#include "utils/logger.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cctype>

namespace themis {
namespace query {

// =============================================================================
// Construction / destruction
// =============================================================================

PlanCache::PlanCache(const Config& config)
    : config_(config) {
    THEMIS_INFO("PlanCache initialized: max_entries={}, max_age={}s, drift_factor={}",
                config_.max_entries,
                config_.max_plan_age.count(),
                config_.statistics_drift_factor);
}

PlanCache::~PlanCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
    lru_list_.clear();
    table_index_.clear();
    THEMIS_DEBUG("PlanCache destroyed: hits={}, misses={}, invalidations={}",
                 stats_.hits.load(std::memory_order_acquire),
                 stats_.misses.load(std::memory_order_acquire),
                 stats_.invalidations.load(std::memory_order_acquire));
}

// =============================================================================
// Static helper: fingerprint
// =============================================================================

std::string PlanCache::fingerprint(const std::string& query) {
    const std::string normalized = normalizeQueryTemplate(query);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(normalized.data()),
           normalized.size(), hash);

    std::ostringstream ss = {};
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return ss.str();
}

std::string PlanCache::normalizeQueryTemplate(std::string_view query) {
    std::string normalized = {};
    normalized.reserve(query.size());

    bool in_single_quote = false;
    bool in_double_quote = false;
    bool literal_placeholder_inserted = false;
    bool last_was_space = false;

    for (size_t i = 0; i < query.size(); ++i) {
        const char ch = query[i];

        if (in_single_quote) {
            if (ch == '\'') {
                if (i + 1 < query.size() && query[i + 1] == '\'') {
                    ++i;
                    continue;
                }
                in_single_quote = false;
                literal_placeholder_inserted = false;
            }
            continue;
        }

        if (in_double_quote) {
            normalized.push_back(ch);
            if (ch == '"') {
                if (i + 1 < query.size() && query[i + 1] == '"') {
                    normalized.push_back(query[++i]);
                    continue;
                }
                in_double_quote = false;
            }
            last_was_space = false;
            continue;
        }

        if (ch == '\'') {
            if (!literal_placeholder_inserted) {
                normalized.push_back('?');
                literal_placeholder_inserted = true;
            }
            in_single_quote = true;
            last_was_space = false;
            continue;
        }

        if (ch == '"') {
            normalized.push_back(ch);
            in_double_quote = true;
            last_was_space = false;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            if (!normalized.empty() && !last_was_space) {
                normalized.push_back(' ');
                last_was_space = true;
            }
            continue;
        }

        // Treat a leading '+' or '-' sign as part of a numeric literal when it
        // is not preceded by an identifier character (so "x-1" keeps its minus
        // but "x=-1" or "WHERE n=-1" normalises the sign away with the digit).
        if ((ch == '+' || ch == '-') &&
            (i == 0 || (!std::isalnum(static_cast<unsigned char>(query[static_cast<int>(i - 1)])) &&
                        query[static_cast<int>(i - 1)] != '_' && query[static_cast<int>(i - 1)] != '@')) &&
            i + 1 < query.size() &&
            std::isdigit(static_cast<unsigned char>(query[i + 1])) != 0) {
            // The sign is a numeric prefix; skip it so the following digit
            // handler collapses the whole signed literal into a single '?'.
            last_was_space = false;
            continue;
        }

        const bool numeric_literal =
            (std::isdigit(static_cast<unsigned char>(ch)) != 0) &&
            (i == 0 ||
             (!std::isalnum(static_cast<unsigned char>(query[static_cast<int>(i - 1)])) &&
              query[static_cast<int>(i - 1)] != '_' && query[static_cast<int>(i - 1)] != '@'));
        if (numeric_literal) {
            if (normalized.empty() || normalized.back() != '?') {
                normalized.push_back('?');
            }
            while (i + 1 < query.size()) {
                const char next = query[i + 1];
                const bool continue_numeric =
                    (std::isdigit(static_cast<unsigned char>(next)) != 0) ||
                    next == '.' || next == '_' ||
                    next == 'x' || next == 'X' ||
                    ((next >= 'a' && next <= 'f') || (next >= 'A' && next <= 'F'));
                if (!continue_numeric) {
                    break;
                }
                ++i;
            }
            last_was_space = false;
            continue;
        }

        normalized.push_back(ch);
        last_was_space = false;
    }

    if (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    return normalized;
}

// =============================================================================
// get (THREAD-SAFE with deadline propagation - GAP-5)
// =============================================================================

std::optional<PlanCache::CachedPlan> PlanCache::get(
    const std::string& query,
    const Statistics&  current_stats,
    const std::string& topology_fingerprint,
    std::optional<std::chrono::steady_clock::time_point> deadline)
{
    const std::string fp = makeCacheKey(query, topology_fingerprint);

    // Check deadline before attempting lock acquisition (GAP-5)
    if (deadline.has_value()) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline.value()) {
            // Deadline already exceeded, fail fast without acquiring lock
            stats_.misses.fetch_add(1, std::memory_order_release);
            THEMIS_DEBUG("PlanCache deadline exceeded on entry: fp={}", fp.substr(0, 16));
            return std::nullopt;
        }
    }

    // Acquire cache lock after pre-lock deadline check (no timed wait on std::mutex).
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = cache_.find(fp);
    if (it == cache_.end()) {
        // Use atomic increment for thread-safe counter update (GAP-4)
        stats_.misses.fetch_add(1, std::memory_order_release);
        THEMIS_DEBUG("PlanCache miss (not found): fp={}", fp.substr(0, 16));
        return std::nullopt;
    }

    CachedPlan& cached = it->second.plan;

    // --- Age check (24 h by default) ----------------------------------------
    if (cached.isExpired(config_.max_plan_age)) {
        THEMIS_DEBUG("PlanCache miss (expired): fp={}", fp.substr(0, 16));
        stats_.evictions.fetch_add(1, std::memory_order_release);
        removeEntry_locked(it);
        stats_.misses.fetch_add(1, std::memory_order_release);
        return std::nullopt;
    }

    // --- Statistics drift check ---------------------------------------------
    if (isDriftExceeded(cached.statistics_snapshot, current_stats)) {
        THEMIS_DEBUG("PlanCache miss (stats drift): fp={}", fp.substr(0, 16));
        stats_.stat_drifts.fetch_add(1, std::memory_order_release);
        removeEntry_locked(it);
        stats_.misses.fetch_add(1, std::memory_order_release);
        return std::nullopt;
    }

    // --- Cache hit ----------------------------------------------------------
    // Move to front of LRU list
    lru_list_.erase(it->second.lru_it);
    lru_list_.push_front(fp);
    it->second.lru_it = lru_list_.begin();

    // Use atomic increment for thread-safe counter update (GAP-4)
    stats_.hits.fetch_add(1, std::memory_order_release);
    THEMIS_DEBUG("PlanCache hit: fp={}", fp.substr(0, 16));
    return cached;
}

// =============================================================================
// put (THREAD-SAFE with deadline propagation - GAP-5)
// =============================================================================

void PlanCache::put(const std::string&                query,
                    const QueryOptimizer::Plan&        plan,
                    const Statistics&                  stats,
                    const std::vector<ParameterInfo>&  params,
                    const std::vector<std::string>&    tables,
                    const std::string&                 topology_fingerprint,
                    std::optional<std::chrono::steady_clock::time_point> deadline)
{
    const std::string fp = makeCacheKey(query, topology_fingerprint);

    // Check deadline before attempting lock acquisition (GAP-5)
    if (deadline.has_value()) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline.value()) {
            THEMIS_DEBUG("PlanCache deadline exceeded on put: fp={}", fp.substr(0, 16));
            return;  // Fail silently, don't cache
        }
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);

    // If already present, update in-place (refresh)
    auto existing = cache_.find(fp);
    if (existing != cache_.end()) {
        removeEntry_locked(existing);
    }

    // Evict LRU if at capacity
    size_t current_size = stats_.current_size.load(std::memory_order_acquire);
    while (current_size >= config_.max_entries && !cache_.empty()) {
        evictLRU_locked();
        current_size = stats_.current_size.load(std::memory_order_acquire);
    }

    // Build entry
    CachedPlan cp;
    cp.query_fingerprint    = fp;
    cp.plan                 = plan;
    cp.parameters           = params;
    cp.created_at           = std::chrono::system_clock::now();
    cp.statistics_snapshot  = stats;
    cp.referenced_tables    = tables;
    cp.topology_fingerprint = topology_fingerprint;
    cp.estimated_size_bytes = estimatePlanSizeBytes(cp);
    cp.consecutive_execution_failures = 0;

    if (config_.max_memory_bytes > 0) {
        const double safe_threshold =
            std::max(0.0, std::min(1.0, config_.memory_eviction_threshold));
        const size_t threshold_bytes = static_cast<size_t>(
            static_cast<double>(config_.max_memory_bytes) * safe_threshold);
        size_t current_memory = stats_.current_memory_bytes.load(std::memory_order_acquire);
        while (!cache_.empty() &&
               (current_memory + cp.estimated_size_bytes > threshold_bytes)) {
            evictLRU_locked();
            current_memory = stats_.current_memory_bytes.load(std::memory_order_acquire);
        }
    }

    // Insert into LRU list (most recent at front)
    lru_list_.push_front(fp);

    Entry entry;
    entry.plan   = std::move(cp);
    entry.lru_it = lru_list_.begin();

    cache_.emplace(fp, std::move(entry));
    // Use atomic operations for counter updates (GAP-4)
    stats_.current_size.fetch_add(1, std::memory_order_release);
    stats_.current_memory_bytes.fetch_add(cp.estimated_size_bytes, std::memory_order_release);

    // Update table index
    for (const auto& tbl : tables) {
        table_index_[tbl].push_back(fp);
    }

    THEMIS_DEBUG("PlanCache stored: fp={}, tables={}", fp.substr(0, 16), tables.size());
}

bool PlanCache::recordExecutionFailure(const std::string& query,
                                       const std::string& topology_fingerprint) {
    const std::string fp = makeCacheKey(query, topology_fingerprint);
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = cache_.find(fp);
    if (it == cache_.end()) {
        return false;
    }

    auto& cached = it->second.plan;
    cached.consecutive_execution_failures += 1;
    if (cached.consecutive_execution_failures < config_.max_consecutive_failures) {
        return false;
    }

    // Use atomic operations for stats update (GAP-4)
    removeEntry_locked(it);
    stats_.evictions.fetch_add(1, std::memory_order_release);
    return true;
}

// =============================================================================
// invalidateTable (THREAD-SAFE - GAP-4)
// =============================================================================

size_t PlanCache::invalidateTable(const std::string& table) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto tidx = table_index_.find(table);
    if (tidx == table_index_.end()) {
        return 0;
    }

    // Copy fingerprints to avoid iterator invalidation
    std::vector<std::string> fps = tidx->second;
    
    // Sort fingerprints for deterministic invalidation order (Batch 1C determinism gate).
    // This ensures plan cache invalidation notifications are always in the same order,
    // which is critical for deterministic behavior in distributed query planning.
    std::sort(fps.begin(), fps.end());
    
    size_t count = 0;

    for (const auto& fp : fps) {
        auto it = cache_.find(fp);
        if (it != cache_.end()) {
            removeEntry_locked(it);
            ++count;
        }
    }

    // The table entry itself is cleaned up inside removeEntry_locked
    // (it removes fp from all table_index_ vectors); explicitly erase now
    // in case the table had entries that were already gone.
    table_index_.erase(table);

    // Use atomic operation for stats update (GAP-4)
    stats_.invalidations.fetch_add(count, std::memory_order_release);
    THEMIS_INFO("PlanCache invalidated {} plan(s) for table '{}' (sorted order)", count, table);
    return count;
}

// =============================================================================
// evictExpired (THREAD-SAFE - GAP-4)
// =============================================================================

size_t PlanCache::evictExpired() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    std::vector<std::unordered_map<std::string, Entry>::iterator> to_remove;

    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.plan.isExpired(config_.max_plan_age)) {
            to_remove.push_back(it);
        }
    }

    for (auto& it : to_remove) {
        removeEntry_locked(it);
        // Use atomic operation for stats update (GAP-4)
        stats_.evictions.fetch_add(1, std::memory_order_release);
    }

    if (!to_remove.empty()) {
        THEMIS_DEBUG("PlanCache evicted {} expired plan(s)", to_remove.size());
    }
    return to_remove.size();
}

// =============================================================================
// clear (THREAD-SAFE - GAP-4)
// =============================================================================

void PlanCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
    lru_list_.clear();
    table_index_.clear();
    // Use atomic stores to reset counters (GAP-4)
    stats_.current_size.store(0, std::memory_order_release);
    stats_.current_memory_bytes.store(0, std::memory_order_release);
    THEMIS_DEBUG("PlanCache cleared");
}

// =============================================================================
// getStats (THREAD-SAFE - GAP-4)
// =============================================================================

PlanCache::CacheStats PlanCache::getStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // Read all atomic counters with acquire semantics (GAP-4)
    CacheStats s;
    s.hits.store(stats_.hits.load(std::memory_order_acquire), std::memory_order_relaxed);
    s.misses.store(stats_.misses.load(std::memory_order_acquire), std::memory_order_relaxed);
    s.invalidations.store(stats_.invalidations.load(std::memory_order_acquire), std::memory_order_relaxed);
    s.evictions.store(stats_.evictions.load(std::memory_order_acquire), std::memory_order_relaxed);
    s.stat_drifts.store(stats_.stat_drifts.load(std::memory_order_acquire), std::memory_order_relaxed);
    s.current_size.store(cache_.size(), std::memory_order_relaxed);
    s.current_memory_bytes.store(stats_.current_memory_bytes.load(std::memory_order_acquire), std::memory_order_relaxed);
    return s;
}

size_t PlanCache::estimateCurrentMemoryBytes() const {
    // No lock needed: read atomic counter with acquire semantics (GAP-4, fast path)
    return stats_.current_memory_bytes.load(std::memory_order_acquire);
}

// =============================================================================
// Private helpers (THREAD-SAFE - GAP-4)
// =============================================================================

void PlanCache::evictLRU_locked() {
    if (lru_list_.empty()) {
      return;
    }

    const std::string& fp = lru_list_.back();
    auto it = cache_.find(fp);
    if (it != cache_.end()) {
        removeEntry_locked(it);
        // Use atomic operation for stats update (GAP-4)
        stats_.evictions.fetch_add(1, std::memory_order_release);
    }
}

void PlanCache::removeEntry_locked(
    std::unordered_map<std::string, Entry>::iterator it)
{
    const std::string fp = it->first;

    // Remove from table index
    for (const auto& tbl : it->second.plan.referenced_tables) {
        auto tidx = table_index_.find(tbl);
        if (tidx != table_index_.end()) {
            auto& vec = tidx->second;
            vec.erase(std::remove(vec.begin(), vec.end(), fp), vec.end());
            if (vec.empty()) {
                table_index_.erase(tidx);
            }
        }
    }

    // Remove from LRU list
    lru_list_.erase(it->second.lru_it);

    // Remove from main map and update memory counter atomically (GAP-4)
    size_t current_memory = stats_.current_memory_bytes.load(std::memory_order_acquire);
    if (current_memory >= it->second.plan.estimated_size_bytes) {
        stats_.current_memory_bytes.fetch_sub(it->second.plan.estimated_size_bytes, std::memory_order_release);
    } else {
        stats_.current_memory_bytes.store(0, std::memory_order_release);
    }
    cache_.erase(it);

    // Decrement size counter atomically (GAP-4)
    size_t current_size = stats_.current_size.load(std::memory_order_acquire);
    if (current_size > 0) {
        stats_.current_size.fetch_sub(1, std::memory_order_release);
    }
}

bool PlanCache::isDriftExceeded(const Statistics& snapshot,
                                const Statistics& current) const {
    for (const auto& [tbl, snap_count] : snapshot.table_cardinalities) {
        auto cur_it = current.table_cardinalities.find(tbl);
        if (cur_it == current.table_cardinalities.end()) {
            // Table no longer present in current stats — treat as drift
            return true;
        }

        size_t cur_count = cur_it->second;

        if (snap_count == 0 || cur_count == 0) {
            // Zero vs non-zero is always drift; equal zeros are fine
            if (snap_count != cur_count) {
              return true;
            }
            continue;
        }

        double ratio = static_cast<double>(
            std::max(snap_count, cur_count)) /
            static_cast<double>(std::min(snap_count, cur_count));

        if (ratio >= config_.statistics_drift_factor) {
            THEMIS_DEBUG("PlanCache stats drift: table='{}' snap={} cur={} ratio={:.1f}",
                         tbl, snap_count, cur_count, ratio);
            return true;
        }
    }
    return false;
}

std::string PlanCache::makeCacheKey(const std::string& query,
                                    const std::string& topology_fingerprint) const {
    if (topology_fingerprint.empty()) {
        return fingerprint(query);
    }

    const std::string composite = normalizeQueryTemplate(query) + "|topology=" + topology_fingerprint;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(composite.data()),
           composite.size(), hash);

    std::ostringstream ss = {};
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return ss.str();
}

size_t PlanCache::estimatePlanSizeBytes(const CachedPlan& plan) {
    size_t total = sizeof(CachedPlan);
    total += plan.query_fingerprint.size();
    total += plan.topology_fingerprint.size();
    total += plan.referenced_tables.size() * sizeof(std::string);
    total += plan.parameters.size() * sizeof(ParameterInfo);
    total += plan.statistics_snapshot.table_cardinalities.size() *
             (sizeof(std::string) + sizeof(size_t));

    for (const auto& table : plan.referenced_tables) {
        total += table.size();
    }
    for (const auto& parameter : plan.parameters) {
        total += parameter.name.size();
        total += parameter.type.size();
        total += parameter.sample_value.size();
    }
    for (const auto& [table, _] : plan.statistics_snapshot.table_cardinalities) {
        total += table.size();
    }
    for (const auto& hint : plan.plan.nlp_suggested_indexes) {
        total += hint.size();
    }
    for (const auto& [key, value] : plan.plan.nlp_hints) {
        total += static_cast<int>(key.size()) + value.size();
    }

    return total;
}

} // namespace query
} // namespace themis
