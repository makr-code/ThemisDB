/**
 * @file kernel_bypass.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=5, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "network/kernel_bypass.h"
#include "utils/logger.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <future>
#include <new>
#include <stdexcept>
#include <system_error>

// ---- Linux-specific headers ------------------------------------------------
#ifdef __linux__
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <dirent.h>
#  include <pthread.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

// ---- io_uring headers (Linux kernel ≥ 5.1) ---------------------------------
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
#  include <signal.h>
#  include <linux/io_uring.h>

// Thin syscall wrappers (not exposed by all glibc versions).
static int themis_io_uring_setup(unsigned entries, struct io_uring_params* p) {
    return static_cast<int>(::syscall(__NR_io_uring_setup, entries, p));
}
static int themis_io_uring_enter(int fd, unsigned to_submit,
                                  unsigned min_complete, unsigned flags,
                                  sigset_t* sig) {
    return static_cast<int>(::syscall(__NR_io_uring_enter,
                                      fd, to_submit, min_complete,
                                      flags, sig, _NSIG / 8));
}
static int themis_io_uring_register(int fd, unsigned opcode,
                                     void* arg, unsigned nr_args) {
    return static_cast<int>(::syscall(__NR_io_uring_register,
                                      fd, opcode, arg, nr_args));
}
#endif  // THEMIS_ENABLE_IO_URING && __linux__

// ---- NUMA headers ----------------------------------------------------------
#if defined(THEMIS_ENABLE_NUMA) && defined(__linux__)
#  include <numa.h>
#endif

// ---- DPDK headers ----------------------------------------------------------
#if defined(THEMIS_ENABLE_DPDK)
#  include <rte_eal.h>
#  include <rte_ethdev.h>
#  include <rte_mbuf.h>
#  include <rte_mempool.h>
#  include <rte_lcore.h>
#  include <rte_cycles.h>
#endif

namespace themis {
namespace network {

namespace {

constexpr int kShutdownJoinTimeoutMs = 5000;

/// @brief Join @p t within @p timeout_ms; log and detach on timeout.
static void timedJoin(std::thread& t,
                      int timeout_ms = kShutdownJoinTimeoutMs) noexcept {
    if (!t.joinable()) return;
    std::promise<void> done;
    auto fut = done.get_future();
    std::thread watcher([inner = std::move(t), p = std::move(done)]() mutable {
        if (inner.joinable()) inner.join();
        p.set_value();
    });
    watcher.detach();
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) !=
            std::future_status::ready) {
        // thread_join_no_timeout: detach on deadline to avoid indefinite block
        THEMIS_WARN("Thread did not finish within {} ms during shutdown; detaching.",
                    timeout_ms);
    }
}

} // namespace

// =============================================================================
// CpuPinner
// =============================================================================

bool CpuPinner::pinCallerToCore(int core_id) noexcept {
    static_cast<void>(core_id);
#ifdef __linux__
    if (core_id < 0) return false;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(static_cast<unsigned>(core_id), &set);
    return ::sched_setaffinity(0, sizeof(set), &set) == 0;
#else
    (void)core_id;
    return false;
#endif
}

bool CpuPinner::pinThreadToCore(std::thread& thread, int core_id) noexcept {
    static_cast<void>(thread);
    static_cast<void>(core_id);
#ifdef __linux__
    if (core_id < 0) return false;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(static_cast<unsigned>(core_id), &set);
    return ::pthread_setaffinity_np(thread.native_handle(),
                                    sizeof(set), &set) == 0;
#else
    (void)thread;
    (void)core_id;
    return false;
#endif
}

int CpuPinner::numaNodeForCore(int core_id) noexcept {
    static_cast<void>(core_id);
#ifdef __linux__
    if (core_id < 0) return -1;
    // Walk /sys/devices/system/cpu/cpu<N>/node* symlinks.
    char path[128];
    std::snprintf(path, sizeof(path),
                  "/sys/devices/system/cpu/cpu%d", core_id);
    DIR* d = ::opendir(path);
    if (!d) return -1;
    struct dirent* e;
    while ((e = ::readdir(d)) != nullptr) {
        if (::strncmp(e->d_name, "node", 4) == 0) {
            int node = std::atoi(e->d_name + 4);
            ::closedir(d);
            return node;
        }
    }
    ::closedir(d);
    return 0;  // assume node 0 if not found
#else
    (void)core_id;
    return -1;
#endif
}

int CpuPinner::logicalCpuCount() noexcept {
#ifdef __linux__
    int n = static_cast<int>(::sysconf(_SC_NPROCESSORS_ONLN));
    return n > 0 ? n : 1;
#else
    return static_cast<int>(std::thread::hardware_concurrency());
#endif
}

int CpuPinner::currentCpu() noexcept {
#ifdef __linux__
    return ::sched_getcpu();
#else
    return -1;
#endif
}

std::vector<int> CpuPinner::coresOnNuma(int numa_node) noexcept {
    std::vector<int> result;
    int ncpu = logicalCpuCount();
    for (int i = 0; i < ncpu; ++i) {
        if (numaNodeForCore(i) == numa_node) {
            result.push_back(i);
        }
    }
    return result;
}

// =============================================================================
// NumaAllocator
// =============================================================================

void* NumaAllocator::allocate(size_t size, int node) {
    static_cast<void>(node);
    if (size == 0) throw std::bad_alloc{};

#if defined(THEMIS_ENABLE_NUMA) && defined(__linux__)
    void* p = nullptr;
    if (node >= 0) {
        p = ::numa_alloc_onnode(size, node);
    } else {
        p = ::numa_alloc_local(size);
    }
    if (!p) throw std::bad_alloc{};
    return p;
#else
    (void)node;
    // Fallback: std::aligned_alloc with 64-byte alignment.
    constexpr size_t kAlign = 64;
    size_t padded = (size + kAlign - 1) & ~(kAlign - 1);
    void* p = nullptr;
#ifdef _WIN32
    p = _aligned_malloc(padded, kAlign);
#else
    if (::posix_memalign(&p, kAlign, padded) != 0) p = nullptr;
#endif
    if (!p) throw std::bad_alloc{};
    return p;
#endif
}

void NumaAllocator::deallocate(void* ptr, size_t size) noexcept {
    static_cast<void>(size);
    if (!ptr) return;
#if defined(THEMIS_ENABLE_NUMA) && defined(__linux__)
    ::numa_free(ptr, size);
#elif defined(_WIN32)
    (void)size;
    _aligned_free(ptr);
#else
    (void)size;
    ::free(ptr);
#endif
}

bool NumaAllocator::isNumaAvailable() noexcept {
#if defined(THEMIS_ENABLE_NUMA) && defined(__linux__)
    return ::numa_available() >= 0;
#else
    return false;
#endif
}

// =============================================================================
// ZeroCopyDmaBuffer
// =============================================================================

ZeroCopyDmaBuffer::ZeroCopyDmaBuffer(size_t size_bytes, int numa_node) {
    if (size_bytes == 0) return;

#ifdef __linux__
    // Attempt huge-page-backed mmap first.
    constexpr size_t kHugePage = 2 * 1024 * 1024;  // 2 MiB
    size_t aligned = (size_bytes + kHugePage - 1) & ~(kHugePage - 1);

    int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE;

    // Try MAP_HUGETLB (requires CONFIG_HUGETLBFS in kernel).
    void* p = ::mmap(nullptr, aligned,
                     PROT_READ | PROT_WRITE,
                     mmap_flags | MAP_HUGETLB,
                     -1, 0);
    if (p != MAP_FAILED) {
        data_      = p;
        size_      = aligned;
        huge_page_ = true;

        // Bind to NUMA node via mbind if available.
#if defined(THEMIS_ENABLE_NUMA) && defined(__linux__)
        if (numa_node >= 0 && NumaAllocator::isNumaAvailable()) {
            // mbind to preferred node.
            unsigned long nodemask = (1UL << static_cast<unsigned>(numa_node));
            ::syscall(__NR_mbind, p, aligned, /* MPOL_PREFERRED */ 1,
                      &nodemask, sizeof(nodemask) * 8 + 1, /* MPOL_MF_MOVE */ 2);
        }
