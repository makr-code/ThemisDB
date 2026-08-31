/**
 * @file command_timeout_manager.hpp
 * @brief Command execution timeout management with graceful recovery
 * 
 * Provides robust timeout handling for child process execution, including:
 * - Deadline-based timeout tracking using std::chrono::steady_clock
 * - Graceful process termination on timeout
 * - Resource cleanup and error reporting
 * - No silent hangs or resource leaks
 */

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Manages command execution with timeout enforcement
 * 
 * Uses steady_clock (recommended per benchmark hygiene) to track execution
 * deadlines. When a timeout occurs, the process is terminated gracefully
 * (SIGTERM) followed by forceful termination (SIGKILL) if needed.
 */
class CommandTimeoutManager {
public:
    /**
     * @brief Create a timeout manager with specified duration
     * @param timeout Duration until timeout occurs
     */
    explicit CommandTimeoutManager(std::chrono::milliseconds timeout)
        : timeout_(timeout),
          start_time_(std::chrono::steady_clock::now()) {}
    
    /**
     * @brief Check if timeout has been exceeded
     * @return true if timeout has expired
     */
    bool hasTimedOut() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_
        );
        return elapsed >= timeout_;
    }
    
    /**
     * @brief Get remaining time until timeout
     * @return Duration remaining, or 0ms if already timed out
     */
    std::chrono::milliseconds getRemaining() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_
        );
        if (elapsed >= timeout_) {
            return std::chrono::milliseconds(0);
        }
        return timeout_ - elapsed;
    }
    
    /**
     * @brief Get elapsed time since creation
     * @return Elapsed duration
     */
    std::chrono::milliseconds getElapsed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_
        );
    }
    
    /**
     * @brief Terminate a process with grace period
     * 
     * Attempts SIGTERM first, waits briefly, then SIGKILL if needed.
     * Always cleans up child process via waitpid.
     * 
     * @param pid Process ID to terminate
     * @return Exit status from waitpid, or -1 if error
     */
    static int terminateProcess(int pid) {
#ifdef _WIN32
        HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
        if (process == nullptr) {
            return -1;
        }

        DWORD wait_result = WaitForSingleObject(process, 100);
        if (wait_result == WAIT_TIMEOUT) {
            TerminateProcess(process, 1);
        }

        DWORD exit_code = 0;
        GetExitCodeProcess(process, &exit_code);
        CloseHandle(process);
        return static_cast<int>(exit_code);
#else
        // First try SIGTERM for graceful shutdown
        if (kill(pid, SIGTERM) == 0) {
            // Wait 100ms for graceful termination
            std::chrono::milliseconds grace(100);
            auto start = std::chrono::steady_clock::now();

            while (std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start) < grace) {
                int status = 0;
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (result == pid) {
                    return status;  // Process exited
                }
                if (result < 0) {
                    return -1;  // Error
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Grace period expired, force kill
        kill(pid, SIGKILL);

        // Ensure process is reaped
        int status = 0;
        waitpid(pid, &status, 0);
        return status;
#endif
    }
    
private:
    std::chrono::milliseconds timeout_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
