/**
 * @file gocryptfs_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=22, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: gocryptfs_backend.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 634
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=7, M=44, L=0
 * PR History (last 5): #4111 feat: migrate plugins to fl... (2026-03-12) | #1200 Add Multi-Level Encrypted U... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "gocryptfs_backend.hpp"
#include "timed_file_operation.hpp"
#include "pipe_guard.hpp"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <string.h>
#include <filesystem>

namespace themis {
namespace plugins {
namespace user_storage {

namespace {
inline void secureZero(void* ptr, size_t len) {
    volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
    while (len--) {
        *p++ = 0;
    }
}
}

struct GocryptfsBackend::Impl {
    std::string gocryptfs_binary;
    bool initialized;
    KeyDerivationService* kdf_service;  // not owned; may be nullptr

    Impl() : gocryptfs_binary("gocryptfs"), initialized(false), kdf_service(nullptr) {}
};

GocryptfsBackend::GocryptfsBackend()
    : impl_(std::make_unique<Impl>()) {
}

GocryptfsBackend::GocryptfsBackend(KeyDerivationService* kdf_service)
    : impl_(std::make_unique<Impl>()) {
    impl_->kdf_service = kdf_service;
}

GocryptfsBackend::~GocryptfsBackend() = default;

Result<void> GocryptfsBackend::initialize([[maybe_unused]] const std::string& config_json) {
    // For now, simple initialization
    // Could parse config_json to customize gocryptfs_binary path
    impl_->initialized = true;
    return Result<void>();
}

Result<void> GocryptfsBackend::checkAvailability() {
    // Check if gocryptfs is available in PATH
    auto result = executeCommandSafe({"which", "gocryptfs"});
    if (result.isError()) {
        return Result<void>::error(
            "gocryptfs not found in PATH. Please install: apt-get install gocryptfs"
        );
    }
    
    // Check FUSE availability
#ifdef __linux__
    struct stat st;
    if (stat("/dev/fuse", &st) != 0) {
        return Result<void>::error(
            "FUSE not available. Please load fuse kernel module or install fuse"
        );
    }
#endif
    
    return Result<void>();
}

std::string GocryptfsBackend::getBackendVersion() const {
    auto result = const_cast<GocryptfsBackend*>(this)->executeCommandSafe(
        {"gocryptfs", "-version"}
    );
    if (result.isSuccess()) {
        return result.value();
    }
    return "unknown";
}

Result<void> GocryptfsBackend::createContainer(
    const std::string& encrypted_dir,
    const std::string& mount_point,
    const std::vector<uint8_t>& key_material
) {
    // Create directories if they don't exist
    if (!directoryExists(encrypted_dir)) {
        if (!createDirectory(encrypted_dir)) {
            return Result<void>::error("Failed to create encrypted directory: " + encrypted_dir);
        }
    }
    
    if (!directoryExists(mount_point)) {
        if (!createDirectory(mount_point)) {
            return Result<void>::error("Failed to create mount point: " + mount_point);
        }
    }

    // Resolve key: derive via Argon2id when KDF is configured; also writes
    // the per-container salt file on first use.
    auto key_result = resolveKey(encrypted_dir, key_material, /*create_salt=*/true);
    if (key_result.isError()) {
        return Result<void>::error(key_result.error());
    }
    const std::vector<uint8_t>& effective_key = key_result.value();

    // Initialize gocryptfs container via stdin key delivery.
    std::vector<std::string> args = {
        impl_->gocryptfs_binary,
        "-init",
        "-passfile", "/dev/stdin",
        encrypted_dir
    };

    auto result = executeCommandWithStdin(args, effective_key);
    if (result.isError()) {
        return Result<void>::error("Failed to initialize gocryptfs container: " + result.error());
    }

    return Result<void>();
}

