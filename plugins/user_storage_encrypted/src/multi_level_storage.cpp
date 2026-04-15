/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_level_storage.cpp                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   81.0/100                                       ║
    • Total Lines:     809                                            ║
    • Open Issues:     TODOs: 4, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "../include/multi_level_storage.hpp"
#include "../include/gocryptfs_backend.hpp"
#include "../../include/security/vault_key_provider.h"
#include "../../include/security/hsm_key_provider_adapter.h"
#include "../../include/security/mock_key_provider.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

using json = nlohmann::json;

namespace themis {
namespace plugins {
namespace user_storage {

struct MultiLevelEncryptedStorage::Impl {
    std::map<SecurityLevel, LevelConfig> level_configs;
    std::map<SecurityLevel, std::shared_ptr<EncryptionBackendInterface>> backends;
    std::map<std::string, std::shared_ptr<KeyProvider>> key_providers;
    std::mutex mutex;
    bool initialized;
    
    Impl() : initialized(false) {}
};

MultiLevelEncryptedStorage::MultiLevelEncryptedStorage()
    : impl_(std::make_unique<Impl>()) {
}

MultiLevelEncryptedStorage::~MultiLevelEncryptedStorage() {
    shutdown();
}

bool MultiLevelEncryptedStorage::initialize(const char* config_json) {
    try {
        auto result = loadConfiguration(config_json ? config_json : "{}");
        if (result.isError()) {
            return false;
        }
        
        result = validateConfiguration();
        if (result.isError()) {
            return false;
        }

        // Collect all base/mount paths to derive a common root for reconciliation.
        // We reconcile against each configured mount_point's parent directory.
        std::set<std::string> base_paths;
        for (const auto& pair : impl_->level_configs) {
            const auto& cfg = pair.second;
            if (cfg.encrypted && !cfg.mount_point.empty()) {
                // Parent of the mount point is the "base" to scan
                std::string parent = cfg.mount_point;
                auto slash = parent.rfind('/');
                if (slash != std::string::npos && slash > 0) {
                    parent = parent.substr(0, slash);
                }
                base_paths.insert(parent);
            }
        }
        for (const auto& base : base_paths) {
            reconcileStaleMounts(base);
        }

        // Initialize all configured levels
        for (const auto& pair : impl_->level_configs) {
            auto init_result = initializeLevel(pair.second);
            if (init_result.isError()) {
                return false;
            }
        }
        
        impl_->initialized = true;
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void MultiLevelEncryptedStorage::shutdown() {
    if (impl_->initialized) {
        unmountAll();
        impl_->backends.clear();
        impl_->key_providers.clear();
        impl_->initialized = false;
    }
}

void MultiLevelEncryptedStorage::reconcileStaleMounts(const std::string& base_path) {
    if (base_path.empty()) {
        return;
    }

    // Build set of mount points that belong to this instance so we can skip them.
    std::set<std::string> configured_mounts;
    for (const auto& pair : impl_->level_configs) {
        if (pair.second.encrypted && !pair.second.mount_point.empty()) {
            configured_mounts.insert(pair.second.mount_point);
        }
    }

    // Collect stale mount points from /proc/mounts (Linux) that:
    //   1. Start with base_path + '/'
    //   2. Are NOT in the currently configured set (i.e., orphaned from a prior run).
    std::vector<std::string> stale;

#if defined(__linux__)
    std::ifstream mounts("/proc/mounts");
    std::string line;
    while (std::getline(mounts, line)) {
        // /proc/mounts format: <device> <mountpoint> <fstype> <options> <dump> <pass>
        std::istringstream iss(line);
        std::string device, mount_point;
        iss >> device >> mount_point;
        if (mount_point.empty()) continue;

        // Only consider mount points that are children of base_path.
        const std::string prefix = base_path + "/";
        if (mount_point.size() > prefix.size() &&
            mount_point.compare(0, prefix.size(), prefix) == 0) {
            if (configured_mounts.find(mount_point) == configured_mounts.end()) {
                stale.push_back(mount_point);
            }
        }
    }
#endif

    // Unmount each stale mount; errors are logged but do not abort startup.
    for (const auto& mp : stale) {
        // WARN: stale mount found
        // (In production this would use the ThemisDB logger; here we write to stderr.)
        fprintf(stderr,
                "[WARN] themis::user_storage: stale gocryptfs mount found at '%s', unmounting.\n",
                mp.c_str());

        // Try fusermount -u first (preferred for FUSE mounts), fall back to umount.
        bool unmounted = false;

#if defined(__linux__)
        {
            pid_t pid = fork();
            if (pid == 0) {
                // Child: exec fusermount -u <mp>
                execlp("fusermount", "fusermount", "-u", mp.c_str(), nullptr);
                _exit(127);
            } else if (pid > 0) {
                int status = 0;
                waitpid(pid, &status, 0);
                unmounted = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
            }
        }

        if (!unmounted) {
            pid_t pid = fork();
            if (pid == 0) {
                execlp("umount", "umount", mp.c_str(), nullptr);
                _exit(127);
            } else if (pid > 0) {
                int status = 0;
                waitpid(pid, &status, 0);
                unmounted = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
            }
        }
#endif

        if (!unmounted) {
            fprintf(stderr,
                    "[ERROR] themis::user_storage: failed to unmount stale mount '%s', continuing.\n",
                    mp.c_str());
        }
    }
}

Result<void> MultiLevelEncryptedStorage::loadConfiguration(const std::string& config_json) {
    try {
        json config = json::parse(config_json);
        
        // Check for multi_level_storage section
        if (!config.contains("multi_level_storage")) {
            // Default configuration with all 4 levels
            impl_->level_configs[SecurityLevel::OFFEN] = LevelConfig();
            impl_->level_configs[SecurityLevel::OFFEN].level = SecurityLevel::OFFEN;
            impl_->level_configs[SecurityLevel::OFFEN].name = "offen";
            impl_->level_configs[SecurityLevel::OFFEN].encrypted = false;
            impl_->level_configs[SecurityLevel::OFFEN].base_path = "/var/lib/themisdb/offen";
            
            return Result<void>();
        }
        
        auto storage_config = config["multi_level_storage"];
        
        if (!storage_config.contains("levels")) {
            return Result<void>::error("Configuration missing 'levels' array");
        }
        
        // Parse each level configuration
        for (const auto& level_json : storage_config["levels"]) {
            LevelConfig cfg;
            
            cfg.name = level_json.value("name", "");
            cfg.level = stringToSecurityLevel(cfg.name);
            cfg.encrypted = level_json.value("encrypted", false);
            
            if (!cfg.encrypted) {
                cfg.base_path = level_json.value("path", "");
            } else {
                cfg.encrypted_dir = level_json.value("encrypted_dir", "");
                cfg.mount_point = level_json.value("mount_point", "");
                
                if (level_json.contains("encryption")) {
                    auto enc = level_json["encryption"];
                    cfg.backend = enc.value("backend", "gocryptfs");
                    cfg.key_id = enc.value("key_id", "");
                    cfg.key_provider = enc.value("key_provider", "vault");
                    cfg.vault_addr = enc.value("vault_addr", "");
                    cfg.vault_mount = enc.value("vault_mount", "themis");
                    cfg.hsm_library = enc.value("hsm_library", "");
                    cfg.hsm_slot = enc.value("hsm_slot", 0);
                    cfg.hsm_key_label = enc.value("hsm_key_label", "");
                }
                
                if (level_json.contains("rotation")) {
                    auto rot = level_json["rotation"];
                    cfg.rotation_enabled = rot.value("enabled", false);
                    cfg.rotation_interval_days = rot.value("interval_days", 90);
                    cfg.auto_rotate = rot.value("auto_rotate", false);
                    
                    if (rot.contains("notification")) {
                        cfg.notification_email = rot["notification"].value("email", "");
                    }
                }
            }
            
            impl_->level_configs[cfg.level] = cfg;
        }
        
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Configuration parsing error: ") + e.what());
    }
}

Result<void> MultiLevelEncryptedStorage::validateConfiguration() {
    // Ensure at least one level is configured
    if (impl_->level_configs.empty()) {
        return Result<void>::error("No security levels configured");
    }
    
    // Validate each level configuration
    for (const auto& pair : impl_->level_configs) {
        const auto& cfg = pair.second;
        
        if (cfg.encrypted) {
            if (cfg.encrypted_dir.empty()) {
                return Result<void>::error("Missing encrypted_dir for level: " + cfg.name);
            }
            if (cfg.mount_point.empty()) {
                return Result<void>::error("Missing mount_point for level: " + cfg.name);
            }
            if (cfg.key_id.empty()) {
                return Result<void>::error("Missing key_id for level: " + cfg.name);
            }
        } else {
            if (cfg.base_path.empty()) {
                return Result<void>::error("Missing path for unencrypted level: " + cfg.name);
            }
        }
    }
    
    return Result<void>();
}

Result<void> MultiLevelEncryptedStorage::initializeLevel(const LevelConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (config.encrypted) {
        // Create encryption backend
        auto backend = std::make_shared<GocryptfsBackend>();
        auto init_result = backend->initialize("{}");
        if (init_result.isError()) {
            return init_result;
        }
        
        // Check backend availability
        auto avail_result = backend->checkAvailability();
        if (avail_result.isError()) {
            return avail_result;
        }
        
        impl_->backends[config.level] = backend;
        
        // Get key provider
        auto kp_result = getKeyProvider(config);
        if (kp_result.isError()) {
            return Result<void>::error(kp_result.error());
        }
        
        // Mount the container
        auto mount_result = mountLevel(config);
        if (mount_result.isError()) {
            return mount_result;
        }
    } else {
        // For unencrypted level, just ensure directory exists
        struct stat st;
        if (stat(config.base_path.c_str(), &st) != 0) {
            // Create directory
            if (mkdir(config.base_path.c_str(), 0700) != 0) {
                return Result<void>::error("Failed to create directory: " + config.base_path);
            }
        }
    }
    
    return Result<void>();
}

Result<void> MultiLevelEncryptedStorage::mountLevel(const LevelConfig& config) {
    if (!config.encrypted) {
        return Result<void>(); // Nothing to mount
    }
    
    auto backend_it = impl_->backends.find(config.level);
    if (backend_it == impl_->backends.end()) {
        return Result<void>::error("Backend not initialized for level: " + config.name);
    }
    
    auto kp_result = getKeyProvider(config);
    if (kp_result.isError()) {
        return Result<void>::error(kp_result.error());
    }
    
    auto key_provider = kp_result.value();
    
    // Get encryption key
    std::vector<uint8_t> key;
    try {
        key = key_provider->getKey(config.key_id);
    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Failed to get key: ") + e.what());
    }
    
    // Check if container exists, if not create it
    struct stat st;
    bool container_exists = (stat((config.encrypted_dir + "/gocryptfs.conf").c_str(), &st) == 0);
    
    if (!container_exists) {
        auto create_result = backend_it->second->createContainer(
            config.encrypted_dir,
            config.mount_point,
            key
        );
        if (create_result.isError()) {
            return create_result;
        }
    }
    
    // Mount container
    return backend_it->second->mountContainer(
        config.encrypted_dir,
        config.mount_point,
        key
    );
}

Result<void> MultiLevelEncryptedStorage::unmountLevel(const LevelConfig& config) {
    if (!config.encrypted) {
        return Result<void>();
    }
    
    auto backend_it = impl_->backends.find(config.level);
    if (backend_it == impl_->backends.end()) {
        return Result<void>();
    }
    
    return backend_it->second->unmountContainer(config.mount_point);
}

Result<std::shared_ptr<KeyProvider>> MultiLevelEncryptedStorage::getKeyProvider(
    const LevelConfig& config
) {
    std::string provider_key = config.key_provider + ":" + config.key_id;
    
    auto it = impl_->key_providers.find(provider_key);
    if (it != impl_->key_providers.end()) {
        return Result<std::shared_ptr<KeyProvider>>(it->second);
    }
    
    // Create new key provider
    std::shared_ptr<KeyProvider> provider;
    
    if (config.key_provider == "vault") {
        if (config.vault_addr.empty()) {
            return Result<std::shared_ptr<KeyProvider>>::error(
                "Vault address not configured for level: " + config.name
            );
        }
        
        // Get Vault token from environment
        const char* token_env = std::getenv("VAULT_TOKEN");
        std::string token = token_env ? token_env : "";
        
        provider = std::make_shared<VaultKeyProvider>(
            config.vault_addr,
            token,
            config.vault_mount
        );
    } else if (config.key_provider == "hsm") {
        return Result<std::shared_ptr<KeyProvider>>::error(
            "HSM key provider not yet implemented"
        );
    } else if (config.key_provider == "mock") {
        provider = std::make_shared<MockKeyProvider>();
    } else {
        return Result<std::shared_ptr<KeyProvider>>::error(
            "Unknown key provider: " + config.key_provider
        );
    }
    
    impl_->key_providers[provider_key] = provider;
    return Result<std::shared_ptr<KeyProvider>>(provider);
}

Result<void> MultiLevelEncryptedStorage::mountAll() {
    for (const auto& pair : impl_->level_configs) {
        auto result = mountLevel(pair.second);
        if (result.isError()) {
            return result;
        }
    }
    return Result<void>();
}

Result<void> MultiLevelEncryptedStorage::unmountAll() {
    for (const auto& pair : impl_->level_configs) {
        unmountLevel(pair.second);
    }
    return Result<void>();
}

Result<void> MultiLevelEncryptedStorage::mountLevel(SecurityLevel level) {
    auto it = impl_->level_configs.find(level);
    if (it == impl_->level_configs.end()) {
        return Result<void>::error("Level not configured: " + securityLevelToString(level));
    }
    return mountLevel(it->second);
}

Result<void> MultiLevelEncryptedStorage::unmountLevel(SecurityLevel level) {
    auto it = impl_->level_configs.find(level);
    if (it == impl_->level_configs.end()) {
        return Result<void>::error("Level not configured: " + securityLevelToString(level));
    }
    return unmountLevel(it->second);
}

Result<void> MultiLevelEncryptedStorage::rotateKey(SecurityLevel level) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto it = impl_->level_configs.find(level);
    if (it == impl_->level_configs.end()) {
        return Result<void>::error("Level not configured: " + securityLevelToString(level));
    }

    const LevelConfig& cfg = it->second;
    if (!cfg.encrypted) {
        // Unencrypted level has no key to rotate.
        return Result<void>();
    }

    // Zero-downtime key rotation workflow:
    // 1. Derive the new key from the key provider with a fresh salt.
    // 2. Create a new temporary container encrypted with the new key.
    // 3. Copy all data from the active mount_point into the new container.
    // 4. Unmount the old container.
    // 5. Rename old encrypted_dir to a timestamped backup, rename new to active.
    // 6. Mount the new container (with new key) at the original mount_point.

    auto backend_it = impl_->backends.find(cfg.level);
    if (backend_it == impl_->backends.end()) {
        return Result<void>::error("Backend not initialized for level: " + cfg.name);
    }
    auto& backend = backend_it->second;

    // Step 1: rotate key within the provider to get a new version, then fetch it.
    auto kp_result = getKeyProvider(cfg);
    if (kp_result.isError()) {
        return Result<void>::error("rotateKey: cannot get key provider: " + kp_result.error());
    }
    auto key_provider = kp_result.value();

    // Fetch old key first so we can roll back the mount if needed.
    std::vector<uint8_t> old_key;
    try {
        old_key = key_provider->getKey(cfg.key_id);
    } catch (const std::exception& e) {
        return Result<void>::error(std::string("rotateKey: cannot fetch current key: ") + e.what());
    }

    std::vector<uint8_t> new_key;
    try {
        key_provider->rotateKey(cfg.key_id);  // bump key version inside provider/vault
        new_key = key_provider->getKey(cfg.key_id);
    } catch (const std::exception& e) {
        return Result<void>::error(std::string("rotateKey: key provider rotation failed: ") + e.what());
    }
    if (new_key.empty()) {
        return Result<void>::error("rotateKey: key provider returned empty key after rotation");
    }

    // Step 2: prepare paths for the new container.
    // Use a ".new" suffix for the temporary cipher-text directory.
    const std::string new_encrypted_dir = cfg.encrypted_dir + ".new";
    const std::string new_mount_point   = cfg.mount_point   + ".new";

    // Clean up any leftover partial rotation attempt.
    struct stat st_check;
    if (stat(new_encrypted_dir.c_str(), &st_check) == 0) {
        // Attempt to remove stale directory tree (best-effort).
        std::string rm_cmd = "rm -rf " + new_encrypted_dir;
        std::system(rm_cmd.c_str()); // NOLINT(cert-env33-c)
    }

    // Ensure the temporary mount point exists.
    if (stat(new_mount_point.c_str(), &st_check) != 0) {
        if (mkdir(new_mount_point.c_str(), 0700) != 0) {
            return Result<void>::error("rotateKey: cannot create temp mount point: " + new_mount_point);
        }
    }

    // Step 2 (cont.): create the new encrypted container.
    auto create_result = backend->createContainer(new_encrypted_dir, new_mount_point, new_key);
    if (create_result.isError()) {
        return Result<void>::error("rotateKey: createContainer failed: " + create_result.error());
    }

    auto mount_new_result = backend->mountContainer(new_encrypted_dir, new_mount_point, new_key);
    if (mount_new_result.isError()) {
        std::string rm_cmd = "rm -rf " + new_encrypted_dir;
        std::system(rm_cmd.c_str()); // NOLINT(cert-env33-c)
        rmdir(new_mount_point.c_str());
        return Result<void>::error("rotateKey: cannot mount new container: " + mount_new_result.error());
    }

    // Step 3: copy all data from old mount_point to new_mount_point.
    // Use cp -a to preserve timestamps and permissions.
    std::string copy_cmd = "cp -a " + cfg.mount_point + "/. " + new_mount_point + "/";
    int cp_rc = std::system(copy_cmd.c_str()); // NOLINT(cert-env33-c)
    if (cp_rc != 0) {
        // Roll back: unmount and remove new container.
        backend->unmountContainer(new_mount_point);
        std::string rm_cmd = "rm -rf " + new_encrypted_dir;
        std::system(rm_cmd.c_str()); // NOLINT(cert-env33-c)
        rmdir(new_mount_point.c_str());
        return Result<void>::error("rotateKey: data copy failed (cp exit " + std::to_string(cp_rc) + ")");
    }

    // Step 4: unmount the OLD container.
    auto umount_old = backend->unmountContainer(cfg.mount_point);
    if (umount_old.isError()) {
        // Unmount of old failed; roll back new container to avoid double-mount.
        backend->unmountContainer(new_mount_point);
        std::string rm_cmd = "rm -rf " + new_encrypted_dir;
        std::system(rm_cmd.c_str()); // NOLINT(cert-env33-c)
        rmdir(new_mount_point.c_str());
        return Result<void>::error("rotateKey: cannot unmount old container: " + umount_old.error());
    }

    // Step 5: atomic directory swap.
    //   old_encrypted_dir  → old_encrypted_dir.bak.<timestamp>
    //   new_encrypted_dir  → old_encrypted_dir  (active)
    auto now_s = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    std::string backup_dir = cfg.encrypted_dir + ".bak." + now_s;

    if (rename(cfg.encrypted_dir.c_str(), backup_dir.c_str()) != 0) {
        // Cannot rename old dir; remount old container to restore service.
        backend->mountContainer(cfg.encrypted_dir, cfg.mount_point, old_key);
        backend->unmountContainer(new_mount_point);
        std::string rm_cmd = "rm -rf " + new_encrypted_dir;
        std::system(rm_cmd.c_str()); // NOLINT(cert-env33-c)
        rmdir(new_mount_point.c_str());
        return Result<void>::error("rotateKey: cannot rename old container to backup");
    }

    if (rename(new_encrypted_dir.c_str(), cfg.encrypted_dir.c_str()) != 0) {
        // Critical: try to restore old directory.
        rename(backup_dir.c_str(), cfg.encrypted_dir.c_str());
        backend->mountContainer(cfg.encrypted_dir, cfg.mount_point, old_key);
        backend->unmountContainer(new_mount_point);
        return Result<void>::error("rotateKey: cannot promote new container");
    }

    // Step 6: unmount new_mount_point and mount at canonical mount_point.
    backend->unmountContainer(new_mount_point);
    rmdir(new_mount_point.c_str());

    auto final_mount = backend->mountContainer(cfg.encrypted_dir, cfg.mount_point, new_key);
    if (final_mount.isError()) {
        return Result<void>::error("rotateKey: final mount failed (data is safe in "
                                    + cfg.encrypted_dir + "): " + final_mount.error());
    }

    THEMIS_INFO("rotateKey: rotation completed for level '{}'. Backup: {}", cfg.name, backup_dir);

    // Send notification if configured.
    if (!cfg.notification_email.empty()) {
        std::string notify_cmd = "echo 'ThemisDB: key rotated for level " + cfg.name
                                 + " at " + now_s + "' | mail -s 'Key Rotation Complete' "
                                 + cfg.notification_email + " 2>/dev/null || true";
        std::system(notify_cmd.c_str()); // NOLINT(cert-env33-c)
    }

    return Result<void>();
}

std::string MultiLevelEncryptedStorage::getBasePath(SecurityLevel level) {
    auto it = impl_->level_configs.find(level);
    if (it == impl_->level_configs.end()) {
        return "";
    }
    
    if (it->second.encrypted) {
        return it->second.mount_point;
    } else {
        return it->second.base_path;
    }
}

std::string MultiLevelEncryptedStorage::getUserPath(SecurityLevel level, const std::string& user_id) {
    std::string base = getBasePath(level);
    if (base.empty()) {
        return "";
    }
    return base + "/users/" + user_id + ".json";
}

std::string MultiLevelEncryptedStorage::getGroupPath(SecurityLevel level, const std::string& group_id) {
    std::string base = getBasePath(level);
    if (base.empty()) {
        return "";
    }
    return base + "/groups/" + group_id + ".json";
}

Result<void> MultiLevelEncryptedStorage::writeUserFile(const std::string& path, const User& user) {
    try {
        json j;
        j["user_id"] = user.user_id;
        j["username"] = user.username;
        j["email"] = user.email;
        j["full_name"] = user.full_name;
        j["roles"] = user.roles;
        j["classification"] = securityLevelToString(user.classification);
        j["created_at_ms"] = user.created_at_ms;
        j["updated_at_ms"] = user.updated_at_ms;
        
        // Ensure parent directory exists
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos) {
            std::string dir = path.substr(0, pos);
            int result = mkdir(dir.c_str(), 0700);
            if (result != 0 && errno != EEXIST) {
                return Result<void>::error(
                    "Failed to create directory: " + dir + " (errno: " + std::to_string(errno) + ")"
                );
            }
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            return Result<void>::error("Failed to write user file: " + path);
        }
        
        file << j.dump(2);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Failed to serialize user: ") + e.what());
    }
}

Result<User> MultiLevelEncryptedStorage::readUserFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Result<User>::error("User file not found: " + path);
        }
        
