/**
 * @file hardware_telemetry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.5
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// TelemetryConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for the anonymous hardware + performance telemetry.
 *
 * All fields carry safe defaults.  The feature is **disabled** by default.
 */
struct TelemetryConfig {
    /// Master on/off switch – must be explicitly set to `true` to enable.
    bool enabled = false;

    /// HTTP(S) endpoint receiving the JSON telemetry payload.
    std::string endpoint_url = "https://api.themisdb.org/telemetry.php";

    /// How often to send a report (seconds).  Minimum enforced: 86400 (24 h).
    int send_interval_seconds = 86400;

    // ── Hardware field switches ───────────────────────────────────────────

    /// Include CPU model string (e.g. "Intel Core i7-12700K").
    bool include_cpu_model = true;

    /// Include logical CPU core count.
    bool include_cpu_cores = true;

    /// Include total RAM in megabytes (rounded down to nearest 1024 MiB bucket).
    bool include_ram_mb = true;

    /// Include OS family string (e.g. "Linux", "Windows", "macOS").
    bool include_os = true;

    /// Include CPU architecture string (e.g. "x86_64", "aarch64").
    bool include_arch = true;

    // ── Performance field switches ────────────────────────────────────────

    /// Include anonymous runtime performance metrics (query latency, QPS,
    /// cache hit rate, process memory, uptime, connections).
    /// Requires a IPerformanceMetricsProvider to be wired via
    /// HardwareTelemetryReporter::setPerformanceProvider().
    /// All values are coarse/bucketed – no query content is ever included.
    bool include_performance = false;

    // ── Transport settings ────────────────────────────────────────────────

    /// HTTP timeout per send attempt (seconds).
    int http_timeout_seconds = 10;

    /// Maximum number of consecutive send failures before giving up until the
    /// next scheduled interval.
    int max_retries = 2;
};

// ---------------------------------------------------------------------------
// PerformanceSnapshot
// ---------------------------------------------------------------------------

/**
 * @brief Anonymous runtime performance metrics for one reporting interval.
 *
 * All values are coarse or bucketed to prevent workload fingerprinting.
 * No query text, table names, or user data is ever included.
 */
struct PerformanceSnapshot {
    /// Average query execution latency across the last interval (microseconds).
    /// 0 if no queries were executed or metrics are unavailable.
    uint64_t avg_query_latency_us = 0;

    /// P99 query execution latency across the last interval (microseconds).
    uint64_t p99_query_latency_us = 0;

    /// Approximate queries per second over the last interval, bucketed to the
    /// nearest power of 2 (1, 2, 4, 8, …).  0 if unavailable.
    uint32_t queries_per_second_bucket = 0;

    /// Query cache hit rate as an integer percentage 0–100.
    /// 255 = unavailable / not applicable.
    uint8_t cache_hit_rate_pct = 255;

    /// Process resident-set size in MiB, rounded down to the nearest 64 MiB
    /// bucket.  0 if unavailable.
    uint32_t process_rss_mb_bucket = 0;

    /// Process uptime in seconds since the ThemisDB server started.
    uint64_t uptime_seconds = 0;

    /// Number of currently active client connections, bucketed to the nearest
    /// power of 2.  0 if unavailable.
    uint32_t active_connections_bucket = 0;

    /// Total on-disk database size in MiB, rounded down to the nearest
    /// 512 MiB bucket.  0 if unavailable.
    uint32_t db_size_mb_bucket = 0;
};

// ---------------------------------------------------------------------------
// HardwareSnapshot
// ---------------------------------------------------------------------------

/**
 * @brief A single anonymised telemetry snapshot (hardware + optional perf).
 *
 * Fields that could not be determined are left as their default empty/zero
 * values; the receiver is expected to treat them as "unknown".
 */
struct HardwareSnapshot {
    /// Random UUID identifying this ThemisDB instance for the current run.
    /// Not persisted to disk – changes on every restart.
    std::string instance_id;

    /// ThemisDB version string (e.g. "2.0.0").
    std::string themis_version;

    /// CPU model name string (may be empty if not determinable).
    std::string cpu_model;

    /// Number of logical CPU cores (0 if not determinable).
    unsigned int cpu_cores = 0;

