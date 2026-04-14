/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gocryptfs_backend.cpp                              ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 07:07:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     650                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8e5567bf5e  2026-03-24  feat(user_storage_encrypted): v0.1.0 stdin key delivery, ... ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "gocryptfs_backend.hpp"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

namespace themis {
namespace plugins {
namespace user_storage {

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

Result<void> GocryptfsBackend::initialize(const std::string& config_json) {
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
    // Use executeCommandSafe via a const-safe helper via Impl
    std::vector<std::string> args = {impl_->gocryptfs_binary, "-version"};
    auto result = const_cast<GocryptfsBackend*>(this)->executeCommandSafe(args);
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
    
    // Build hex key string for stdin delivery
    std::ostringstream hex_key;
    for (uint8_t byte : key_material) {
        hex_key << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    // Initialize gocryptfs container — deliver key via stdin (no temp file on disk)

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

    auto result = deliverKeyViaStdin(args, hex_key.str());
    
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
    
    // Build hex key string for stdin delivery
    std::ostringstream hex_key;
    for (uint8_t byte : key_material) {
        hex_key << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    // Mount gocryptfs — deliver key via stdin (no temp file on disk)

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

    auto result = deliverKeyViaStdin(args, hex_key.str());
    
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

Result<std::string> GocryptfsBackend::createPasswordFile(
    const std::vector<uint8_t>& key_material
) {
    // Create secure temporary file outside encrypted directory
    char temp_template[] = "/tmp/gocryptfs_key_XXXXXX";
    int fd = mkstemp(temp_template);
    if (fd == -1) {
        return Result<std::string>::error("Failed to create secure temporary file");
    }
    
    // Set restrictive permissions (600) before writing
    if (fchmod(fd, 0600) != 0) {
        close(fd);
        unlink(temp_template);
        return Result<std::string>::error("Failed to set password file permissions");
// ---------------------------------------------------------------------------
// Stdin key delivery (Feature 1)
// ---------------------------------------------------------------------------

Result<void> GocryptfsBackend::deliverKeyViaStdin(
    int write_fd,
    const std::vector<uint8_t>& key_material
) {
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

    while (written < total) {
        ssize_t n = write(write_fd, ptr + written, static_cast<size_t>(total - written));
        if (n < 0) {
            if (errno == EINTR) continue;
            // Securely clear before returning error.
            explicit_bzero(hex_key.data(), hex_key.size());
            return Result<void>::error("Failed to write key to stdin pipe");
        }
        written += n;
    }

    // Securely clear key material from the stack buffer.
    explicit_bzero(hex_key.data(), hex_key.size());
    return Result<void>();
}

Result<std::string> GocryptfsBackend::executeCommandWithStdin(
    const std::vector<std::string>& args,
    const std::vector<uint8_t>& key_material
) {
    if (args.empty()) {
        return Result<std::string>::error("Empty command arguments");
    }

    // stdout pipe: parent reads child output.
    int stdout_pipe[2];
    if (pipe(stdout_pipe) != 0) {
        return Result<std::string>::error("Failed to create stdout pipe");
    }

    // stdin pipe: parent writes key to child's stdin.
    int stdin_pipe[2];
    if (pipe(stdin_pipe) != 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return Result<std::string>::error("Failed to create stdin pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        return Result<std::string>::error("Failed to fork process");
    }

    if (pid == 0) {
        // Child: wire up stdin and stdout/stderr, then exec.
        close(stdin_pipe[1]);   // close write end of stdin pipe in child
        close(stdout_pipe[0]);  // close read end of stdout pipe in child

        if (dup2(stdin_pipe[0], STDIN_FILENO) == -1)  { _exit(127); }
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) { _exit(127); }
        if (dup2(stdout_pipe[1], STDERR_FILENO) == -1) { _exit(127); }

        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

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
    close(stdin_pipe[0]);   // read end belongs to child
    close(stdout_pipe[1]);  // write end belongs to child

    // Deliver key via stdin pipe BEFORE reading stdout to prevent deadlock.
    // A hex-encoded 32-byte key is 64 bytes — well within the pipe buffer.
    auto deliver_result = deliverKeyViaStdin(stdin_pipe[1], key_material);
    close(stdin_pipe[1]);  // Signal EOF so gocryptfs sees end-of-passphrase.
    if (deliver_result.isError()) {
        waitpid(pid, nullptr, 0);
        return Result<std::string>::error(deliver_result.error());
    }

    // Read child output.
    std::string output;
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytes_read);
    }
    close(stdout_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        return Result<std::string>::error(
            "Command failed with exit code " + std::to_string(exit_code) + ": " + output
        );
    }

    return Result<std::string>(output);
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

        // Write salt as raw bytes to the salt file (mode 0600).
        int fd = open(salt_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (fd == -1) {
            return Result<std::vector<uint8_t>>::error(
                "Failed to create salt file: " + salt_file
            );
        }
        ssize_t written = write(fd, salt.data(), salt.size());
        close(fd);
        if (written != static_cast<ssize_t>(salt.size())) {
            return Result<std::vector<uint8_t>>::error(
                "Failed to write salt to: " + salt_file
            );
        }
    } else {
        // Load existing salt.
        int fd = open(salt_file.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd == -1) {
            return Result<std::vector<uint8_t>>::error(
                "Salt file not found (was container created with KDF?): " + salt_file
            );
        }
        salt.resize(16);
        ssize_t n = read(fd, salt.data(), salt.size());
        close(fd);
        unlink(temp_template);
        return Result<std::string>::error("Failed to write key to temporary file");
    }
    
    close(fd);
    return Result<std::string>(std::string(temp_template));
}

Result<std::string> GocryptfsBackend::deliverKeyViaStdin(
    const std::vector<std::string>& args,
    std::string key_hex
) {
    auto result = executeCommandWithStdin(args, key_hex);
    // Securely zero key material from memory after use
    explicit_bzero(key_hex.data(), key_hex.size());
    return result;
}

Result<std::string> GocryptfsBackend::executeCommandWithStdin(
        if (n != static_cast<ssize_t>(salt.size())) {
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
// Remaining helpers
// ---------------------------------------------------------------------------

Result<std::string> GocryptfsBackend::executeCommand(
    const std::string& command,
    const std::vector<std::string>& args,
    const std::string& stdin_data
) {
    if (args.empty()) {
        return Result<std::string>::error("Empty command arguments");
    }

    // stdout/stderr pipe
    int out_pipe[2];
    if (pipe(out_pipe) != 0) {
        return Result<std::string>::error("Failed to create output pipe");
    }
    // stdin pipe
    int in_pipe[2];
    if (pipe(in_pipe) != 0) {
        close(out_pipe[0]);
        close(out_pipe[1]);
        return Result<std::string>::error("Failed to create stdin pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(out_pipe[0]); close(out_pipe[1]);
        close(in_pipe[0]);  close(in_pipe[1]);
        return Result<std::string>::error("Failed to fork process");
    }

    if (pid == 0) {
        // Child process
        close(in_pipe[1]);   // close write end of stdin pipe
        close(out_pipe[0]);  // close read end of stdout pipe

        dup2(in_pipe[0],  STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(out_pipe[1], STDERR_FILENO);
        close(in_pipe[0]);
        close(out_pipe[1]);

        std::vector<char*> c_args;
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        _exit(127);
    }

    // Parent process
    close(in_pipe[0]);   // close read end of stdin pipe
    close(out_pipe[1]);  // close write end of stdout pipe

    // Write stdin_data to child's stdin
    const char* ptr = stdin_data.c_str();
    size_t remaining = stdin_data.size();
    while (remaining > 0) {
        ssize_t written = write(in_pipe[1], ptr, remaining);
        if (written < 0) {
            close(in_pipe[1]);
            close(out_pipe[0]);
            waitpid(pid, nullptr, 0);
            return Result<std::string>::error("Failed to write to stdin pipe");
        }
        ptr += written;
        remaining -= static_cast<size_t>(written);
    }
    close(in_pipe[1]); // signal EOF to child

    // Read child output
    std::string output;
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(out_pipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytes_read);
    }
    close(out_pipe[0]);

    int status;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        return Result<std::string>::error(
            "Command failed with exit code " + std::to_string(exit_code) + ": " + output
        );
    }

    return Result<std::string>(output);
}

Result<std::string> GocryptfsBackend::executeCommandSafe(
    const std::vector<std::string>& args
) {
    if (args.empty()) {
        return Result<std::string>::error("Empty command arguments");
    }
    
    // Use fork/exec for safe execution without shell
    int pipe_fd[2];
    if (pipe(pipe_fd) != 0) {
        return Result<std::string>::error("Failed to create pipe");
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return Result<std::string>::error("Failed to fork process");
    }
    
    if (pid == 0) {
        // Child process
        close(pipe_fd[0]); // Close read end
        
        // Redirect stdout and stderr to pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        close(pipe_fd[1]);
        
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
    close(pipe_fd[1]); // Close write end
    
    // Read output
    std::string output;
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytes_read);
    }
    close(pipe_fd[0]);
    
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
}

bool GocryptfsBackend::directoryExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool GocryptfsBackend::createDirectory(const std::string& path) {
    // Create directory with permissions 0700
    return mkdir(path.c_str(), 0700) == 0;
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
