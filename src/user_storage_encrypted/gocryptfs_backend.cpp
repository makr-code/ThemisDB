/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gocryptfs_backend.cpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-16 04:20:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   76.0/100                                       ║
    • Total Lines:     348                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9ab72c508  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250db  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
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

namespace themis {
namespace plugins {
namespace user_storage {

struct GocryptfsBackend::Impl {
    std::string gocryptfs_binary;
    bool initialized;
    
    Impl() : gocryptfs_binary("gocryptfs"), initialized(false) {}
};

GocryptfsBackend::GocryptfsBackend() 
    : impl_(std::make_unique<Impl>()) {
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
    std::vector<std::string> args = {
        impl_->gocryptfs_binary,
        "-init",
        "-passfile", "/dev/stdin",
        encrypted_dir
    };

    auto result = deliverKeyViaStdin(args, hex_key.str());
    
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
    std::vector<std::string> args = {
        impl_->gocryptfs_binary,
        "-passfile", "/dev/stdin",
        encrypted_dir,
        mount_point
    };

    auto result = deliverKeyViaStdin(args, hex_key.str());
    
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
    }
    
    // Write key as hex string
    std::ostringstream hex_key;
    for (uint8_t byte : key_material) {
        hex_key << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    
    std::string key_str = hex_key.str();
    if (write(fd, key_str.c_str(), key_str.size()) != static_cast<ssize_t>(key_str.size())) {
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
