/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plan_cache.cpp                                     ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:50:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     341                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • cebe1e7f8d  2026-03-14  feat(query): implement Query Plan Caching (v1.7.0, Issue ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Query Plan Cache implementation — v1.7.0

#include "query/plan_cache.h"
#include "utils/logger.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

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
                 stats_.hits, stats_.misses, stats_.invalidations);
}

// =============================================================================
// Static helper: fingerprint
// =============================================================================

std::string PlanCache::fingerprint(const std::string& query) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(query.data()),
           query.size(), hash);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return ss.str();
}

// =============================================================================
// get
// =============================================================================

std::optional<PlanCache::CachedPlan> PlanCache::get(
    const std::string& query,
    const Statistics&  current_stats)
{
    const std::string fp = fingerprint(query);

    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = cache_.find(fp);
    if (it == cache_.end()) {
        ++stats_.misses;
        THEMIS_DEBUG("PlanCache miss (not found): fp={}", fp.substr(0, 16));
        return std::nullopt;
    }

    CachedPlan& cached = it->second.plan;

    // --- Age check (24 h by default) ----------------------------------------
    if (cached.isExpired(config_.max_plan_age)) {
        THEMIS_DEBUG("PlanCache miss (expired): fp={}", fp.substr(0, 16));
        ++stats_.evictions;
        removeEntry_locked(it);
        ++stats_.misses;
        return std::nullopt;
    }

    // --- Statistics drift check ---------------------------------------------
    if (isDriftExceeded(cached.statistics_snapshot, current_stats)) {
        THEMIS_DEBUG("PlanCache miss (stats drift): fp={}", fp.substr(0, 16));
        ++stats_.stat_drifts;
        removeEntry_locked(it);
        ++stats_.misses;
        return std::nullopt;
    }

    // --- Cache hit ----------------------------------------------------------
    // Move to front of LRU list
    lru_list_.erase(it->second.lru_it);
    lru_list_.push_front(fp);
    it->second.lru_it = lru_list_.begin();

    ++stats_.hits;
    THEMIS_DEBUG("PlanCache hit: fp={}", fp.substr(0, 16));
    return cached;
}

// =============================================================================
// put
// =============================================================================

void PlanCache::put(const std::string&                query,
                    const QueryOptimizer::Plan&        plan,
                    const Statistics&                  stats,
                    const std::vector<ParameterInfo>&  params,
                    const std::vector<std::string>&    tables)
{
    const std::string fp = fingerprint(query);

    std::lock_guard<std::mutex> lock(cache_mutex_);

    // If already present, update in-place (refresh)
    auto existing = cache_.find(fp);
    if (existing != cache_.end()) {
        // Remove old table-index entries
        for (const auto& tbl : existing->second.plan.referenced_tables) {
            auto& fps = table_index_[tbl];
            fps.erase(std::remove(fps.begin(), fps.end(), fp), fps.end());
        }
        lru_list_.erase(existing->second.lru_it);
        cache_.erase(existing);
        --stats_.current_size;
    }

    // Evict LRU if at capacity
    while (stats_.current_size >= config_.max_entries && !cache_.empty()) {
        evictLRU_locked();
    }

    // Build entry
    CachedPlan cp;
    cp.query_fingerprint    = fp;
    cp.plan                 = plan;
    cp.parameters           = params;
    cp.created_at           = std::chrono::system_clock::now();
    cp.statistics_snapshot  = stats;
    cp.referenced_tables    = tables;

    // Insert into LRU list (most recent at front)
    lru_list_.push_front(fp);

    Entry entry;
    entry.plan   = std::move(cp);
    entry.lru_it = lru_list_.begin();

    cache_.emplace(fp, std::move(entry));
    ++stats_.current_size;

    // Update table index
    for (const auto& tbl : tables) {
        table_index_[tbl].push_back(fp);
    }

    THEMIS_DEBUG("PlanCache stored: fp={}, tables={}", fp.substr(0, 16), tables.size());
}

// =============================================================================
// invalidateTable
// =============================================================================

size_t PlanCache::invalidateTable(const std::string& table) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto tidx = table_index_.find(table);
    if (tidx == table_index_.end()) {
        return 0;
    }

    // Copy fingerprints to avoid iterator invalidation
    std::vector<std::string> fps = tidx->second;
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

    stats_.invalidations += count;
    THEMIS_INFO("PlanCache invalidated {} plan(s) for table '{}'", count, table);
    return count;
}

// =============================================================================
// evictExpired
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
        ++stats_.evictions;
    }

    if (!to_remove.empty()) {
        THEMIS_DEBUG("PlanCache evicted {} expired plan(s)", to_remove.size());
    }
    return to_remove.size();
}

// =============================================================================
// clear
// =============================================================================

void PlanCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
    lru_list_.clear();
    table_index_.clear();
    stats_.current_size = 0;
    THEMIS_DEBUG("PlanCache cleared");
}

// =============================================================================
// getStats
// =============================================================================

PlanCache::CacheStats PlanCache::getStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto s = stats_;
    s.current_size = cache_.size();
    return s;
}

// =============================================================================
// Private helpers
// =============================================================================

void PlanCache::evictLRU_locked() {
    if (lru_list_.empty()) return;

    const std::string& fp = lru_list_.back();
    auto it = cache_.find(fp);
    if (it != cache_.end()) {
        removeEntry_locked(it);
        ++stats_.evictions;
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

    // Remove from main map
    cache_.erase(it);

    if (stats_.current_size > 0) {
        --stats_.current_size;
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
            if (snap_count != cur_count) return true;
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

} // namespace query
} // namespace themis
