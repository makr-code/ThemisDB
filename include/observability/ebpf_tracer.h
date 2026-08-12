/**
 * @file ebpf_tracer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace observability {

/**
 * @brief Kernel event types collected by the eBPF tracer.
 *
 * On Linux these map directly to @c PERF_COUNT_SW_* software-event IDs
 * (and to BPF tracepoint attach points when @c THEMIS_ENABLE_EBPF is
 * defined).  On other platforms only @c NONE events are returned.
 */
enum class EbpfProbeType {
    CONTEXT_SWITCH, ///< Voluntary / involuntary context switches
    PAGE_FAULT,     ///< Minor + major page faults
    CPU_MIGRATION,  ///< Thread migrated to a different CPU
    TASK_CLOCK,     ///< CPU time consumed by the process (nanoseconds)
    NONE            ///< Placeholder / unsupported event
};

/**
 * @brief A single kernel-level event sample collected from the process.
 */
struct KernelEvent {
    EbpfProbeType type{EbpfProbeType::NONE};
    std::chrono::system_clock::time_point timestamp;
    /// Cumulative counter delta since the last collection interval
    int64_t delta{0};
    /// Human-readable description of the event source
    std::string description;
};

/**
 * @brief Statistics snapshot exported by EbpfTracer::getStats().
 */
struct EbpfTracerStats {
    int64_t context_switches_total{0}; ///< Cumulative context switches observed
    int64_t page_faults_total{0};      ///< Cumulative page faults observed
    int64_t cpu_migrations_total{0};   ///< Cumulative CPU migrations observed
    int64_t task_clock_ns_total{0};    ///< Cumulative CPU time in nanoseconds
    uint64_t collection_cycles{0};     ///< Number of completed collection cycles
};

/**
 * @brief Configuration for the eBPF kernel-level tracer.
 *
 * ### Platform behaviour
 *
 * | Condition                                          | Behaviour                                        |
 * |----------------------------------------------------|--------------------------------------------------|
 * | Linux + `THEMIS_ENABLE_EBPF` defined               | Full BPF program attach via libbpf (if present)  |
 * | Linux only (no `THEMIS_ENABLE_EBPF`)               | `perf_event_open(2)` software counters           |
 * | Non-Linux                                          | No-op; all counters stay at zero                 |
 *
 * ### Overhead
 * Expected CPU overhead is < 0.1 % per enabled probe type at the default
 * @c collection_interval of 1 second.
 */
struct EbpfTracerConfig {
    /// Whether the tracer is active on construction.
    bool enabled = false;

    /// How often the background thread reads perf counters.
    std::chrono::milliseconds collection_interval{1000};

    /// Maximum number of @c KernelEvent entries kept in the ring buffer.
    /// Older events are evicted when the limit is reached.
    size_t max_events_retained = 3600; // 1 h at 1-second intervals

    /// Enable context-switch tracking.
    bool probe_context_switches = true;
    /// Enable page-fault tracking.
    bool probe_page_faults = true;
    /// Enable CPU-migration tracking.
    bool probe_cpu_migrations = true;
    /// Enable task-clock (CPU time) tracking.
    bool probe_task_clock = true;
};

/**
 * @brief eBPF-based, low-overhead kernel-level tracer for ThemisDB.
 *
 * ### Overview
 * On Linux this class opens one `perf_event_open(2)` file descriptor per
 * enabled probe type and polls the counters from a background thread at the
 * configured @c collection_interval.  Delta values (change since the last
 * poll) are accumulated in a ring buffer of @c KernelEvent objects and
 * exported as Prometheus-style counters via @c MetricsCollector:
 *
 * | Prometheus metric name                  | Unit    |
 * |-----------------------------------------|---------|
 * | `themis_ebpf_context_switches_total`    | events  |
 * | `themis_ebpf_page_faults_total`         | events  |
 * | `themis_ebpf_cpu_migrations_total`      | events  |
 * | `themis_ebpf_task_clock_ns_total`       | ns      |
 * | `themis_ebpf_collection_cycles_total`   | cycles  |
 *
 * When @c THEMIS_ENABLE_EBPF is defined and @c libbpf is available at link
 * time, additional BPF programs can be attached (reserved for future
 * extension; the current implementation uses perf_event_open only).
 *
 * ### Thread safety
 * All public methods are thread-safe.
 *
 * ### Usage
 * ```cpp
 * EbpfTracerConfig cfg;
 * cfg.enabled              = true;
 * cfg.collection_interval  = std::chrono::milliseconds(500);
 *
 * EbpfTracer tracer(cfg);
 * tracer.start();
 *
 * // ... workload ...
 *
 * auto stats = tracer.getStats();
 * tracer.stop();
 * ```
 */
class EbpfTracer {
public:
    explicit EbpfTracer(const EbpfTracerConfig& config = EbpfTracerConfig{});
    ~EbpfTracer();

    // Non-copyable, non-movable
    EbpfTracer(const EbpfTracer&) = delete;
    EbpfTracer& operator=(const EbpfTracer&) = delete;

    /**
     * @brief Start the background collection loop.
     * No-op if already running.
     */
    void start();

    /**
     * @brief Stop the background collection loop.
     * Blocks until the background thread has exited.
     */
    void stop();

    /**
     * @brief Returns true when the tracer is actively collecting events.
     */
    bool isEnabled() const;

    /**
     * @brief Dynamically enable or disable tracing without restart.
     *
     * Calling @c enable() on a stopped-but-configured tracer restarts the
     * background thread.  Calling @c disable() stops the thread and keeps
     * the collected data intact.
     */
    void enable();
    void disable();

    /**
     * @brief Return the cumulative statistics since the last @c reset() or
     *        construction.
     */
    EbpfTracerStats getStats() const;

    /**
     * @brief Return a snapshot of the recent event ring buffer.
     *
     * Events are ordered oldest-first.  The buffer size is bounded by
     * @c EbpfTracerConfig::max_events_retained.
     */
    std::vector<KernelEvent> getRecentEvents() const;

    /**
     * @brief Register a callback invoked when at least one kernel event was
     *        collected in a cycle.
     *
     * The callback receives the @c KernelEvent objects collected during that
     * cycle.  It is called from the background thread; implementations must
     * be thread-safe.  The callback is NOT invoked on cycles where no new
     * events were observed (e.g. zero deltas or non-Linux platforms).
     */
    void registerEventCallback(
        std::function<void(const std::vector<KernelEvent>&)> cb);

    /**
     * @brief Reset all accumulated counters and clear the event ring buffer.
     */
    void reset();

    /**
     * @brief Retrieve the active configuration.
     */
    EbpfTracerConfig getConfig() const;

    /**
     * @brief Returns true if the current platform supports perf_event_open(2)
     *        software counters.
     *
     * On non-Linux platforms this always returns false and the tracer
     * operates in no-op mode.
     */
    static bool isPlatformSupported() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
