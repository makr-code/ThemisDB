/**
 * @file user_registration_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <vector>
#include "utils/expected.h"

namespace themis {
namespace security {

/**
 * @brief User Registration Data
 * 
 * Data structure for user registration information that can be
 * provided by external plugins (Apache Arrow, WebDAV, etc.)
 */
struct UserRegistrationData {
    std::string user_id;
    std::string password_hash;  // Pre-hashed password from plugin
    std::vector<std::string> roles;
    std::unordered_map<std::string, std::string> attributes;
    
    // Source information
    std::string source;  // e.g., "arrow", "webdav", "direct"
    std::string source_uri;  // Original source URI if applicable
};

/**
 * @brief User Registration Plugin Interface
 * 
 * Interface for plugins that provide user registration functionality.
 * Primary implementations: Apache Arrow and WebDAV.
 * 
 * Plugins can integrate with external identity providers and
 * automatically synchronize users into ThemisDB.
 */
class IUserRegistrationPlugin {
public:
    virtual ~IUserRegistrationPlugin() = default;
    
    /**
     * @brief Get plugin name
     * @return Plugin name (e.g., "arrow", "webdav")
     */
    [[nodiscard]] virtual std::string getName() const = 0;
    
    /**
     * @brief Check if plugin is available and configured
     * @return true if plugin can be used
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;
    
    /**
     * @brief Register user through plugin
     * 
     * The plugin is responsible for:
     * - Validating user credentials with external system
     * - Hashing passwords if needed
     * - Mapping user attributes and roles
     * 
     * @param user_id User identifier
     * @param password Plain text password (will be hashed by plugin)
     * @param attributes Optional additional attributes
     * @return Result<UserRegistrationData> Registration data or error
     */
    [[nodiscard]] virtual Result<UserRegistrationData> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::unordered_map<std::string, std::string>& attributes = {}
    ) = 0;
    
    /**
     * @brief Authenticate user through plugin
     * 
     * Verifies credentials against external system (e.g., WebDAV server).
     * If successful, returns user data that can be synchronized locally.
     * 
     * @param user_id User identifier
     * @param password Plain text password
     * @return Result<UserRegistrationData> User data if authenticated
     */
    [[nodiscard]] virtual Result<UserRegistrationData> authenticateUser(
        const std::string& user_id,
        const std::string& password
    ) = 0;
    
    /**
     * @brief Synchronize users from external source
     * 
     * Batch operation to import/sync users from external system.
     * Used for Apache Arrow-based bulk imports or WebDAV directory sync.
     * 
     * @return Result<vector<UserRegistrationData>> List of users or error
     */
    [[nodiscard]] virtual Result<std::vector<UserRegistrationData>> syncUsers() = 0;
    
    /**
     * @brief Update user information from external source
     * 
     * @param user_id User identifier
     * @return Result<UserRegistrationData> Updated user data or error
     */
    [[nodiscard]] virtual Result<UserRegistrationData> updateUser(const std::string& user_id) = 0;
};

/**
 * @brief User Registration Plugin Manager
 * 
 * Manages multiple user registration plugins and delegates
 * registration requests to the appropriate plugin.
 */
class UserRegistrationPluginManager {
public:
    /**
     * @brief Register a plugin
     * @param plugin Plugin instance
     */
    void registerPlugin(std::shared_ptr<IUserRegistrationPlugin> plugin);
    
    /**
     * @brief Get plugin by name
     * @param name Plugin name
     * @return Plugin instance or nullptr
     */
    std::shared_ptr<IUserRegistrationPlugin> getPlugin(const std::string& name) const;
    
    /**
     * @brief Get all available plugins
     * @return List of available plugins
     */
    std::vector<std::shared_ptr<IUserRegistrationPlugin>> getAvailablePlugins() const;
    
    /**
     * @brief Get default plugin (first available)
     * @return Default plugin or nullptr
     */
    std::shared_ptr<IUserRegistrationPlugin> getDefaultPlugin() const;
    
    /**
     * @brief Register user using specified plugin
     * @param plugin_name Plugin to use (empty = use default)
     * @param user_id User identifier
     * @param password Password
     * @param attributes Additional attributes
     * @return Result<UserRegistrationData> Registration data or error
     */
    Result<UserRegistrationData> registerUser(
        const std::string& plugin_name,
        const std::string& user_id,
        const std::string& password,
        const std::unordered_map<std::string, std::string>& attributes = {}
    );

private:
    std::unordered_map<std::string, std::shared_ptr<IUserRegistrationPlugin>> plugins_;
    mutable std::mutex mutex_;
};

/**
 * @brief Factory: create the embedded (in-memory) user registration plugin.
 *
 * Returns a ready-to-use plugin backed by PBKDF2-SHA256 password hashing.
 * Intended for standalone deployments and unit tests.
 *
 * NOT recommended for production enterprise deployments.
 */
std::shared_ptr<IUserRegistrationPlugin> createEmbeddedUserRegistrationPlugin();

} // namespace security
} // namespace themis