    /// Total RAM in megabytes, rounded down to the nearest 1 024 MiB bucket
    /// (0 if not determinable).
    uint64_t total_ram_mb = 0;

    /// OS family: "Linux", "Windows", "macOS", or "Unknown".
    std::string os_family;

    /// CPU architecture: "x86_64", "aarch64", "arm", etc.
    std::string cpu_arch;

    /// UTC timestamp (seconds since epoch) when the snapshot was taken.
    int64_t timestamp_utc = 0;

    /// Optional performance metrics (present only when include_performance is
    /// true and a IPerformanceMetricsProvider has been wired).
    std::optional<PerformanceSnapshot> performance;

    // ── Build provenance fields ───────────────────────────────────────────

    /// Build channel: "official" (signed CI build) or "community" (self-compiled).
    /// Sourced from the compile-time constant THEMIS_BUILD_CHANNEL.
    std::string build_channel;

    /// Short Git SHA of the source tree at build time (e.g. "a1b2c3d").
    /// Sourced from THEMIS_BUILD_ID.
    std::string build_id;

    /// True when the Ed25519 build signature embedded at compile-time has been
    /// verified successfully against the hard-coded public key.
    /// Always false for community builds.
    bool build_verified = false;

    /**
     * @brief Serialize to a compact JSON string.
     * @return JSON object string suitable for an HTTP POST body.
     */
    [[nodiscard]] std::string toJson() const;
};

// ---------------------------------------------------------------------------
// IHardwareInfoProvider – injectable for testability
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for querying host hardware facts.
 *
 * The default production implementation reads /proc/cpuinfo, /proc/meminfo,
 * and uname(2) on POSIX systems, and the Win32 API on Windows.  Tests inject
 * a mock that returns canned values without touching the real OS.
 */
class IHardwareInfoProvider {
public:
    virtual ~IHardwareInfoProvider() = default;

    /// @return CPU model name (e.g. "Intel Core i7-12700K"), empty if unknown.
    [[nodiscard]] virtual std::string cpuModel() const = 0;

    /// @return Logical CPU core count, 0 if unknown.
    [[nodiscard]] virtual unsigned int cpuCores() const = 0;

    /// @return Total RAM in megabytes, 0 if unknown.
    [[nodiscard]] virtual uint64_t totalRamMb() const = 0;

    /// @return OS family string ("Linux", "Windows", "macOS", "Unknown").
    [[nodiscard]] virtual std::string osFamily() const = 0;

    /// @return CPU architecture string ("x86_64", "aarch64", …).
    [[nodiscard]] virtual std::string cpuArch() const = 0;
};

/**
 * @brief Production implementation of IHardwareInfoProvider.
 *
 * Reads /proc/cpuinfo + /proc/meminfo on Linux, uname(2) for OS/arch, and
 * the Win32 API on Windows.
 */
class SystemHardwareInfoProvider final : public IHardwareInfoProvider {
public:
    [[nodiscard]] std::string cpuModel() const override;
    [[nodiscard]] unsigned int cpuCores() const override;
    [[nodiscard]] uint64_t totalRamMb() const override;
    [[nodiscard]] std::string osFamily() const override;
    [[nodiscard]] std::string cpuArch() const override;
};

// ---------------------------------------------------------------------------
// IPerformanceMetricsProvider – injectable for testability
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for collecting anonymous runtime performance facts.
 *
 * Implement this interface and wire it via
 * `HardwareTelemetryReporter::setPerformanceProvider()` to include performance
 * data in telemetry reports.  All returned values must be coarse/bucketed –
 * the interface contract forbids returning query text, table names, or any
 * user-identifiable data.
 */
class IPerformanceMetricsProvider {
public:
    virtual ~IPerformanceMetricsProvider() = default;

    /**
     * @brief Collect and return a fresh PerformanceSnapshot.
     *
     * This method must be non-blocking and complete in < 5 ms.  The reporter
     * calls it from the background telemetry thread.
     *
     * @return Populated PerformanceSnapshot.
     */
    [[nodiscard]] virtual PerformanceSnapshot collect() const = 0;
};

// ---------------------------------------------------------------------------
// IHttpSender – injectable for testability  (same pattern as NotificationWebhook)
// ---------------------------------------------------------------------------

