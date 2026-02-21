/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gocryptfs_backend.hpp                              ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     116                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
    Result<void> createPasswordFile(
        const std::string& path,
        const std::vector<uint8_t>& key_material
    );
    
    Result<std::string> executeCommand(
        const std::string& command,
        const std::vector<std::string>& args,
        const std::string& stdin_data = ""
    );
    
    Result<std::string> executeCommandSafe(
        const std::vector<std::string>& args
    );
    
    bool directoryExists(const std::string& path);
    bool createDirectory(const std::string& path);
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
