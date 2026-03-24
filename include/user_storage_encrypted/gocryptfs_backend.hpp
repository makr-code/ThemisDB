/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gocryptfs_backend.hpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-16 04:11:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     112                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9ab72c508  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250db  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "encryption_backend_interface.hpp"
#include <memory>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief gocryptfs encryption backend implementation
 * 
 * gocryptfs provides FUSE-based filesystem encryption with:
 * - AES-256-GCM encryption
 * - Filename encryption
 * - Per-file IV (initialization vector)
 * - Authenticated encryption
 * 
 * Requirements:
 * - gocryptfs binary in PATH (>= v2.0)
 * - FUSE support (libfuse on Linux, macFUSE on macOS)
 * - Appropriate permissions for mount operations
 * 
 * Platform Support:
 * - Linux: ✅ Full support
 * - macOS: ✅ Via macFUSE
 * - Windows: ⚠️ Via WinFsp/Dokany (experimental)
 * - Raspberry Pi: ✅ ARM64 support
 */
class GocryptfsBackend : public EncryptionBackendInterface {
public:
    GocryptfsBackend();
    ~GocryptfsBackend() override;
    
    // EncryptionBackendInterface implementation
    Result<void> initialize(const std::string& config_json) override;
    
    Result<void> createContainer(
        const std::string& encrypted_dir,
        const std::string& mount_point,
        const std::vector<uint8_t>& key_material
    ) override;
    
    Result<void> mountContainer(
        const std::string& encrypted_dir,
        const std::string& mount_point,
        const std::vector<uint8_t>& key_material
    ) override;
    
    Result<void> unmountContainer(
        const std::string& mount_point
    ) override;
    
    bool isMounted(const std::string& mount_point) override;
    
    std::string getBackendName() const override {
        return "gocryptfs";
    }
    
    std::string getBackendVersion() const override;
    
    Result<void> checkAvailability() override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Helper methods

    /// Create a secure temporary password file; returns the path.
    Result<std::string> createPasswordFile(
        const std::vector<uint8_t>& key_material
    );

    /// Execute command and pipe key_hex to its stdin; zeroes key_hex on return.
    Result<std::string> deliverKeyViaStdin(
        const std::vector<std::string>& args,
        std::string key_hex
    );

    /// Execute command with arbitrary stdin data.
    Result<std::string> executeCommandWithStdin(
        const std::vector<std::string>& args,
        const std::string& stdin_data
    );

    /// Execute command safely via fork/execvp (no shell).
    Result<std::string> executeCommandSafe(
        const std::vector<std::string>& args
    );

    bool directoryExists(const std::string& path);
    bool createDirectory(const std::string& path);
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
