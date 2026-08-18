/**
 * @file plugin_hot_plug_monitor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/plugin_hot_plug_monitor.h"
#include "plugins/plugin_manager.h"
#include "utils/logger.h"
#include <filesystem>
#include <map>
#include <thread>
#include <chrono>
#include <algorithm>
#include <set>

#ifdef _WIN32
    #include <windows.h>
#elif defined(__APPLE__)
    #include <CoreServices/CoreServices.h>
    #include <sys/event.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#else
    #include <sys/inotify.h>
    #include <unistd.h>
    #include <errno.h>
    #include <cstring>
    #include <poll.h>
#endif

namespace themis {
namespace plugins {

namespace fs = std::filesystem;

// ============================================================================
// RAII Helper Classes
// ============================================================================

// RAII wrapper for file descriptors (Unix/Linux/macOS)
struct FileDescriptorDeleter {
    void operator()(int* fd_ptr) const noexcept {
        if (fd_ptr && *fd_ptr >= 0) {
            try {
                close(*fd_ptr);
                *fd_ptr = -1;
            } catch (...) {
                THEMIS_WARN("Exception during file descriptor cleanup");
            }
        }
        delete fd_ptr;
    }
};

using UniqueFileDescriptor = std::unique_ptr<int, FileDescriptorDeleter>;

// Helper function to create RAII-wrapped file descriptor
inline UniqueFileDescriptor makeUniqueFileDescriptor(int fd) {
    auto fd_ptr = std::make_unique<int>(fd);
    return UniqueFileDescriptor(fd_ptr.release());
}

// ============================================================================
// Constructor & Destructor
// ============================================================================

PluginHotPlugMonitor::PluginHotPlugMonitor(
    PluginManager* manager,
    const std::string& directory,
    const HotPlugConfig& config
) : plugin_manager_(manager),
    watch_directory_(directory),
    config_(config)
{
#ifdef _WIN32
    dir_handle_ = nullptr;
#elif defined(__APPLE__)
    fs_event_stream_ = nullptr;
#else
    inotify_fd_ = -1;
    watch_descriptor_ = -1;
#endif
}

PluginHotPlugMonitor::~PluginHotPlugMonitor() {
    stop();
}

// ============================================================================
// Platform-specific helpers
// ============================================================================

bool PluginHotPlugMonitor::isPluginFile(const std::string& filename) const {
    // Reject temporary/incomplete files produced by editors, package managers,
    // and build tools during write operations (e.g. vim swaps, wget .part files).
    // These files are never valid plugin binaries or manifests.
    if (filename.starts_with(".") ||
        filename.ends_with(".tmp") ||
        filename.ends_with(".part") ||
        filename.ends_with(".download") ||
        filename.ends_with("~")) {
        return false;
    }

    // Check for plugin-related file extensions
    // Note: ends_with is C++20, but this project uses C++20
    return filename.ends_with(".dll") ||
           filename.ends_with(".so") ||
           filename.ends_with(".dylib") ||
           filename == "plugin.json";
}

std::string PluginHotPlugMonitor::extractPluginName(const std::string& filepath) const {
    fs::path p(filepath);
    
    // If it's plugin.json, the parent directory name is the plugin name
    if (p.filename() == "plugin.json") {
        return p.parent_path().filename().string();
    }
    
    // For .dll/.so/.dylib files, use the stem (filename without extension)
    return p.stem().string();
}

// ============================================================================
// Event Handling
// ============================================================================

void PluginHotPlugMonitor::handleFileEvent(
    const std::string& filename,
    FileEvent event
) {
    // Only process plugin files
    if (!isPluginFile(filename)) {
        return;
    }
    
    std::string full_path = watch_directory_ + "/" + filename;
    std::string plugin_name = extractPluginName(full_path);
    
    if (plugin_name.empty()) {
        return;
    }
    
    try {
        switch (event) {
            case FileEvent::CREATED:
                if (config_.auto_load) {
                    THEMIS_INFO("New plugin detected: {}", filename);
                    
                    // Wait for file to be fully written
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    
                    // Rescan directory to discover new plugin
                    (void)plugin_manager_->scanPluginDirectory(watch_directory_);
                    
                    // Try to load the plugin
                    auto result = plugin_manager_->loadPlugin(plugin_name);
                    if (result) {
                        THEMIS_INFO("Auto-loaded plugin: {}", plugin_name);
                    } else {
                        THEMIS_WARN("Failed to auto-load plugin {}: {}", plugin_name, result.error().message());
                    }
                }
                break;
                
            case FileEvent::MODIFIED:
                if (config_.auto_reload) {
                    THEMIS_INFO("Plugin modified: {}", filename);
                    
                    // Wait for file to be fully written
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    
                    // Hot-reload if already loaded
                    if (plugin_manager_->isPluginLoaded(plugin_name)) {
                        auto reload_result = plugin_manager_->reloadPlugin(plugin_name);
                        if (reload_result.has_value()) {
                            THEMIS_INFO("Auto-reloaded plugin: {}", plugin_name);
                        } else {
                            THEMIS_WARN("Failed to reload plugin: {}", plugin_name);
                        }
                    }
                }
                break;
                
            case FileEvent::DELETED:
                if (config_.auto_unload) {
                    THEMIS_INFO("Plugin removed: {}", filename);
                    
                    // Unload if loaded
                    if (plugin_manager_->isPluginLoaded(plugin_name)) {
                        auto unload_result = plugin_manager_->unloadPlugin(plugin_name);
                        if (unload_result.has_value()) {
                            THEMIS_INFO("Auto-unloaded plugin: {}", plugin_name);
                        } else {
                            THEMIS_WARN("Failed to auto-unload plugin: {}", plugin_name);
                        }
                    }
                }
                break;
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error handling file event for {}: {}", filename, e.what());
    }
}

// ============================================================================
// Linux Implementation (inotify)
// ============================================================================

#ifndef _WIN32
#ifndef __APPLE__

void PluginHotPlugMonitor::watchDirectoryLinux() {
    char buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    
    // Use poll to allow for interruption
    struct pollfd pfd;
    pfd.fd = inotify_fd_;
    pfd.events = POLLIN;
    
    while (running_) {
        int poll_result = poll(&pfd, 1, 1000);  // 1 second timeout
        
        if (poll_result < 0) {
            if (errno != EINTR) {
                THEMIS_ERROR("poll error: {}", strerror(errno));
            }
            continue;
        }
        
        if (poll_result == 0) {
            // Timeout, check if still running
            continue;
        }
        
        ssize_t length = read(inotify_fd_, buffer, sizeof(buffer));
        if (length < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                THEMIS_ERROR("inotify read error: {}", strerror(errno));
            }
            continue;
        }
        
        // Process all events in the buffer
        for (char* ptr = buffer; ptr < buffer + length; ) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);
            
            if (event->len > 0) {
                FileEvent file_event;
                
                if (event->mask & (IN_CREATE | IN_MOVED_TO)) {
                    file_event = FileEvent::CREATED;
                } else if (event->mask & IN_MODIFY) {
                    file_event = FileEvent::MODIFIED;
                } else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                    file_event = FileEvent::DELETED;
                } else {
                    // Ignore other events
                    ptr += sizeof(struct inotify_event) + event->len;
                    continue;
                }
                
                handleFileEvent(event->name, file_event);
            }
            
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
}

#endif // !__APPLE__
#endif // !_WIN32

// ============================================================================
// Windows Implementation (ReadDirectoryChangesW)
// ============================================================================

#ifdef _WIN32

void PluginHotPlugMonitor::watchDirectoryWindows() {
    char buffer[4096];
    DWORD bytes_returned;
    
    while (running_) {
        BOOL success = ReadDirectoryChangesW(
            dir_handle_,
            buffer,
            sizeof(buffer),
            TRUE,  // Watch subdirectories
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytes_returned,
            nullptr,
            nullptr
        );
        
        if (!success || !running_) {
            break;
        }
        
        if (bytes_returned == 0) {
            continue;
        }
        
        // Process events
        FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
        
        bool has_more_events = true;
        while (has_more_events) {
            // Convert filename from wide char to narrow
            int filename_length = fni->FileNameLength / sizeof(wchar_t);
            std::wstring wfilename(fni->FileName, filename_length);
            std::string filename;
            filename.reserve(wfilename.size());
            for (wchar_t wc : wfilename) {
                filename.push_back(static_cast<char>(wc));
            }
            
            FileEvent event;
            bool should_handle = true;
            switch (fni->Action) {
                case FILE_ACTION_ADDED:
                    event = FileEvent::CREATED;
                    break;
                case FILE_ACTION_MODIFIED:
                    event = FileEvent::MODIFIED;
                    break;
                case FILE_ACTION_REMOVED:
                    event = FileEvent::DELETED;
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    // A rename/move into the directory counts as a new file
                    event = FileEvent::CREATED;
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    // A rename/move out of the directory counts as a deletion
                    event = FileEvent::DELETED;
                    break;
                default:
                    // Ignore other actions
                    should_handle = false;
                    break;
            }
            
            if (should_handle) {
                handleFileEvent(filename, event);
            }
            
            // Check if there are more events
            if (fni->NextEntryOffset == 0) {
                has_more_events = false;
            } else {
                fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<char*>(fni) + fni->NextEntryOffset
                );
            }
        }
    }
}

#endif // _WIN32

// ============================================================================
// macOS Implementation (kqueue)
// ============================================================================

#ifdef __APPLE__

void PluginHotPlugMonitor::watchDirectoryMacOS() {
    // Use kqueue for macOS
    int kq = kqueue();
    if (kq == -1) {
        THEMIS_ERROR("Failed to create kqueue: {}", strerror(errno));
        return;
    }
    // Wrap kqueue file descriptor for RAII cleanup
    auto kq_guard = makeUniqueFileDescriptor(kq);
    
    // Open directory for monitoring (add O_NONBLOCK to prevent indefinite blocking)
    int dir_fd = open(watch_directory_.c_str(), O_RDONLY | O_NONBLOCK);
    if (dir_fd == -1) {
        THEMIS_ERROR("Failed to open directory: {}", strerror(errno));
        return;  // kq_guard will automatically close kq
    }
    // Wrap directory file descriptor for RAII cleanup
    auto dir_guard = makeUniqueFileDescriptor(dir_fd);
    
    // Setup kevent for directory monitoring
    struct kevent change;
    EV_SET(&change, dir_fd, EVFILT_VNODE,
           EV_ADD | EV_ENABLE | EV_CLEAR,
           NOTE_WRITE | NOTE_EXTEND | NOTE_DELETE,
           0, nullptr);
    
    // Add kevent with timeout to prevent indefinite blocking
    struct timespec timeout = {};
    timeout.tv_sec = 1;  // 1 second timeout for kevent operations
    timeout.tv_nsec = 0;
    
    if (kevent(kq, &change, 1, nullptr, 0, &timeout) == -1) {
        THEMIS_ERROR("Failed to add kevent: {}", strerror(errno));
        return;  // Both kq_guard and dir_guard will automatically cleanup
    }
    
    // Track files we've seen along with their last-write timestamps
    std::map<std::string, fs::file_time_type> known_files;
    auto scan_directory = [&]() {
        try {
            for (const auto& entry : fs::directory_iterator(watch_directory_)) {
                // Skip symlinks pointing to non-existent targets
                if (entry.is_symlink()) {
                    std::error_code ec;
                    if (!fs::exists(entry.path(), ec) || ec) {
                        THEMIS_WARN("Skipping broken symlink: {}", entry.path().string());
                        continue;
                    }
                }
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (isPluginFile(filename)) {
                        std::error_code ec;
                        auto mtime = entry.last_write_time(ec);
                        if (!ec) {
                            current_files[filename] = mtime;
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Error scanning plugin directory: {}", e.what());
        }
        
        // Detect new files and modified files (by mtime)
        for (const auto& [file, mtime] : current_files) {
            auto prev = known_files.find(file);
            if (prev == known_files.end()) {
                handleFileEvent(file, FileEvent::CREATED);
            } else if (prev->second != mtime) {
                handleFileEvent(file, FileEvent::MODIFIED);
            }
        }
        
        // Detect deleted files
        for (const auto& [file, _] : known_files) {
            if (current_files.find(file) == current_files.end()) {
                handleFileEvent(file, FileEvent::DELETED);
            }
        }
        
        known_files = current_files;
    };
    
    // Initial scan
    scan_directory();
    
    // Monitor loop
    struct kevent event;
    struct timespec timeout;
    timeout.tv_sec = 1;
    timeout.tv_nsec = 0;
    
    while (running_) {
        int nev = kevent(kq, nullptr, 0, &event, 1, &timeout);
        
        if (nev < 0) {
            if (errno != EINTR) {
                THEMIS_ERROR("kevent error: {}", strerror(errno));
            }
            continue;
        }
        
        if (nev > 0) {
            // Directory changed, rescan to detect what changed
            scan_directory();
        }
    }
    
    close(dir_fd);
    close(kq);
}

#endif // __APPLE__

// ============================================================================
// Public Interface
// ============================================================================

bool PluginHotPlugMonitor::start() {
    if (running_) {
        THEMIS_WARN("Hot-plug monitor already running");
        return false;
    }
    
    // Verify directory exists
    if (!fs::exists(watch_directory_)) {
        THEMIS_ERROR("Watch directory does not exist: {}", watch_directory_);
        return false;
    }
    
    if (!fs::is_directory(watch_directory_)) {
        THEMIS_ERROR("Watch path is not a directory: {}", watch_directory_);
        return false;
    }
    
    running_ = true;
    
#ifdef _WIN32
    // Windows implementation
    dir_handle_ = CreateFileA(
        watch_directory_.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );
    
    if (dir_handle_ == INVALID_HANDLE_VALUE) {
        THEMIS_ERROR("Failed to open directory for watching: {}", watch_directory_);
        running_ = false;
        return false;
    }
    
    monitor_thread_ = std::thread([this]() {
        watchDirectoryWindows();
    });
    
#elif defined(__APPLE__)
    // macOS implementation
    monitor_thread_ = std::thread([this]() {
        watchDirectoryMacOS();
    });
    
#else
    // Linux implementation
    inotify_fd_ = inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ < 0) {
        THEMIS_ERROR("Failed to initialize inotify: {}", strerror(errno));
        running_ = false;
        return false;
    }
    
    watch_descriptor_ = inotify_add_watch(
        inotify_fd_,
        watch_directory_.c_str(),
        IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM
    );
    
    if (watch_descriptor_ < 0) {
        THEMIS_ERROR("Failed to add inotify watch: {}", strerror(errno));
        close(inotify_fd_);
        inotify_fd_ = -1;
        running_ = false;
        return false;
    }
    
    monitor_thread_ = std::thread([this]() {
        watchDirectoryLinux();
    });
#endif
    
    THEMIS_INFO("Hot-plug monitoring started for: {}", watch_directory_);
    return true;
}

void PluginHotPlugMonitor::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;

#ifdef _WIN32
    // On Windows, ReadDirectoryChangesW can block indefinitely.
    // Closing the watched directory handle first unblocks the monitor thread.
    if (dir_handle_ != nullptr && dir_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(dir_handle_);
        dir_handle_ = nullptr;
    }
#endif
    
    // Wait for thread to finish with a timeout to prevent indefinite blocking
    // If thread doesn't respond within 5 seconds, log a warning and continue
    if (monitor_thread_.joinable()) {
        // Try to join with a timeout using std::thread features
        // Note: C++20 does not have direct timeout join, so we use a timed wait approach
        auto start_time = std::chrono::steady_clock::now();
        const auto timeout_duration = std::chrono::seconds(5);
        
        // Periodically check if thread is still alive
        while (monitor_thread_.joinable()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > timeout_duration) {
                THEMIS_WARN("Plugin hot-plug monitor thread did not join within {} ms", 
                    std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration).count());
                break;
            }
        }
        
        // If still joinable after timeout, attempt final join without blocking
        if (monitor_thread_.joinable()) {
            try {
                monitor_thread_.detach();
            } catch (...) {
                THEMIS_WARN("Failed to detach monitor thread");
            }
        }
    }
    
#ifdef _WIN32
    // Handle already closed before join to unblock ReadDirectoryChangesW.
#elif defined(__APPLE__)
    // Cleanup handled in thread
#else
    if (watch_descriptor_ >= 0) {
        inotify_rm_watch(inotify_fd_, watch_descriptor_);
        watch_descriptor_ = -1;
    }
    if (inotify_fd_ >= 0) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
#endif
    
    THEMIS_INFO("Hot-plug monitoring stopped");
}

} // namespace plugins
} // namespace themis