Result<void> GocryptfsBackend::mountContainer(
    const std::string& encrypted_dir,
    const std::string& mount_point,
    const std::vector<uint8_t>& key_material
) {
    // Check if already mounted
    if (isMounted(mount_point)) {
        return Result<void>(); // Already mounted is success
    }

    // Resolve key: derive via Argon2id when KDF is configured; reads the
    // existing per-container salt file.
    auto key_result = resolveKey(encrypted_dir, key_material, /*create_salt=*/false);
    if (key_result.isError()) {
        return Result<void>::error(key_result.error());
    }
    const std::vector<uint8_t>& effective_key = key_result.value();

    // Mount gocryptfs via stdin key delivery.
    std::vector<std::string> args = {
        impl_->gocryptfs_binary,
        "-passfile", "/dev/stdin",
        encrypted_dir,
        mount_point
    };

    auto result = executeCommandWithStdin(args, effective_key);
    if (result.isError()) {
        return Result<void>::error("Failed to mount gocryptfs container: " + result.error());
    }

    return Result<void>();
}

Result<void> GocryptfsBackend::unmountContainer(const std::string& mount_point) {
    if (!isMounted(mount_point)) {
        return Result<void>(); // Not mounted is success
    }
    
    // Unmount using safe argument passing
#ifdef __linux__
    std::vector<std::string> args = {"fusermount", "-u", mount_point};
    auto result = executeCommandSafe(args);
#else
    std::vector<std::string> args = {"umount", mount_point};
    auto result = executeCommandSafe(args);
#endif
    
    if (result.isError()) {
        return Result<void>::error("Failed to unmount container: " + result.error());
    }
    
    return Result<void>();
}

bool GocryptfsBackend::isMounted(const std::string& mount_point) {
    // Check /proc/mounts (Linux) or mount output
#ifdef __linux__
    std::ifstream mounts("/proc/mounts");
    std::string line;
    while (std::getline(mounts, line)) {
        if (line.find(mount_point) != std::string::npos) {
            return true;
        }
    }
    return false;
#else
    // For macOS/BSD, check mount output
    auto result = executeCommandSafe({"mount"});
    if (result.isSuccess()) {
        return result.value().find(mount_point) != std::string::npos;
    }
    return false;
#endif
}

// ---------------------------------------------------------------------------
// Stdin key delivery (Feature 1)
// ---------------------------------------------------------------------------

Result<void> GocryptfsBackend::deliverKeyViaStdin(
    int write_fd,
    const std::vector<uint8_t>& key_material
) {
#if defined(_WIN32)
    (void)write_fd;
    (void)key_material;
    return Result<void>::error("gocryptfs backend is not supported on Windows");
#else
    // Build hex string + newline so gocryptfs terminates the read cleanly.
    std::string hex_key;
    hex_key.reserve(key_material.size() * 2 + 1);
    for (uint8_t byte : key_material) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned>(byte));
        hex_key += buf;
    }
    hex_key += '\n';

    const char* ptr   = hex_key.data();
    ssize_t     total = static_cast<ssize_t>(hex_key.size());
    ssize_t     written = 0;

    // Use timed I/O operations to prevent indefinite blocking on pipe
    TimedFileOperation timed_io(write_fd, std::chrono::seconds(5));

    while (written < total) {
        auto n = timed_io.write(ptr + written, static_cast<size_t>(total - written));
        if (!n.has_value()) {
            // Timeout or I/O error
            secureZero(hex_key.data(), hex_key.size());
            if (errno == EAGAIN) {
                return Result<void>::error("Timeout: write to key stdin pipe blocked");
            }
            return Result<void>::error("Failed to write key to stdin pipe");
        }
        if (n.value() < 0) {
            if (errno == EINTR) continue;
            // Securely clear before returning error.
            secureZero(hex_key.data(), hex_key.size());
            return Result<void>::error("Failed to write key to stdin pipe");
        }
        written += n.value();
    }

    // Securely clear key material from the stack buffer.
    secureZero(hex_key.data(), hex_key.size());
    return Result<void>();
#endif
}

