/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_hot_plug_monitor.h                          ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     161                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

namespace themis {
namespace plugins {

// Forward declaration
class PluginManager;

/**
 * @brief Configuration options for hot-plug monitoring
 */
struct HotPlugConfig {
    bool enabled = false;
    bool auto_load = true;       // Auto-load new plugins
    bool auto_reload = true;     // Auto-reload modified plugins
    bool auto_unload = true;     // Auto-unload deleted plugins
    int watch_interval_ms = 100; // Polling interval (fallback mode)
};

/**
 * @brief Event types for plugin file system changes
 */
enum class FileEvent {
    CREATED,
    MODIFIED,
    DELETED
};

/**
 * @brief Hot-plug filesystem monitor for automatic plugin discovery
 * 
 * Monitors a directory for plugin file changes using platform-native APIs:
 * - Linux: inotify
 * - Windows: ReadDirectoryChangesW
 * - macOS: FSEvents or kqueue
 * 
 * Thread-Safety: All methods are thread-safe
 */
class PluginHotPlugMonitor {
private:
    std::string watch_directory_;
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    PluginManager* plugin_manager_;
    HotPlugConfig config_;
    
#ifdef _WIN32
    void* dir_handle_;  // HANDLE for Windows
#elif defined(__APPLE__)
    void* fs_event_stream_;  // FSEventStreamRef for macOS
#else
    int inotify_fd_;
    int watch_descriptor_;
#endif
    
public:
    /**
     * @brief Construct a hot-plug monitor
     * @param manager Pointer to PluginManager
     * @param directory Directory to monitor
     * @param config Configuration options
     */
    PluginHotPlugMonitor(
        PluginManager* manager,
        const std::string& directory,
        const HotPlugConfig& config = HotPlugConfig()
    );
    
    ~PluginHotPlugMonitor();
    
    // Prevent copying
    PluginHotPlugMonitor(const PluginHotPlugMonitor&) = delete;
    PluginHotPlugMonitor& operator=(const PluginHotPlugMonitor&) = delete;
    
    /**
     * @brief Start monitoring the directory
     * @return true if started successfully
     */
    bool start();
    
    /**
     * @brief Stop monitoring
     */
    void stop();
    
    /**
     * @brief Check if monitor is running
     * @return true if running
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Get configuration
     * @return Current configuration
     */
    const HotPlugConfig& getConfig() const { return config_; }
    
private:
    /**
     * @brief Platform-specific monitoring implementations
     */
#ifdef _WIN32
    void watchDirectoryWindows();
#elif defined(__APPLE__)
    void watchDirectoryMacOS();
#else
    void watchDirectoryLinux();
#endif
    
    /**
     * @brief Handle file system event
     * @param filename File that changed
     * @param event Event type
     */
    void handleFileEvent(const std::string& filename, FileEvent event);
    
    /**
     * @brief Check if filename is a plugin file
     * @param filename Filename to check
     * @return true if it's a plugin file
     */
    bool isPluginFile(const std::string& filename) const;
    
    /**
     * @brief Extract plugin name from file path
     * @param filepath Full path to plugin file
     * @return Plugin name or empty string if not found
     */
    std::string extractPluginName(const std::string& filepath) const;
};

} // namespace plugins
} // namespace themis
