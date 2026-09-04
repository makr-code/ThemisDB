/**
 * @file multi_level_storage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=2, H=8, M=18, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "multi_level_storage.hpp"
#include "gocryptfs_backend.hpp"
#include <security/vault_key_provider.h>
#include <security/mock_key_provider.h>
#include <security/hsm_provider.h>
#include <security/hsm_key_provider_adapter.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <cstdio>
#include <sys/stat.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include <filesystem>
#include <spdlog/spdlog.h>
#include <errno.h>
#include <stdexcept>

using json = nlohmann::json;

namespace themis {
namespace plugins {
namespace user_storage {

struct MultiLevelEncryptedStorage::Impl {
    std::map<SecurityLevel, LevelConfig> level_configs;
    std::map<SecurityLevel, std::shared_ptr<EncryptionBackendInterface>> backends;
    std::map<std::string, std::shared_ptr<KeyProvider>> key_providers;
    std::mutex mutex;
    bool initialized = {};
    StorageMetrics metrics;

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
        
        // Reconcile any orphaned FUSE mounts from a previous crash before
        // initialising levels (non-fatal: log and continue).
        reconcileStaleMounts();

        // Reconcile stale mounts from a prior crash before bringing up new mounts.
        std::set<std::string> base_paths = {};

        for (const auto& pair : impl_->level_configs) {
            const auto& cfg = pair.second;
            if (cfg.encrypted && !cfg.mount_point.empty()) {
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
    } catch (...) {
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
    std::set<std::string> configured_mounts = {};

    for (const auto& pair : impl_->level_configs) {
        if (pair.second.encrypted && !pair.second.mount_point.empty()) {
            configured_mounts.insert(pair.second.mount_point);
        }
    }

    // Collect stale mount points from /proc/mounts (Linux) that:
    //   1. Start with base_path + '/'
    //   2. Are NOT in the currently configured set (orphaned from a prior run).
    std::vector<std::string> stale;

#if defined(__linux__)
    std::ifstream mounts("/proc/mounts");
    std::string line = {};
    while (std::getline(mounts, line)) {
        // /proc/mounts: <device> <mountpoint> <fstype> <options> <dump> <pass>
        std::istringstream iss(line);
        std::string device, mount_point;
        iss >> device >> mount_point;
        if (mount_point.empty()) {
          continue;
        }

        const std::string prefix = base_path + "/";
        if (static_cast<int>(mount_point.size()) > prefix.size() &&
            mount_point.compare(0, prefix.size(), prefix) == 0) {
            if (configured_mounts.find(mount_point) == configured_mounts.end()) {
                stale.push_back(mount_point);
            }
        }
    }
#endif

    // Unmount each stale mount — errors are logged but never abort startup.
    for (const auto& mp : stale) {
        fprintf(stderr,
                "[WARN] themis::user_storage: stale gocryptfs mount at '%s', unmounting.\n",
                mp.c_str());

        bool unmounted = false;

#if defined(__linux__)
        {
            pid_t pid = fork();
            if (pid == 0) {
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
        std::error_code ec = {};
        if (!std::filesystem::exists(config.base_path, ec)) {
            std::filesystem::create_directories(config.base_path, ec);
            if (ec) {
                return Result<void>::error("Failed to create directory: " + config.base_path +
                                           " (" + ec.message() + ")");
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
    auto mount_result = backend_it->second->mountContainer(
        config.encrypted_dir,
        config.mount_point,
        key
    );
    if (mount_result.isSuccess()) {
        ++impl_->metrics.mounts_active;
        ++impl_->metrics.mount_ops_total;
    }
    return mount_result;
}

Result<void> MultiLevelEncryptedStorage::unmountLevel(const LevelConfig& config) {
    if (!config.encrypted) {
        return Result<void>();
    }
    
    auto backend_it = impl_->backends.find(config.level);
    if (backend_it == impl_->backends.end()) {
        return Result<void>();
    }
    
    auto unmount_result = backend_it->second->unmountContainer(config.mount_point);
    if (unmount_result.isSuccess()) {
        if (impl_->metrics.mounts_active > 0) {
            --impl_->metrics.mounts_active;
        }
        ++impl_->metrics.unmount_ops_total;
    }
    return unmount_result;
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
        if (config.hsm_library.empty()) {
            return Result<std::shared_ptr<KeyProvider>>::error(
                "HSM library path not configured for level: " + config.name
            );
        }

        // Build HSMProvider config from LevelConfig HSM fields
        themis::security::HSMConfig hsm_cfg;
        hsm_cfg.library_path = config.hsm_library;
        hsm_cfg.slot_id      = config.hsm_slot;
        hsm_cfg.key_label    = config.hsm_key_label.empty()
                               ? "themis-kek"
                               : config.hsm_key_label;

        auto hsm = std::make_shared<themis::security::HSMProvider>(hsm_cfg);
        if (!hsm->initialize()) {
            return Result<std::shared_ptr<KeyProvider>>::error(
                "Failed to initialize HSM provider for level: " + config.name +
                " (library: " + config.hsm_library + ")"
            );
        }

        themis::security::HSMKeyProviderAdapter::Config adapter_cfg;
        adapter_cfg.kek_label = hsm_cfg.key_label;
        provider = std::make_shared<themis::security::HSMKeyProviderAdapter>(hsm, adapter_cfg);
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
    auto it = impl_->level_configs.find(level);
    if (it == impl_->level_configs.end()) {
        return Result<void>::error("Level not configured: " + securityLevelToString(level));
    }
    const LevelConfig& cfg = it->second;

    if (!cfg.encrypted) {
        // Unencrypted level — no key to rotate
        return Result<void>();
    }

    // Step 1: obtain key provider and derive a new key
    auto kp_result = getKeyProvider(cfg);
    if (kp_result.isError()) {
        return Result<void>::error("Key rotation failed – cannot obtain key provider: " +
                                   kp_result.error());
    }
    auto key_provider = kp_result.value();

    std::vector<uint8_t> new_key;
    try {
        // Request a fresh key version from the provider (returns new version ID).
        uint32_t new_version = key_provider->rotateKey(cfg.key_id);
        // Retrieve the actual key material for the new version.
        new_key = key_provider->getKey(cfg.key_id, new_version);
    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Key rotation failed – provider error: ") +
                                   e.what());
    }
    if (new_key.empty()) {
        return Result<void>::error("Key rotation failed – provider returned empty key");
    }

    // Step 2: create a new encrypted container next to the current one
    std::string new_encrypted_dir  = cfg.encrypted_dir  + ".rotation_new";
    std::string new_mount_point    = cfg.mount_point     + ".rotation_new";

    auto backend_it = impl_->backends.find(level);
    if (backend_it == impl_->backends.end()) {
        return Result<void>::error("Key rotation failed – backend not initialised for level: " +
                                   cfg.name);
    }
    auto& backend = backend_it->second;

    // Create the new container
    auto create_result = backend->createContainer(new_encrypted_dir, new_mount_point, new_key);
    if (create_result.isError()) {
        return Result<void>::error("Key rotation failed – cannot create new container: " +
                                   create_result.error());
    }

    // Mount both containers
    auto mount_new = backend->mountContainer(new_encrypted_dir, new_mount_point, new_key);
    if (mount_new.isError()) {
        // Best-effort cleanup
        std::error_code cleanup_ec = {};
        std::filesystem::remove_all(new_encrypted_dir, cleanup_ec);
        return Result<void>::error("Key rotation failed – cannot mount new container: " +
                                   mount_new.error());
    }

    // Step 3: copy data from the current mount point to the new one using std::filesystem.
    try {
        std::filesystem::copy(
            cfg.mount_point,
            new_mount_point,
            std::filesystem::copy_options::recursive |
            std::filesystem::copy_options::overwrite_existing
        );
    } catch (const std::filesystem::filesystem_error& e) {
        backend->unmountContainer(new_mount_point);
        std::filesystem::remove_all(new_mount_point);
        std::filesystem::remove_all(new_encrypted_dir);
        return Result<void>::error(
            std::string("Key rotation failed – data copy error: ") + e.what());
    }

    // Step 4: unmount both containers, then atomically swap directories
    unmountLevel(cfg);
    backend->unmountContainer(new_mount_point);

    std::string backup_encrypted_dir = cfg.encrypted_dir + ".rotation_backup";
    if (::rename(cfg.encrypted_dir.c_str(), backup_encrypted_dir.c_str()) != 0) {
        return Result<void>::error("Key rotation failed – cannot rename old container to backup");
    }
    if (::rename(new_encrypted_dir.c_str(), cfg.encrypted_dir.c_str()) != 0) {
        // Try to undo the first rename
        ::rename(backup_encrypted_dir.c_str(), cfg.encrypted_dir.c_str());
        return Result<void>::error("Key rotation failed – cannot rename new container into place");
    }

    // Step 5: re-mount with the new key and invalidate cached key provider entry
    std::string provider_key = cfg.key_provider + ":" + cfg.key_id;
    impl_->key_providers.erase(provider_key);

    auto remount = mountLevel(cfg);
    if (remount.isError()) {
        return Result<void>::error("Key rotation succeeded but re-mount failed: " +
                                   remount.error());
    }

    // Step 6: remove the backup container securely.
    std::error_code ec = {};
    std::filesystem::remove_all(backup_encrypted_dir, ec);
    if (ec) {
        spdlog::warn("Key rotation: could not remove backup container '{}': {}",
                     backup_encrypted_dir, ec.message());
    }

    // Record metric — rotation succeeded
    ++impl_->metrics.key_rotations_total;

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

// ── Prometheus metrics ────────────────────────────────────────────────────

std::string MultiLevelEncryptedStorage::getMetricsText() const {
    std::string out = {};
    out.reserve(512);

    // user_storage_mounts_active
    out += "# HELP user_storage_mounts_active Currently mounted encrypted containers\n";
    out += "# TYPE user_storage_mounts_active gauge\n";
    out += "user_storage_mounts_active " +
           std::to_string(impl_->metrics.mounts_active.load()) + "\n";

    // user_storage_mount_operations_total (split by operation label)
    out += "# HELP user_storage_mount_operations_total Total mount/unmount operations\n";
    out += "# TYPE user_storage_mount_operations_total counter\n";
    out += "user_storage_mount_operations_total{operation=\"mount\"} " +
           std::to_string(impl_->metrics.mount_ops_total.load()) + "\n";
    out += "user_storage_mount_operations_total{operation=\"unmount\"} " +
           std::to_string(impl_->metrics.unmount_ops_total.load()) + "\n";

    // user_storage_key_rotations_total
    out += "# HELP user_storage_key_rotations_total Key rotation callbacks fired\n";
    out += "# TYPE user_storage_key_rotations_total counter\n";
    out += "user_storage_key_rotations_total " +
           std::to_string(impl_->metrics.key_rotations_total.load()) + "\n";

    // user_storage_container_size_bytes
    out += "# HELP user_storage_container_size_bytes Sum of encrypted container sizes on disk\n";
    out += "# TYPE user_storage_container_size_bytes gauge\n";
    out += "user_storage_container_size_bytes " +
           std::to_string(impl_->metrics.container_size_bytes.load()) + "\n";

    return out;
}

void MultiLevelEncryptedStorage::recordKeyRotation(SecurityLevel /*level*/) {
    ++impl_->metrics.key_rotations_total;
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
        const auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) {
            std::error_code ec = {};
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                return Result<void>::error(
                    "Failed to create directory: " + parent.string() + " (" + ec.message() + ")"
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
        const auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) {
            std::error_code ec = {};
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                return Result<void>::error(
                    "Failed to create directory: " + parent.string() + " (" + ec.message() + ")"
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
    std::string base = getBasePath(level);
    if (base.empty()) {
        return Result<std::vector<User>>::error("Invalid security level");
    }

    const auto users_dir = std::filesystem::path(base) / "users";
    std::error_code ec = {};
    if (!std::filesystem::exists(users_dir, ec) || !std::filesystem::is_directory(users_dir, ec)) {
        return Result<std::vector<User>>(std::vector<User>{});
    }

    std::vector<User> users = {};

    for (const auto& entry : std::filesystem::directory_iterator(users_dir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".json") {
            auto result = readUserFile(entry.path().string());
            if (!result.isError()) {
                users.push_back(result.value());
            }
        }
    }

    return Result<std::vector<User>>(users);
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
    std::string base = getBasePath(level);
    if (base.empty()) {
        return Result<std::vector<Group>>::error("Invalid security level");
    }

    const auto groups_dir = std::filesystem::path(base) / "groups";
    std::error_code ec = {};
    if (!std::filesystem::exists(groups_dir, ec) || !std::filesystem::is_directory(groups_dir, ec)) {
        return Result<std::vector<Group>>(std::vector<Group>{});
    }

    std::vector<Group> groups = {};

    for (const auto& entry : std::filesystem::directory_iterator(groups_dir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".json") {
            auto result = readGroupFile(entry.path().string());
            if (!result.isError()) {
                groups.push_back(result.value());
            }
        }
    }

    return Result<std::vector<Group>>(groups);
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

void MultiLevelEncryptedStorage::reconcileStaleMounts() {
    // Collect known mount points from configuration
    std::vector<std::string> known_mount_points = {};

    for (const auto& pair : impl_->level_configs) {
        const auto& cfg = pair.second;
        if (cfg.encrypted && !cfg.mount_point.empty()) {
            known_mount_points.push_back(cfg.mount_point);
        }
    }

#ifdef __linux__
    // Scan /proc/mounts for FUSE entries whose mount point is one of ours
    std::ifstream mounts_file("/proc/mounts");
    if (!mounts_file) {
        return;
    }

    std::vector<std::string> stale_mounts;
    std::string line = {};
    while (std::getline(mounts_file, line)) {
        // /proc/mounts columns: device mountpoint fstype options dump pass
        std::istringstream iss(line);
        std::string device, mount_point, fstype;
        iss >> device >> mount_point >> fstype;

        // Only consider FUSE mounts (gocryptfs uses fuse.gocryptfs)
        if (fstype.find("fuse") == std::string::npos) {
            continue;
        }

        for (const auto& known : known_mount_points) {
            if (mount_point == known) {
                stale_mounts.push_back(mount_point);
                break;
            }
        }
    }

    // Unmount stale FUSE mounts non-fatally
    for (const auto& mp : stale_mounts) {
        // Try fusermount -u first, fall back to umount
        std::vector<std::string> args_fuse = {"fusermount", "-u", mp};
        std::vector<std::string> args_umount = {"umount", mp};

        // Fork/exec fusermount
        {
            pid_t pid = fork();
            if (pid == 0) {
                std::vector<char*> c_args = {};

                for (const auto& a : args_fuse) {
                    c_args.push_back(const_cast<char*>(a.c_str()));
                }
                c_args.push_back(nullptr);
                execvp(c_args[0], c_args.data());
                _exit(127);
            } else if (pid > 0) {
                int status = 0;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    continue; // Successfully unmounted
                }
            }
        }

        // Fallback: umount
        {
            pid_t pid = fork();
            if (pid == 0) {
                std::vector<char*> c_args = {};

                for (const auto& a : args_umount) {
                    c_args.push_back(const_cast<char*>(a.c_str()));
                }
                c_args.push_back(nullptr);
                execvp(c_args[0], c_args.data());
                _exit(127);
            } else if (pid > 0) {
                int status = 0;
                waitpid(pid, &status, 0);
                // Non-fatal regardless of outcome
            }
        }
    }
#endif
}

} // namespace user_storage
} // namespace plugins
} // namespace themis

// Plugin export
#if defined(THEMIS_PLUGIN_EXPORTS)
THEMIS_PLUGIN_IMPL(themis::plugins::user_storage::MultiLevelEncryptedStorage)
#endif