Result<std::string> GocryptfsBackend::executeCommandWithStdin(
    const std::vector<std::string>& args,
    const std::vector<uint8_t>& key_material
) {
#if defined(_WIN32)
    (void)args;
    (void)key_material;
    return Result<std::string>::error("gocryptfs backend is not supported on Windows");
#else
    if (args.empty()) {
        return Result<std::string>::error("Empty command arguments");
    }

    // stdout pipe: parent reads child output.
    auto stdout_pipe = PipeGuard::create();
    if (!stdout_pipe.isValid()) {
        return Result<std::string>::error("Failed to create stdout pipe");
    }

    // stdin pipe: parent writes key to child's stdin.
    auto stdin_pipe = PipeGuard::create();
    if (!stdin_pipe.isValid()) {
        return Result<std::string>::error("Failed to create stdin pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
        return Result<std::string>::error("Failed to fork process");
    }

    if (pid == 0) {
        // Child: wire up stdin and stdout/stderr, then exec.
        stdin_pipe.closeWrite();   // close write end of stdin pipe in child
        stdout_pipe.closeRead();   // close read end of stdout pipe in child

        if (dup2(stdin_pipe.readFd(), STDIN_FILENO) == -1)  { _exit(127); }
        if (dup2(stdout_pipe.writeFd(), STDOUT_FILENO) == -1) { _exit(127); }
        if (dup2(stdout_pipe.writeFd(), STDERR_FILENO) == -1) { _exit(127); }

        stdin_pipe.closeRead();
        stdout_pipe.closeWrite();

        std::vector<char*> c_args;
        c_args.reserve(args.size() + 1);
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        _exit(127);
    }

    // Parent: close child-only ends.
    stdin_pipe.closeRead();   // read end belongs to child
    stdout_pipe.closeWrite();  // write end belongs to child

    // Deliver key via stdin pipe BEFORE reading stdout to prevent deadlock.
    // A hex-encoded 32-byte key is 64 bytes — well within the pipe buffer.
    auto deliver_result = deliverKeyViaStdin(stdin_pipe.writeFd(), key_material);
    stdin_pipe.closeWrite();  // Signal EOF so gocryptfs sees end-of-passphrase.
    if (deliver_result.isError()) {
        waitpid(pid, nullptr, 0);
        return Result<std::string>::error(deliver_result.error());
    }

    // Read child output with timeout.
    std::string output;
    char buffer[1024];
    TimedFileOperation read_io(stdout_pipe.readFd(), std::chrono::seconds(10));
    
    while (true) {
        auto bytes_read = read_io.read(buffer, sizeof(buffer));
        if (!bytes_read.has_value()) {
            if (errno == EAGAIN) {
                // Timeout on read
                waitpid(pid, nullptr, 0);
                return Result<std::string>::error("Timeout: reading from child process");
            }
            // Error
            break;
        }
        if (bytes_read.value() <= 0) {
            break;  // EOF
        }
        output.append(buffer, bytes_read.value());
    }
    
    stdout_pipe.closeRead();

    int status = 0;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        return Result<std::string>::error(
            "Command failed with exit code " + std::to_string(exit_code) + ": " + output
        );
    }

    return Result<std::string>(output);
#endif
}

// ---------------------------------------------------------------------------
// KDF helper (Feature 2)
// ---------------------------------------------------------------------------