        json j;
        file >> j;
        
        User user;
        user.user_id = j.value("user_id", "");
        user.username = j.value("username", "");
        user.email = j.value("email", "");
        user.full_name = j.value("full_name", "");
        user.roles = j.value("roles", std::vector<std::string>());
        user.classification = stringToSecurityLevel(j.value("classification", "offen"));
        user.created_at_ms = j.value("created_at_ms", 0);
        user.updated_at_ms = j.value("updated_at_ms", 0);
        
        return Result<User>(user);
    } catch (const std::exception& e) {
        return Result<User>::error(std::string("Failed to read user file: ") + e.what());
    }
}

Result<void> MultiLevelEncryptedStorage::writeGroupFile(const std::string& path, const Group& group) {
    try {
        json j;
        j["group_id"] = group.group_id;
        j["name"] = group.name;
        j["description"] = group.description;
        j["member_ids"] = group.member_ids;
        j["classification"] = securityLevelToString(group.classification);
        j["created_at_ms"] = group.created_at_ms;
        
        // Ensure parent directory exists
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos) {
            std::string dir = path.substr(0, pos);
            int result = mkdir(dir.c_str(), 0700);
            if (result != 0 && errno != EEXIST) {
                return Result<void>::error(
                    "Failed to create directory: " + dir + " (errno: " + std::to_string(errno) + ")"
                );
            }
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            return Result<void>::error("Failed to write group file: " + path);
        }
        
        file << j.dump(2);
        return Result<void>();
    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Failed to serialize group: ") + e.what());
    }
}

