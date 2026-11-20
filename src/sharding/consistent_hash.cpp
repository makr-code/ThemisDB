/**
 * @file consistent_hash.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/consistent_hash.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <openssl/evp.h>
#include <unordered_set>

#ifdef __has_include
#if __has_include(<xxhash.h>)
#include <xxhash.h>
#define THEMIS_CONSISTENT_HASH_HAS_XXHASH
#endif
#endif

namespace themis::sharding {

namespace {

void appendU64(std::string& output, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
}

std::optional<std::string> ownershipFingerprint(
    uint64_t epoch,
    const std::vector<RingOwnershipSnapshot::Entry>& entries
) {
    std::string canonical{"themis-ring-ownership-v1"};
    appendU64(canonical, epoch);
    for (const auto& entry : entries) {
        appendU64(canonical, entry.token);
        appendU64(canonical, static_cast<uint64_t>(entry.owner.size()));
        canonical.append(entry.owner);
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_size = 0;
    if (EVP_Digest(canonical.data(), canonical.size(), digest.data(), &digest_size,
                   EVP_sha256(), nullptr) != 1) {
        return std::nullopt;
    }

    constexpr char kHex[] = "0123456789abcdef";
    std::string fingerprint;
    fingerprint.reserve(static_cast<size_t>(digest_size) * 2);
    for (unsigned int i = 0; i < digest_size; ++i) {
        fingerprint.push_back(kHex[digest[i] >> 4]);
        fingerprint.push_back(kHex[digest[i] & 0x0f]);
    }
    return fingerprint;
}

} // namespace

bool RingOwnershipSnapshot::ownsToken(uint64_t token, std::string_view owner) const {
    return std::any_of(segments_.begin(), segments_.end(), [&](const auto& segment) {
        return segment.owner == owner &&
               token >= segment.lower_inclusive &&
               (segment.upper_exclusive.at_ring_end ||
                token < segment.upper_exclusive.value);
    });
}

std::optional<RingOwnershipSnapshot> RingOwnershipSnapshot::fromEntries(
    uint64_t topology_epoch,
    std::vector<Entry> entries
) {
    if (topology_epoch == 0) {
        return std::nullopt;
    }
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].owner.empty() ||
            (i > 0 && entries[i - 1].token >= entries[i].token)) {
            return std::nullopt;
        }
    }

    RingOwnershipSnapshot snapshot;
    snapshot.topology_epoch_ = topology_epoch;
    snapshot.entries_ = std::move(entries);

    if (snapshot.entries_.size() == 1) {
        snapshot.segments_.push_back({
            0,
            {0, true},
            snapshot.entries_.front().owner,
            true
        });
    } else if (snapshot.entries_.size() > 1) {
        const auto& first = snapshot.entries_.front();
        const auto& last = snapshot.entries_.back();

        // The first token owns (last, first] across the ring boundary.
        // The high interval is empty only when the last token is UINT64_MAX.
        if (last.token != UINT64_MAX) {
            snapshot.segments_.push_back({
                last.token + 1,
                {0, true},
                first.owner,
                false
            });
        }
        snapshot.segments_.push_back({
            0,
            {first.token + 1, false},
            first.owner,
            false
        });

        for (size_t i = 1; i < snapshot.entries_.size(); ++i) {
            const auto& previous = snapshot.entries_[i - 1];
            const auto& current = snapshot.entries_[i];
            snapshot.segments_.push_back({
                previous.token + 1,
                current.token == UINT64_MAX
                    ? RingTokenBoundary{0, true}
                    : RingTokenBoundary{current.token + 1, false},
                current.owner,
                false
            });
        }
    }

    auto fingerprint = ownershipFingerprint(topology_epoch, snapshot.entries_);
    if (!fingerprint) {
        return std::nullopt;
    }
    snapshot.fingerprint_ = std::move(*fingerprint);
    return snapshot;
}

/**
 * @brief Mix64.
 * @param[in] x Input parameter.
 * @return Return value.
 * @details Implements mix64 without additional internal calls.
 */
static uint64_t mix64(uint64_t x) {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33;
        return x;
}

/**
 * @brief Add Shard.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] virtual_nodes Input parameter.
 * @details Calls: lock(), find(), end(), erase(), reserve(), size(), std::to_string(), hash().
 */
