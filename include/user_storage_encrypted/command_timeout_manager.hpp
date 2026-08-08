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
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

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
    static int terminateProcess(pid_t pid) {
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
                usleep(10000);  // Sleep 10ms before retry
            }
        }
        
        // Grace period expired, force kill
        kill(pid, SIGKILL);
        
        // Ensure process is reaped
        int status = 0;
        waitpid(pid, &status, 0);
        return status;
    }
    
private:
    std::chrono::milliseconds timeout_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
