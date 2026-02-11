#include "../include/gocryptfs_backend.hpp"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>
#include <unistd.h>

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
    
    // Create temporary password file
    std::string password_file = encrypted_dir + "/.gocryptfs.password";
    auto pw_result = createPasswordFile(password_file, key_material);
    if (pw_result.isError()) {
        return pw_result;
    }
    
    // Initialize gocryptfs container
    // gocryptfs -init -passfile <file> <encrypted_dir>
    auto result = executeCommand(
        impl_->gocryptfs_binary,
        {"-init", "-passfile", password_file, encrypted_dir}
    );
    
    // Clean up password file
    std::remove(password_file.c_str());
    
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
    
    // Create temporary password file
    std::string password_file = encrypted_dir + "/.gocryptfs.password";
    auto pw_result = createPasswordFile(password_file, key_material);
    if (pw_result.isError()) {
        return pw_result;
    }
    
    // Mount gocryptfs
    // gocryptfs -passfile <file> <encrypted_dir> <mount_point>
    auto result = executeCommand(
        impl_->gocryptfs_binary,
        {"-passfile", password_file, encrypted_dir, mount_point}
    );
    
    // Clean up password file
    std::remove(password_file.c_str());
    
    if (result.isError()) {
        return Result<void>::error("Failed to mount gocryptfs container: " + result.error());
    }
    
    return Result<void>();
}

Result<void> GocryptfsBackend::unmountContainer(const std::string& mount_point) {
    if (!isMounted(mount_point)) {
        return Result<void>(); // Not mounted is success
    }
    
    // Unmount using fusermount -u (Linux) or umount (macOS/BSD)
#ifdef __linux__
    auto result = executeCommand("fusermount", {"-u", mount_point});
#else
    auto result = executeCommand("umount", {mount_point});
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
    // Write key material to file (base64 or hex)
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<void>::error("Failed to create password file: " + path);
    }
    
    // Set restrictive permissions (600)
    chmod(path.c_str(), 0600);
    
    // Write key as hex string
    for (uint8_t byte : key_material) {
        file << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    
    file.close();
    return Result<void>();
}

Result<std::string> GocryptfsBackend::executeCommand(
    const std::string& command,
    const std::vector<std::string>& args,
    const std::string& stdin_data
) {
    // Build command string
    std::ostringstream cmd;
    cmd << command;
    for (const auto& arg : args) {
        cmd << " " << arg;
    }
    
    // Add stderr redirect
    cmd << " 2>&1";
    
    // Execute command
    std::array<char, 128> buffer;
    std::string output;
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        return Result<std::string>::error("Failed to execute command: " + command);
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    
    int exit_code = pclose(pipe);
    
    if (exit_code != 0) {
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
