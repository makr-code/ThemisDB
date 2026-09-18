/**
 * @file consistent_hash.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
 * @brief Fnv1a64.
 * @param[in] s Input parameter.
 * @return Return value.
 * @details Calls: mix64().
 */
uint64_t ConsistentHashRing::fnv1a64(const std::string& s) {
    return mix64(themis::hash::fnv1a64(s));
}

/**
 * @brief Virtual Key.
 * @param[in] node Input parameter.
 * @param[in] idx Input parameter.
 * @return Return value.
 * @details Calls: fnv1a64(), std::to_string().
 */
uint64_t ConsistentHashRing::virtualKey(const std::string& node, size_t idx) {
    return fnv1a64(node + '#' + std::to_string(idx));
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ConsistentHashRing::ConsistentHashRing(size_t virtual_nodes)
    : virtual_nodes_(virtual_nodes == 0 ? 1 : virtual_nodes)
{}


/**
 * @brief Add Node.
 * @param[in] node Input parameter.
 * @details Calls: lock(), count(), insert(), emplace(), virtualKey().
 */
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

/**
 * @brief Remove Node.
 * @param[in] node Input parameter.
 * @details Calls: lock(), count(), erase(), virtualKey().
 */
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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    return nodes_.size();
}

bool ConsistentHashRing::empty() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    return nodes_.empty();
}

} // namespace utils
} // namespace themis
