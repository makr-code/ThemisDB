/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_file_watcher.h                              ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-04-15 18:44:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace themis {
namespace governance {

class PolicyEngine;

/**
 * @brief Background file-watcher for automatic governance policy hot-reload.
 *
 * Monitors the YAML policy file that was last loaded into a `PolicyEngine` and
 * calls `PolicyEngine::reloadIfChanged()` whenever the file's modification time
 * changes, enabling zero-downtime policy updates without restarting the server.
 *
 * The watcher runs a background thread that polls at `poll_interval` intervals.
 * A reload is triggered only after the file has been stable (no further changes
 * detected) for at least `debounce` milliseconds, reducing the risk of reading a
 * partially-written file.
 *
 * Thread-safety: `start()` and `stop()` may be called from any thread.
 * The watcher does not outlive the `PolicyEngine` reference; callers must stop
 * the watcher before destroying the engine.
 */
class PolicyFileWatcher {
public:
    struct Config {
        /// How often the watcher polls the file system (default: 200 ms).
        std::chrono::milliseconds poll_interval{200};
        /// Minimum quiet period after a detected change before triggering a
        /// reload (default: 500 ms, per FUTURE_ENHANCEMENTS.md spec).
        std::chrono::milliseconds debounce{500};
        /// Optional callback invoked after each reload attempt.
        /// Signature: reload_cb(success, error_message)
        std::function<void(bool, const std::string&)> reload_cb;
    };

    /**
     * @brief Construct a watcher bound to the given PolicyEngine.
     *
     * @param engine  Governance PolicyEngine to reload on file change.
     *                Must outlive this watcher.
     * @param config  Watcher configuration.
     */
    explicit PolicyFileWatcher(PolicyEngine& engine);
    explicit PolicyFileWatcher(PolicyEngine& engine, Config config);

    ~PolicyFileWatcher();

    // Non-copyable, non-movable (holds a reference to the engine)
    PolicyFileWatcher(const PolicyFileWatcher&) = delete;
    PolicyFileWatcher& operator=(const PolicyFileWatcher&) = delete;

    /**
     * @brief Start the background monitoring thread.
     * @return true  if started (or already running).
     */
    bool start();

    /**
     * @brief Stop the monitoring thread and join it synchronously.
     */
    void stop();

    /// @return true if the background thread is currently running.
    bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    /// @return Number of successful policy reloads since the last start().
    uint64_t reloadSuccessCount() const noexcept {
        return reload_success_count_.load(std::memory_order_relaxed);
    }

    /// @return Number of failed reload attempts since the last start().
    uint64_t reloadFailureCount() const noexcept {
        return reload_failure_count_.load(std::memory_order_relaxed);
    }

private:
    void run();

    PolicyEngine& engine_;
    Config config_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> reload_success_count_{0};
    std::atomic<uint64_t> reload_failure_count_{0};
};

} // namespace governance
} // namespace themis
