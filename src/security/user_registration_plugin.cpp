/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            user_registration_plugin.cpp                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     137                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "security/user_registration_plugin.h"
#include "utils/logger.h"

namespace themis {
namespace security {

// ============================================================================
// UserRegistrationPluginManager
// ============================================================================

void UserRegistrationPluginManager::registerPlugin(std::shared_ptr<IUserRegistrationPlugin> plugin) {
    if (!plugin) {
        THEMIS_WARN("Attempted to register null user registration plugin");
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    plugins_[plugin->getName()] = plugin;
    THEMIS_INFO("Registered user registration plugin: {}", plugin->getName());
}

std::shared_ptr<IUserRegistrationPlugin> UserRegistrationPluginManager::getPlugin(
    const std::string& name
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it != plugins_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<IUserRegistrationPlugin>> 
UserRegistrationPluginManager::getAvailablePlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<IUserRegistrationPlugin>> available;
    for (const auto& [name, plugin] : plugins_) {
        if (plugin && plugin->isAvailable()) {
            available.push_back(plugin);
        }
    }
    
    return available;
}

std::shared_ptr<IUserRegistrationPlugin> 
UserRegistrationPluginManager::getDefaultPlugin() const {
    auto available = getAvailablePlugins();
    
    if (available.empty()) {
        return nullptr;
    }
    
    // Prefer arrow, then webdav, then first available
    for (const auto& plugin : available) {
        if (plugin->getName() == "arrow") {
            return plugin;
        }
    }
    
    for (const auto& plugin : available) {
        if (plugin->getName() == "webdav") {
            return plugin;
        }
    }
    
    return available.front();
}

Result<UserRegistrationData> UserRegistrationPluginManager::registerUser(
    const std::string& plugin_name,
    const std::string& user_id,
    const std::string& password,
    const std::unordered_map<std::string, std::string>& attributes
) {
    std::shared_ptr<IUserRegistrationPlugin> plugin;
    
    if (plugin_name.empty()) {
        plugin = getDefaultPlugin();
        if (!plugin) {
            return themis::Err<UserRegistrationData>(
                errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                "No user registration plugins available"
            );
        }
    } else {
        plugin = getPlugin(plugin_name);
        if (!plugin) {
            return themis::Err<UserRegistrationData>(
                errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
                "User registration plugin not found: " + plugin_name
            );
        }
        
        if (!plugin->isAvailable()) {
            return themis::Err<UserRegistrationData>(
                errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
                "User registration plugin not available: " + plugin_name
            );
        }
    }
    
    THEMIS_INFO("Registering user '{}' via plugin '{}'", user_id, plugin->getName());
    return plugin->registerUser(user_id, password, attributes);
}

} // namespace security
} // namespace themis
