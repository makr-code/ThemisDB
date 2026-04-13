/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gocryptfs_backend.hpp                              ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:22:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     199                                            ║
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

#pragma once

#include "encryption_backend_interface.hpp"
#include "key_derivation_service.hpp"
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
 * Key delivery: key material is delivered to gocryptfs via a stdin pipe
 * ("-passfile /dev/stdin"), never via a temporary file on disk.
 *
 * Optional KDF: when a KeyDerivationService is supplied the raw key_material
 * argument is treated as the master key; the per-container derived key is
 * computed with Argon2id and used as the gocryptfs passphrase.
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
    /**
     * @brief Construct backend without KDF (key_material used directly).
     */
    GocryptfsBackend();

    /**
     * @brief Construct backend with optional Argon2id KDF.
     *
     * @param kdf_service  Pointer to KDF service (not owned; must outlive this
     *                     object).  Pass nullptr to use key_material directly.
     */
    explicit GocryptfsBackend(KeyDerivationService* kdf_service);

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

    // --- Stdin-based key delivery (Feature 1) ---

    /**
     * @brief Fork, exec gocryptfs and deliver the key via stdin pipe.
     *
     * The child receives the key as a hex string on STDIN.  gocryptfs is
     * invoked with "-passfile /dev/stdin" so that no key material touches
     * the filesystem.  The pipe write buffer is cleared with explicit_bzero
     * after the write completes.
     *
     * @param args        Full argv for execvp (args[0] = executable path)
     * @param key_material Key bytes to hex-encode and pipe to stdin
     * @return            Child stdout/stderr output on success, error otherwise
     */
    Result<std::string> executeCommandWithStdin(
        const std::vector<std::string>& args,
        const std::vector<uint8_t>& key_material
    );

    /**
     * @brief Write hex-encoded key to write_fd, then clear the buffer.
     *
     * @param write_fd     Write end of the stdin pipe (closed on return)
     * @param key_material Key bytes
     */
    Result<void> deliverKeyViaStdin(
        int write_fd,
        const std::vector<uint8_t>& key_material
    );

    // --- Legacy / internal helpers ---

    /** @deprecated Use executeCommandSafe() */
    Result<std::string> executeCommand(
        const std::string& command,
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

    // --- KDF helpers (Feature 2) ---

    /**
     * @brief Derive or return key for a container.
     *
     * If a KDF service is configured: reads/writes the per-container salt
     * file ("{encrypted_dir}/.themis_kdf_salt"), then derives the key via
     * Argon2id.  Otherwise returns key_material unchanged.
     *
     * @param encrypted_dir  Ciphertext directory (salt file lives here)
     * @param key_material   Master key (or direct key when no KDF)
     * @param create_salt    When true, generate and persist a new salt
     */
    Result<std::vector<uint8_t>> resolveKey(
        const std::string& encrypted_dir,
        const std::vector<uint8_t>& key_material,
        bool create_salt
    );

    bool directoryExists(const std::string& path);
    bool createDirectory(const std::string& path);
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
