/**
 * @file config_file_watcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=5, H=6, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "config/config_file_watcher.h"

#include <filesystem>
#include <set>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <vector>

// ── Platform headers ──────────────────────────────────────────────────────────
#if defined(__linux__)
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <map>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace themis {
namespace config {

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

/// Returns true if the filename extension is .yaml, .yml, or .json.
bool isWatchedExtension(const std::string &filename) {
    auto pos = filename.rfind('.');
    if (pos == std::string::npos) {
        return false;
    }
    std::string ext = filename.substr(pos);
    return ext == ".yaml" || ext == ".yml" || ext == ".json";
}

} // anonymous namespace

// ── ConfigFileWatcher implementation ─────────────────────────────────────────

ConfigFileWatcher::ConfigFileWatcher(std::string watch_path, std::function<void()> callback,
                                     std::chrono::milliseconds debounce)
    : watch_path_(std::move(watch_path)), callback_(std::move(callback)), debounce_(debounce) {}

ConfigFileWatcher::~ConfigFileWatcher() {
    stop();
}

bool ConfigFileWatcher::start() {
    if (running_.load(std::memory_order_acquire)) {
        return true; // already running – idempotent
    }

    if (!std::filesystem::exists(watch_path_)) {
        spdlog::warn("ConfigFileWatcher: watch path '{}' does not exist – watcher not started", watch_path_);
        return false;
    }

#if defined(__linux__)
    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) {
        spdlog::warn("ConfigFileWatcher: pipe2 failed: {}", strerror(errno));
        return false;
    }
    pipe_read_fd_  = fds[0];
    pipe_write_fd_ = fds[1];
#elif defined(__APPLE__)
    kqueue_fd_ = kqueue();
    if (kqueue_fd_ == -1) {
        spdlog::warn("ConfigFileWatcher: kqueue() failed: {}", strerror(errno));
        return false;
    }
    int fds[2];
    if (pipe(fds) != 0) {
        ::close(kqueue_fd_);
        kqueue_fd_ = -1;
        spdlog::warn("ConfigFileWatcher: pipe failed: {}", strerror(errno));
        return false;
    }
    pipe_read_fd_  = fds[0];
    pipe_write_fd_ = fds[1];
    // Make read end non-blocking
    fcntl(pipe_read_fd_, F_SETFL, O_NONBLOCK);
#elif defined(_WIN32)
    stop_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (stop_event_ == nullptr) {
        spdlog::warn("ConfigFileWatcher: CreateEvent failed: {}", GetLastError());
        return false;
    }
#endif

    running_.store(true, std::memory_order_release);
    try {
        thread_ = std::thread(&ConfigFileWatcher::watchLoop, this);
    } catch (const std::system_error &) {
        running_.store(false, std::memory_order_release);
        // Roll back OS resources allocated above
#if defined(__linux__)
        if (pipe_write_fd_ != -1) {
            ::close(pipe_write_fd_);
            pipe_write_fd_ = -1;
        }
        if (pipe_read_fd_ != -1) {
            ::close(pipe_read_fd_);
            pipe_read_fd_ = -1;
        }
#elif defined(__APPLE__)
        if (pipe_write_fd_ != -1) {
            ::close(pipe_write_fd_);
            pipe_write_fd_ = -1;
        }
        if (pipe_read_fd_ != -1) {
            ::close(pipe_read_fd_);
            pipe_read_fd_ = -1;
        }
        if (kqueue_fd_ != -1) {
            ::close(kqueue_fd_);
            kqueue_fd_ = -1;
        }
#elif defined(_WIN32)
        if (stop_event_ != nullptr) {
            CloseHandle(static_cast<HANDLE>(stop_event_));
            stop_event_ = nullptr;
        }
#endif
        spdlog::warn("ConfigFileWatcher: failed to start watcher thread for '{}'", watch_path_);
        return false;
    } catch (const std::string &) {
        running_.store(false, std::memory_order_release);
        // Roll back OS resources allocated above
#if defined(__linux__)
        if (pipe_write_fd_ != -1) {
            ::close(pipe_write_fd_);
            pipe_write_fd_ = -1;
        }
        if (pipe_read_fd_ != -1) {
            ::close(pipe_read_fd_);
            pipe_read_fd_ = -1;
        }
#elif defined(__APPLE__)
        if (pipe_write_fd_ != -1) {
            ::close(pipe_write_fd_);
            pipe_write_fd_ = -1;
        }
        if (pipe_read_fd_ != -1) {
            ::close(pipe_read_fd_);
            pipe_read_fd_ = -1;
        }
        if (kqueue_fd_ != -1) {
            ::close(kqueue_fd_);
            kqueue_fd_ = -1;
        }
#elif defined(_WIN32)
        if (stop_event_ != nullptr) {
            CloseHandle(static_cast<HANDLE>(stop_event_));
            stop_event_ = nullptr;
        }
#endif
        spdlog::warn("ConfigFileWatcher: failed to start watcher thread for '{}'", watch_path_);
        return false;
    } catch (const char *) {
        running_.store(false, std::memory_order_release);
        // Roll back OS resources allocated above
#if defined(__linux__)
        if (pipe_write_fd_ != -1) {
            ::close(pipe_write_fd_);
            pipe_write_fd_ = -1;
        }
        if (pipe_read_fd_ != -1) {
            ::close(pipe_read_fd_);
            pipe_read_fd_ = -1;
        }
#elif defined(__APPLE__)
        if (pipe_write_fd_ != -1) {
            ::close(pipe_write_fd_);
            pipe_write_fd_ = -1;
        }
        if (pipe_read_fd_ != -1) {
            ::close(pipe_read_fd_);
            pipe_read_fd_ = -1;
        }
        if (kqueue_fd_ != -1) {
            ::close(kqueue_fd_);
            kqueue_fd_ = -1;
        }
#elif defined(_WIN32)
        if (stop_event_ != nullptr) {
            CloseHandle(static_cast<HANDLE>(stop_event_));
            stop_event_ = nullptr;
        }
#endif
        spdlog::warn("ConfigFileWatcher: failed to start watcher thread for '{}'", watch_path_);
        return false;
    }
    spdlog::info("ConfigFileWatcher: started watching '{}' (debounce {}ms)", watch_path_, debounce_.count());
    return true;
}

void ConfigFileWatcher::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return; // was not running
    }

    // Signal the background thread to exit
#if defined(__linux__)
    if (pipe_write_fd_ != -1) {
        char dummy = 1;
        const ssize_t rc = ::write(pipe_write_fd_, &dummy, sizeof(dummy));
        if (rc == -1) {
            spdlog::warn("ConfigFileWatcher: failed to signal stop pipe for '{}': {}",
                         watch_path_, strerror(errno));
        }
    }
#elif defined(__APPLE__)
    if (pipe_write_fd_ != -1) {
        char dummy = 1;
        const ssize_t rc = ::write(pipe_write_fd_, &dummy, sizeof(dummy));
        if (rc == -1) {
            spdlog::warn("ConfigFileWatcher: failed to signal stop pipe for '{}': {}",
                         watch_path_, strerror(errno));
        }
    }
#elif defined(_WIN32)
    if (stop_event_ != nullptr) {
        SetEvent(static_cast<HANDLE>(stop_event_));
    }
#endif

    if (thread_.joinable()) {
        thread_.join();
    }

    // Close OS resources
#if defined(__linux__)
    if (pipe_write_fd_ != -1) {
        ::close(pipe_write_fd_);
        pipe_write_fd_ = -1;
    }
    if (pipe_read_fd_ != -1) {
        ::close(pipe_read_fd_);
        pipe_read_fd_ = -1;
    }
#elif defined(__APPLE__)
    if (pipe_write_fd_ != -1) {
        ::close(pipe_write_fd_);
        pipe_write_fd_ = -1;
    }
    if (pipe_read_fd_ != -1) {
        ::close(pipe_read_fd_);
        pipe_read_fd_ = -1;
    }
    if (kqueue_fd_ != -1) {
        ::close(kqueue_fd_);
        kqueue_fd_ = -1;
    }
#elif defined(_WIN32)
    if (stop_event_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(stop_event_));
        stop_event_ = nullptr;
    }
#endif

    spdlog::info("ConfigFileWatcher: stopped watching '{}'", watch_path_);
}

void ConfigFileWatcher::watchLoop() {
#if defined(__linux__)
    watchLoopInotify();
#elif defined(__APPLE__)
    watchLoopKqueue();
#elif defined(_WIN32)
    watchLoopReadDirChanges();
#else
    // Unsupported platform – log and exit immediately
    spdlog::warn("ConfigFileWatcher: file-system watching is not supported on this platform");
    running_.store(false, std::memory_order_release);
#endif
}

// ── Debounce helper ───────────────────────────────────────────────────────────

void ConfigFileWatcher::scheduleCallback() {
    {
        std::lock_guard<std::mutex> lk(debounce_mutex_);
        last_event_time_ = std::chrono::steady_clock::now();
        event_pending_   = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Linux – inotify
// ─────────────────────────────────────────────────────────────────────────────
#if defined(__linux__)

void ConfigFileWatcher::watchLoopInotify() {
    int ifd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (ifd == -1) {
        spdlog::warn("ConfigFileWatcher: inotify_init1 failed: {}", strerror(errno));
        running_.store(false, std::memory_order_release);
        return;
    }

    // Map watch-descriptor -> directory path for sub-directory tracking
    std::map<int, std::string> wd_to_path;

    // Helper: add a watch on a single directory
    auto add_watch = [&](const std::string &dir) {
        int wd = inotify_add_watch(ifd, dir.c_str(),
                                   IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_MODIFY | IN_DONT_FOLLOW);
        if (wd == -1) {
            spdlog::debug("ConfigFileWatcher: inotify_add_watch('{}') failed: {}", dir, strerror(errno));
        } else {
            wd_to_path[wd] = dir;
        }
    };

    // Recursively watch the config directory tree
    add_watch(watch_path_);
    try {
        for (auto &entry : std::filesystem::recursive_directory_iterator(
                 watch_path_, std::filesystem::directory_options::skip_permission_denied)) {
            if (entry.is_directory()) {
                add_watch(entry.path().string());
            }
        }
    } catch (const std::exception &ex) {
        spdlog::debug("ConfigFileWatcher: recursive dir scan error: {}", ex.what());
    }

    // poll: fd[0] = inotify, fd[1] = stop pipe
    struct pollfd pfds[2];
    pfds[0].fd     = ifd;
    pfds[0].events = POLLIN;
    pfds[1].fd     = pipe_read_fd_;
    pfds[1].events = POLLIN;

    constexpr int kBufSize = 4096;
    alignas(struct inotify_event) char buf[kBufSize];

    while (running_.load(std::memory_order_acquire)) {
        // Calculate timeout for debounce: if an event is pending fire after
        // debounce_ ms from the last event; otherwise wait indefinitely.
        int timeout_ms = -1;
        {
            std::lock_guard<std::mutex> lk(debounce_mutex_);
            if (event_pending_) {
                auto elapsed   = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()
                                                                                       - last_event_time_);
                auto remaining = debounce_ - elapsed;
                timeout_ms     = static_cast<int>(std::max(std::chrono::milliseconds(0), remaining).count());
            }
        }

        int nfds = poll(pfds, 2, timeout_ms);

        if (nfds < 0) {
            if (errno == EINTR)
                continue;
            spdlog::warn("ConfigFileWatcher: poll error: {}", strerror(errno));
            break;
        }

        if (nfds == 0) {
            // Timeout – check debounce
            bool should_fire = false;
            {
                std::lock_guard<std::mutex> lk(debounce_mutex_);
                if (event_pending_) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - last_event_time_);
                    if (elapsed >= debounce_) {
                        should_fire    = true;
                        event_pending_ = false;
                    }
                }
            }
            if (should_fire && callback_) {
                try {
                    callback_();
                } catch (const std::exception &ex) {
                    spdlog::warn("ConfigFileWatcher: callback threw: {}", ex.what());
                }
            }
            continue;
        }

        // Stop pipe signalled
        if (pfds[1].revents & POLLIN) {
            break;
        }

        // inotify events
        if (pfds[0].revents & POLLIN) {
            ssize_t len = read(ifd, buf, kBufSize);
            if (len <= 0)
                continue;

            const char *ptr = buf;
            while (ptr < buf + len) {
                const auto *ev = reinterpret_cast<const struct inotify_event *>(ptr);
                ptr += sizeof(struct inotify_event) + ev->len;

                // If a new sub-directory was created, add a watch on it
                if ((ev->mask & IN_CREATE) && (ev->mask & IN_ISDIR) && ev->len > 0) {
                    auto it = wd_to_path.find(ev->wd);
                    if (it != wd_to_path.end()) {
                        std::string new_dir = it->second + "/" + ev->name;
                        add_watch(new_dir);
                    }
                }

                // Check if a watched extension changed
                if (ev->len > 0 && isWatchedExtension(ev->name)) {
                    spdlog::debug("ConfigFileWatcher: inotify event for '{}'", ev->name);
                    scheduleCallback();
                }
            }
        }
    }

    // Clean up inotify watches
    for (auto &[wd, _] : wd_to_path) {
        inotify_rm_watch(ifd, wd);
    }
    ::close(ifd);
}

#endif // __linux__

// ─────────────────────────────────────────────────────────────────────────────
// macOS – kqueue
// ─────────────────────────────────────────────────────────────────────────────
#if defined(__APPLE__)

void ConfigFileWatcher::watchLoopKqueue() {
    // kqueue watches individual file descriptors. We open each .yaml/.json file
    // and every directory under watch_path_ and register NOTE_WRITE/NOTE_RENAME/
    // NOTE_DELETE kevents.
    // Map watch-descriptor (fd) -> directory/file path for event tracking.
    // Also maintain a reverse set of all registered paths to avoid duplicates.
    std::map<int, std::string> fd_to_path;
    std::set<std::string> registered_paths;

    auto register_path = [&](const std::string &path) {
        if (registered_paths.count(path))
            return; // already registered
        int fd = ::open(path.c_str(), O_RDONLY | O_EVTONLY | O_CLOEXEC);
        if (fd == -1) {
            spdlog::debug("ConfigFileWatcher: open('{}') failed: {}", path, strerror(errno));
            return;
        }
        struct kevent ev{};
        EV_SET(&ev, static_cast<uintptr_t>(fd), EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR,
               NOTE_WRITE | NOTE_RENAME | NOTE_DELETE | NOTE_ATTRIB, 0, nullptr);
        if (kevent(kqueue_fd_, &ev, 1, nullptr, 0, nullptr) == -1) {
            spdlog::debug("ConfigFileWatcher: kevent register for '{}' failed: {}", path, strerror(errno));
            ::close(fd);
            return;
        }
        fd_to_path[fd] = path;
        registered_paths.insert(path);
    };

    // Register the watch root and all files/dirs under it
    register_path(watch_path_);
    try {
        for (auto &entry : std::filesystem::recursive_directory_iterator(
                 watch_path_, std::filesystem::directory_options::skip_permission_denied)) {
            register_path(entry.path().string());
        }
    } catch (const std::exception &ex) {
        spdlog::debug("ConfigFileWatcher: recursive dir scan error: {}", ex.what());
    }

    // Also register the stop-pipe read end so we can wake up
    {
        struct kevent ev{};
        EV_SET(&ev, static_cast<uintptr_t>(pipe_read_fd_), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
        kevent(kqueue_fd_, &ev, 1, nullptr, 0, nullptr);
    }

    constexpr int kMaxEvents = 16;
    struct kevent events[kMaxEvents];

    while (running_.load(std::memory_order_acquire)) {
        // Calculate timeout
        struct timespec ts_buf{};
        struct timespec *ts = nullptr;
        {
            std::lock_guard<std::mutex> lk(debounce_mutex_);
            if (event_pending_) {
                auto elapsed   = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()
                                                                                       - last_event_time_);
                auto remaining = std::max(std::chrono::milliseconds(0), debounce_ - elapsed);
                ts_buf.tv_sec  = remaining.count() / 1000;
                ts_buf.tv_nsec = (remaining.count() % 1000) * 1'000'000L;
                ts             = &ts_buf;
            }
        }

        int n = kevent(kqueue_fd_, nullptr, 0, events, kMaxEvents, ts);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            spdlog::warn("ConfigFileWatcher: kevent wait failed: {}", strerror(errno));
            break;
        }

        if (n == 0) {
            // Timeout – debounce check
            bool should_fire = false;
            {
                std::lock_guard<std::mutex> lk(debounce_mutex_);
                if (event_pending_) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - last_event_time_);
                    if (elapsed >= debounce_) {
                        should_fire    = true;
                        event_pending_ = false;
                    }
                }
            }
            if (should_fire && callback_) {
                try {
                    callback_();
                } catch (const std::exception &ex) {
                    spdlog::warn("ConfigFileWatcher: callback threw: {}", ex.what());
                }
            }
            continue;
        }

        for (int i = 0; i < n; ++i) {
            // Stop pipe triggered
            if (static_cast<int>(events[i].ident) == pipe_read_fd_) {
                goto done;
            }

            auto it = fd_to_path.find(static_cast<int>(events[i].ident));
            if (it == fd_to_path.end())
                continue;

            const std::string &path = it->second;
            bool is_dir             = std::filesystem::is_directory(path);

            if (is_dir) {
                // Re-scan directory for new files/sub-dirs not yet registered
                try {
                    for (auto &entry : std::filesystem::directory_iterator(path)) {
                        const std::string entry_path = entry.path().string();
                        if (registered_paths.count(entry_path))
                            continue;
                        if (entry.is_regular_file() && isWatchedExtension(entry_path)) {
                            register_path(entry_path);
                        } else if (entry.is_directory()) {
                            register_path(entry_path);
                        }
                    }
                } catch (const std::filesystem::filesystem_error &) {
                } catch (const std::exception &) {
                } catch (const std::string &) {
                } catch (const char *) {
                }
                scheduleCallback();
            } else if (isWatchedExtension(path)) {
                spdlog::debug("ConfigFileWatcher: kqueue event for '{}'", path);
                scheduleCallback();
            }
        }
    }
done:
    for (auto &[fd, _] : fd_to_path) {
        ::close(fd);
    }
}

#endif // __APPLE__

// ─────────────────────────────────────────────────────────────────────────────
// Windows – ReadDirectoryChangesW
// ─────────────────────────────────────────────────────────────────────────────
#if defined(_WIN32)

void ConfigFileWatcher::watchLoopReadDirChanges() {
    // Convert watch_path_ to wide string
    std::wstring wide_path(watch_path_.begin(), watch_path_.end());

    HANDLE dir_handle
        = CreateFileW(wide_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

    if (dir_handle == INVALID_HANDLE_VALUE) {
        spdlog::warn("ConfigFileWatcher: CreateFileW failed: {}", GetLastError());
        running_.store(false, std::memory_order_release);
        return;
    }

    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (overlapped.hEvent == nullptr) {
        spdlog::warn("ConfigFileWatcher: CreateEvent (overlapped) failed: {}", GetLastError());
        CloseHandle(dir_handle);
        running_.store(false, std::memory_order_release);
        return;
    }

    constexpr DWORD kBufSize = 65536;
    alignas(DWORD) BYTE buf[kBufSize];

    HANDLE wait_handles[2] = {overlapped.hEvent, static_cast<HANDLE>(stop_event_)};

    auto issue_read = [&]() -> bool {
        ResetEvent(overlapped.hEvent);
        DWORD bytes_returned = 0;
        BOOL ok              = ReadDirectoryChangesW(dir_handle, buf, kBufSize,
                                                     /*bWatchSubtree=*/TRUE,
                                                     FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME
                                                         | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                                     &bytes_returned, &overlapped, nullptr);
        if (!ok && GetLastError() != ERROR_IO_PENDING) {
            spdlog::warn("ConfigFileWatcher: ReadDirectoryChangesW failed: {}", GetLastError());
            return false;
        }
        return true;
    };

    if (!issue_read()) {
        CloseHandle(overlapped.hEvent);
        CloseHandle(dir_handle);
        running_.store(false, std::memory_order_release);
        return;
    }

    while (running_.load(std::memory_order_acquire)) {
        // Timeout for debounce
        DWORD timeout_ms = INFINITE;
        {
            std::lock_guard<std::mutex> lk(debounce_mutex_);
            if (event_pending_) {
                auto elapsed   = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()
                                                                                       - last_event_time_);
                auto remaining = std::max(std::chrono::milliseconds(0), debounce_ - elapsed);
                timeout_ms     = static_cast<DWORD>(remaining.count());
            }
        }

        DWORD result = WaitForMultipleObjects(2, wait_handles, FALSE, timeout_ms);

        if (result == WAIT_OBJECT_0 + 1 || result == WAIT_ABANDONED_0 + 1) {
            // Stop event
            break;
        }

        if (result == WAIT_TIMEOUT) {
            // Debounce expired
            bool should_fire = false;
            {
                std::lock_guard<std::mutex> lk(debounce_mutex_);
                if (event_pending_) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - last_event_time_);
                    if (elapsed >= debounce_) {
                        should_fire    = true;
                        event_pending_ = false;
                    }
                }
            }
            if (should_fire && callback_) {
                try {
                    callback_();
                } catch (const std::exception &ex) {
                    spdlog::warn("ConfigFileWatcher: callback threw: {}", ex.what());
                }
            }
            continue;
        }

        // result == WAIT_OBJECT_0: overlapped I/O completed
        DWORD bytes_transferred = 0;
        if (!GetOverlappedResult(dir_handle, &overlapped, &bytes_transferred, FALSE)) {
            spdlog::warn("ConfigFileWatcher: GetOverlappedResult failed: {}", GetLastError());
            break;
        }

        if (bytes_transferred > 0) {
            const BYTE *ptr = buf;
            for (;;) {
                const auto *info = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(ptr);
                // Convert wide filename to narrow
                int len = WideCharToMultiByte(CP_UTF8, 0, info->FileName,
                                              static_cast<int>(info->FileNameLength / sizeof(WCHAR)), nullptr, 0,
                                              nullptr, nullptr);
                std::string filename(static_cast<size_t>(len), '\0');
                WideCharToMultiByte(CP_UTF8, 0, info->FileName, static_cast<int>(info->FileNameLength / sizeof(WCHAR)),
                                    filename.data(), len, nullptr, nullptr);

                if (isWatchedExtension(filename)) {
                    spdlog::debug("ConfigFileWatcher: ReadDirChanges event for '{}'", filename);
                    scheduleCallback();
                }

                if (info->NextEntryOffset == 0) {
                    break;
                }
                ptr += info->NextEntryOffset;
            }
        }

        if (!issue_read()) {
            break;
        }
    }

    CancelIo(dir_handle);
    CloseHandle(overlapped.hEvent);
    CloseHandle(dir_handle);
}

#endif // _WIN32

} // namespace config
} // namespace themis