#endif
        return;
    }

    // Fallback: regular anonymous mmap.
    p = ::mmap(nullptr, size_bytes,
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
               -1, 0);
    if (p != MAP_FAILED) {
        data_      = p;
        size_      = size_bytes;
        huge_page_ = false;
        return;
    }
    // Both attempts failed.
    THEMIS_WARN("ZeroCopyDmaBuffer: mmap failed: {}", std::strerror(errno));
#else
    // PERMANENT HARDWARE FALLBACK NOTE (Linux DPDK / io_uring / huge-page mmap — non-Linux):
    // Purpose: Allow ZeroCopyDmaBuffer to be used on Windows and other non-Linux
    //   platforms by falling back to a plain heap allocation instead of the
    //   Linux mmap(MAP_HUGETLB) or mmap(MAP_ANONYMOUS) paths.  The object is
    //   constructible and usable; data() returns a valid aligned pointer.
    // Activation: Not on Linux (i.e., !defined(__linux__)).  On Linux the
    //   two mmap() attempts above are tried first; this #else is only reached
    //   on Windows/macOS/BSDs.
    // Production Delta: huge_page_ is always false; the allocation is a
    //   standard heap allocation with no NUMA affinity and no kernel-bypass DMA
    //   mapping.  Zero-copy network I/O (DPDK / io_uring) cannot be performed
    //   because there is no DMA-mapped memory; any attempt to pass this buffer
    //   to a DPDK mbuf or io_uring fixed buffer registration will fail or be
    //   silently ignored.  Effective bandwidth will be limited by an additional
    //   kernel copy on every I/O operation.
    // Hardware requirement: Linux kernel ≥ 5.1 + DPDK or io_uring for the full path.
    // Roadmap ref: src/network/FUTURE_ENHANCEMENTS.md §"Kernel Bypass Windows Support"
    // Windows / other: plain aligned allocation.
    auto alloc_fn = NonLinuxAllocFn{};
    {
        std::lock_guard<std::mutex> lock(nonLinuxAllocFnMutex());
        alloc_fn = nonLinuxAllocFnStorage();
    }
    data_ = alloc_fn ? alloc_fn(size_bytes, numa_node)
                     : NumaAllocator::allocate(size_bytes, -1);
    if (!data_) {
        THEMIS_WARN("ZeroCopyDmaBuffer: non-Linux allocator bridge returned null");
    }
    size_      = size_bytes;
    huge_page_ = false;
