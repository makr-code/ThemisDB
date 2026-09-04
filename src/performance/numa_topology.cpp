/**
 * @file numa_topology.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/numa_topology.h"
#include <stdexcept>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>

#ifdef __linux__
#   include <dirent.h>
#   include <pthread.h>
#   include <sched.h>
#   include <unistd.h>
#   include <sys/syscall.h>
#endif

#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#endif

namespace themis {
namespace performance {

// ============================================================================
// NumaTopology helpers
// ============================================================================

int NumaTopology::node_of_cpu([[maybe_unused]] int cpu_id) const noexcept {
    for (const auto& node : nodes) {
        for (int c : node.cpu_ids) {
            if (c == cpu_id) return node.node_id;
        }
    }
    return -1;
}

int NumaTopology::local_node() const noexcept {
#ifdef __linux__
    // getcpu syscall returns the current CPU without requiring libnuma
    unsigned cpu = 0;
    unsigned node_num = 0;
#   ifdef SYS_getcpu
    if (syscall(SYS_getcpu, &cpu, &node_num, nullptr) == 0) {
        return static_cast<int>(node_num);
    }
#   endif
    // Fallback: derive from sched_getcpu
    int c = sched_getcpu();
    if (c >= 0) return node_of_cpu(c);
#endif
    return (num_nodes > 0) ? nodes[0].node_id : 0;
}

// ============================================================================
// Linux helpers
// ============================================================================

#ifdef __linux__

static std::vector<int> parse_cpu_list(const std::string& list_str) {
    std::vector<int> cpus;
    std::stringstream ss(list_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // token is either "N" or "N-M"
        size_t dash = token.find('-');
        if (dash == std::string::npos) {
            try { cpus.push_back(std::stoi(token)); } catch (...) {}
        } else {
            try {
                int lo = std::stoi(token.substr(0, dash));
                int hi = std::stoi(token.substr(dash + 1));
                for (int c = lo; c <= hi; ++c) cpus.push_back(c);
            } catch (...) {}
        }
    }
    return cpus;
}

static std::string read_sysfs_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::string content;
    std::getline(f, content);
    // Trim trailing whitespace/newlines
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' '))
        content.pop_back();
    return content;
}

static NumaTopology detect_linux() noexcept {
    NumaTopology topo;

    // Iterate /sys/devices/system/node/nodeX directories
    DIR* dir = opendir("/sys/devices/system/node");
    if (!dir) {
        // No NUMA sysfs – build a single-node topology from /proc/cpuinfo
        NumaNode node0;
        node0.node_id = 0;
        long nproc = sysconf(_SC_NPROCESSORS_ONLN);
        if (nproc < 0) nproc = 1;
        for (int i = 0; i < static_cast<int>(nproc); ++i) node0.cpu_ids.push_back(i);
        node0.distances = {10};
        topo.nodes.push_back(node0);
        topo.num_nodes = 1;
        topo.num_cpus = static_cast<int>(nproc);
        return topo;
    }

    struct dirent* entry;
    std::vector<int> node_ids;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (name.rfind("node", 0) == 0 && name.size() > 4) {
            try {
                int id = std::stoi(name.substr(4));
                node_ids.push_back(id);
            } catch (...) {}
        }
    }
    closedir(dir);
    std::sort(node_ids.begin(), node_ids.end());

    for (int nid : node_ids) {
        NumaNode node;
        node.node_id = nid;
        std::string base = "/sys/devices/system/node/node" + std::to_string(nid);

        // CPU list
        std::string cpu_list_str = read_sysfs_file(base + "/cpulist");
        if (!cpu_list_str.empty()) node.cpu_ids = parse_cpu_list(cpu_list_str);

        // Memory (meminfo)
        std::ifstream meminfo(base + "/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal") != std::string::npos) {
                    std::istringstream iss(line);
                    std::string tok;
                    uint64_t kb = 0;
                    // Format: "Node N MemTotal: XXXX kB"
                    while (iss >> tok) {
                        try { kb = std::stoull(tok); } catch (...) {}
                    }
                    node.memory_bytes = kb * 1024;
                    break;
                }
            }
        }

        // Distances
        std::string dist_str = read_sysfs_file(base + "/distance");
        if (!dist_str.empty()) {
            std::istringstream iss(dist_str);
            int d;
            while (iss >> d) node.distances.push_back(d);
        }

        topo.num_cpus += static_cast<int>(node.cpu_ids.size());
        topo.nodes.push_back(std::move(node));
    }

    topo.num_nodes = static_cast<int>(topo.nodes.size());
    if (topo.num_nodes == 0) {
        // Fallback
        long nproc = sysconf(_SC_NPROCESSORS_ONLN);
        if (nproc < 0) nproc = 1;
        NumaNode node0;
        node0.node_id = 0;
        for (int i = 0; i < static_cast<int>(nproc); ++i) node0.cpu_ids.push_back(i);
        node0.distances = {10};
        topo.nodes.push_back(node0);
        topo.num_nodes = 1;
        topo.num_cpus = static_cast<int>(nproc);
    }
    return topo;
}

#endif // __linux__

// ============================================================================
// Windows helpers
// ============================================================================

#ifdef _WIN32

static NumaTopology detect_windows() noexcept {
    NumaTopology topo;
    ULONG highest_node = 0;
    if (!GetNumaHighestNodeNumber(&highest_node)) {
        // Fallback: single node
        NumaNode node0;
        node0.node_id = 0;
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        for (DWORD i = 0; i < si.dwNumberOfProcessors; ++i) node0.cpu_ids.push_back(static_cast<int>(i));
        node0.distances = {10};
        topo.nodes.push_back(node0);
        topo.num_nodes = 1;
        topo.num_cpus = static_cast<int>(si.dwNumberOfProcessors);
        return topo;
    }

    for (ULONG n = 0; n <= highest_node; ++n) {
        NumaNode node;
        node.node_id = static_cast<int>(n);

        GROUP_AFFINITY affinity;
        if (GetNumaNodeProcessorMaskEx(static_cast<USHORT>(n), &affinity)) {
            KAFFINITY mask = affinity.Mask;
            int cpu_base = static_cast<int>(affinity.Group) * 64;
            for (int bit = 0; bit < 64; ++bit) {
                if (mask & (static_cast<KAFFINITY>(1) << bit)) {
                    node.cpu_ids.push_back(cpu_base + bit);
                }
            }
        }

        ULONGLONG mem_kb = 0;
        GetNumaAvailableMemoryNodeEx(static_cast<USHORT>(n), &mem_kb);
        node.memory_bytes = mem_kb * 1024;

        topo.num_cpus += static_cast<int>(node.cpu_ids.size());
        topo.nodes.push_back(std::move(node));
    }

    topo.num_nodes = static_cast<int>(topo.nodes.size());
    return topo;
}

#endif // _WIN32

// ============================================================================
// NumaTopologyDetector
// ============================================================================

namespace {
std::mutex g_topo_mutex;
std::atomic<bool> g_topo_valid{false};
NumaTopology g_cached_topo;
} // anonymous namespace

const NumaTopology& NumaTopologyDetector::detect() noexcept {
    if (g_topo_valid.load(std::memory_order_acquire)) {
        return g_cached_topo;
    }
    std::lock_guard<std::mutex> lk(g_topo_mutex);
    if (!g_topo_valid.load(std::memory_order_relaxed)) {
        g_cached_topo = detect_impl();
        g_topo_valid.store(true, std::memory_order_release);
    }
    return g_cached_topo;
}

void NumaTopologyDetector::invalidate_cache() noexcept {
    std::lock_guard<std::mutex> lk(g_topo_mutex);
    g_topo_valid.store(false, std::memory_order_relaxed);
}

NumaTopology NumaTopologyDetector::detect_impl() noexcept {
#if defined(__linux__)
    return detect_linux();
#elif defined(_WIN32)
    return detect_windows();
#else
    // Generic fallback (macOS, FreeBSD, …)
    NumaTopology topo;
    NumaNode node0;
    node0.node_id = 0;
    unsigned int nproc = std::thread::hardware_concurrency();
    if (nproc == 0) nproc = 1;
    for (unsigned int i = 0; i < nproc; ++i) node0.cpu_ids.push_back(static_cast<int>(i));
    node0.distances = {10};
    topo.nodes.push_back(node0);
    topo.num_nodes = 1;
    topo.num_cpus = static_cast<int>(nproc);
    return topo;
#endif
}

// ============================================================================
// ThreadPinner
// ============================================================================

#ifdef __linux__

static bool set_affinity_from_cpu_set(cpu_set_t* cs) noexcept {
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), cs) == 0;
}

bool ThreadPinner::pin_to_cpu([[maybe_unused]] int cpu_id) noexcept {
    if (cpu_id < 0) return false;
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(static_cast<unsigned int>(cpu_id), &cs);
    return set_affinity_from_cpu_set(&cs);
}

bool ThreadPinner::pin_to_node([[maybe_unused]] int node_id) noexcept {
    const auto& topo = NumaTopologyDetector::detect();
    for (const auto& node : topo.nodes) {
        if (node.node_id == node_id) {
            return pin_to_cpus(node.cpu_ids);
        }
    }
    return false;
}

bool ThreadPinner::pin_to_cpus(const std::vector<int>& cpu_ids) noexcept {
    if (cpu_ids.empty()) return false;
    cpu_set_t cs;
    CPU_ZERO(&cs);
    for (int c : cpu_ids) {
        if (c >= 0) CPU_SET(static_cast<unsigned int>(c), &cs);
    }
    return set_affinity_from_cpu_set(&cs);
}

bool ThreadPinner::unpin() noexcept {
    // Set affinity to all online CPUs
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nproc <= 0) nproc = 1;
    cpu_set_t cs;
    CPU_ZERO(&cs);
    for (long i = 0; i < nproc; ++i) CPU_SET(static_cast<unsigned int>(i), &cs);
    return set_affinity_from_cpu_set(&cs);
}

std::vector<int> ThreadPinner::current_affinity() noexcept {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cs) != 0) return {};
    std::vector<int> cpus;
    for (int i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(static_cast<unsigned int>(i), &cs)) cpus.push_back(i);
    }
    return cpus;
}

int ThreadPinner::current_node() noexcept {
    int cpu = sched_getcpu();
    if (cpu < 0) return -1;
    return NumaTopologyDetector::detect().node_of_cpu(cpu);
}

#elif defined(_WIN32)

bool ThreadPinner::pin_to_cpu([[maybe_unused]] int cpu_id) noexcept {
    if (cpu_id < 0) return false;
    GROUP_AFFINITY affinity{};
    affinity.Group = static_cast<WORD>(cpu_id / 64);
    affinity.Mask  = static_cast<KAFFINITY>(1) << (cpu_id % 64);
    return SetThreadGroupAffinity(GetCurrentThread(), &affinity, nullptr) != 0;
}

bool ThreadPinner::pin_to_node([[maybe_unused]] int node_id) noexcept {
    if (node_id < 0) return false;
    GROUP_AFFINITY affinity{};
    if (!GetNumaNodeProcessorMaskEx(static_cast<USHORT>(node_id), &affinity)) return false;
    return SetThreadGroupAffinity(GetCurrentThread(), &affinity, nullptr) != 0;
}

bool ThreadPinner::pin_to_cpus(const std::vector<int>& cpu_ids) noexcept {
    if (cpu_ids.empty()) return false;
    // All CPUs must be in the same processor group for SetThreadGroupAffinity
    WORD group = static_cast<WORD>(cpu_ids[0] / 64);
    KAFFINITY mask = 0;
    for (int c : cpu_ids) {
        if (static_cast<WORD>(c / 64) == group) {
            mask |= static_cast<KAFFINITY>(1) << (c % 64);
        }
    }
    GROUP_AFFINITY affinity{};
    affinity.Group = group;
    affinity.Mask  = mask;
    return SetThreadGroupAffinity(GetCurrentThread(), &affinity, nullptr) != 0;
}

bool ThreadPinner::unpin() noexcept {
    // Reset to default affinity by setting all bits in group 0
    GROUP_AFFINITY affinity{};
    affinity.Group = 0;
    affinity.Mask  = static_cast<KAFFINITY>(-1);
    return SetThreadGroupAffinity(GetCurrentThread(), &affinity, nullptr) != 0;
}

std::vector<int> ThreadPinner::current_affinity() noexcept {
    GROUP_AFFINITY affinity{};
    if (!GetThreadGroupAffinity(GetCurrentThread(), &affinity)) return {};
    std::vector<int> cpus;
    int base = static_cast<int>(affinity.Group) * 64;
    for (int bit = 0; bit < 64; ++bit) {
        if (affinity.Mask & (static_cast<KAFFINITY>(1) << bit)) {
            cpus.push_back(base + bit);
        }
    }
    return cpus;
}

int ThreadPinner::current_node() noexcept {
    GROUP_AFFINITY affinity{};
    if (!GetThreadGroupAffinity(GetCurrentThread(), &affinity)) return -1;
    // Find the first set bit
    for (int bit = 0; bit < 64; ++bit) {
        if (affinity.Mask & (static_cast<KAFFINITY>(1) << bit)) {
            int cpu_id = static_cast<int>(affinity.Group) * 64 + bit;
            return NumaTopologyDetector::detect().node_of_cpu(cpu_id);
        }
    }
    return -1;
}

#else // Generic fallback

bool ThreadPinner::pin_to_cpu([[maybe_unused]] int /*cpu_id*/) noexcept { return false; }
bool ThreadPinner::pin_to_node([[maybe_unused]] int /*node_id*/) noexcept { return false; }
bool ThreadPinner::pin_to_cpus(const std::vector<int>& /*cpu_ids*/) noexcept { return false; }
bool ThreadPinner::unpin() noexcept { return false; }

std::vector<int> ThreadPinner::current_affinity() noexcept {
    // Return all logical CPUs as a best-effort fallback
    unsigned int nproc = std::thread::hardware_concurrency();
    if (nproc == 0) nproc = 1;
    std::vector<int> cpus;
    for (unsigned int i = 0; i < nproc; ++i) cpus.push_back(static_cast<int>(i));
    return cpus;
}

int ThreadPinner::current_node() noexcept { return 0; }

#endif

} // namespace performance
} // namespace themis


