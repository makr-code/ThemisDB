/**
 * @file multi_level_storage.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "encryption_backend_interface.hpp"
#include "user_models.hpp"
#include "security_level.hpp"
#include "../../include/plugins/plugin_interface.h"
#include "../../include/security/key_provider.h"
#include <atomic>
#include <memory>
#include <map>
#include <mutex>
#include <string>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Configuration for a single security level container
 */
struct LevelConfig {
    SecurityLevel level;
    std::string name;              // e.g., "vs-nfd"
    bool encrypted;                // false for "offen" level
    std::string base_path;         // For unencrypted: direct path
    std::string encrypted_dir;     // For encrypted: cipher dir
    std::string mount_point;       // For encrypted: mount point
    std::string backend;           // "gocryptfs", "fscrypt", etc.
    std::string key_id;            // Key identifier for KeyProvider
    std::string key_provider;      // "vault", "hsm", "mock"
    
    // Key provider specific config
    std::string vault_addr;
    std::string vault_mount;
    std::string hsm_library;
    uint32_t hsm_slot;
    std::string hsm_key_label;
    
    // Rotation config
    bool rotation_enabled;
    int rotation_interval_days;
    bool auto_rotate;
    std::string notification_email;
    
    LevelConfig() 
        : level(SecurityLevel::OFFEN)
        , encrypted(false)
        , backend("gocryptfs")
        , key_provider("vault")
        , hsm_slot(0)
        , rotation_enabled(false)
        , rotation_interval_days(90)
        , auto_rotate(false)
    {}
};

/**
 * @brief Prometheus-ready operational metrics for MultiLevelEncryptedStorage.
 *
 * All counters and gauges are std::atomic for lock-free reads from getMetricsText().
 */
struct StorageMetrics {
    std::atomic<int64_t> mounts_active{0};         ///< Currently mounted containers
    std::atomic<int64_t> mount_ops_total{0};        ///< Total mount operations
    std::atomic<int64_t> unmount_ops_total{0};      ///< Total unmount operations
    std::atomic<int64_t> key_rotations_total{0};    ///< Key rotation callbacks fired
    std::atomic<int64_t> container_size_bytes{0};   ///< Sum of container sizes on disk (bytes)

    // non-copyable (atomics are not copyable)
    StorageMetrics() = default;
    StorageMetrics(const StorageMetrics&) = delete;
    StorageMetrics& operator=(const StorageMetrics&) = delete;
};

/**
 * @brief Multi-Level Encrypted Storage Plugin
 * 
 * Provides encrypted user/group storage with:
 * - 4 security classification levels (offen, vs-nfd, geheim, streng-geheim)
 * - Separate encrypted containers per level
 * - Integration with HashiCorp Vault and HSM
 * - Automatic key rotation
 * - Zero-downtime key rotation
 * 
 * Architecture:
 * - offen: Unencrypted filesystem storage
 * - vs-nfd: gocryptfs + Vault (90-day rotation)
 * - geheim: gocryptfs + Vault (60-day rotation)
 * - streng-geheim: gocryptfs + HSM (30-day rotation)
 * 
 * Thread Safety: All methods are thread-safe
 */
class MultiLevelEncryptedStorage : public ::themis::plugins::IThemisPlugin {
public:
    MultiLevelEncryptedStorage();
    ~MultiLevelEncryptedStorage() override;
    
    // IThemisPlugin interface
    const char* getName() const override {
        return "user_storage_encrypted";
    }
    
    const char* getVersion() const override {
        return "1.0.0";
    }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;
    }
    
    PluginCapabilities getCapabilities() const override {
        PluginCapabilities caps;
        caps.thread_safe = true;
        caps.supports_batching = false;
        caps.supports_transactions = false;
        caps.gpu_accelerated = false;
        return caps;
    }
    
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return this; }
    
    // User Management API
    Result<void> createUser(const User& user, SecurityLevel level);
    Result<User> getUser(const std::string& user_id, SecurityLevel level);
    Result<void> updateUser(const User& user, SecurityLevel level);
    Result<void> deleteUser(const std::string& user_id, SecurityLevel level);
    Result<std::vector<User>> listUsers(SecurityLevel level);
    
    // Group Management API
    Result<void> createGroup(const Group& group, SecurityLevel level);
    Result<Group> getGroup(const std::string& group_id, SecurityLevel level);
    Result<void> updateGroup(const Group& group, SecurityLevel level);
    Result<void> deleteGroup(const std::string& group_id, SecurityLevel level);
    Result<std::vector<Group>> listGroups(SecurityLevel level);
    
    // Container Management
    Result<void> mountAll();
    Result<void> unmountAll();
    Result<void> mountLevel(SecurityLevel level);
    Result<void> unmountLevel(SecurityLevel level);
    Result<void> rotateKey(SecurityLevel level);
    
    // Health Check
    Result<HealthStatus> checkHealth();
    Result<HealthStatus> checkLevelHealth(SecurityLevel level);

    // ── Prometheus Metrics (v0.3.0) ──────────────────────────────────────────

    /**
     * @brief Emit Prometheus text format (v0.0.4) for encrypted-storage metrics.
     *
     * Exposed metric families:
     *  - `user_storage_mounts_active`          Gauge   Currently mounted containers
     *  - `user_storage_mount_operations_total`  Counter Total mount + unmount ops (label: operation)
     *  - `user_storage_key_rotations_total`     Counter Key rotation callbacks fired (label: level)
     *  - `user_storage_container_size_bytes`    Gauge   Sum of encrypted container sizes on disk
     *
     * Thread-safe (reads std::atomic values).
     */
    std::string getMetricsText() const;

    /**
     * @brief Manually record a key rotation event for @p level.
     *
     * Called automatically by `rotateKey()`; exposed for testing and for
     * callers that manage rotation outside this class.
     */
    void recordKeyRotation(SecurityLevel level);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Configuration helpers
    Result<void> loadConfiguration(const std::string& config_json);
    Result<void> validateConfiguration();
    
    // Container operations
    Result<void> initializeLevel(const LevelConfig& config);
    Result<void> mountLevel(const LevelConfig& config);
    Result<void> unmountLevel(const LevelConfig& config);

    /**
     * @brief Reconcile stale gocryptfs mounts left over from a previous crash.
     *
     * Scans /proc/mounts for any mount point that is a direct child of
     * @p base_path and is currently not among the configured mount points.
     * Each stale mount is unmounted via "fusermount -u" (Linux) / "umount"
     * (macOS).  A WARN-level log message is emitted per stale mount; if
     * unmounting fails the error is logged and startup continues — it is
     * never fatal.
     *
     * @param base_path  Directory prefix to scan (e.g. "/var/lib/themisdb").
     */
    void reconcileStaleMounts(const std::string& base_path);
    
    // Key provider management
    Result<std::shared_ptr<KeyProvider>> getKeyProvider(const LevelConfig& config);
    
    // File operations
    Result<void> writeUserFile(const std::string& path, const User& user);
    Result<User> readUserFile(const std::string& path);
    Result<void> writeGroupFile(const std::string& path, const Group& group);
    Result<Group> readGroupFile(const std::string& path);
    
    // Path helpers
    std::string getUserPath(SecurityLevel level, const std::string& user_id);
    std::string getGroupPath(SecurityLevel level, const std::string& group_id);
    std::string getBasePath(SecurityLevel level);

    // Stale mount cleanup
    void reconcileStaleMounts();
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
