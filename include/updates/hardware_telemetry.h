/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hardware_telemetry.h                               ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-14                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file hardware_telemetry.h
 * @brief Anonymous hardware telemetry reporter for the Updates module.
 *
 * Collects anonymised hardware facts about the host system and periodically
 * sends them as a JSON POST to a configurable HTTP endpoint (default:
 * https://telemetry.themisdb.io/v1/hardware).  The feature is **opt-in** and
 * disabled by default; it is controlled via the `updates.telemetry.enabled`
 * key in the YAML configuration file.
 *
 * ## Privacy guarantees
 *  - No hostname, IP address, username, or database content is ever collected.
 *  - A random, ephemeral instance UUID is generated at construction time and
 *    is not persisted to disk.
 *  - Only coarse hardware facts (CPU model/cores, total RAM bucket, OS family,
 *    CPU architecture) are included together with the ThemisDB version string.
 *
 * ## Design
 *  - `HardwareSnapshot`          – POD holding one telemetry sample.
 *  - `IHardwareInfoProvider`     – injectable interface for OS/hardware facts
 *                                   (enables unit tests without real /proc).
 *  - `IHttpSender`               – injectable HTTP POST interface
 *                                   (same pattern as NotificationWebhook).
 *  - `HardwareTelemetryReporter` – orchestrator; collect + serialize + send +
 *                                   background scheduling.
 *
 * ## Usage
 * @code
 *   TelemetryConfig cfg;
 *   cfg.enabled = true;
 *   cfg.endpoint_url = "https://telemetry.example.com/v1/hardware";
 *   cfg.send_interval_seconds = 3600;
 *
 *   HardwareTelemetryReporter reporter(cfg);
 *   reporter.startBackgroundReporting();
 *   // … later …
 *   reporter.stopBackgroundReporting();
 * @endcode
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// TelemetryConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for the anonymous hardware telemetry reporter.
 *
 * All fields carry safe defaults.  The feature is **disabled** by default.
 */
struct TelemetryConfig {
    /// Master on/off switch – must be explicitly set to `true` to enable.
    bool enabled = false;

    /// HTTP(S) endpoint receiving the JSON telemetry payload.
    std::string endpoint_url = "https://telemetry.themisdb.io/v1/hardware";

    /// How often to send a report (seconds).  Default: 1 h.
    int send_interval_seconds = 3600;

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

    /// HTTP timeout per send attempt (seconds).
    int http_timeout_seconds = 10;

    /// Maximum number of consecutive send failures before giving up until the
    /// next scheduled interval.
    int max_retries = 2;
};

// ---------------------------------------------------------------------------
// HardwareSnapshot
// ---------------------------------------------------------------------------

/**
 * @brief A single anonymised snapshot of host hardware facts.
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
 * @brief Collects and reports anonymous hardware telemetry.
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
     * @brief Collect a hardware snapshot from the host system.
     *
     * Only includes fields that are enabled in the current TelemetryConfig.
     * Always fills `instance_id`, `themis_version`, and `timestamp_utc`.
     *
     * @return Populated HardwareSnapshot.
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

    TelemetryConfig                        config_;
    std::shared_ptr<IHardwareInfoProvider> hw_provider_;
    TelemetryHttpSendFunc                  http_sender_;
    std::string                            instance_id_;

    std::thread           bg_thread_;
    std::atomic<bool>     running_{false};
    std::atomic<bool>     stop_requested_{false};
};

} // namespace updates
} // namespace themis
