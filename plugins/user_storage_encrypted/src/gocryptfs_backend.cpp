/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gocryptfs_backend.cpp                              ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   71.0/100                                       ║
    • Total Lines:     352                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "../include/gocryptfs_backend.hpp"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <sstream>
#include <fstream>
#include <iomanip>
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
    auto result = executeCommand("which", {"gocryptfs"});
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
    auto result = const_cast<GocryptfsBackend*>(this)->executeCommand(
        "gocryptfs", {"-version"}
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
    
    // Create secure temporary password file
    std::string password_file;
    auto pw_result = createPasswordFile(password_file, key_material);
    if (pw_result.isError()) {
        return pw_result;
    }
    
    // Initialize gocryptfs container
    // Using vector for safe argument passing
    std::vector<std::string> args = {
        impl_->gocryptfs_binary,
        "-init",
        "-passfile", password_file,
        encrypted_dir
    };
    
    auto result = executeCommandSafe(args);
    
    // Clean up password file securely
    unlink(password_file.c_str());
    
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
    
    // Create secure temporary password file
    std::string password_file;
    auto pw_result = createPasswordFile(password_file, key_material);
    if (pw_result.isError()) {
        return pw_result;
    }
    
    // Mount gocryptfs using safe argument passing
    std::vector<std::string> args = {
        impl_->gocryptfs_binary,
        "-passfile", password_file,
        encrypted_dir,
        mount_point
    };
    
    auto result = executeCommandSafe(args);
    
    // Clean up password file securely
    unlink(password_file.c_str());
    
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
    auto result = executeCommand("mount", {});
    if (result.isSuccess()) {
        return result.value().find(mount_point) != std::string::npos;
    }
    return false;
#endif
}

Result<void> GocryptfsBackend::createPasswordFile(
    const std::string& path,
    const std::vector<uint8_t>& key_material
) {
    // Create secure temporary file outside encrypted directory
    char temp_template[] = "/tmp/gocryptfs_key_XXXXXX";
    int fd = mkstemp(temp_template);
    if (fd == -1) {
        return Result<void>::error("Failed to create secure temporary file");
    }
    
    // Set restrictive permissions (600) before writing
    if (fchmod(fd, 0600) != 0) {
        close(fd);
        unlink(temp_template);
        return Result<void>::error("Failed to set password file permissions");
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
        return Result<void>::error("Failed to write key to temporary file");
    }
    
    close(fd);
    
    // Store the temporary file path for later cleanup
    const_cast<std::string&>(path) = temp_template;
    
    return Result<void>();
}

Result<std::string> GocryptfsBackend::executeCommand(
    const std::string& command,
    const std::vector<std::string>& args,
    const std::string& stdin_data
) {
    // Deprecated: Use executeCommandSafe instead
    // This version is kept for backward compatibility with simple commands
    std::vector<std::string> full_args = {command};
    full_args.insert(full_args.end(), args.begin(), args.end());
    return executeCommandSafe(full_args);
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