Result<std::vector<uint8_t>> GocryptfsBackend::resolveKey(
    const std::string& encrypted_dir,
    const std::vector<uint8_t>& key_material,
    bool create_salt
) {
    if (!impl_->kdf_service) {
        // No KDF configured: use key_material directly.
        return Result<std::vector<uint8_t>>(key_material);
    }

    const std::string salt_file = encrypted_dir + "/.themis_kdf_salt";

    std::vector<uint8_t> salt;

    if (create_salt) {
        // Generate a fresh 16-byte salt and persist it.
        try {
            salt = impl_->kdf_service->generateSalt(16);
        } catch (const std::exception& ex) {
            return Result<std::vector<uint8_t>>::error(
                std::string("Salt generation failed: ") + ex.what()
            );
        }

        // Write salt as raw bytes to the salt file.
        std::ofstream out(salt_file, std::ios::binary | std::ios::trunc);
        if (!out) {
            return Result<std::vector<uint8_t>>::error(
                "Failed to create salt file: " + salt_file
            );
        }
        out.write(reinterpret_cast<const char*>(salt.data()), static_cast<std::streamsize>(salt.size()));
        if (!out) {
            return Result<std::vector<uint8_t>>::error(
                "Failed to write salt to: " + salt_file
            );
        }
    } else {
        // Load existing salt.
        std::ifstream in(salt_file, std::ios::binary);
        if (!in) {
            return Result<std::vector<uint8_t>>::error(
                "Salt file not found (was container created with KDF?): " + salt_file
            );
        }
        salt.resize(16);
        in.read(reinterpret_cast<char*>(salt.data()), static_cast<std::streamsize>(salt.size()));
        if (!in) {
            return Result<std::vector<uint8_t>>::error(
                "Failed to read salt from: " + salt_file
            );
        }
    }

    try {
        // container_id = encrypted_dir path; user_id = empty (not in API).
        std::vector<uint8_t> derived = impl_->kdf_service->derive(
            key_material, "", encrypted_dir, salt
        );
        return Result<std::vector<uint8_t>>(std::move(derived));
    } catch (const std::exception& ex) {
        return Result<std::vector<uint8_t>>::error(
            std::string("Key derivation failed: ") + ex.what()
        );
    }
}


// ---------------------------------------------------------------------------
// Public string-based stdin helpers (test/integration interface)
// ---------------------------------------------------------------------------

Result<std::string> GocryptfsBackend::executeCommandWithStdin(
    const std::vector<std::string>& args,
    const std::string& stdin_data
) {
#if defined(_WIN32)
    (void)args;
    (void)stdin_data;
    return Result<std::string>::error("gocryptfs backend is not supported on Windows");
#else
    if (args.empty()) {
        return Result<std::string>::error("Empty command arguments");
    }

    // stdout pipe: parent reads child output.
    auto stdout_pipe = PipeGuard::create();
    if (!stdout_pipe.isValid()) {
        return Result<std::string>::error("Failed to create stdout pipe");
    }

    // stdin pipe: parent writes data to child's stdin.
    auto stdin_pipe = PipeGuard::create();
    if (!stdin_pipe.isValid()) {
        return Result<std::string>::error("Failed to create stdin pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
        return Result<std::string>::error("Failed to fork process");
    }

    if (pid == 0) {
        // Child: wire up stdin and stdout/stderr, then exec.
        stdin_pipe.closeWrite();
        stdout_pipe.closeRead();

        if (dup2(stdin_pipe.readFd(), STDIN_FILENO) == -1)  { _exit(127); }
        if (dup2(stdout_pipe.writeFd(), STDOUT_FILENO) == -1) { _exit(127); }
        if (dup2(stdout_pipe.writeFd(), STDERR_FILENO) == -1) { _exit(127); }

        stdin_pipe.closeRead();
        stdout_pipe.closeWrite();

        std::vector<char*> c_args;
        c_args.reserve(args.size() + 1);
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        _exit(127);
    }

    // Parent: close child-only ends.
    stdin_pipe.closeRead();
    stdout_pipe.closeWrite();

    // Write stdin_data to child's stdin with timeout.
    const char* ptr = stdin_data.c_str();
    size_t remaining = stdin_data.size();
    TimedFileOperation write_io(stdin_pipe.writeFd(), std::chrono::seconds(5));
    
    while (remaining > 0) {
        auto written = write_io.write(ptr, remaining);
        if (!written.has_value()) {
            if (errno == EAGAIN) {
                stdin_pipe.closeWrite();
                stdout_pipe.closeRead();
                waitpid(pid, nullptr, 0);
                return Result<std::string>::error("Timeout: write to stdin pipe blocked");
            }
            stdin_pipe.closeWrite();
            stdout_pipe.closeRead();
            waitpid(pid, nullptr, 0);
            return Result<std::string>::error("Failed to write to stdin pipe");
        }
        if (written.value() < 0) {
            if (errno == EINTR) continue;
            stdin_pipe.closeWrite();
            stdout_pipe.closeRead();
            waitpid(pid, nullptr, 0);
            return Result<std::string>::error("Failed to write to stdin pipe");
        }
        ptr += written.value();
        remaining -= static_cast<size_t>(written.value());
    }
    stdin_pipe.closeWrite();  // Signal EOF to child.

    // Read child output with timeout.
    std::string output;
    char buffer[1024];
    TimedFileOperation read_io(stdout_pipe.readFd(), std::chrono::seconds(10));
    
    while (true) {
        auto bytes_read = read_io.read(buffer, sizeof(buffer));
        if (!bytes_read.has_value()) {
            if (errno == EAGAIN) {
                // Timeout on read
                stdout_pipe.closeRead();
                waitpid(pid, nullptr, 0);
                return Result<std::string>::error("Timeout: reading from child process");
            }
            // Error
            break;
        }
        if (bytes_read.value() <= 0) {
            break;  // EOF
        }
        output.append(buffer, bytes_read.value());
    }
    stdout_pipe.closeRead();

    int status = 0;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        const int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        return Result<std::string>::error(
            "Command failed with exit code " + std::to_string(exit_code) + ": " + output
        );
    }

    return Result<std::string>(output);
#endif
}