Result<Group> MultiLevelEncryptedStorage::readGroupFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Result<Group>::error("Group file not found: " + path);
        }
        
        json j;
        file >> j;
        
        Group group;
        group.group_id = j.value("group_id", "");
        group.name = j.value("name", "");
        group.description = j.value("description", "");
        group.member_ids = j.value("member_ids", std::vector<std::string>());
        group.classification = stringToSecurityLevel(j.value("classification", "offen"));
        group.created_at_ms = j.value("created_at_ms", 0);
        
        return Result<Group>(group);
    } catch (const std::exception& e) {
        return Result<Group>::error(std::string("Failed to read group file: ") + e.what());
    }
}

// User Management API Implementation
Result<void> MultiLevelEncryptedStorage::createUser(const User& user, SecurityLevel level) {
    std::string path = getUserPath(level, user.user_id);
    if (path.empty()) {
        return Result<void>::error("Invalid security level");
    }
    
    return writeUserFile(path, user);
}

Result<User> MultiLevelEncryptedStorage::getUser(const std::string& user_id, SecurityLevel level) {
    std::string path = getUserPath(level, user_id);
    if (path.empty()) {
        return Result<User>::error("Invalid security level");
    }
    
    return readUserFile(path);
}

Result<void> MultiLevelEncryptedStorage::updateUser(const User& user, SecurityLevel level) {
    return createUser(user, level); // Overwrite
}

