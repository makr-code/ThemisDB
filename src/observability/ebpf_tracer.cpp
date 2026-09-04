/**
 * @file ebpf_tracer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/ebpf_tracer.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Platform detection
// ---------------------------------------------------------------------------
#if defined(__linux__)
#  define THEMIS_EBPF_LINUX 1
#  include <sys/ioctl.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <cerrno>
#  include <linux/perf_event.h>
#else
#  define THEMIS_EBPF_LINUX 0
#endif

namespace themis {
namespace observability {

// ============================================================================
// Internal helpers (Linux-only)
// ============================================================================
namespace {

#if THEMIS_EBPF_LINUX

/** Thin syscall wrapper — same pattern as pmu_counters.cpp */
static long perf_event_open_syscall(struct perf_event_attr* attr,
                                     pid_t pid, int cpu,
                                     int group_fd,
                                     unsigned long flags) noexcept
{
    return ::syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/**
 * @brief Open a single PERF_TYPE_SOFTWARE perf counter for the current
 *        process across all CPUs.
 *
 * @param sw_config  A PERF_COUNT_SW_* constant.
 * @return           A non-negative file descriptor, or -1 on failure.
 */
static int openSoftwareCounter([[maybe_unused]] uint64_t sw_config) noexcept {
    struct perf_event_attr attr{};
    attr.type        = PERF_TYPE_SOFTWARE;
    attr.size        = sizeof(attr);
    attr.config      = sw_config;
    attr.disabled    = 1;
    // Track all threads of the calling process; inherit into children.
    attr.inherit     = 1;
    attr.inherit_stat = 1;

    long fd = perf_event_open_syscall(&attr,
                                      0,   // current process
                                      -1,  // all CPUs
                                      -1,  // no group leader
                                      0);
    return (fd < 0) ? -1 : static_cast<int>(fd);
}

/** Enable a perf counter fd; no-op if fd < 0. */
static void enableCounter([[maybe_unused]] int fd) noexcept {
    if (fd >= 0) {
        ::ioctl(fd, PERF_EVENT_IOC_RESET,  0);
        ::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
}

/** Disable a perf counter fd; no-op if fd < 0. */
static void disableCounter([[maybe_unused]] int fd) noexcept {
    if (fd >= 0) {
        ::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    }
}

/** Read the current absolute count from a perf fd.  Returns -1 on error. */
static int64_t readCounter([[maybe_unused]] int fd) noexcept {
    if (fd < 0) {
      return -1;
    }
    uint64_t value = 0;
    ssize_t n = ::read(fd, &value, sizeof(value));
    return (n == static_cast<ssize_t>(sizeof(value)))
               ? static_cast<int64_t>(value)
               : -1;
}

/** Close a perf fd; no-op if fd < 0. */
static void closeCounter(int& fd) noexcept {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

#endif // THEMIS_EBPF_LINUX

} // anonymous namespace

// ============================================================================
// EbpfTracer::Impl
// ============================================================================

/** @brief EbpfTracer::Impl. */
class EbpfTracer::Impl {
public:
    explicit Impl(const EbpfTracerConfig& cfg)
        : config_(cfg)
    {
#if THEMIS_EBPF_LINUX
        fd_ctx_sw_   = -1;
        fd_pg_fault_ = -1;
        fd_cpu_mig_  = -1;
        fd_task_clk_ = -1;
        prev_ctx_sw_   = 0;
        prev_pg_fault_ = 0;
        prev_cpu_mig_  = 0;
        prev_task_clk_ = 0;
#endif
        if (cfg.enabled) {
            start();
        }
    }

    ~Impl() {
        stop();
    }

    // -----------------------------------------------------------------------
    void start() {
        std::unique_lock<std::mutex> lk(mu_);
        if (running_) {
          return;
        }

#if THEMIS_EBPF_LINUX
        openCounters();
#endif
        running_ = true;
        lk.unlock();

        thread_ = std::thread(&Impl::collectionLoop, this);
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lk(mu_);
            if (!running_) {
              return;
            }
            running_ = false;
        }
        cv_.notify_all();
        if (thread_.joinable()) {
          thread_.join();
        }

#if THEMIS_EBPF_LINUX
        closeCounters();
#endif
    }

    bool isEnabled() const {
        std::lock_guard<std::mutex> lk(mu_);
        return running_;
    }

    void enable() {
        start();
    }

    void disable() {
        stop();
    }

    EbpfTracerStats getStats() const {
        std::lock_guard<std::mutex> lk(mu_);
        return stats_;
    }

    std::vector<KernelEvent> getRecentEvents() const {
        std::lock_guard<std::mutex> lk(mu_);
        return {events_.begin(), events_.end()};
    }

    void registerEventCallback(std::function<void(const std::vector<KernelEvent>&)> cb) {
        std::lock_guard<std::mutex> lk(mu_);
        callback_ = std::move([[maybe_unused]] cb);
    }

    void reset() {
        {
            std::lock_guard<std::mutex> lk(mu_);
            stats_ = {};
            events_.clear();
        }
#if THEMIS_EBPF_LINUX
        // Signal the background thread to re-read baselines so that subsequent
        // deltas start from zero.  Using an atomic flag avoids a data race: the
        // background thread is the only writer of prev_* outside the lock, so
        // letting it do the re-read is the only race-free approach.
        pending_reset_.store(true, std::memory_order_release);
#endif
    }

    EbpfTracerConfig getConfig() const {
        std::lock_guard<std::mutex> lk(mu_);
        return config_;
    }

    static bool isPlatformSupported() noexcept {
#if THEMIS_EBPF_LINUX
        return true;
#else
        return false;
#endif
    }

private:
    // -----------------------------------------------------------------------
    // Platform-specific counter management
    // -----------------------------------------------------------------------
#if THEMIS_EBPF_LINUX
    void openCounters() {
        if (config_.probe_context_switches) {
            fd_ctx_sw_ = openSoftwareCounter(PERF_COUNT_SW_CONTEXT_SWITCHES);
            if (fd_ctx_sw_ >= 0) {
                enableCounter(fd_ctx_sw_);
                prev_ctx_sw_ = readCounter(fd_ctx_sw_);
                if (prev_ctx_sw_ < 0) {
                  prev_ctx_sw_ = 0;
                }
            }
        }
        if (config_.probe_page_faults) {
            fd_pg_fault_ = openSoftwareCounter(PERF_COUNT_SW_PAGE_FAULTS);
            if (fd_pg_fault_ >= 0) {
                enableCounter(fd_pg_fault_);
                prev_pg_fault_ = readCounter(fd_pg_fault_);
                if (prev_pg_fault_ < 0) {
                  prev_pg_fault_ = 0;
                }
            }
        }
        if (config_.probe_cpu_migrations) {
            fd_cpu_mig_ = openSoftwareCounter(PERF_COUNT_SW_CPU_MIGRATIONS);
            if (fd_cpu_mig_ >= 0) {
                enableCounter(fd_cpu_mig_);
                prev_cpu_mig_ = readCounter(fd_cpu_mig_);
                if (prev_cpu_mig_ < 0) {
                  prev_cpu_mig_ = 0;
                }
            }
        }
        if (config_.probe_task_clock) {
            fd_task_clk_ = openSoftwareCounter(PERF_COUNT_SW_TASK_CLOCK);
            if (fd_task_clk_ >= 0) {
                enableCounter(fd_task_clk_);
                prev_task_clk_ = readCounter(fd_task_clk_);
                if (prev_task_clk_ < 0) {
                  prev_task_clk_ = 0;
                }
            }
        }
    }

    void closeCounters() noexcept {
        disableCounter(fd_ctx_sw_);
        disableCounter(fd_pg_fault_);
        disableCounter(fd_cpu_mig_);
        disableCounter(fd_task_clk_);

        closeCounter(fd_ctx_sw_);
        closeCounter(fd_pg_fault_);
        closeCounter(fd_cpu_mig_);
        closeCounter(fd_task_clk_);

        prev_ctx_sw_   = 0;
        prev_pg_fault_ = 0;
        prev_cpu_mig_  = 0;
        prev_task_clk_ = 0;
    }

    /**
     * @brief Read all perf counters and return a batch of delta events.
     *        Called from the background thread while NOT holding mu_.
     *
     *        If a reset was requested via pending_reset_, the function
     *        re-reads all baselines and returns an empty batch so that the
     *        next call produces fresh deltas.
     */
    std::vector<KernelEvent> collectDeltas() {
        // Handle a pending reset: re-read baselines and return no events.
        // pending_reset_ is only cleared here (background thread), so there
        // is no race with reset() which only sets it.
        if (pending_reset_.exchange(false, std::memory_order_acq_rel)) {
            if (fd_ctx_sw_ >= 0) {
                int64_t v = readCounter(fd_ctx_sw_);
                prev_ctx_sw_ = (v >= 0) ? v : 0;
            }
            if (fd_pg_fault_ >= 0) {
                int64_t v = readCounter(fd_pg_fault_);
                prev_pg_fault_ = (v >= 0) ? v : 0;
            }
            if (fd_cpu_mig_ >= 0) {
                int64_t v = readCounter(fd_cpu_mig_);
                prev_cpu_mig_ = (v >= 0) ? v : 0;
            }
            if (fd_task_clk_ >= 0) {
                int64_t v = readCounter(fd_task_clk_);
                prev_task_clk_ = (v >= 0) ? v : 0;
            }
            return {};
        }

        auto now = std::chrono::system_clock::now();
        std::vector<KernelEvent> batch;

        auto delta = [&](int fd, int64_t& prev, EbpfProbeType type,
                         const std::string& desc) {
            if (fd < 0) {
              return;
            }
            int64_t cur = readCounter(fd);
            if (cur < 0) {
              return;
            }
            int64_t d = cur - prev;
            if (d < 0) d = 0; // counter reset or wrap
            prev = cur;
            if (d == 0) {
              return;
            }
            KernelEvent ev;
            ev.type        = type;
            ev.timestamp   = now;
            ev.delta       = d;
            ev.description = desc;
            batch.push_back(std::move(ev));
        };

        // prev_* are only modified here (background thread) and in the reset
        // path above, so no additional synchronisation is needed.
        delta(fd_ctx_sw_,   prev_ctx_sw_,   EbpfProbeType::CONTEXT_SWITCH, "context_switches");
        delta(fd_pg_fault_, prev_pg_fault_, EbpfProbeType::PAGE_FAULT,     "page_faults");
        delta(fd_cpu_mig_,  prev_cpu_mig_,  EbpfProbeType::CPU_MIGRATION,  "cpu_migrations");
        delta(fd_task_clk_, prev_task_clk_, EbpfProbeType::TASK_CLOCK,     "task_clock_ns");

        return batch;
    }

    int fd_ctx_sw_;
    int fd_pg_fault_;
    int fd_cpu_mig_;
    int fd_task_clk_;

    int64_t prev_ctx_sw_;
    int64_t prev_pg_fault_;
    int64_t prev_cpu_mig_;
    int64_t prev_task_clk_;
#endif // THEMIS_EBPF_LINUX

    // -----------------------------------------------------------------------
    // Background collection loop (all platforms)
    // -----------------------------------------------------------------------
    void collectionLoop() {
        // Cache the interval once — config_ is read-only after construction.
        const auto interval = config_.collection_interval;

        while (true) {
            // Wait for interval or stop signal
            {
                std::unique_lock<std::mutex> wlk(mu_);
                cv_.wait_for(wlk, interval, [this]{ return !running_; });
                if (!running_) {
                  break;
                }
            }

            std::vector<KernelEvent> batch;
#if THEMIS_EBPF_LINUX
            // Collect counter deltas outside the lock
            batch = collectDeltas();
#endif
            // Accumulate stats and publish metrics under the lock,
            // regardless of whether events were collected.
            {
                std::lock_guard<std::mutex> slk(mu_);
                for (const auto& ev : batch) {
                    accumulateEvent([[maybe_unused]] ev);
                    appendEvent([[maybe_unused]] ev);
                }
                ++stats_.collection_cycles;
                publishMetrics();
            }

            // Fire callback outside the lock, only when there are events.
            if (!batch.empty()) {
                std::function<void(const std::vector<KernelEvent>&)> cb;
                {
                    std::lock_guard<std::mutex> slk(mu_);
                    cb = callback_;
                }
                if (cb) {
                    cb(batch);
                }
            }
        }
    }

    void accumulateEvent([[maybe_unused]] const KernelEvent& ev) {
        switch (ev.type) {
            case EbpfProbeType::CONTEXT_SWITCH:
                stats_.context_switches_total += ev.delta;
                break;
            case EbpfProbeType::PAGE_FAULT:
                stats_.page_faults_total += ev.delta;
                break;
            case EbpfProbeType::CPU_MIGRATION:
                stats_.cpu_migrations_total += ev.delta;
                break;
            case EbpfProbeType::TASK_CLOCK:
                stats_.task_clock_ns_total += ev.delta;
                break;
            default:
                break;
        }
    }

    void appendEvent([[maybe_unused]] const KernelEvent& ev) {
        events_.push_back([[maybe_unused]] ev);
        while ([[maybe_unused]] events_.size() > config_.max_events_retained) {
            events_.pop_front();
        }
    }

    void publishMetrics() {
        // Called with mu_ held
        auto& mc = MetricsCollector::getInstance();
        if (stats_.context_switches_total > 0) {
            mc.setGauge("themis_ebpf_context_switches_total",
                        static_cast<double>(stats_.context_switches_total));
        }
        if (stats_.page_faults_total > 0) {
            mc.setGauge("themis_ebpf_page_faults_total",
                        static_cast<double>(stats_.page_faults_total));
        }
        if (stats_.cpu_migrations_total > 0) {
            mc.setGauge("themis_ebpf_cpu_migrations_total",
                        static_cast<double>(stats_.cpu_migrations_total));
        }
        if (stats_.task_clock_ns_total > 0) {
            mc.setGauge("themis_ebpf_task_clock_ns_total",
                        static_cast<double>(stats_.task_clock_ns_total));
        }
        mc.setGauge("themis_ebpf_collection_cycles_total",
                    static_cast<double>(stats_.collection_cycles));
    }

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::thread thread_;
    bool running_{false};

#if THEMIS_EBPF_LINUX
    /// Set by reset() to signal the background thread to re-baseline prev_*.
    /// Only cleared by the background thread — no data race with prev_*.
    std::atomic<bool> pending_reset_{false};
#endif

    EbpfTracerConfig config_;
    EbpfTracerStats stats_;
    std::deque<KernelEvent> events_;
    std::function<void(const std::vector<KernelEvent>&)> callback_;
};

// ============================================================================
// EbpfTracer – public API (thin delegation to Impl)
// ============================================================================

EbpfTracer::EbpfTracer(const EbpfTracerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

EbpfTracer::~EbpfTracer() = default;

void EbpfTracer::start()  { impl_->start();  }
void EbpfTracer::stop()   { impl_->stop();   }
bool EbpfTracer::isEnabled() const { return impl_->isEnabled(); }
void EbpfTracer::enable()  { impl_->enable();  }
void EbpfTracer::disable() { impl_->disable(); }

EbpfTracerStats EbpfTracer::getStats() const { return impl_->getStats(); }

std::vector<KernelEvent> EbpfTracer::getRecentEvents() const {
    return impl_->getRecentEvents();
}

void EbpfTracer::registerEventCallback(
        std::function<void([[maybe_unused]] const std::vector<KernelEvent>&)> cb) {
    impl_->registerEventCallback([[maybe_unused]] std::move(cb));
}

void EbpfTracer::reset() { impl_->reset(); }

EbpfTracerConfig EbpfTracer::getConfig() const { return impl_->getConfig(); }

bool EbpfTracer::isPlatformSupported() noexcept {
    return Impl::isPlatformSupported();
}

} // namespace observability
} // namespace themis