void ConsistentHashRing::addShard(const std::string& shard_id, size_t virtual_nodes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // If shard already exists, remove it first
    if (shard_tokens_.find(shard_id) != shard_tokens_.end()) {
        // Don't call removeShard here to avoid double-locking
        auto& tokens = shard_tokens_[shard_id];
        for (uint64_t token : tokens) {
            ring_.erase(token);
        }
        shard_tokens_.erase(shard_id);
    }
    
    std::vector<uint64_t> tokens;
    tokens.reserve(virtual_nodes);
    
    // Generate virtual nodes.
    // Collisions are possible with any finite hash width; if we overwrite an
    // existing token in ring_, we silently lose virtual nodes and skew load.
    // Resolve collisions with deterministic probing to preserve ring density.
    // Build the virtual-node key as "<shard_id>#<i>" without ostringstream.
    std::string vnode_key = {};
    vnode_key.reserve(shard_id.size() + 1 + 20); // 20 digits covers uint64_t max
    for (size_t i = 0; i < virtual_nodes; ++i) {
        vnode_key = shard_id;
        vnode_key += '#';
        vnode_key += std::to_string(i);
        uint64_t token = hash(vnode_key);

        size_t probe = 0;
        while (ring_.find(token) != ring_.end()) {
            token = mix64(token + 0x9e3779b97f4a7c15ULL + probe);
            ++probe;
        }

        ring_[token] = shard_id;
        tokens.push_back(token);
    }
    
    shard_tokens_[shard_id] = std::move(tokens);
}

/**
 * @brief Remove Shard.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: lock(), find(), end(), erase().
 */
void ConsistentHashRing::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shard_tokens_.find(shard_id);
    if (it == shard_tokens_.end()) {
        return; // Shard not found
    }
    
    // Remove all virtual nodes from the ring
    for (uint64_t token : it->second) {
        ring_.erase(token);
    }
    
    shard_tokens_.erase(it);
}

std::string ConsistentHashRing::getShardForHash(uint64_t hash) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (ring_.empty()) {
        return "";
    }
    
    // Find the first shard at or after this hash (clockwise search)
    auto it = ring_.lower_bound(hash);
    
    // If we've gone past the end, wrap around to the beginning
    if (it == ring_.end()) {
        it = ring_.begin();
    }
    
    return it->second;
}

std::string ConsistentHashRing::getShardForURN(const URN& urn) const {
    return getShardForHash(urn.hash());
}

std::optional<std::string> ConsistentHashRing::getNode(const std::string& key) const {
    auto shard = getShardForHash(hash(key));
    if (shard.empty()) {
        return std::nullopt;
    }
    return shard;
}

std::optional<uint64_t> ConsistentHashRing::resolveRoutingToken(
    const RoutingKeyDescriptor& descriptor
) const {
    if (descriptor.placement_epoch == 0 || descriptor.key_bytes.empty()) {
        return std::nullopt;
    }

    switch (descriptor.key_type) {
        case RoutingKeyType::PartitionKey:
            if (descriptor.algorithm != RoutingHashAlgorithm::Fnv1aMix64V1) {
                return std::nullopt;
            }
            return hash(std::string(descriptor.key_bytes));

        case RoutingKeyType::UrnUuid:
            if (descriptor.algorithm != RoutingHashAlgorithm::XxHash64Seed0V1) {
                return std::nullopt;
            }
            {
                URN urn;
                urn.uuid = std::string(descriptor.key_bytes);
                if (!urn.isValidUUID()) {
                    return std::nullopt;
                }
            }
#ifdef THEMIS_CONSISTENT_HASH_HAS_XXHASH
            return XXH64(descriptor.key_bytes.data(), descriptor.key_bytes.size(), 0);
#else
            return std::nullopt;
#endif
    }
    return std::nullopt;
}

bool ConsistentHashRing::ownsKeyInSnapshot(
    const RingOwnershipSnapshot& snapshot,
    const RoutingKeyDescriptor& descriptor,
    std::string_view shard_id
) const {
    if (descriptor.placement_epoch == 0 ||
        descriptor.placement_epoch != snapshot.topologyEpoch()) {
        return false;
    }
    const auto token = resolveRoutingToken(descriptor);
    return token && snapshot.ownsToken(*token, shard_id);
}

std::optional<RingOwnershipSnapshot> ConsistentHashRing::snapshotOwnership(
    uint64_t topology_epoch
) const {
    if (topology_epoch == 0) {
        return std::nullopt;
    }

    std::vector<RingOwnershipSnapshot::Entry> entries;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        entries.reserve(ring_.size());
        for (const auto& [token, owner] : ring_) {
            entries.push_back({token, owner});
        }
    }
    return RingOwnershipSnapshot::fromEntries(topology_epoch, std::move(entries));
}

std::vector<std::string> ConsistentHashRing::getReplicaNodes(const std::string& key, size_t count) const {
    auto nodes = getSuccessors(hash(key), count + 1);
    if (!nodes.empty()) {
        nodes.erase(nodes.begin()); // drop primary
    }
    if (nodes.size() > count) {
        nodes.resize(count);
    }
    return nodes;
}