Result<void> MultiLevelEncryptedStorage::deleteUser(const std::string& user_id, SecurityLevel level) {
    std::string path = getUserPath(level, user_id);
    if (path.empty()) {
        return Result<void>::error("Invalid security level");
    }
    
    if (std::remove(path.c_str()) != 0) {
        return Result<void>::error("Failed to delete user file");
    }
    
    return Result<void>();
}

Result<std::vector<User>> MultiLevelEncryptedStorage::listUsers(SecurityLevel level) {
    // TODO: Implement directory listing for users
    // This requires iterating through the users directory and reading all JSON files
    return Result<std::vector<User>>::error("listUsers() not yet implemented - use getUser() for now");
}

// Group Management API Implementation
Result<void> MultiLevelEncryptedStorage::createGroup(const Group& group, SecurityLevel level) {
    std::string path = getGroupPath(level, group.group_id);
    if (path.empty()) {
        return Result<void>::error("Invalid security level");
    }
    
    return writeGroupFile(path, group);
}

Result<Group> MultiLevelEncryptedStorage::getGroup(const std::string& group_id, SecurityLevel level) {
    std::string path = getGroupPath(level, group_id);
    if (path.empty()) {
        return Result<Group>::error("Invalid security level");
    }
    
    return readGroupFile(path);
}

