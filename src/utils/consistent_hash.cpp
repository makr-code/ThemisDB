/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            consistent_hash.cpp                                ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:21:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7dbe96ab71  2026-03-13  refactor(sharding): improve hash functions and update dis... ║
    • 15a0bb6700  2026-03-09  feat(utils): add BloomFilter, ConsistentHashRing, RateLim... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/consistent_hash.h"

#include <algorithm>
#include <mutex>
#include <string>

namespace themis {
namespace utils {

static uint64_t mix64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

// ---------------------------------------------------------------------------
// FNV-1a 64-bit
// ---------------------------------------------------------------------------

uint64_t ConsistentHashRing::fnv1a64(const std::string& s) {
    constexpr uint64_t basis = 14695981039346656037ULL;
    constexpr uint64_t prime = 1099511628211ULL;
    uint64_t h = basis;
    for (unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= prime;
    }
    return mix64(h);
}

uint64_t ConsistentHashRing::virtualKey(const std::string& node, size_t idx) {
    return fnv1a64(node + '#' + std::to_string(idx));
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ConsistentHashRing::ConsistentHashRing(size_t virtual_nodes)
    : virtual_nodes_(virtual_nodes == 0 ? 1 : virtual_nodes)
{}

// ---------------------------------------------------------------------------
// Mutation
// ---------------------------------------------------------------------------

void ConsistentHashRing::addNode(const std::string& node) {
    std::unique_lock lock(mutex_);
    if (nodes_.count(node)) return;
    nodes_.insert(node);
    for (size_t i = 0; i < virtual_nodes_; ++i) {
        ring_.emplace(virtualKey(node, i), node);
    }
}

void ConsistentHashRing::removeNode(const std::string& node) {
    std::unique_lock lock(mutex_);
    if (!nodes_.count(node)) return;
    nodes_.erase(node);
    for (size_t i = 0; i < virtual_nodes_; ++i) {
        ring_.erase(virtualKey(node, i));
    }
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

std::string ConsistentHashRing::getNode(const std::string& key) const {
    std::shared_lock lock(mutex_);
    if (ring_.empty()) return {};

    uint64_t h = fnv1a64(key);
    auto it = ring_.lower_bound(h);
    if (it == ring_.end()) {
        it = ring_.begin(); // wrap around
    }
    return it->second;
}

std::vector<std::string> ConsistentHashRing::getNodes(const std::string& key, size_t n) const {
    std::shared_lock lock(mutex_);
    if (ring_.empty() || n == 0) return {};

    uint64_t h = fnv1a64(key);
    auto it = ring_.lower_bound(h);
    if (it == ring_.end()) it = ring_.begin();

    std::vector<std::string> result;
    result.reserve(std::min(n, nodes_.size()));

    // Walk the ring for at most ring_.size() steps to avoid infinite loop.
    // This guarantees we visit every virtual slot at most once regardless of
    // where the start iterator falls relative to ring_.begin().
    size_t steps = ring_.size();
    for (size_t i = 0; i < steps && result.size() < n; ++i, ++it) {
        if (it == ring_.end()) it = ring_.begin();

        const std::string& node = it->second;
        bool already_seen = false;
        for (const auto& r : result) {
            if (r == node) { already_seen = true; break; }
        }
        if (!already_seen) {
            result.push_back(node);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

size_t ConsistentHashRing::nodeCount() const {
    std::shared_lock lock(mutex_);
    return nodes_.size();
}

bool ConsistentHashRing::empty() const {
    std::shared_lock lock(mutex_);
    return nodes_.empty();
}

} // namespace utils
} // namespace themis