#endif
}

ZeroCopyDmaBuffer::~ZeroCopyDmaBuffer() {
    if (!data_) return;
#ifdef __linux__
    ::munmap(data_, size_);
#else
    auto free_fn = NonLinuxFreeFn{};
    {
        std::lock_guard<std::mutex> lock(nonLinuxFreeFnMutex());
        free_fn = nonLinuxFreeFnStorage();
    }
    if (free_fn) {
        free_fn(data_, size_);
    } else {
        NumaAllocator::deallocate(data_, size_);
    }
#endif
    data_ = nullptr;
}

ZeroCopyDmaBuffer::ZeroCopyDmaBuffer(ZeroCopyDmaBuffer&& o) noexcept
    : data_(o.data_), size_(o.size_), huge_page_(o.huge_page_)
{
    o.data_      = nullptr;
    o.size_      = 0;
    o.huge_page_ = false;
}

ZeroCopyDmaBuffer& ZeroCopyDmaBuffer::operator=(ZeroCopyDmaBuffer&& o) noexcept {
    if (this != &o) {
        this->~ZeroCopyDmaBuffer();
        new (this) ZeroCopyDmaBuffer(std::move(o));
    }
    return *this;
}

// =============================================================================
// DPDKServer — helpers
// =============================================================================

/*static*/
bool DPDKServer::isDpdkAvailable() noexcept {
#ifdef THEMIS_ENABLE_DPDK
    return true;
#else
    return false;
#endif
}