Result<std::string> GocryptfsBackend::deliverKeyViaStdin(
    const std::vector<std::string>& args,
    const std::string& key_hex
) {
    auto result = executeCommandWithStdin(args, key_hex);
    // No caller-visible copy of key_hex to clear here; the internal write
    // buffer is zeroed by explicit_bzero inside executeCommandWithStdin would
    // require refactoring.  The key material is minimally held in this frame.
    return result;
}

// ---------------------------------------------------------------------------
// Remaining helpers
// ---------------------------------------------------------------------------

Result<std::string> GocryptfsBackend::executeCommandSafe(
    const std::vector<std::string>& args
) {
#if defined(_WIN32)
    (void)args;
    return Result<std::string>::error("gocryptfs backend is not supported on Windows");
#else
    if (args.empty()) {
        return Result<std::string>::error("Empty command arguments");
    }
    
    // Use fork/exec for safe execution without shell
    auto pipe = PipeGuard::create();
    if (!pipe.isValid()) {
        return Result<std::string>::error("Failed to create pipe");
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        return Result<std::string>::error("Failed to fork process");
    }
    
    if (pid == 0) {
        // Child process
        pipe.closeRead(); // Close read end
        
        // Redirect stdout and stderr to pipe
        dup2(pipe.writeFd(), STDOUT_FILENO);
        dup2(pipe.writeFd(), STDERR_FILENO);
        pipe.closeWrite();
        
        // Prepare arguments for execvp
        std::vector<char*> c_args;
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);
        
        // Execute command
        execvp(c_args[0], c_args.data());
        
        // If execvp returns, it failed
        _exit(127);
    }
    
    // Parent process
    pipe.closeWrite(); // Close write end
    
    // Read output with timeout
    std::string output;
    char buffer[1024];
    TimedFileOperation read_io(pipe.readFd(), std::chrono::seconds(10));
    
    while (true) {
        auto bytes_read = read_io.read(buffer, sizeof(buffer));
        if (!bytes_read.has_value()) {
            if (errno == EAGAIN) {
                // Timeout on read
                pipe.closeRead();
                waitpid(pid, nullptr, 0);
                return Result<std::string>::error("Timeout: reading command output");
            }
            // Error
            break;
        }
        if (bytes_read.value() <= 0) {
            break;  // EOF
        }
        output.append(buffer, bytes_read.value());
    }
    pipe.closeRead();
    
    // Wait for child to finish
    int status;
    waitpid(pid, &status, 0);
    
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        return Result<std::string>::error(
            "Command failed with exit code " + std::to_string(exit_code) + ": " + output
        );
    }
    
    return Result<std::string>(output);
#endif
}

bool GocryptfsBackend::directoryExists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool GocryptfsBackend::createDirectory(const std::string& path) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        return !ec;
    }
    std::filesystem::create_directories(path, ec);
    return !ec;
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