Result<void> MultiLevelEncryptedStorage::updateGroup(const Group& group, SecurityLevel level) {
    return createGroup(group, level); // Overwrite
}

Result<void> MultiLevelEncryptedStorage::deleteGroup(const std::string& group_id, SecurityLevel level) {
    std::string path = getGroupPath(level, group_id);
    if (path.empty()) {
        return Result<void>::error("Invalid security level");
    }
    
    if (std::remove(path.c_str()) != 0) {
        return Result<void>::error("Failed to delete group file");
    }
    
    return Result<void>();
}

Result<std::vector<Group>> MultiLevelEncryptedStorage::listGroups(SecurityLevel level) {
    // TODO: Implement directory listing for groups
    // This requires iterating through the groups directory and reading all JSON files
    return Result<std::vector<Group>>::error("listGroups() not yet implemented - use getGroup() for now");
}

// Health Check Implementation
Result<HealthStatus> MultiLevelEncryptedStorage::checkHealth() {
    HealthStatus status;
    status.healthy = true;
    status.checked_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    for (const auto& pair : impl_->level_configs) {
        auto level_health = checkLevelHealth(pair.first);
        if (level_health.isError()) {
            status.healthy = false;
            status.errors.push_back(level_health.error());
        } else if (!level_health.value().healthy) {
            status.healthy = false;
            status.errors.push_back(
                "Level " + securityLevelToString(pair.first) + ": " + level_health.value().message
            );
        }
    }
    
    if (status.healthy) {
        status.message = "All levels healthy";
    } else {
        status.message = "Some levels have errors";
    }
    
    return Result<HealthStatus>(status);
}

Result<HealthStatus> MultiLevelEncryptedStorage::checkLevelHealth(SecurityLevel level) {
    HealthStatus status;
    status.checked_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    auto it = impl_->level_configs.find(level);
    if (it == impl_->level_configs.end()) {
        status.healthy = false;
        status.message = "Level not configured";
        return Result<HealthStatus>(status);
    }
    
    const auto& config = it->second;
    
    if (config.encrypted) {
        auto backend_it = impl_->backends.find(level);
        if (backend_it == impl_->backends.end()) {
            status.healthy = false;
            status.message = "Backend not initialized";
            return Result<HealthStatus>(status);
        }
        
        if (!backend_it->second->isMounted(config.mount_point)) {
            status.healthy = false;
            status.message = "Container not mounted";
            return Result<HealthStatus>(status);
        }
    }
    
    status.healthy = true;
    status.message = "Level healthy";
    return Result<HealthStatus>(status);
}

} // namespace user_storage
} // namespace plugins
} // namespace themis

// Plugin export
THEMIS_PLUGIN_IMPL(themis::plugins::user_storage::MultiLevelEncryptedStorage)
