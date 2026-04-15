/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            consistent_hash.h                                  ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:06:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     104                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15a0bb6700  2026-03-09  feat(utils): add BloomFilter, ConsistentHashRing, RateLim... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace utils {

/**
 * @brief Consistent hash ring for distributed key-to-node routing.
 *
 * Each physical node is expanded into `virtual_nodes` virtual replicas so
 * that load is distributed evenly even with small cluster sizes. Keys are
 * mapped to nodes via clockwise lookup on the hash ring.
 *
 * Hashing: FNV-1a 64-bit (no external dependencies).
 * Thread-safety: std::shared_mutex (concurrent reads, exclusive writes).
 */
class ConsistentHashRing {
public:
    /**
     * @param virtual_nodes  Number of virtual replicas per physical node.
     *                       Higher values improve distribution at the cost
     *                       of memory. 150 is a reasonable default.
     */
    explicit ConsistentHashRing(size_t virtual_nodes = 150);

    /**
     * @brief Add a physical node to the ring.
     * No-op if the node is already present.
     */
    void addNode(const std::string& node);

    /**
     * @brief Remove a physical node and all its virtual replicas.
     * No-op if the node is not present.
     */
    void removeNode(const std::string& node);

    /**
     * @brief Get the node responsible for @p key.
     * @return Node name, or empty string if the ring is empty.
     */
    std::string getNode(const std::string& key) const;

    /**
     * @brief Get up to @p n distinct successor nodes for replication.
     *
     * Walks the ring clockwise starting from the hash of @p key and
     * collects distinct physical nodes until @p n are found or the ring
     * is exhausted.
     *
     * @return Vector of up to min(n, nodeCount()) distinct node names.
     */
    std::vector<std::string> getNodes(const std::string& key, size_t n) const;

    /** @brief Number of physical nodes in the ring. */
    size_t nodeCount() const;

    /** @brief True when no physical nodes have been added. */
    bool empty() const;

private:
    /// FNV-1a 64-bit hash of an arbitrary string.
    static uint64_t fnv1a64(const std::string& s);

    /// Hash used for virtual-node slots: "<node>#<idx>".
    static uint64_t virtualKey(const std::string& node, size_t idx);

    size_t virtual_nodes_;
    mutable std::shared_mutex mutex_;
    std::map<uint64_t, std::string>   ring_;      ///< hash → physical node
    std::unordered_set<std::string>   nodes_;     ///< set of physical nodes
};

} // namespace utils
} // namespace themis
