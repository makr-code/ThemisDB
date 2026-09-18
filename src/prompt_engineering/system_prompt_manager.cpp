/**
 * @file system_prompt_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/system_prompt_manager.h"

namespace themis {
namespace prompt_engineering {

// ---------------------------------------------------------------------------
// SystemPrompt serialization
// ---------------------------------------------------------------------------

nlohmann::json SystemPrompt::toJson() const {
    return {
        {"id",          id},
        {"content",     content},
        {"role",        SystemPromptManager::roleToString(role)},
        {"custom_role", custom_role},
        {"version",     version},
        {"active",      active},
        {"metadata",    metadata}
    };
}

/**
 * @brief From Json.
 * @param[in] j Input parameter.
 * @return Return value.
 * @details Calls: value(), SystemPromptManager::stringToRole(), contains(), is_object().
 */
SystemPrompt SystemPrompt::fromJson(const nlohmann::json& j) {
    SystemPrompt sp;
    sp.id          = j.value("id", "");
    sp.content     = j.value("content", "");
    sp.role        = SystemPromptManager::stringToRole(j.value("role", "DEFAULT"));
    sp.custom_role = j.value("custom_role", "");
    sp.version     = j.value("version", "1.0");
    sp.active      = j.value("active", true);
    if (j.contains("metadata") && j["metadata"].is_object()) {
        sp.metadata = j["metadata"];
    }
    return sp;
}

// ---------------------------------------------------------------------------
// Role ↔ string helpers
// ---------------------------------------------------------------------------

/**
 * @brief Role To String.
 * @param[in] role Input parameter.
 * @return Return value.
 * @details Implements roleToString without additional internal calls.
 */
std::string SystemPromptManager::roleToString(Role role) {
    switch (role) {
        case Role::DEFAULT:   return "DEFAULT";
        case Role::USER:      return "USER";
        case Role::ASSISTANT: return "ASSISTANT";
        case Role::ADMIN:     return "ADMIN";
        case Role::SYSTEM:    return "SYSTEM";
        case Role::CUSTOM:    return "CUSTOM";
    }
    return "DEFAULT";
}

/**
 * @brief String To Role.
 * @param[in] role_str Input parameter.
 * @return Return value.
 * @details Implements stringToRole without additional internal calls.
 */
Role SystemPromptManager::stringToRole(const std::string& role_str) {
    if (role_str == "USER") {
      return Role::USER;
    }
    if (role_str == "ASSISTANT") {
      return Role::ASSISTANT;
    }
    if (role_str == "ADMIN") {
      return Role::ADMIN;
    }
    if (role_str == "SYSTEM") {
      return Role::SYSTEM;
    }
    if (role_str == "CUSTOM") {
      return Role::CUSTOM;
    }
    return Role::DEFAULT;
}

// ---------------------------------------------------------------------------
// Private helper — context injection
// ---------------------------------------------------------------------------

std::string SystemPromptManager::injectContext(
    const std::string& content,
    const std::unordered_map<std::string, std::string>& context) {

    std::string result = content;
    for (const auto& [key, value] : context) {
        const std::string placeholder = "{" + key + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos,placeholder.size(), value);
            pos += value.size();
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Standard role API
// ---------------------------------------------------------------------------

/**
 * @brief Set Prompt.
 * @param[in] role Input parameter.
 * @param[in] content Input parameter.
 * @param[in] version Input parameter.
 * @details Calls: roleToString(), lock(), std::move().
 */
void SystemPromptManager::setPrompt(Role role, const std::string& content,
                                    const std::string& version) {
    const std::string key = roleToString(role);
    std::lock_guard<std::mutex> lock(mutex_);

    SystemPrompt sp;
    sp.id      = key;
    sp.content = content;
    sp.role    = role;
    sp.version = version;
    sp.active  = true;

    prompts_[key] = std::move(sp);
}

std::optional<SystemPrompt> SystemPromptManager::getPrompt(Role role) const {
    const std::string key = roleToString(role);
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = prompts_.find(key);
    if (it == prompts_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::string SystemPromptManager::getPromptContent(
    Role role, const std::string& default_content) const {

    auto opt = getPrompt(role);
    if (!opt || !opt->active) {
        return default_content;
    }
    return opt->content;
}

/**
 * @brief Remove Prompt.
 * @param[in] role Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: roleToString(), lock(), erase().
 */
bool SystemPromptManager::removePrompt(Role role) {
    const std::string key = roleToString(role);
    std::lock_guard<std::mutex> lock(mutex_);
    return prompts_.erase(key) > 0;
}

// ---------------------------------------------------------------------------
// Custom role API
// ---------------------------------------------------------------------------

/**
 * @brief Set Custom Prompt.
 * @param[in] role_name Name of the role.
 * @param[in] content Input parameter.
 * @param[in] version Input parameter.
 * @details Calls: lock(), std::move().
 */
void SystemPromptManager::setCustomPrompt(const std::string& role_name,
                                          const std::string& content,
                                          const std::string& version) {
    std::lock_guard<std::mutex> lock(mutex_);

    SystemPrompt sp;
    sp.id          = role_name;
    sp.content     = content;
    sp.role        = Role::CUSTOM;
    sp.custom_role = role_name;
    sp.version     = version;
    sp.active      = true;

    prompts_[role_name] = std::move(sp);
}

std::optional<SystemPrompt> SystemPromptManager::getCustomPrompt(
    const std::string& role_name) const {

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = prompts_.find(role_name);
    if (it == prompts_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::string SystemPromptManager::getCustomPromptContent(
    const std::string& role_name,
    const std::string& default_content) const {

    auto opt = getCustomPrompt(role_name);
    if (!opt || !opt->active) {
        return default_content;
    }
    return opt->content;
}

/**
 * @brief Remove Custom Prompt.
 * @param[in] role_name Name of the role.
 * @return True when the operation succeeds.
 * @details Calls: lock(), erase().
 */
bool SystemPromptManager::removeCustomPrompt(const std::string& role_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return prompts_.erase(role_name) > 0;
}

// ---------------------------------------------------------------------------
// Listing
// ---------------------------------------------------------------------------

std::vector<SystemPrompt> SystemPromptManager::listPrompts() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SystemPrompt> result = {};

    result.reserve(prompts_.size());
    for (const auto& [key, sp] : prompts_) {
        result.push_back(sp);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

std::string SystemPromptManager::renderPrompt(
    Role role,
    const std::unordered_map<std::string, std::string>& context) const {

    std::string content = getPromptContent(role);
    if (content.empty()) {
        return {};
    }
    return injectContext(content, context);
}

std::string SystemPromptManager::renderCustomPrompt(
    const std::string& role_name,
    const std::unordered_map<std::string, std::string>& context) const {

    std::string content = getCustomPromptContent(role_name);
    if (content.empty()) {
        return {};
    }
    return injectContext(content, context);
}

} // namespace prompt_engineering
} // namespace themis