/**
 * @brief Callable type for sending an HTTP POST request.
 *
 * Signature: `bool(url, body, content_type, timeout_seconds)`
 *  - Returns `true`  on HTTP 2xx.
 *  - Returns `false` on network error or non-2xx response.
 */
using TelemetryHttpSendFunc =
    std::function<bool(const std::string& /*url*/,
                       const std::string& /*body*/,
                       const std::string& /*content_type*/,
                       int               /*timeout_seconds*/)>;

// ---------------------------------------------------------------------------
// HardwareTelemetryReporter
// ---------------------------------------------------------------------------

/**
 * @brief Collects and reports anonymous hardware + performance telemetry.
 *
 * Thread-safe.  The background thread is started via
 * `startBackgroundReporting()` and must be stopped via
 * `stopBackgroundReporting()` (or the destructor) before the process exits.
 */
class HardwareTelemetryReporter {
public:
    /**
     * @brief Construct with configuration and optional injectable dependencies.
     *
     * @param config         Telemetry configuration.
     * @param hw_provider    Hardware info provider (nullptr → production impl).
     * @param http_sender    HTTP POST function (nullptr → libcurl impl).
     */
    explicit HardwareTelemetryReporter(
        TelemetryConfig config,
        std::shared_ptr<IHardwareInfoProvider> hw_provider = nullptr,
        TelemetryHttpSendFunc http_sender = nullptr);

    ~HardwareTelemetryReporter();

    // Non-copyable, non-movable (owns a background thread).
    HardwareTelemetryReporter(const HardwareTelemetryReporter&) = delete;
    HardwareTelemetryReporter& operator=(const HardwareTelemetryReporter&) = delete;

    /**
     * @brief Wire a performance metrics provider.
     *
     * Must be called before `startBackgroundReporting()`.  Safe to call with
     * `nullptr` to detach a previously wired provider.
     *
     * @param provider  Implementation of IPerformanceMetricsProvider.
     */
    void setPerformanceProvider(
        std::shared_ptr<IPerformanceMetricsProvider> provider);

    /**
     * @brief Collect a telemetry snapshot from the host system.
     *
     * Includes hardware fields controlled by TelemetryConfig flags.  Appends
     * a PerformanceSnapshot when `include_performance` is true and a provider
     * has been wired.
     *
     * @return Populated HardwareSnapshot (with optional performance data).
     */
    [[nodiscard]] HardwareSnapshot collect() const;

    /**
     * @brief Serialize `snapshot` and POST it to the configured endpoint.
     *
     * @param snapshot  The snapshot to send.
     * @return `true` if the remote endpoint accepted the payload (HTTP 2xx).
     */
    bool send(const HardwareSnapshot& snapshot) const;

    /**
     * @brief Perform one full collect-and-send cycle.
     *
     * Equivalent to `send(collect())`.  Returns `false` if telemetry is
     * disabled or if the HTTP POST failed.
     */
    bool report();

    /**
     * @brief Start the background reporting thread.
     *
     * Has no effect if telemetry is disabled or if the thread is already
     * running.
     */
    void startBackgroundReporting();

    /**
     * @brief Stop the background reporting thread.
     *
     * Blocks until the thread has exited.  Safe to call multiple times.
     */
    void stopBackgroundReporting();

    /// @return `true` if the background thread is currently running.
    [[nodiscard]] bool isRunning() const noexcept;

    /// @return A copy of the current configuration.
    [[nodiscard]] TelemetryConfig config() const noexcept;

    /// @return The random instance UUID assigned at construction time.
    [[nodiscard]] const std::string& instanceId() const noexcept;

private:
    /// Run-loop executed by the background thread.
    void runLoop();

    /// Generate a random UUID v4 string.
    static std::string generateUuid();

    TelemetryConfig                              config_;
    std::shared_ptr<IHardwareInfoProvider>       hw_provider_;
    
    // CRITICAL: Thread synchronization for perf_provider_ (data_race fix)
    mutable std::mutex                           perf_provider_mutex_;
    std::shared_ptr<IPerformanceMetricsProvider> perf_provider_;
    
    TelemetryHttpSendFunc                        http_sender_;
    std::string                                  instance_id_;

    std::thread           bg_thread_;
    std::atomic<bool>     running_{false};
    std::atomic<bool>     stop_requested_{false};
};

} // namespace updates
} // namespace themis
