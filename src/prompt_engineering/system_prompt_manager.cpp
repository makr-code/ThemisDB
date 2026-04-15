/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            system_prompt_manager.cpp                          ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:43:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     237                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d135ff3ad9  2026-03-09  feat(prompt_engineering): implement ChainOfThoughtBuilder... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

Role SystemPromptManager::stringToRole(const std::string& role_str) {
    if (role_str == "USER")      return Role::USER;
    if (role_str == "ASSISTANT") return Role::ASSISTANT;
    if (role_str == "ADMIN")     return Role::ADMIN;
    if (role_str == "SYSTEM")    return Role::SYSTEM;
    if (role_str == "CUSTOM")    return Role::CUSTOM;
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
            result.replace(pos, placeholder.size(), value);
            pos += value.size();
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Standard role API
// ---------------------------------------------------------------------------

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

bool SystemPromptManager::removePrompt(Role role) {
    const std::string key = roleToString(role);
    std::lock_guard<std::mutex> lock(mutex_);
    return prompts_.erase(key) > 0;
}

// ---------------------------------------------------------------------------
// Custom role API
// ---------------------------------------------------------------------------

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

bool SystemPromptManager::removeCustomPrompt(const std::string& role_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return prompts_.erase(role_name) > 0;
}

// ---------------------------------------------------------------------------
// Listing
// ---------------------------------------------------------------------------

std::vector<SystemPrompt> SystemPromptManager::listPrompts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SystemPrompt> result;
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