std::vector<std::string> ConsistentHashRing::getSuccessors(uint64_t hash, size_t count) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (ring_.empty() || count == 0) {
        return {};
    }
    
    std::vector<std::string> result;
    std::unordered_set<std::string> seen; // Track unique shards; O(1) avg vs O(log n) for std::set
    result.reserve(count);
    seen.reserve(count);
    
    // Start from the position at or after the hash
    auto it = ring_.lower_bound(hash);
    if (it == ring_.end()) {
        it = ring_.begin();
    }
    
    // Collect up to 'count' unique shards
    size_t iterations = 0;
    const size_t max_iterations = ring_.size(); // Prevent infinite loop
    
    while ( result.size() < count && iterations < max_iterations) {
        if (seen.find(it->second) == seen.end()) {
            result.push_back(it->second);
            seen.insert(it->second);
        }
        
        ++it;
        if (it == ring_.end()) {
            it = ring_.begin(); // Wrap around
        }
        ++iterations;
    }
    
    return result;
}

std::pair<uint64_t, uint64_t> ConsistentHashRing::getShardRange(const std::string& shard_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shard_tokens_.find(shard_id);
    if (it == shard_tokens_.end() || it->second.empty()) {
        return {0, 0};
    }
    
    // Find min and max tokens for this shard
    const auto& tokens = it->second;
    uint64_t min_token = *std::min_element(tokens.begin(), tokens.end());
    uint64_t max_token = *std::max_element(tokens.begin(), tokens.end());
    
    return {min_token, max_token};
}

std::vector<std::string> ConsistentHashRing::getAllShards() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> shards = {};

    shards.reserve(shard_tokens_.size());
    
    for (const auto& [shard_id, _] : shard_tokens_) {
        shards.push_back(shard_id);
    }
    
    return shards;
}

std::vector<std::string> ConsistentHashRing::getShardsInRange(
    uint64_t hash_start, uint64_t hash_end
) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    if (ring_.empty()) {
        return {};
    }

    std::vector<std::string> result;
    std::unordered_set<std::string> seen;

    auto collect = [&](auto from, auto to_exclusive) {
        // Walk from 'from' to just before 'to_exclusive', collecting shards.
        for (auto it = from; it != to_exclusive; ++it) {
            seen.insert(it->second);
        }
    };

    if (hash_start > hash_end) {
        // Wrap-around range: [hash_start, ring_max] ∪ [ring_min, hash_end]
        // Part 1: hash_start → end of ring
        auto it_start = ring_.lower_bound(hash_start);
        if (it_start == ring_.end()) {
            it_start = ring_.begin();
        }
        collect(it_start, ring_.end());

        // Part 2: beginning of ring → hash_end (inclusive)
        for (auto it = ring_.begin(); it != ring_.end() && it->first <= hash_end; ++it) {
            seen.insert(it->second);
        }
    } else {
        // Normal (non-wrapping) range: [hash_start, hash_end]
        auto it = ring_.lower_bound(hash_start);
        if (it == ring_.end()) {
            it = ring_.begin(); // wrap-around: start is past the last token
        }
        while (it != ring_.end() && it->first <= hash_end) {
            seen.insert(it->second);
            ++it;
        }
    }

    result.assign(seen.begin(), seen.end());
    return result;
}

double ConsistentHashRing::getBalanceFactor() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shard_tokens_.empty()) {
        return 0.0;
    }
    
    // Calculate mean number of virtual nodes per shard
    double total_nodes = static_cast<double>(ring_.size());
    double mean = total_nodes / static_cast<double>(shard_tokens_.size());
    
    // Calculate standard deviation
    double variance = 0.0;
    for (const auto& [shard_id, tokens] : shard_tokens_) {
        double diff = static_cast<double>(tokens.size()) - mean;
        variance += diff * diff;
    }
    variance /= static_cast<double>(shard_tokens_.size());
    
    double std_dev = std::sqrt(variance);
    
    // Return as percentage of mean
    return (std_dev / mean) * 100.0;
}

uint64_t ConsistentHashRing::hash(const std::string& key) const {
    constexpr uint64_t kFNVOffsetBasis = UINT64_C(14695981039346656037);
    constexpr uint64_t kFNVPrime = UINT64_C(1099511628211);
    uint64_t h = kFNVOffsetBasis;
    for (unsigned char c : key) {
        h ^= static_cast<uint64_t>(c);
        h *= kFNVPrime;
    }
    return mix64(h);
}

} // namespace themis::sharding
