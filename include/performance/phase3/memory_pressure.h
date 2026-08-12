/**
 * @file memory_pressure.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Memory Pressure Monitoring with Automatic Cache Eviction
// Performance Module - Phase 3
//
// Monitors system RAM pressure (via OS facilities) and automatically
// triggers registered cache eviction callbacks when thresholds are exceeded.
//
// Design:
//  - Polls OS memory stats at a configurable interval (default: 500ms)
//  - Classifies pressure into four levels: NORMAL, MODERATE, HIGH, CRITICAL
//  - Calls registered eviction callbacks in registration order when pressure >= threshold
//  - Thread-safe: callbacks registered/unregistered while monitor is running
//
// Platform support:
//  - Linux:   /proc/meminfo
//  - macOS:   sysctl (vm.page_free_count / vm.page_active_count)
//  - Windows: GlobalMemoryStatusEx
//  - Fallback: conservative estimate based on configured total

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace performance {
namespace phase3 {

/**
 * @brief System memory pressure monitor with automatic cache eviction.
 *
 * Registers eviction callbacks that are invoked automatically when system
 * memory usage exceeds configurable thresholds.  A background polling
 * thread samples OS-level memory statistics and classifies pressure into
 * four levels.
 *
 * Typical usage:
 * @code
 *   SystemMemoryPressureMonitor monitor;
 *   monitor.register_eviction_callback(PressureLevel::HIGH, [&cache]() {
 *       cache.evict_fraction(0.25);  // free 25% of cache under high pressure
 *   });
 *   monitor.start();
 *   // ... run workload ...
 *   monitor.stop();
 * @endcode
 */
class SystemMemoryPressureMonitor {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Classified memory pressure levels.
     *
     * Levels are ordered: NORMAL < MODERATE < HIGH < CRITICAL.
     * Default thresholds (percentage of total RAM used):
     *   NORMAL   < 70 %
     *   MODERATE  70 – 84 %
     *   HIGH      85 – 94 %
     *   CRITICAL >= 95 %
     */
    enum class PressureLevel {
        NORMAL,    ///< < 70% system RAM used
        MODERATE,  ///< 70-84% system RAM used
        HIGH,      ///< 85-94% system RAM used
        CRITICAL   ///< >= 95% system RAM used
    };

    /**
     * @brief Snapshot of system memory state at a single point in time.
     */
    struct MemorySnapshot {
        size_t total_bytes{0};      ///< Physical RAM installed
        size_t available_bytes{0};  ///< Available to new allocations
        size_t used_bytes{0};       ///< Bytes in use (total - available)
        double usage_percent{0.0};  ///< [0, 100]
        PressureLevel level{PressureLevel::NORMAL};
        std::chrono::steady_clock::time_point sampled_at;
    };

    /**
     * @brief Thresholds (%) separating the four pressure levels.
     *
     * usage_percent < moderate_threshold  → NORMAL
     * usage_percent < high_threshold      → MODERATE
     * usage_percent < critical_threshold  → HIGH
     * usage_percent >= critical_threshold → CRITICAL
     */
    struct PressureThresholds {
        double moderate_threshold{70.0};
        double high_threshold{85.0};
        double critical_threshold{95.0};
        PressureThresholds() noexcept = default;
    };

    /**
     * @brief Configuration passed to the constructor.
     */
    struct Config {
        /// How often the background thread samples OS memory stats.
        std::chrono::milliseconds poll_interval{500};
        /// Pressure thresholds (percentages of total RAM used).
        PressureThresholds thresholds;
        /// Override total memory (bytes). 0 = auto-detect via OS.
        size_t total_memory_override_bytes{0};
        Config() noexcept = default;
    };

    /**
     * @brief Eviction callback signature.
     *
     * Called on the monitor's internal polling thread; must be fast
     * (non-blocking) to avoid delaying subsequent samples.
     */
    using EvictionCallback = std::function<void()>;

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    SystemMemoryPressureMonitor();
    explicit SystemMemoryPressureMonitor(Config config);
    ~SystemMemoryPressureMonitor();

    // Non-copyable, non-movable (background thread owns *this pointer).
    SystemMemoryPressureMonitor(const SystemMemoryPressureMonitor&) = delete;
    SystemMemoryPressureMonitor& operator=(const SystemMemoryPressureMonitor&) = delete;

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Start background polling thread.
     *
     * Safe to call only once; subsequent calls are no-ops.
     */
    void start();

    /**
     * @brief Stop background polling thread and join it.
     *
     * Blocks until the polling thread exits.  Safe to call even if start()
     * was never called.
     */
    void stop();

    /**
     * @brief Returns true if the polling thread is currently running.
     */
    bool is_running() const noexcept;

    // -----------------------------------------------------------------------
    // Callback registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register an eviction callback for a minimum pressure level.
     *
     * The callback is invoked every polling cycle in which
     * current_level >= trigger_level.
     *
     * @param trigger_level  Minimum pressure level that activates the callback.
     * @param callback       Function to call; must be thread-safe.
     * @return Handle that can be passed to unregister_eviction_callback().
     */
    size_t register_eviction_callback(PressureLevel trigger_level,
                                      EvictionCallback callback);

    /**
     * @brief Unregister a previously registered callback.
     *
     * @param handle Handle returned by register_eviction_callback().
     */
    void unregister_eviction_callback(size_t handle);

    // -----------------------------------------------------------------------
    // Observation
    // -----------------------------------------------------------------------

    /**
     * @brief Take an immediate OS memory snapshot (may be called from any thread).
     */
    MemorySnapshot sample() const;

    /**
     * @brief Return the most recent snapshot collected by the polling thread.
     *
     * Valid only after start() has been called and at least one poll cycle
     * has elapsed; returns a zero-initialised snapshot otherwise.
     */
    MemorySnapshot last_snapshot() const;

    /**
     * @brief Cumulative number of times eviction callbacks were invoked.
     */
    uint64_t eviction_trigger_count() const noexcept;

    /**
     * @brief Human-readable description of a pressure level.
     */
    static std::string level_name(PressureLevel level);

private:
    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /// Read OS memory statistics and return a fresh snapshot.
    MemorySnapshot read_os_memory() const;

    /// Classify usage_percent into a PressureLevel using current thresholds.
    PressureLevel classify(double usage_percent) const noexcept;

    /// Main loop executed by the background polling thread.
    void poll_loop();

    /// Invoke all callbacks whose trigger_level <= current snapshot level.
    void trigger_callbacks(PressureLevel current_level);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    Config config_;

    struct CallbackEntry {
        size_t handle;
        PressureLevel trigger_level;
        EvictionCallback callback;
    };

    mutable std::mutex callbacks_mutex_;
    std::vector<CallbackEntry> callbacks_;
    size_t next_handle_{1};

    mutable std::mutex snapshot_mutex_;
    MemorySnapshot last_snapshot_;

    std::atomic<uint64_t> eviction_trigger_count_{0};
    std::atomic<bool> running_{false};
    std::thread poll_thread_;
};

} // namespace phase3
} // namespace performance
} // namespace themis