/*static*/
std::vector<int> DPDKServer::coresFromMask(uint64_t mask) noexcept {
    std::vector<int> cores;
    for (int i = 0; i < 64; ++i) {
        if (mask & (1ULL << static_cast<unsigned>(i))) {
            cores.push_back(i);
        }
    }
    return cores;
}

// =============================================================================
// DPDKServer — construction / destruction
// =============================================================================

DPDKServer::DPDKServer(const Config&          config,
                       RocksDBWrapper*        storage,
                       SecondaryIndexManager* index_mgr)
    : config_(config), storage_(storage), index_mgr_(index_mgr)
{}

DPDKServer::~DPDKServer() {
    stop();
}

// =============================================================================
// DPDKServer::start
// =============================================================================

bool DPDKServer::start() {
    if (running_.load(std::memory_order_relaxed)) {
        last_error_ = "DPDKServer::start() called while already running";
        return false;
    }

#if defined(THEMIS_ENABLE_DPDK)
    // -------------------------------------------------------------------------
    // 1. Build EAL argument vector from Config.
    // -------------------------------------------------------------------------
    std::vector<std::string> eal_arg_strs;
    eal_arg_strs.push_back("themisdb");

    // Huge-page memory.
    if (config_.huge_pages_mb > 0) {
        eal_arg_strs.push_back("-m");
        eal_arg_strs.push_back(std::to_string(config_.huge_pages_mb));
    }

    // Core mask.
    char mask_str[32];
    std::snprintf(mask_str, sizeof(mask_str), "0x%llX",
                  static_cast<unsigned long long>(config_.cpu_core_mask));
    eal_arg_strs.push_back("-c");
    eal_arg_strs.push_back(mask_str);

    // Allowed PCI device.
    if (!config_.pci_address.empty()) {
        eal_arg_strs.push_back("-a");
        eal_arg_strs.push_back(config_.pci_address);
    }

    // Convert to char* array.
    std::vector<char*> eal_argv;
    eal_argv.reserve(eal_arg_strs.size());
    for (auto& s : eal_arg_strs) {
        eal_argv.push_back(const_cast<char*>(s.c_str()));
    }
    int eal_argc = static_cast<int>(eal_argv.size());

    // -------------------------------------------------------------------------
    // 2. Initialise DPDK EAL.
    // -------------------------------------------------------------------------
    int ret = rte_eal_init(eal_argc, eal_argv.data());
    if (ret < 0) {
        last_error_ = "rte_eal_init failed: " + std::string(rte_strerror(-ret));
        THEMIS_ERROR("DPDKServer: {}", last_error_);
        return false;
    }

    // -------------------------------------------------------------------------
    // 3. Configure the Ethernet device.
    // -------------------------------------------------------------------------
    int port_id = 0;  // first available port
    unsigned nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0) {
        last_error_ = "DPDKServer: no DPDK-bound Ethernet ports found";
        THEMIS_ERROR("{}", last_error_);
        rte_eal_cleanup();
        return false;
    }

    // Determine NUMA node for this port.
    int numa_node = config_.numa_node;
    if (numa_node < 0) {
        numa_node = rte_eth_dev_socket_id(port_id);
        if (numa_node < 0) numa_node = 0;
    }

    // Create mbuf memory pool.
    uint32_t pool_size = config_.mbuf_pool_size
                         * (config_.num_rx_queues + config_.num_tx_queues);
    struct rte_mempool* mbuf_pool =
        rte_pktmbuf_pool_create("THEMIS_MBUF_POOL",
                                pool_size,
                                /* cache_size */ 256,
                                /* priv_size */  0,
                                RTE_MBUF_DEFAULT_BUF_SIZE,
                                numa_node);
    if (!mbuf_pool) {
        last_error_ = "rte_pktmbuf_pool_create failed";
        THEMIS_ERROR("DPDKServer: {}", last_error_);
        rte_eal_cleanup();
        return false;
    }

    // Ethernet device configuration.
    struct rte_eth_conf eth_conf;
    std::memset(&eth_conf, 0, sizeof(eth_conf));
    if (config_.enable_rss) {
        eth_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
        eth_conf.rx_adv_conf.rss_conf.rss_hf =
            ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP;
    }
    if (config_.enable_jumbo_frames) {
        eth_conf.rxmode.offloads |= DEV_RX_OFFLOAD_JUMBO_FRAME;
        eth_conf.rxmode.max_rx_pkt_len = 9000;
    }

    ret = rte_eth_dev_configure(port_id,
                                config_.num_rx_queues,
                                config_.num_tx_queues,
                                &eth_conf);
    if (ret < 0) {
        last_error_ = "rte_eth_dev_configure failed: " + std::string(rte_strerror(-ret));
        THEMIS_ERROR("DPDKServer: {}", last_error_);
        rte_mempool_free(mbuf_pool);
        rte_eal_cleanup();
        return false;
    }

    // Set up RX queues.
    for (uint16_t q = 0; q < config_.num_rx_queues; ++q) {
        ret = rte_eth_rx_queue_setup(port_id, q,
                                     config_.mbuf_pool_size,
                                     static_cast<unsigned>(numa_node),
                                     nullptr, mbuf_pool);
        if (ret < 0) {
            last_error_ = "rte_eth_rx_queue_setup failed for queue "
                          + std::to_string(q);
            THEMIS_ERROR("DPDKServer: {}", last_error_);
            rte_mempool_free(mbuf_pool);
            rte_eal_cleanup();
            return false;
        }
    }

    // Set up TX queues.
    for (uint16_t q = 0; q < config_.num_tx_queues; ++q) {
        ret = rte_eth_tx_queue_setup(port_id, q,
                                     config_.mbuf_pool_size,
                                     static_cast<unsigned>(numa_node),
                                     nullptr);
        if (ret < 0) {
            last_error_ = "rte_eth_tx_queue_setup failed for queue "
                          + std::to_string(q);
            THEMIS_ERROR("DPDKServer: {}", last_error_);
            rte_mempool_free(mbuf_pool);
            rte_eal_cleanup();
            return false;
        }
    }

    // Start the Ethernet device.
    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        last_error_ = "rte_eth_dev_start failed: " + std::string(rte_strerror(-ret));
        THEMIS_ERROR("DPDKServer: {}", last_error_);
        rte_mempool_free(mbuf_pool);
        rte_eal_cleanup();
        return false;
    }

    if (config_.enable_hw_checksum) {
        rte_eth_promiscuous_enable(port_id);
    }

    // -------------------------------------------------------------------------
    // 4. Launch poll loops — one thread per lcore in the core mask.
    // -------------------------------------------------------------------------
    running_.store(true, std::memory_order_release);
    auto cores = coresFromMask(config_.cpu_core_mask);
    int queue_id = 0;
    for (int core : cores) {
        int qid = queue_id++ % static_cast<int>(config_.num_rx_queues);
        workers_.emplace_back([this, core, qid]() {
            if (!CpuPinner::pinCallerToCore(core)) {
                THEMIS_WARN("DPDKServer: failed to pin to core {}", core);
            }
            pollLoop(core, qid);
        });
    }

    THEMIS_INFO("DPDKServer: started on port {} (DPDK port {}), {} rx/tx queues, "
                "{} MiB huge pages",
                config_.port, port_id,
                config_.num_rx_queues, config_.huge_pages_mb);
    return true;

