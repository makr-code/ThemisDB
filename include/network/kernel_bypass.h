/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            kernel_bypass.h                                    ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-15 07:07:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     595                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 303d17b93c  2026-04-13  feat(network): Kernel Bypass (DPDK/io_uring) — v1.9.0 (#4... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file kernel_bypass.h
 * @brief Kernel bypass networking for ultra-low latency applications.
 *
 * Provides two independent bypass paths:
 *
 * 1. **DPDKServer** (guarded by `THEMIS_ENABLE_DPDK`)
 *    - User-space PMD poll-mode driver; bypasses the kernel TCP/IP stack
 *      entirely for maximum throughput on 10G/40G/100G NICs.
 *    - Supports CPU core pinning, NUMA-local memory allocation, and
 *      huge-page backing for the packet buffer pool (mbuf pool).
 *    - Configurable RX/TX queue count for multi-queue RSS.
 *    - Target: 1–10 µs one-way latency, 100 Gbps line rate.
 *
 * 2. **IoUringServer** (guarded by `THEMIS_ENABLE_IO_URING` + Linux)
 *    - io_uring with `IORING_SETUP_SQPOLL` for kernel-side SQ polling;
 *      eliminates `io_uring_enter()` syscalls on the hot path.
 *    - Multi-operation submission (accept, recv, send) batched into a
 *      single ring; zero per-operation syscalls in steady state.
 *    - Target: 10–50 µs round-trip, 10 Gbps throughput.
 *
 * Both servers expose a uniform `start()` / `stop()` / `isRunning()`
 * interface.  When the required build flag is absent the implementation
 * falls through to a safe, documented no-op that sets an error flag
 * accessible via `lastError()`.
 *
 * Supporting utilities
 * --------------------
 * - **CpuPinner** — pin calling thread or a target thread to a logical
 *   CPU core; query NUMA node for a core.
 * - **NumaAllocator** — NUMA-aware `allocate()` / `free()` wrappers
 *   backed by `libnuma` when available.
 * - **ZeroCopyDmaBuffer** — huge-page-backed, DMA-capable buffer for
 *   use with DPDK mbufs or io_uring fixed buffers.
 *
 * Usage (DPDK):
 * @code
 *   DPDKServer::Config cfg;
 *   cfg.port         = 8772;
 *   cfg.pci_address  = "0000:05:00.0";
 *   cfg.num_rx_queues = 4;
 *   cfg.num_tx_queues = 4;
 *   cfg.cpu_core_mask = 0x0F;   // cores 0-3
 *   cfg.huge_pages_mb = 2048;
 *   DPDKServer srv(cfg, storage, index_mgr);
 *   srv.start();
 * @endcode
 *
 * Usage (io_uring):
 * @code
 *   IoUringServer::Config cfg;
 *   cfg.port             = 8773;
 *   cfg.ring_size        = 4096;
 *   cfg.sq_thread_cpu    = 2;
 *   cfg.sq_thread_idle_ms = 1000;
 *   IoUringServer srv(cfg, storage, index_mgr);
 *   srv.start();
 * @endcode
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef __linux__
#  include <sched.h>   // cpu_set_t / sched_setaffinity
#endif

namespace themis {

class RocksDBWrapper;
class SecondaryIndexManager;

namespace network {

// =============================================================================
// CpuPinner — CPU core and NUMA affinity helper
// =============================================================================

/**
 * @brief CPU core pinning and NUMA topology helper.
 *
 * All methods are Linux-only (no-op on other platforms).  Errors are
 * reported via the return value; no exceptions are thrown.
 */
class CpuPinner {
public:
    /**
     * @brief Pin the calling thread to a single logical CPU core.
     *
     * @param core_id   Zero-based logical CPU index.
     * @return true on success; false if the core is out-of-range or the
     *         syscall fails.
     */
    static bool pinCallerToCore(int core_id) noexcept;

    /**
     * @brief Pin an arbitrary thread to a single logical CPU core.
     *
     * @param thread    Target thread.  If the thread has not been joined
     *                  or detached the behaviour is undefined.
     * @param core_id   Zero-based logical CPU index.
     * @return true on success.
     */
    static bool pinThreadToCore(std::thread& thread, int core_id) noexcept;

    /**
     * @brief Return the NUMA node that owns @p core_id, or -1 on error.
     *
     * Reads `/sys/devices/system/cpu/cpu<N>/node<M>` symlinks.
     */
    static int numaNodeForCore(int core_id) noexcept;

    /**
     * @brief Return the number of logical CPUs visible to the process.
     */
    static int logicalCpuCount() noexcept;

    /**
     * @brief Return the index of the CPU on which the caller is currently
     *        executing, or -1 on error.
     */
    static int currentCpu() noexcept;

    /**
     * @brief Return all core IDs that belong to the specified NUMA node.
     */
    static std::vector<int> coresOnNuma(int numa_node) noexcept;
};

// =============================================================================
// NumaAllocator — NUMA-aware raw memory allocator
// =============================================================================

/**
 * @brief Allocate memory local to a NUMA node.
 *
 * When `libnuma` is available (guarded by `THEMIS_ENABLE_NUMA`) the
 * allocation is placed on the specified node.  Otherwise the request
 * falls through to `std::aligned_alloc`.
 *
 * Allocations are always aligned to at least 64 bytes (cache line).
 */
class NumaAllocator {
public:
    /**
     * @brief Allocate @p size bytes on NUMA node @p node.
     *
     * @param size  Allocation size in bytes.  Must be > 0.
     * @param node  Target NUMA node (-1 = system default).
     * @return Pointer to the allocation; never null (throws std::bad_alloc).
     */
    static void* allocate(size_t size, int node = -1);

    /**
     * @brief Release memory previously allocated via `allocate()`.
     *
     * @param ptr   Pointer returned by `allocate()`.
     * @param size  Original allocation size.
     */
    static void deallocate(void* ptr, size_t size) noexcept;

    /**
     * @brief Whether NUMA-aware allocation is available on this system.
     */
    static bool isNumaAvailable() noexcept;
};

// =============================================================================
// ZeroCopyDmaBuffer — huge-page-backed DMA buffer
// =============================================================================

/**
 * @brief Contiguous, physically-backed buffer suitable for DMA and
 *        io_uring fixed-buffer registration.
 *
 * Memory is allocated via huge pages (2 MiB) when available; falls back
 * to `mmap(MAP_HUGETLB)` and then to `mmap(MAP_ANONYMOUS)`.
 *
 * Non-copyable; move-only.
 */
class ZeroCopyDmaBuffer {
public:
    /**
     * @brief Allocate a zero-copy buffer.
     *
     * @param size_bytes  Requested allocation in bytes.
     * @param numa_node   NUMA node preference (-1 = no preference).
     */
    explicit ZeroCopyDmaBuffer(size_t size_bytes, int numa_node = -1);
    ~ZeroCopyDmaBuffer();

    ZeroCopyDmaBuffer(const ZeroCopyDmaBuffer&)            = delete;
    ZeroCopyDmaBuffer& operator=(const ZeroCopyDmaBuffer&) = delete;
    ZeroCopyDmaBuffer(ZeroCopyDmaBuffer&&) noexcept;
    ZeroCopyDmaBuffer& operator=(ZeroCopyDmaBuffer&&) noexcept;

    /** @return Pointer to the start of the buffer; null if allocation failed. */
    void*  data()       noexcept { return data_; }
    const void* data() const noexcept { return data_; }

    /** @return Allocation size in bytes. */
    size_t size() const noexcept { return size_; }

    /** @return true if the buffer was backed by huge pages. */
    bool isHugePage() const noexcept { return huge_page_; }

    /** @return true if the allocation succeeded. */
    bool valid() const noexcept { return data_ != nullptr; }

private:
    void*  data_      = nullptr;
    size_t size_      = 0;
    bool   huge_page_ = false;
};

// =============================================================================
// DPDKServer
// =============================================================================

/**
 * @brief DPDK poll-mode server for ultra-high-throughput, low-latency I/O.
 *
 * The server bypasses the kernel TCP/IP stack entirely.  It requires:
 *   - A DPDK-compatible NIC bound to VFIO or UIO.
 *   - Huge pages configured on the host (e.g. `vm.nr_hugepages`).
 *   - Build flag `THEMIS_ENABLE_DPDK`.
 *
 * When `THEMIS_ENABLE_DPDK` is absent the class compiles without change
 * but `start()` returns false and `lastError()` explains why.
 *
 * Performance characteristics:
 *   - Latency:    1–10 µs end-to-end
 *   - Throughput: up to 100 Gbps on compatible NICs
 *   - CPU model:  spinning poll loop (no interrupts, no context switches)
 */
class DPDKServer {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Listening port (ThemisDB wire protocol).
        uint16_t port = 8772;

        /// PCI address of the DPDK-bound NIC (e.g. "0000:05:00.0").
        std::string pci_address;

        /// Number of RX queues (RSS multi-queue).  Must be ≥ 1.
        uint16_t num_rx_queues = 1;

        /// Number of TX queues.  Must be ≥ 1.
        uint16_t num_tx_queues = 1;

        /// CPU core mask for lcore assignment (bit N = core N).
        uint64_t cpu_core_mask = 0x01;  // core 0 only

        /// Huge-page memory pool size in MiB.
        uint32_t huge_pages_mb = 1024;

        /// Number of mbufs in the packet buffer pool per queue.
        uint32_t mbuf_pool_size = 8192;

        /// Receive burst size (packets per poll iteration).
        uint16_t rx_burst_size = 32;

        /// Transmit burst size.
        uint16_t tx_burst_size = 32;

        /// NUMA node for the mbuf pool (-1 = autodetect from PCI address).
        int numa_node = -1;

        /// Maximum number of concurrent connections (user-space TCP).
        uint32_t max_connections = 4096;

        /// Enable RSS (Receive Side Scaling) hash.
        bool enable_rss = true;

        /// Enable hardware checksum offload.
        bool enable_hw_checksum = true;

        /// Enable jumbo frames (up to 9 KB).
        bool enable_jumbo_frames = false;
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    struct Stats {
        uint64_t rx_packets      = 0; ///< Total packets received
        uint64_t tx_packets      = 0; ///< Total packets transmitted
        uint64_t rx_bytes        = 0; ///< Total bytes received
        uint64_t tx_bytes        = 0; ///< Total bytes transmitted
        uint64_t rx_dropped      = 0; ///< Packets dropped on RX (ring full)
        uint64_t tx_errors       = 0; ///< TX errors (NIC rejected)
        uint64_t connections     = 0; ///< Active connections (user-space TCP)
        uint64_t requests        = 0; ///< Total ThemisDB requests handled
        uint64_t poll_iterations = 0; ///< Poll loop iterations
        double   avg_latency_us  = 0; ///< Exponential moving average latency
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a DPDKServer with the given configuration.
     *
     * Does not open any ports or allocate huge pages until `start()` is
     * called.  The `storage` and `index_mgr` pointers are stored but not
     * dereferenced during construction; they may be null for unit tests
     * that never call `start()`.
     */
    explicit DPDKServer(const Config&        config,
                        RocksDBWrapper*      storage   = nullptr,
                        SecondaryIndexManager* index_mgr = nullptr);

    ~DPDKServer();

    DPDKServer(const DPDKServer&)            = delete;
    DPDKServer& operator=(const DPDKServer&) = delete;

    /**
     * @brief Initialise DPDK EAL, configure the NIC port, and start
     *        the RX/TX poll loop on the configured lcore set.
     *
     * @return true if DPDK was successfully initialised and the poll loop
     *         started; false otherwise (check `lastError()`).
     */
    bool start();

    /**
     * @brief Signal the poll loop to stop and join all lcore threads.
     *
     * Safe to call if `start()` was never called or already returned false.
     */
    void stop();

    /** @return true while the poll loop is running. */
    bool isRunning() const noexcept { return running_.load(std::memory_order_relaxed); }

    /** @return Configuration passed at construction. */
    const Config& config() const noexcept { return config_; }

    /** @return Snapshot of current statistics. */
    Stats stats() const noexcept;

    /**
     * @brief Human-readable description of the last error, or empty on
     *        success.
     */
    const std::string& lastError() const noexcept { return last_error_; }

    /**
     * @brief Whether DPDK support was compiled in.
     *
     * Equivalent to checking `THEMIS_ENABLE_DPDK` at the call site.
     */
    static bool isDpdkAvailable() noexcept;

    /**
     * @brief Return the set of CPU cores derived from `cpu_core_mask`.
     */
    static std::vector<int> coresFromMask(uint64_t mask) noexcept;

private:
    Config                  config_;
    RocksDBWrapper*         storage_   = nullptr;
    SecondaryIndexManager*  index_mgr_ = nullptr;

    std::atomic<bool>       running_{false};
    std::string             last_error_;

    mutable std::mutex      stats_mutex_;
    Stats                   stats_{};

    // Worker threads (one per lcore derived from cpu_core_mask).
    std::vector<std::thread> workers_;

    // Internal poll loop (runs per lcore thread).
    void pollLoop(int core_id, int queue_id);
};

// =============================================================================
// IoUringServer
// =============================================================================

/**
 * @brief io_uring-based async TCP server for low-latency, high-throughput I/O.
 *
 * Uses `IORING_SETUP_SQPOLL` so that the kernel polls the submission queue
 * without requiring `io_uring_enter()` on the hot path.  In steady state,
 * all accept/recv/send operations complete with zero syscalls on the
 * application side.
 *
 * Operations batched per io_uring ring:
 *   - `IORING_OP_ACCEPT`  — non-blocking accept loop
 *   - `IORING_OP_RECV`    — per-connection read
 *   - `IORING_OP_SEND`    — per-connection write (zero-copy via fixed buffers)
 *
 * Guarded by `THEMIS_ENABLE_IO_URING` and `__linux__`.  On other
 * platforms (or when the build flag is absent) `start()` returns false.
 *
 * Performance characteristics:
 *   - Latency:    10–50 µs round-trip
 *   - Throughput: up to 10 Gbps
 *   - CPU model:  SQ polling thread in kernel; application threads can sleep
 */
class IoUringServer {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Listening address (IPv4/IPv6 dotted notation or host name).
        std::string host = "0.0.0.0";

        /// Listening port.
        uint16_t port = 8773;

        /// io_uring SQ/CQ ring depth (must be power of two).
        uint32_t ring_size = 4096;

        /// CPU core for the kernel SQ polling thread (-1 = no pinning).
        int sq_thread_cpu = -1;

        /// Milliseconds the kernel SQ thread idles before parking.
        uint32_t sq_thread_idle_ms = 2000;

        /// Number of application-side worker threads.
        uint32_t num_worker_threads = 4;

        /// Maximum number of concurrent connections.
        uint32_t max_connections = 16384;

        /// Receive buffer size per connection (bytes).
        uint32_t recv_buf_size = 65536;

        /// Send buffer size per connection (bytes).
        uint32_t send_buf_size = 65536;

        /// Number of fixed buffers registered with the ring.
        uint32_t num_fixed_buffers = 256;

        /// Enable `IORING_FEAT_FAST_POLL` if available.
        bool enable_fast_poll = true;

        /// Enable `IORING_FEAT_NODROP` if available.
        bool enable_nodrop = true;
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    struct Stats {
        uint64_t accept_completions  = 0; ///< Accepted connections
        uint64_t recv_completions    = 0; ///< Completed recv operations
        uint64_t send_completions    = 0; ///< Completed send operations
        uint64_t recv_bytes          = 0; ///< Total bytes received
        uint64_t send_bytes          = 0; ///< Total bytes sent
        uint64_t errors              = 0; ///< CQE errors (res < 0)
        uint64_t connections         = 0; ///< Current active connections
        uint64_t requests            = 0; ///< ThemisDB requests processed
        uint64_t sq_poll_wakeups     = 0; ///< Times the SQ polling thread woke
        uint64_t zero_copy_sends     = 0; ///< Sends using fixed buffers
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Construct an IoUringServer with the given configuration.
     *
     * Safe to construct when io_uring is unavailable; `start()` will
     * return false in that case.
     */
    explicit IoUringServer(const Config&          config,
                           RocksDBWrapper*        storage   = nullptr,
                           SecondaryIndexManager* index_mgr = nullptr);

    ~IoUringServer();

    IoUringServer(const IoUringServer&)            = delete;
    IoUringServer& operator=(const IoUringServer&) = delete;

    /**
     * @brief Bind the listening socket, set up the io_uring ring, and
     *        start worker threads.
     *
     * On success the server starts accepting connections immediately.
     *
     * @return true on success; false otherwise (see `lastError()`).
     */
    bool start();

    /**
     * @brief Drain in-flight operations, close the ring, and join all
     *        worker threads.
     */
    void stop();

    /** @return true while worker threads are running. */
    bool isRunning() const noexcept { return running_.load(std::memory_order_relaxed); }

    /** @return Configuration passed at construction. */
    const Config& config() const noexcept { return config_; }

    /** @return Snapshot of current statistics. */
    Stats stats() const noexcept;

    /**
     * @return Human-readable description of the last error, or empty on
     *         success.
     */
    const std::string& lastError() const noexcept { return last_error_; }

    /**
     * @brief Whether io_uring is supported on this host.
     *
     * Checks both the compile flag and the kernel version at runtime.
     */
    static bool isIoUringAvailable() noexcept;

    /**
     * @brief Return the kernel io_uring API version as (major << 8 | minor),
     *        or 0 if unavailable.
     */
    static uint32_t ioUringVersion() noexcept;

private:
    Config                  config_;
    RocksDBWrapper*         storage_   = nullptr;
    SecondaryIndexManager*  index_mgr_ = nullptr;

    std::atomic<bool>       running_{false};
    std::string             last_error_;

    // Listening socket file descriptor.
    int                     listen_fd_ = -1;

    // io_uring ring file descriptor (-1 = not initialised).
    int                     ring_fd_   = -1;

    mutable std::mutex      stats_mutex_;
    Stats                   stats_{};

    std::vector<std::thread> workers_;

    // Fixed buffer backing store for zero-copy sends.
    std::vector<std::unique_ptr<ZeroCopyDmaBuffer>> fixed_bufs_;

    // Internal helpers.
    bool setupListenSocket();
    bool setupIoUring();
    void workerLoop(int worker_id);
    void teardown();
};

} // namespace network
} // namespace themis
