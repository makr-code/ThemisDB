/**
 * @file consistent_hash.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/consistent_hash.h"
#include "utils/hash_util.h"

#include <algorithm>
#include <mutex>
#include <string>

namespace themis {
namespace utils {

static uint64_t mix64([[maybe_unused]] uint64_t x) {
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
    return mix64(themis::hash::fnv1a64(s));
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
    if (nodes_.count(node)) {
      return;
    }
    nodes_.insert(node);
    for (size_t i = 0; i < virtual_nodes_; ++i) {
        ring_.emplace(virtualKey(node, i), node);
    }
}

void ConsistentHashRing::removeNode(const std::string& node) {
    std::unique_lock lock(mutex_);
    if (!nodes_.count(node)) {
      return;
    }
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
    if (it == ring_.end()) {
      it = ring_.begin();
    }

    std::vector<std::string> result = {};

    result.reserve(std::min(n, nodes_.size()));

    // Walk the ring for at most ring_.size() steps to avoid infinite loop.
    // This guarantees we visit every virtual slot at most once regardless of
    // where the start iterator falls relative to ring_.begin().
    size_t steps = ring_.size();
    for (size_t i = 0; i < steps && result.size() < n; ++i, ++it) {
        if (it == ring_.end()) {
          it = ring_.begin();
        }

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
