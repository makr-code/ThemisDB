/**
 * @file numa_topology.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <thread>

namespace themis {
namespace performance {

/**
 * @brief NUMA node descriptor
 *
 * Captures per-node CPU list, available memory, and inter-node distances
 * as reported by the OS topology interfaces.
 */
struct NumaNode {
    int node_id = -1;
    std::vector<int> cpu_ids;       ///< Logical CPU IDs belonging to this node
    uint64_t memory_bytes = 0;      ///< Total memory on this node (bytes)
    std::vector<int> distances;     ///< Relative access cost to every other node
};

/**
 * @brief System-wide NUMA topology snapshot
 */
struct NumaTopology {
    std::vector<NumaNode> nodes;    ///< One entry per NUMA node (ordered by node_id)
    int num_nodes = 0;
    int num_cpus = 0;               ///< Total logical CPUs across all nodes

    bool is_numa_available() const noexcept { return num_nodes > 1; }

    /// Return the node that owns @p cpu_id, or -1 if not found.
    int node_of_cpu(int cpu_id) const noexcept;

    /// Return a best-effort local node id for the calling thread.
    int local_node() const noexcept;
};

/**
 * @brief Detects the NUMA topology of the current system
 *
 * Reads topology information from OS-provided interfaces:
 *  - Linux : /sys/devices/system/node/
 *  - Windows: GetNumaNodeProcessorMaskEx / GetNumaAvailableMemoryNode
 *  - macOS  : sysctl hw.packages (limited; typically 1 node)
 *
 * Results are cached after the first call.
 */
class NumaTopologyDetector {
public:
    /**
     * @brief Detect and return the system NUMA topology (cached singleton).
     */
    static const NumaTopology& detect() noexcept;

    /**
     * @brief Force re-detection (clears cached result).
     * Only needed after hot-plug events; not required in normal operation.
     */
    static void invalidate_cache() noexcept;

private:
    NumaTopologyDetector() = delete;

    static NumaTopology detect_impl() noexcept;
};

/**
 * @brief Pins the calling thread to a specific CPU core or NUMA node
 *
 * Provides portable wrappers around:
 *  - pthread_setaffinity_np (Linux/glibc)
 *  - SetThreadAffinityMask / SetThreadGroupAffinity (Windows)
 *
 * All methods are best-effort: they return false and leave the thread
 * unpinned when the OS call fails (e.g. permission denied, invalid cpu).
 */
class ThreadPinner {
public:
    /**
     * @brief Pin the calling thread to a single CPU core.
     * @param cpu_id  Logical CPU id (0-based).
     * @return true on success.
     */
    static bool pin_to_cpu(int cpu_id) noexcept;

    /**
     * @brief Pin the calling thread to all CPUs of a NUMA node.
     * @param node_id  NUMA node id.
     * @return true on success.
     */
    static bool pin_to_node(int node_id) noexcept;

    /**
     * @brief Pin the calling thread to an explicit set of CPU ids.
     * @param cpu_ids  Non-empty list of logical CPU ids.
     * @return true on success.
     */
    static bool pin_to_cpus(const std::vector<int>& cpu_ids) noexcept;

    /**
     * @brief Remove any CPU affinity restriction from the calling thread.
     * @return true on success.
     */
    static bool unpin() noexcept;

    /**
     * @brief Query the current CPU affinity set of the calling thread.
     * @return List of CPU ids the thread may run on; empty on error.
     */
    static std::vector<int> current_affinity() noexcept;

    /**
     * @brief Get the NUMA node of the CPU the calling thread is currently running on.
     * @return NUMA node id, or -1 if unavailable.
     */
    static int current_node() noexcept;
};

} // namespace performance
} // namespace themis

