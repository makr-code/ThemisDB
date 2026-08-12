/**
 * @file policy_file_watcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_file_watcher.h"
#include "governance/policy_engine.h"
#include "utils/logger.h"

#include <filesystem>
#include <thread>
#include <chrono>

namespace themis {
namespace governance {

PolicyFileWatcher::PolicyFileWatcher(PolicyEngine& engine)
    : PolicyFileWatcher(engine, Config{})
{}

PolicyFileWatcher::PolicyFileWatcher(PolicyEngine& engine, Config config)
    : engine_(engine)
    , config_(std::move(config))
{
}

PolicyFileWatcher::~PolicyFileWatcher() {
    stop();
}

bool PolicyFileWatcher::start() {
    if (running_.load(std::memory_order_acquire)) {
        return true;  // Already running
    }
    // Reset counters so the new run starts from zero
    reload_success_count_.store(0, std::memory_order_relaxed);
    reload_failure_count_.store(0, std::memory_order_relaxed);

    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&PolicyFileWatcher::run, this);
    THEMIS_INFO("PolicyFileWatcher: started monitoring governance policy file");
    return true;
}

void PolicyFileWatcher::stop() {
    running_.store(false, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
    THEMIS_INFO("PolicyFileWatcher: monitoring thread stopped "
                "(success={}, failure={})",
                reload_success_count_.load(std::memory_order_relaxed),
                reload_failure_count_.load(std::memory_order_relaxed));
}

void PolicyFileWatcher::run() {
    namespace fs = std::filesystem;

    // Track the modification time we last observed so we can detect changes
    // independently of what the engine itself tracks.  We start with the
    // engine's currently-loaded mtime (obtained via a stat of the loaded path).
    std::string watched_path = engine_.getLoadedFilePath();
    fs::file_time_type last_observed_mtime{};
    bool has_baseline = false;

    // Timestamp of when we first saw the current change; used for debounce.
    std::chrono::steady_clock::time_point change_detected_at{};
    bool pending_reload = false;

    if (!watched_path.empty()) {
        std::error_code ec;
        last_observed_mtime = fs::last_write_time(watched_path, ec);
        has_baseline = !ec;
    }

    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(config_.poll_interval);

        if (!running_.load(std::memory_order_acquire)) {
            break;
        }

        // If the engine loaded a file after the watcher started, pick it up.
        const std::string current_path = engine_.getLoadedFilePath();
        if (current_path.empty()) {
            // Nothing loaded yet; keep waiting.
            continue;
        }

        if (current_path != watched_path) {
            // The engine switched to a different file; reset our baseline.
            watched_path  = current_path;
            has_baseline  = false;
            pending_reload = false;
        }

        // Read the current mtime
        std::error_code ec;
        fs::file_time_type current_mtime = fs::last_write_time(watched_path, ec);
        if (ec) {
            // File temporarily unavailable (e.g. being replaced atomically)
            continue;
        }

        if (!has_baseline) {
            last_observed_mtime = current_mtime;
            has_baseline = true;
            continue;
        }

        if (current_mtime != last_observed_mtime) {
            // File has changed – start (or restart) the debounce window.
            if (!pending_reload) {
                change_detected_at = std::chrono::steady_clock::now();
                pending_reload = true;
                THEMIS_DEBUG("PolicyFileWatcher: file change detected, debouncing");
            }
            // Update to the latest mtime so a still-changing file keeps
            // resetting the debounce window.
            last_observed_mtime = current_mtime;
        }

        if (pending_reload) {
            const auto elapsed =
                std::chrono::steady_clock::now() - change_detected_at;
            if (elapsed >= config_.debounce) {
                // Debounce window expired – trigger reload
                std::string err;
                const bool ok = engine_.reloadIfChanged(&err);
                if (ok) {
                    reload_success_count_.fetch_add(1, std::memory_order_relaxed);
                    THEMIS_INFO("PolicyFileWatcher: governance policy reloaded successfully");
                    if (config_.reload_cb) {
                        config_.reload_cb(true, {});
                    }
                } else {
                    reload_failure_count_.fetch_add(1, std::memory_order_relaxed);
                    THEMIS_ERROR("PolicyFileWatcher: governance policy reload failed: {}", err);
                    if (config_.reload_cb) {
                        config_.reload_cb(false, err);
                    }
                }
                pending_reload = false;
                // Re-read the mtime after the reload so we don't immediately
                // re-trigger on the same change.
                ec.clear();
                last_observed_mtime = fs::last_write_time(watched_path, ec);
                if (ec) {
                    has_baseline = false;
                }
            }
        }
    }
}

} // namespace governance
} // namespace themis
