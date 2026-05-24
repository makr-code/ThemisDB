/*
 * ThemisDB | File: distributed_vector_index.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=15, H=105, M=17, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Distributed Vector Index across Shards (Issue #1879)
//
// See include/index/distributed_vector_index.h for the public API and design notes.

#include "index/distributed_vector_index.h"
#include "index/ann_index.h" // ScaNN

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cctype>

namespace themis {
namespace index {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

// MurmurHash3-inspired 64-bit finaliser for string keys.
// Produces a uniform-looking hash without requiring external libraries.
uint64_t murmur_mix64(uint64_t k) noexcept {
    k ^= k >> 33;
    k *= UINT64_C(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= UINT64_C(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;
    return k;
}

uint64_t hashString(const std::string& s) noexcept {
    uint64_t h = UINT64_C(14695981039346656037); // FNV offset basis
    for (unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= UINT64_C(1099511628211); // FNV prime
    }
    return murmur_mix64(h);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DistributedVectorIndex::DistributedVectorIndex(const DistributedVectorIndexConfig& cfg)
    : config_(cfg)
    , next_id_(cfg.num_shards, 0)
    , local_to_global_id_(cfg.num_shards)
    , alive_ids_(cfg.num_shards)
{
    if (cfg.num_shards == 0) {
        throw std::invalid_argument("DistributedVectorIndex: num_shards must be > 0");
    }
    shards_.reserve(cfg.num_shards);
    for (size_t i = 0; i < cfg.num_shards; ++i) {
        shards_.push_back(std::make_unique<ScaNN>());
    }
    buildRing_();
}

DistributedVectorIndex::DistributedVectorIndex(const DistributedVectorIndexConfig& cfg,
                                               std::vector<std::unique_ptr<IAnnIndex>> shards)
    : config_(cfg)
    , shards_(std::move(shards))
    , next_id_(cfg.num_shards, 0)
    , local_to_global_id_(cfg.num_shards)
    , alive_ids_(cfg.num_shards)
{
    if (config_.num_shards == 0) {
        throw std::invalid_argument("DistributedVectorIndex: num_shards must be > 0");
    }
    if (shards_.size() != config_.num_shards) {
        throw std::invalid_argument(
            "DistributedVectorIndex: shards.size() must equal config.num_shards");
    }
    buildRing_();
}

DistributedVectorIndex::DistributedVectorIndex(DistributedVectorIndex&& other) noexcept
    : config_(std::move(other.config_))
    , shards_(std::move(other.shards_))
    , pk_to_shard_(std::move(other.pk_to_shard_))
    , next_id_(std::move(other.next_id_))
    , alive_ids_(std::move(other.alive_ids_))
    , ring_(std::move(other.ring_))
{}

DistributedVectorIndex& DistributedVectorIndex::operator=(DistributedVectorIndex&& other) noexcept {
    if (this != &other) {
        config_      = std::move(other.config_);
        shards_      = std::move(other.shards_);
        pk_to_shard_ = std::move(other.pk_to_shard_);
        next_id_     = std::move(other.next_id_);
        alive_ids_   = std::move(other.alive_ids_);
        ring_        = std::move(other.ring_);
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Ring construction
// ---------------------------------------------------------------------------

void DistributedVectorIndex::buildRing_() {
    ring_.clear();
    if (config_.strategy != ShardingStrategy::CONSISTENT_HASH) {
        return; // HASH and RANGE use modulo; ring is not needed
    }
    const size_t vnodes = config_.virtual_nodes > 0 ? config_.virtual_nodes : 150;
    for (size_t s = 0; s < config_.num_shards; ++s) {
        for (size_t v = 0; v < vnodes; ++v) {
            const std::string token = "shard:" + std::to_string(s) + ":vn:" + std::to_string(v);
            uint64_t h = hashString(token);
            // Resolve rare collision by linear probing on the ring
            while (ring_.count(h)) ++h;
            ring_[h] = s;
        }
    }
}

// ---------------------------------------------------------------------------
// Key → shard routing
// ---------------------------------------------------------------------------

uint64_t DistributedVectorIndex::hashKey_(const std::string& key) const noexcept {
    return hashString(key);
}

size_t DistributedVectorIndex::shardFor_(const std::string& key) const noexcept {
    if (config_.num_shards == 1) return 0;
    const uint64_t h = hashKey_(key);
    switch (config_.strategy) {
    case ShardingStrategy::HASH:
    case ShardingStrategy::RANGE:
        // RANGE uses hash-bucket assignment as an alias for HASH.
        // Proper lexicographic range partitioning is deferred.
        return static_cast<size_t>(h % config_.num_shards);
    case ShardingStrategy::CONSISTENT_HASH: {
        if (ring_.empty()) return 0;
        auto it = ring_.lower_bound(h);
        if (it == ring_.end()) it = ring_.begin();
        return it->second;
    }
    }
    return static_cast<size_t>(h % config_.num_shards); // unreachable
}

size_t DistributedVectorIndex::shardFor(const std::string& key) const {
    return shardFor_(key);
}

std::optional<int64_t> DistributedVectorIndex::parseGlobalIdFromKey_(const std::string& key) {
    const auto pos = key.find_last_of('_');
    if (pos == std::string::npos || pos + 1 >= key.size()) {
        return std::nullopt;
    }
    for (size_t i = pos + 1; i < key.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(key[i]))) {
            return std::nullopt;
        }
    }
    try {
        return std::stoll(key.substr(pos + 1));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// ---------------------------------------------------------------------------
// Mutation
// ---------------------------------------------------------------------------

bool DistributedVectorIndex::insert(const std::string& pk,
                                    const float* vec, size_t dim) {
    if (!vec || dim == 0) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    const size_t target_shard = shardFor_(pk);

    // Retire the old entry (if any) from its alive set.
    auto existing = pk_to_shard_.find(pk);
    auto global_existing = pk_to_global_id_.find(pk);
    if (existing != pk_to_shard_.end()) {
        const size_t old_shard = existing->second.first;
        const int64_t old_id   = existing->second.second;
        alive_ids_[old_shard].erase(old_id);
        local_to_global_id_[old_shard].erase(old_id);
        // The old vector data remains in ScaNN but will be filtered out in
        // search() because old_id is no longer in alive_ids_[old_shard].
    }

    int64_t global_id = 0;
    if (global_existing != pk_to_global_id_.end()) {
        global_id = global_existing->second;
    } else if (auto parsed = parseGlobalIdFromKey_(pk)) {
        global_id = *parsed;
        if (global_id >= next_global_id_) {
            next_global_id_ = global_id + 1;
        }
    } else {
        global_id = next_global_id_++;
    }

    // Always allocate a new ID so ScaNN never gets two live entries for
    // the same logical key (re-adding the same old_id would merely append
    // a duplicate without replacing).
    const int64_t new_id = next_id_[target_shard]++;
    bool ok = shards_[target_shard]->add(new_id, vec, dim);
    if (ok) {
        pk_to_shard_[pk] = {target_shard, new_id};
        pk_to_global_id_[pk] = global_id;
        alive_ids_[target_shard].insert(new_id);
        local_to_global_id_[target_shard][new_id] = global_id;
    } else if (existing != pk_to_shard_.end()) {
        // Rollback: remove the stale routing entry so the key is not
        // half-visible after a failed add.
        pk_to_shard_.erase(existing);
    }
    return ok;
}

bool DistributedVectorIndex::insert(const std::string& pk,
                                    const std::vector<float>& vec) {
    return insert(pk, vec.data(), vec.size());
}

bool DistributedVectorIndex::remove(const std::string& pk) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pk_to_shard_.find(pk);
    if (it == pk_to_shard_.end()) return false;

    const size_t shard_idx = it->second.first;
    const int64_t id       = it->second.second;

    // Remove from alive set so search() filters it out.
    alive_ids_[shard_idx].erase(id);
    local_to_global_id_[shard_idx].erase(id);
    pk_to_global_id_.erase(pk);
    // Erase from routing table.
    pk_to_shard_.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// Query – scatter-gather KNN
// ---------------------------------------------------------------------------

std::vector<AnnSearchResult> DistributedVectorIndex::search(const float* query,
                                                             size_t dim, int k) const {
    if (!query || dim == 0 || k <= 0) return {};

    // Scatter: query every shard for up to k candidates, then filter to alive IDs.
    std::vector<AnnSearchResult> merged;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t s = 0; s < shards_.size(); ++s) {
            auto partial = shards_[s]->search(query, dim, k);
            for (auto& r : partial) {
                // Only include results whose ID is still alive in this shard.
                if (alive_ids_[s].count(r.id)) {
                    auto it = local_to_global_id_[s].find(r.id);
                    if (it != local_to_global_id_[s].end()) {
                        r.id = it->second;
                    }
                    merged.push_back(r);
                }
            }
        }
    }

    // Merge: sort by distance (ascending) and keep top-k.
    std::sort(merged.begin(), merged.end(),
              [](const AnnSearchResult& a, const AnnSearchResult& b) {
                  return a.distance < b.distance;
              });

    if (static_cast<int>(merged.size()) > k) {
        merged.resize(static_cast<size_t>(k));
    }
    return merged;
}

std::vector<AnnSearchResult> DistributedVectorIndex::search(
        const std::vector<float>& query, int k) const {
    return search(query.data(), query.size(), k);
}

// ---------------------------------------------------------------------------
// Introspection
// ---------------------------------------------------------------------------

size_t DistributedVectorIndex::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pk_to_shard_.size();
}

size_t DistributedVectorIndex::numShards() const {
    return shards_.size();
}

std::vector<DistributedShardStats> DistributedVectorIndex::getShardStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DistributedShardStats> stats;
    stats.reserve(shards_.size());
    for (size_t i = 0; i < shards_.size(); ++i) {
        stats.push_back({i, alive_ids_[i].size()});
    }
    return stats;
}

DistributedVectorIndexStats DistributedVectorIndex::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    DistributedVectorIndexStats stats;
    stats.num_shards     = shards_.size();
    stats.min_shard_size = std::numeric_limits<size_t>::max();
    stats.max_shard_size = 0;

    for (size_t i = 0; i < shards_.size(); ++i) {
        const size_t n = alive_ids_[i].size();
        stats.total_vectors += n;
        stats.max_shard_size = std::max(stats.max_shard_size, n);
        stats.min_shard_size = std::min(stats.min_shard_size, n);
    }
    if (stats.num_shards == 0) {
        stats.min_shard_size = 0;
        return stats;
    }
    if (stats.min_shard_size == std::numeric_limits<size_t>::max()) {
        stats.min_shard_size = 0;
    }
    const double mean = static_cast<double>(stats.total_vectors) /
                        static_cast<double>(stats.num_shards);
    if (mean > 0.0) {
        stats.load_imbalance = (static_cast<double>(stats.max_shard_size) -
                                static_cast<double>(stats.min_shard_size)) / mean;
    }
    return stats;
}

} // namespace index
} // namespace themis