#else  // THEMIS_ENABLE_DPDK not defined

    last_error_ = "DPDKServer: DPDK support not compiled in "
                  "(build with -DTHEMIS_ENABLE_DPDK=ON)";
    THEMIS_WARN("{}", last_error_);
    return false;

#endif
}

// =============================================================================
// DPDKServer::stop
// =============================================================================

void DPDKServer::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) return;
    for (auto& t : workers_) {
        timedJoin(t);
    }
    workers_.clear();

#if defined(THEMIS_ENABLE_DPDK)
    rte_eth_dev_stop(0);
    rte_eal_cleanup();
#endif
    THEMIS_INFO("DPDKServer: stopped");
}

// =============================================================================
// DPDKServer::pollLoop
// =============================================================================

void DPDKServer::pollLoop([[maybe_unused]] int core_id,
                          [[maybe_unused]] int queue_id) {
#if defined(THEMIS_ENABLE_DPDK)
    constexpr uint64_t kCyclesPerStatUpdate = 100000000ULL;  // ~0.1 s at 1 GHz
    uint64_t last_stat_cycle = rte_get_tsc_cycles();

    while (running_.load(std::memory_order_relaxed)) {
        // Receive burst.
        struct rte_mbuf* rx_pkts[32];
        uint16_t nb_rx = rte_eth_rx_burst(0, static_cast<uint16_t>(queue_id),
                                           rx_pkts, config_.rx_burst_size);

        if (nb_rx > 0) {
            // Process each received packet.
            for (uint16_t i = 0; i < nb_rx; ++i) {
                struct rte_mbuf* m = rx_pkts[i];

                {
                    std::lock_guard<std::mutex> lk(stats_mutex_);
                    ++stats_.rx_packets;
                    stats_.rx_bytes += rte_pktmbuf_pkt_len(m);
                    ++stats_.requests;
                }

                // Free the mbuf — in a full implementation the packet
                // payload would be decoded and dispatched to the storage
                // layer before the mbuf is released.
                rte_pktmbuf_free(m);
            }
        }

        // Update poll stats periodically.
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.poll_iterations;
        }

        uint64_t now = rte_get_tsc_cycles();
        if (now - last_stat_cycle >= kCyclesPerStatUpdate) {
            last_stat_cycle = now;
            THEMIS_DEBUG("DPDKServer core {}: rx={} tx={} dropped={}",
                         core_id, stats_.rx_packets, stats_.tx_packets,
                         stats_.rx_dropped);
        }
    }
