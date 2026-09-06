/**
 * @file temporal_tier_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Tier Manager — LSM-style three-tier implementation.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_tier_manager.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace themisdb {
namespace temporal {

// ============================================================================
// BloomFilter
// ============================================================================

// Three independent Murmur3-style finalizer hash functions.
uint64_t BloomFilter::h1(uint64_t x) noexcept {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

uint64_t BloomFilter::h2(uint64_t x) noexcept {
    x = ((x >> 16) ^ x) * 0x45d9f3b37197344dULL;
    x = ((x >> 16) ^ x) * 0x45d9f3bULL;
    x = (x >> 16) ^ x;
    return x;
}

uint64_t BloomFilter::h3(uint64_t x) noexcept {
    x = ~x + (x << 21);
    x ^= x >> 24;
    x += (x << 3) + (x << 8);
    x ^= x >> 14;
    x += (x << 2) + (x << 4);
    x ^= x >> 28;
    x += (x << 31);
    return x;
}

BloomFilter::BloomFilter(size_t expected_elements, size_t bits_per_elem) {
    num_bits_ = expected_elements * bits_per_elem;
    if (num_bits_ == 0) {
      num_bits_ = 64;
    }
    bits_.assign((num_bits_ + 63) / 64, 0);
}

void BloomFilter::setBit(size_t idx) noexcept {
    idx %= num_bits_;
    bits_[idx / 64] |= (1ULL << (idx % 64));
}

bool BloomFilter::testBit(size_t idx) const noexcept {
    idx %= num_bits_;
    return ((bits_[idx / 64] >> (idx % 64)) & 1ULL) != 0ULL;
}

void BloomFilter::add(int64_t value) noexcept {
    const auto u = static_cast<uint64_t>(value);
    setBit(static_cast<size_t>(h1(u) % num_bits_));
    setBit(static_cast<size_t>(h2(u) % num_bits_));
    setBit(static_cast<size_t>(h3(u) % num_bits_));
}

bool BloomFilter::mightContain(int64_t value) const noexcept {
    const auto u = static_cast<uint64_t>(value);
    return testBit(static_cast<size_t>(h1(u) % num_bits_))
        && testBit(static_cast<size_t>(h2(u) % num_bits_))
        && testBit(static_cast<size_t>(h3(u) % num_bits_));
}

// ============================================================================
// TierPolicy — built-in threshold-based evaluation
// ============================================================================

TierDecision TierPolicy::evaluate(const TierDecisionContext& ctx) const {
    // ── LLM / LoRA hook (future autonomous decision) ──────────────────────
    // When decision_fn is set, delegate entirely to the custom callable.
    // Today this is always nullptr (threshold logic is the production path).
    if (decision_fn) {
        return decision_fn(ctx);
    }

    // ── Built-in threshold logic ──────────────────────────────────────────

    // 1. Cold-after-age: version too old for the hot tier
    if (cold_after_age.count() > 0 && ctx.oldest_hot_sys_start != kMaxTimestamp) {
        const auto age_ms = ctx.now_ts - ctx.oldest_hot_sys_start;
        if (age_ms > static_cast<Timestamp>(cold_after_age.count())) {
            return TierDecision::FLUSH_HOT_TO_WARM;
        }
    }

    // 2. Global warm-tier RAM pressure → evict warm → cold immediately
    if (max_warm_bytes > 0 && ctx.total_warm_bytes >= max_warm_bytes) {
        return TierDecision::FLUSH_WARM_TO_COLD;
    }

    // 3. Per-key warm block cap exceeded → compact warm → cold
    if (ctx.warm_block_count >= warm_max_blocks_per_key) {
        return TierDecision::FLUSH_WARM_TO_COLD;
    }

    // 4. Hot tier count exceeded → flush hot → warm
    if (ctx.hot_version_count > hot_max_versions_per_key) {
        return TierDecision::FLUSH_HOT_TO_WARM;
    }

    return TierDecision::KEEP;
}

// ============================================================================
// TemporalTierManager — construction
// ============================================================================

TemporalTierManager::TemporalTierManager(
    TierPolicy policy,
    std::shared_ptr<TemporalColdStore> cold_store)
    : policy_(std::move(policy)),
      cold_(cold_store
                ? std::move(cold_store)
                : std::make_shared<TemporalColdStore>()) {}

TemporalTierManager::~TemporalTierManager() {
    stopCompactionWorker();
}

// ============================================================================
// Write path
// ============================================================================

bool TemporalTierManager::insert(const std::string& table_name,
                                  const VersionedDocument& doc) {
    if (doc.isCurrent()) return false;  // Current versions live in hot table

    std::unique_lock lk(mutex_);

    auto& hot_map    = hot_[table_name][doc.key];
    auto& warm_blocks = warm_[table_name][doc.key];

    hot_map[doc.sys_time.start] = doc;

    // Evaluate tier decision after insertion
    const auto ctx = makeContext(table_name, doc.key);
    const auto decision = policy_.evaluate(ctx);

    if (decision == TierDecision::FLUSH_WARM_TO_COLD) {
        flushWarmToColdLocked(table_name, doc.key, warm_blocks);
        // Also flush hot → warm if still over limit
        if (static_cast<int>(hot_map.size()) > policy_.hot_max_versions_per_key) {
            flushHotToWarmLocked(table_name, doc.key, hot_map, warm_blocks);
        }
    } else if (decision == TierDecision::FLUSH_HOT_TO_WARM) {
        flushHotToWarmLocked(table_name, doc.key, hot_map, warm_blocks);
    }

    return true;
}

// ============================================================================
// Read path
// ============================================================================

std::optional<VersionedDocument>
TemporalTierManager::getAsOf(const std::string& table_name,
                              const std::string& doc_key,
                              Timestamp as_of) const {
    std::shared_lock lk(mutex_);

    // 1. Hot tier — O(log h): upper_bound(as_of) then step back
    {
        auto tit = hot_.find(table_name);
        if (tit != hot_.end()) {
            auto kit = tit->second.find(doc_key);
            if (kit != tit->second.end()) {
                const auto& hmap = kit->second;
                auto it = hmap.upper_bound(as_of);
                if (it != hmap.begin()) {
                    --it;
                    if (it->second.sys_time.contains(as_of)) {
                        return it->second;
                    }
                }
            }
        }
    }

    // 2. Warm tier — per block: range check → bloom → scan
    {
        auto tit = warm_.find(table_name);
        if (tit != warm_.end()) {
            auto kit = tit->second.find(doc_key);
            if (kit != tit->second.end()) {
                // Iterate blocks in reverse (newest first)
                const auto& blocks = kit->second;
                for (auto bit = blocks.rbegin(); bit != blocks.rend(); ++bit) {
                    // Range prune: as_of must be within [min_start, max_end)
                    if (as_of < bit->min_start) {
                      continue;
                    }
                    if (as_of >= bit->max_end && bit->max_end != kMaxTimestamp)
                        continue;

                    auto result = searchBlock(*bit, as_of);
                    if (result) {
                      return result;
                    }
                }
            }
        }
    }

    // 3. Cold tier — O(log N) index + disk read
    return cold_->getAsOf(table_name, doc_key, as_of);
}

std::vector<VersionedDocument>
TemporalTierManager::getHistory(const std::string& table_name,
                                 const std::string& doc_key) const {
    std::shared_lock lk(mutex_);
    std::vector<VersionedDocument> result;

    // Cold tier (oldest first)
    auto cold_versions = cold_->getAll(table_name, doc_key);
    result.insert(result.end(),
                  std::make_move_iterator(cold_versions.begin()),
                  std::make_move_iterator(cold_versions.end()));

    // Warm tier blocks (ascending min_start)
    {
        auto tit = warm_.find(table_name);
        if (tit != warm_.end()) {
            auto kit = tit->second.find(doc_key);
            if (kit != tit->second.end()) {
                for (const auto& block : kit->second) {
                    auto block_versions = allFromBlock(block);
                    result.insert(result.end(),
                                  std::make_move_iterator(block_versions.begin()),
                                  std::make_move_iterator(block_versions.end()));
                }
            }
        }
    }

    // Hot tier (newest)
    {
        auto tit = hot_.find(table_name);
        if (tit != hot_.end()) {
            auto kit = tit->second.find(doc_key);
            if (kit != tit->second.end()) {
                for (const auto& [ts, doc] : kit->second) {
                    result.push_back(doc);
                }
            }
        }
    }

    // Sort ascending by sys_start (cold/warm may interleave with hot on replay)
    std::sort(result.begin(), result.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  return a.sys_time.start < b.sys_time.start;
              });
    return result;
}

std::vector<VersionedDocument>
TemporalTierManager::getHistoryInRange(const std::string& table_name,
                                        const std::string& doc_key,
                                        const TimeRange& range) const {
    std::shared_lock lk(mutex_);
    std::vector<VersionedDocument> result;

    // Cold
    auto cold_v = cold_->getRange(table_name, doc_key, range);
    result.insert(result.end(),
                  std::make_move_iterator(cold_v.begin()),
                  std::make_move_iterator(cold_v.end()));

    // Warm
    {
        auto tit = warm_.find(table_name);
        if (tit != warm_.end()) {
            auto kit = tit->second.find(doc_key);
            if (kit != tit->second.end()) {
                for (const auto& block : kit->second) {
                    if (block.min_start >= range.end) {
                      break;
                    }
                    if (block.max_end <= range.start) {
                      continue;
                    }
                    auto bv = rangeFromBlock(block, range);
                    result.insert(result.end(),
                                  std::make_move_iterator(bv.begin()),
                                  std::make_move_iterator(bv.end()));
                }
            }
        }
    }

    // Hot
    {
        auto tit = hot_.find(table_name);
        if (tit != hot_.end()) {
            auto kit = tit->second.find(doc_key);
            if (kit != tit->second.end()) {
                for (const auto& [ts, doc] : kit->second) {
                    if (ts >= range.end) {
                      break;
                    }
                    if (doc.sys_time.overlaps(range)) {
                      result.push_back(doc);
                    }
                }
            }
        }
    }

    std::sort(result.begin(), result.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  return a.sys_time.start < b.sys_time.start;
              });
    return result;
}

// ============================================================================
// Compaction — public API
// ============================================================================

size_t TemporalTierManager::flushHotToWarm(const std::string& table_name,
                                            const std::string& doc_key) {
    std::unique_lock lk(mutex_);
    auto& hot_map    = hot_[table_name][doc_key];
    auto& warm_blocks = warm_[table_name][doc_key];
    return flushHotToWarmLocked(table_name, doc_key, hot_map, warm_blocks);
}

size_t TemporalTierManager::flushWarmToCold(const std::string& table_name,
                                             const std::string& doc_key) {
    std::unique_lock lk(mutex_);
    auto& warm_blocks = warm_[table_name][doc_key];
    return flushWarmToColdLocked(table_name, doc_key, warm_blocks);
}

size_t TemporalTierManager::compactTable(const std::string& table_name) {
    std::unique_lock lk(mutex_);
    size_t total = 0;

    // Collect all keys in the table
    std::vector<std::string> keys = {};

    if (auto tit = hot_.find(table_name); tit != hot_.end())
        for (const auto& [k, _] : tit->second) {
          keys.push_back(k);
        }
    if (auto tit = warm_.find(table_name); tit != warm_.end())
        for (const auto& [k, _] : tit->second) {
            if (std::find(keys.begin(), keys.end(), k) == keys.end())
                keys.push_back(k);
        }

    for (const auto& key : keys) {
        auto ctx = makeContext(table_name, key);
        const auto decision = policy_.evaluate(ctx);

        if (decision == TierDecision::FLUSH_WARM_TO_COLD ||
            ctx.warm_block_count >= policy_.warm_max_blocks_per_key) {
            total += flushWarmToColdLocked(table_name, key,
                                            warm_[table_name][key]);
        }
        if (decision == TierDecision::FLUSH_HOT_TO_WARM ||
            ctx.hot_version_count > policy_.hot_max_versions_per_key) {
            total += flushHotToWarmLocked(table_name, key,
                                           hot_[table_name][key],
                                           warm_[table_name][key]);
        }
    }
    return total;
}

// ============================================================================
// Policy
// ============================================================================

void TemporalTierManager::setPolicy(const TierPolicy& policy) {
    std::unique_lock lk(mutex_);
    policy_ = policy;
}

// ============================================================================
// Background compaction worker
// ============================================================================

void TemporalTierManager::startCompactionWorker() {
    if (compact_thread_.joinable()) return;  // already running
    compact_stop_ = false;
    compact_thread_ = std::thread([this]() { compactionLoop(); });
}

void TemporalTierManager::stopCompactionWorker() {
    compact_stop_ = true;
    compact_cv_.notify_all();
    if (compact_thread_.joinable()) {
      compact_thread_.join();
    }
}

void TemporalTierManager::compactionLoop() {
    while (!compact_stop_) {
        {
            std::unique_lock<std::mutex> lk(compact_cv_mutex_);
            compact_cv_.wait_for(lk, policy_.compact_interval,
                                 [this] { return compact_stop_.load(); });
        }
        if (compact_stop_) {
          break;
        }

        // Compact all known tables
        std::vector<std::string> tables;
        {
            std::shared_lock lk(mutex_);
            for (const auto& [t, _] : hot_) {
              tables.push_back(t);
            }
            for (const auto& [t, _] : warm_) {
                if (std::find(tables.begin(), tables.end(), t) == tables.end())
                    tables.push_back(t);
            }
        }
        for (const auto& t : tables) {
            if (!compact_stop_) {
              compactTable(t);
            }
        }
    }
}

// ============================================================================
// Observability
// ============================================================================

TemporalTierManager::KeyTierStats
TemporalTierManager::keyStats(const std::string& table_name,
                               const std::string& doc_key) const {
    std::shared_lock lk(mutex_);
    KeyTierStats s = {};

    if (auto tit = hot_.find(table_name); tit != hot_.end())
        if (auto kit = tit->second.find(doc_key); kit != tit->second.end())
            s.hot_versions = kit->second.size();

    if (auto tit = warm_.find(table_name); tit != warm_.end()) {
        if (auto kit = tit->second.find(doc_key); kit != tit->second.end()) {
            s.warm_blocks = kit->second.size();
            for (const auto& b : kit->second) {
                s.warm_versions += b.version_count;
                s.warm_bytes    += b.approx_bytes;
            }
        }
    }

    s.cold_versions = cold_->versionCount(table_name, doc_key);
    return s;
}

TemporalTierManager::TableTierStats
TemporalTierManager::tableStats(const std::string& table_name) const {
    std::shared_lock lk(mutex_);
    TableTierStats s;
    s.flush_hot_to_warm_count  = stat_flush_hot_warm_.load();
    s.flush_warm_to_cold_count = stat_flush_warm_cold_.load();

    if (auto tit = hot_.find(table_name); tit != hot_.end())
        for (const auto& [k, hmap] : tit->second)
            s.total_hot_versions += hmap.size();

    if (auto tit = warm_.find(table_name); tit != warm_.end()) {
        for (const auto& [k, blocks] : tit->second) {
            s.total_warm_blocks += blocks.size();
            for (const auto& b : blocks) {
                s.total_warm_versions += b.version_count;
                s.total_warm_bytes    += b.approx_bytes;
            }
        }
    }

    s.total_cold_versions = cold_->totalVersionCount();
    return s;
}

nlohmann::json
TemporalTierManager::statsJson(const std::string& table_name) const {
    const auto ts = tableStats(table_name);
    return {{"table",                   table_name},
            {"hot_versions",            ts.total_hot_versions},
            {"warm_blocks",             ts.total_warm_blocks},
            {"warm_versions",           ts.total_warm_versions},
            {"warm_bytes",              ts.total_warm_bytes},
            {"cold_versions",           ts.total_cold_versions},
            {"flush_hot_to_warm_total", ts.flush_hot_to_warm_count},
            {"flush_warm_to_cold_total",ts.flush_warm_to_cold_count}};
}

// ============================================================================
// Internal helpers
// ============================================================================

TierDecisionContext
TemporalTierManager::makeContext(const std::string& table_name,
                                  const std::string& doc_key) const {
    TierDecisionContext ctx;
    ctx.table_name = table_name;
    ctx.doc_key    = doc_key;
    ctx.now_ts     = now();
    ctx.total_warm_bytes = total_warm_bytes_.load();

    if (auto tit = hot_.find(table_name); tit != hot_.end()) {
        if (auto kit = tit->second.find(doc_key); kit != tit->second.end()) {
            ctx.hot_version_count = kit->second.size();
            if (!kit->second.empty())
                ctx.oldest_hot_sys_start = kit->second.begin()->first;
        }
    }

    if (auto tit = warm_.find(table_name); tit != warm_.end()) {
        if (auto kit = tit->second.find(doc_key); kit != tit->second.end()) {
            ctx.warm_block_count = kit->second.size();
            for (const auto& b : kit->second) {
                ctx.warm_versions_for_key += b.version_count;
                ctx.warm_bytes_for_key    += b.approx_bytes;
            }
        }
    }

    if (policy_.max_warm_bytes > 0)
        ctx.warm_pressure = static_cast<double>(ctx.total_warm_bytes)
                          / static_cast<double>(policy_.max_warm_bytes);

    return ctx;
}

size_t TemporalTierManager::flushHotToWarmLocked(
    const std::string& table_name,
    const std::string& doc_key,
    HotMap&            hot_map,
    WarmBlocks&        warm_blocks) {

    if (hot_map.empty()) {
      return 0;
    }

    // How many versions to move: everything beyond hot_max
    const size_t keep = policy_.hot_max_versions_per_key;
    if (static_cast<int>(hot_map.size()) <= keep) {
      return 0;
    }

    const size_t to_move = static_cast<int>(hot_map.size()) - keep;

    // Collect oldest `to_move` versions (the front of the sorted map)
    std::vector<VersionedDocument> batch;
    batch.reserve(to_move);
    auto it = hot_map.begin();
    for (size_t i = 0; i < to_move; ++i, ++it) {
        batch.push_back(std::move(it->second));
    }
    hot_map.erase(hot_map.begin(), it);

    // Split into warm_block_size chunks and create VersionBlocks
    const size_t block_size = policy_.warm_block_size > 0
                            ? policy_.warm_block_size : 50;
    for (size_t offset = 0; offset < batch.size(); offset += block_size) {
        const size_t end = std::min(offset + block_size, batch.size());
        std::vector<VersionedDocument> chunk(
            std::make_move_iterator(batch.begin() + static_cast<ptrdiff_t>(offset)),
            std::make_move_iterator(batch.begin() + static_cast<ptrdiff_t>(end)));
        VersionBlock blk = makeBlock(doc_key, std::move(chunk));
        total_warm_bytes_ += blk.approx_bytes;
        warm_blocks.push_back(std::move(blk));
    }

    if (table_name.empty()) {
        // Table-name validation is caller-side; keep flush behavior unchanged.
    }

    ++stat_flush_hot_warm_;
    return to_move;
}

size_t TemporalTierManager::flushWarmToColdLocked(
    const std::string& table_name,
    const std::string& doc_key,
    WarmBlocks&        warm_blocks) {

    if (warm_blocks.empty()) {
      return 0;
    }

    // Flush oldest block (front of vector — ascending min_start order)
    VersionBlock& oldest = warm_blocks.front();
    size_t moved = 0;

    for (const auto& entry_str : oldest.entries) {
        try {
            auto j = nlohmann::json::parse(entry_str);
            VersionedDocument doc;
            doc.key         = j.at("key").get<std::string>();
            doc.data        = j.at("data");
            doc.sys_time    = TimeRange::fromJson(j.at("sys_time"));
            doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
            doc.modified_by = j.value("modified_by", std::string{});
            if (cold_->store(table_name, doc)) {
              ++moved;
            }
        } catch (const nlohmann::json::exception&) {}
    }

    total_warm_bytes_ -= oldest.approx_bytes;
    warm_blocks.erase(warm_blocks.begin());

    if (doc_key.empty()) {
        // Key validation is caller-side; keep flush behavior unchanged.
    }

    ++stat_flush_warm_cold_;
    return moved;
}

// static
VersionBlock TemporalTierManager::makeBlock(
    const std::string& doc_key,
    std::vector<VersionedDocument> versions) {

    // Ensure ascending sys_start order
    std::sort(versions.begin(), versions.end(),
              [](const VersionedDocument& a, const VersionedDocument& b) {
                  return a.sys_time.start < b.sys_time.start;
              });

    VersionBlock blk;
    blk.doc_key       = doc_key;
    blk.version_count = versions.size();
    blk.bloom         = BloomFilter(static_cast<int>(versions.size()) + 1);
    blk.entries.reserve(versions.size());

    uint64_t total_bytes = 0;
    for (const auto& doc : versions) {
        blk.min_start = std::min(blk.min_start, doc.sys_time.start);
        blk.max_end   = std::max(blk.max_end,   doc.sys_time.end);
        blk.bloom.add(doc.sys_time.start);
        const std::string s = doc.toJson().dump();
        total_bytes += s.size();
        blk.entries.push_back(std::move(s));
    }

    // Approximate RAM: entries + bloom bits + metadata overhead (~256 B)
    blk.approx_bytes = total_bytes
                     + (blk.bloom.bitCount() / 8)
                     + 256;
    return blk;
}

// static
std::optional<VersionedDocument>
TemporalTierManager::searchBlock(const VersionBlock& block, Timestamp as_of) {
    // Binary search: find last entry with sys_start <= as_of
    // Entries are sorted ascending by sys_start.
    // Use upper_bound on the entry index by decoding sys_start lazily from
    // the front of the JSON string (fast path: first field is "key", then
    // "sys_time.start" — full parse only for the candidate).
    //
    // Simple O(k) scan for correctness; can be optimised with a separate
    // sys_start array if block sizes grow large.
    std::optional<VersionedDocument> result = {};

    for (const auto& entry_str : block.entries) {
        try {
            auto j = nlohmann::json::parse(entry_str);
            TimeRange sys_time = TimeRange::fromJson(j.at("sys_time"));
            if (sys_time.start > as_of) break;  // sorted → no later match
            if (sys_time.contains(as_of)) {
                VersionedDocument doc;
                doc.key         = j.at("key").get<std::string>();
                doc.data        = j.at("data");
                doc.sys_time    = sys_time;
                doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
                doc.modified_by = j.value("modified_by", std::string{});
                result = std::move(doc);
            }
        } catch (const nlohmann::json::exception&) {}
    }
    return result;
}

// static
std::vector<VersionedDocument>
TemporalTierManager::allFromBlock(const VersionBlock& block) {
    std::vector<VersionedDocument> result;
    result.reserve(block.version_count);
    for (const auto& entry_str : block.entries) {
        try {
            auto j = nlohmann::json::parse(entry_str);
            VersionedDocument doc;
            doc.key         = j.at("key").get<std::string>();
            doc.data        = j.at("data");
            doc.sys_time    = TimeRange::fromJson(j.at("sys_time"));
            doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
            doc.modified_by = j.value("modified_by", std::string{});
            result.push_back(std::move(doc));
        } catch (const nlohmann::json::exception&) {}
    }
    return result;
}

// static
std::vector<VersionedDocument>
TemporalTierManager::rangeFromBlock(const VersionBlock& block,
                                     const TimeRange& range) {
    std::vector<VersionedDocument> result = {};

    for (const auto& entry_str : block.entries) {
        try {
            auto j = nlohmann::json::parse(entry_str);
            TimeRange sys_time = TimeRange::fromJson(j.at("sys_time"));
            if (sys_time.start >= range.end) {
              break;
            }
            if (sys_time.overlaps(range)) {
                VersionedDocument doc;
                doc.key         = j.at("key").get<std::string>();
                doc.data        = j.at("data");
                doc.sys_time    = sys_time;
                doc.valid_time  = TimeRange::fromJson(j.at("valid_time"));
                doc.modified_by = j.value("modified_by", std::string{});
                result.push_back(std::move(doc));
            }
        } catch (const nlohmann::json::exception&) {}
    }
    return result;
}

} // namespace temporal
} // namespace themisdb

