/**
 * @file config_file_watcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

namespace themis {
namespace config {

/**
 * ConfigFileWatcher watches a directory tree for changes to `.yaml` and
 * `.json` files and invokes a user-supplied callback after a 200 ms debounce
 * settling window.
 *
 * Platform support:
 *   - Linux  : inotify (recursive watch via inotify_add_watch per sub-dir)
 *   - macOS  : kqueue  (kevent-based file-descriptor watch)
 *   - Windows: ReadDirectoryChangesW
 *
 * Usage:
 * @code
 *   ConfigFileWatcher watcher("/etc/myapp/config",
 *       []() { ConfigPathResolver::clearCache(); },
 *       std::chrono::milliseconds(200));
 *   watcher.start();
 *   // ... later ...
 *   watcher.stop();
 * @endcode
 *
 * Thread-safety:
 *   start() and stop() are thread-safe and idempotent. The callback is
 *   invoked from the watcher's internal background thread; callers must
 *   ensure the callback is itself thread-safe.
 */
class ConfigFileWatcher {
public:
    /**
     * Construct a file watcher.
     *
     * @param watch_path   Directory to watch (recursively).
     * @param callback     Invoked after the debounce window expires.
     * @param debounce     Settling window before the callback fires.
     *                     Defaults to 200 ms.
     */
    explicit ConfigFileWatcher(
        std::string watch_path,
        std::function<void()> callback,
        std::chrono::milliseconds debounce = std::chrono::milliseconds(200));

    ~ConfigFileWatcher();

    // Non-copyable, non-movable
    ConfigFileWatcher(const ConfigFileWatcher&) = delete;
    ConfigFileWatcher& operator=(const ConfigFileWatcher&) = delete;
    ConfigFileWatcher(ConfigFileWatcher&&) = delete;
    ConfigFileWatcher& operator=(ConfigFileWatcher&&) = delete;

    /**
     * Start the background watcher thread.
     * Idempotent: calling start() on an already-running watcher is a no-op.
     *
     * @return true if the watcher was started successfully; false if the
     *         platform does not support file watching or the watch_path is
     *         not accessible.
     */
    bool start();

    /**
     * Stop the background watcher thread and release OS resources.
     * Blocks until the thread has exited. Idempotent.
     */
    void stop();

    /**
     * @return true if the watcher thread is currently running.
     */
    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    /**
     * Return the path being watched.
     */
    const std::string& watchPath() const { return watch_path_; }

    /**
     * Return the configured debounce interval.
     */
    std::chrono::milliseconds debounceInterval() const { return debounce_; }

private:
    // ── Entry point for the watcher thread ──────────────────────────────
    void watchLoop();

    // ── Platform-specific watch loops ───────────────────────────────────
#if defined(__linux__)
    void watchLoopInotify();
#elif defined(__APPLE__)
    void watchLoopKqueue();
#elif defined(_WIN32)
    void watchLoopReadDirChanges();
#endif

    // ── Debounce helper ─────────────────────────────────────────────────
    /**
     * Called from the watch loop whenever a relevant FS event arrives.
     * Resets the debounce timer; the callback fires after `debounce_` ms of
     * inactivity.
     */
    void scheduleCallback();

    // ── Members ─────────────────────────────────────────────────────────
    std::string                watch_path_;
    std::function<void()>      callback_;
    std::chrono::milliseconds  debounce_;

    std::atomic<bool>          running_{false};
    std::thread                thread_;

    // Debounce state
    std::mutex                             debounce_mutex_;
    std::chrono::steady_clock::time_point  last_event_time_;
    bool                                   event_pending_{false};

    // Platform-specific stop signalling
#if defined(__linux__)
    int pipe_read_fd_{-1};
    int pipe_write_fd_{-1};
#elif defined(__APPLE__)
    int kqueue_fd_{-1};
    int pipe_read_fd_{-1};
    int pipe_write_fd_{-1};
#elif defined(_WIN32)
    void* stop_event_{nullptr};   // HANDLE
#endif
};

} // namespace config
} // namespace themis