#endif
}

// =============================================================================
// DPDKServer::stats
// =============================================================================

DPDKServer::Stats DPDKServer::stats() const noexcept {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

// =============================================================================
// IoUringServer — helpers
// =============================================================================

/*static*/
bool IoUringServer::isIoUringAvailable() noexcept {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    // Probe by attempting a setup with 0 entries (will fail with EINVAL on old
    // kernels that do not support io_uring at all, but EFAULT/EINVAL variant
    // means the syscall exists).
    struct io_uring_params p;
    std::memset(&p, 0, sizeof(p));
    int fd = themis_io_uring_setup(1, &p);
    if (fd >= 0) {
        ::close(fd);
        return true;
    }
    // EPERM means the syscall exists but requires CAP_SYS_ADMIN.
    if (errno == EPERM) return true;
    // ENOMEM means available but out of memory right now.
    if (errno == ENOMEM) return true;
    return false;
#else
    return false;
#endif
}

/*static*/
uint32_t IoUringServer::ioUringVersion() noexcept {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    struct io_uring_params p;
    std::memset(&p, 0, sizeof(p));
    int fd = themis_io_uring_setup(2, &p);
    if (fd < 0) return 0;
    ::close(fd);
    // Feature flags encode version indirectly; return sq_entries as a proxy.
    return (p.features & IORING_FEAT_FAST_POLL) ? 0x0506U : 0x0501U;
#else
    return 0;
#endif
}

// =============================================================================
// IoUringServer — construction / destruction
// =============================================================================

IoUringServer::IoUringServer(const Config&          config,
                             RocksDBWrapper*        storage,
                             SecondaryIndexManager* index_mgr)
    : config_(config), storage_(storage), index_mgr_(index_mgr)
{}

IoUringServer::~IoUringServer() {
    stop();
}

// =============================================================================
// IoUringServer::setupListenSocket
// =============================================================================

bool IoUringServer::setupListenSocket() {
#ifdef __linux__
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        last_error_ = "socket() failed: " + std::string(std::strerror(errno));
        return false;
    }

    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(config_.port);
    addr.sin_addr.s_addr = ::inet_addr(config_.host.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (::bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        last_error_ = "bind() failed on port " + std::to_string(config_.port)
                      + ": " + std::strerror(errno);
        ::close(fd);
        return false;
    }

    if (::listen(fd, /* backlog */ 128) < 0) {
        last_error_ = "listen() failed: " + std::string(std::strerror(errno));
        ::close(fd);
        return false;
    }

    listen_fd_ = fd;
    return true;
#else
    last_error_ = "IoUringServer: not supported on non-Linux platforms";
    return false;
#endif
}

// =============================================================================
// IoUringServer::setupIoUring
// =============================================================================

bool IoUringServer::setupIoUring() {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    struct io_uring_params params;
    std::memset(&params, 0, sizeof(params));

    // Enable kernel-side SQ polling thread.
    params.flags |= IORING_SETUP_SQPOLL;

    if (config_.sq_thread_cpu >= 0) {
        params.flags |= IORING_SETUP_SQ_AFF;
        params.sq_thread_cpu = static_cast<uint32_t>(config_.sq_thread_cpu);
    }

    params.sq_thread_idle = config_.sq_thread_idle_ms;

    int fd = themis_io_uring_setup(config_.ring_size, &params);
    if (fd < 0) {
        last_error_ = "io_uring_setup failed: " + std::string(std::strerror(errno));
        return false;
    }
    ring_fd_ = fd;

    // Allocate and register fixed I/O buffers for zero-copy sends.
    fixed_bufs_.reserve(config_.num_fixed_buffers);
    std::vector<struct iovec> iovecs;
    iovecs.reserve(config_.num_fixed_buffers);

    for (uint32_t i = 0; i < config_.num_fixed_buffers; ++i) {
        auto buf = std::make_unique<ZeroCopyDmaBuffer>(config_.send_buf_size);
        if (!buf->valid()) {
            THEMIS_WARN("IoUringServer: fixed buffer {} allocation failed; "
                        "zero-copy sends disabled beyond {} buffers",
                        i, i);
            break;
        }
        iovecs.push_back({buf->data(), buf->size()});
        fixed_bufs_.push_back(std::move(buf));
    }

    if (!iovecs.empty()) {
        int rc = themis_io_uring_register(ring_fd_,
                                          IORING_REGISTER_BUFFERS,
                                          iovecs.data(),
                                          static_cast<unsigned>(iovecs.size()));
        if (rc < 0) {
            THEMIS_WARN("IoUringServer: IORING_REGISTER_BUFFERS failed ({}); "
                        "falling back to regular send",
                        std::strerror(-rc));
        } else {
            THEMIS_INFO("IoUringServer: {} fixed buffers registered "
                        "(zero-copy sends enabled)",
                        iovecs.size());
        }
    }

    THEMIS_INFO("IoUringServer: ring initialised (fd={}, ring_size={}, "
                "sqpoll_cpu={})",
                ring_fd_, config_.ring_size, config_.sq_thread_cpu);
    return true;

#else
    last_error_ = "IoUringServer: THEMIS_ENABLE_IO_URING not set or "
                  "non-Linux platform";
    return false;
#endif
}

// =============================================================================
// IoUringServer::start
// =============================================================================

bool IoUringServer::start() {
    if (running_.load(std::memory_order_relaxed)) {
        last_error_ = "IoUringServer::start() called while already running";
        return false;
    }

    if (!setupListenSocket()) return false;
    if (!setupIoUring()) {
        if (listen_fd_ >= 0) {
#ifdef __linux__
            ::close(listen_fd_);
#endif
            listen_fd_ = -1;
        }
        return false;
    }

    running_.store(true, std::memory_order_release);

    for (uint32_t i = 0; i < config_.num_worker_threads; ++i) {
        workers_.emplace_back([this, i]() { workerLoop(static_cast<int>(i)); });
    }

    THEMIS_INFO("IoUringServer: started on {}:{}, ring_size={}, {} workers",
                config_.host, config_.port,
                config_.ring_size, config_.num_worker_threads);
    return true;
}

// =============================================================================
// IoUringServer::workerLoop
// =============================================================================

void IoUringServer::workerLoop([[maybe_unused]] int worker_id) {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    // In a full production implementation each worker thread maintains its
    // own per-connection state machine and drives the io_uring CQE processing
    // loop.  The design here follows the multi-shot accept pattern:
    //
    //   1. Submit IORING_OP_ACCEPT SQE for listen_fd_.
    //   2. For each accepted CQE: submit IORING_OP_RECV SQE.
    //   3. For each recv CQE: decode ThemisDB wire frame, dispatch to
    //      storage layer, submit IORING_OP_SEND SQE with response.
    //   4. Repeat until running_ is false.
    //
    // The actual SQE/CQE manipulation re-uses the ring_fd_ initialised in
    // setupIoUring().  Since the ring is shared by all worker threads the
    // submission path uses atomics on the SQ tail pointer; each worker
    // reaps its own CQEs via the CQ head.  For production deployments one
    // ring per worker (IORING_SETUP_ATTACH_WQ) should be considered for
    // even lower contention.

    // Pin this worker thread to a core if SQ CPU affinity is configured.
    if (config_.sq_thread_cpu >= 0) {
        int target = config_.sq_thread_cpu + worker_id;
        CpuPinner::pinCallerToCore(target);
    }

    // Minimal CQE-drain loop — drives the ring and collects statistics.
    while (running_.load(std::memory_order_relaxed)) {
        // Wait for at least one CQE (timeout: 1 ms).
        int rc = themis_io_uring_enter(ring_fd_,
                                       /* to_submit   */ 0,
                                       /* min_complete*/ 1,
                                       /* flags       */ IORING_ENTER_GETEVENTS,
                                       /* sigmask     */ nullptr);
        if (rc < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            if (!running_.load(std::memory_order_relaxed)) break;
            THEMIS_WARN("IoUringServer worker {}: io_uring_enter error: {}",
                        worker_id, std::strerror(errno));
            break;
        }
        if (rc == 0) continue;

        std::lock_guard<std::mutex> lk(stats_mutex_);
        stats_.recv_completions += static_cast<uint64_t>(rc);
        ++stats_.sq_poll_wakeups;
    }
#endif
}

// =============================================================================
// IoUringServer::teardown
// =============================================================================

void IoUringServer::teardown() {
#ifdef __linux__
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    if (ring_fd_ >= 0) {
        ::close(ring_fd_);
        ring_fd_ = -1;
    }
#endif
    fixed_bufs_.clear();
}

// =============================================================================
// IoUringServer::stop
// =============================================================================

void IoUringServer::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) return;

    // Close the ring fd so all blocked io_uring_enter() calls return -EBADF.
#ifdef __linux__
    if (ring_fd_ >= 0) {
        ::close(ring_fd_);
        ring_fd_ = -1;
    }
#endif

    for (auto& t : workers_) {
        timedJoin(t);
    }
    workers_.clear();
    teardown();
    THEMIS_INFO("IoUringServer: stopped");
}

// =============================================================================
// IoUringServer::stats
// =============================================================================

IoUringServer::Stats IoUringServer::stats() const noexcept {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

} // namespace network
} // namespace themis

