/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            system_prompt_manager.h                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:12:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d135ff3ad9  2026-03-09  feat(prompt_engineering): implement ChainOfThoughtBuilder... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file system_prompt_manager.h
 * @brief System prompt management with per-role overrides.
 *
 * Manages a set of system prompts keyed by a strongly-typed `Role`
 * enumeration.  Each role can carry its own system prompt string, allowing
 * callers to customise LLM behaviour per context (e.g. a stricter prompt for
 * admin users, a friendlier one for end-users).
 *
 * Features:
 *  - CRUD operations for role-based and custom-named system prompts.
 *  - Context-variable injection using `{placeholder}` syntax (mirrors
 *    `PromptManager::injectContext()`).
 *  - `getPromptContent()` falls back to `default_content` when the requested
 *    role has no registered prompt.
 *  - Thread-safe via a `std::mutex`; suitable for concurrent server use.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

/**
 * @brief Predefined roles for system prompts.
 *
 * `CUSTOM` is a sentinel value; use `setCustomPrompt()` /
 * `getCustomPrompt()` with an arbitrary string key instead.
 */
enum class Role {
    DEFAULT,    ///< Fallback role; returned when no role-specific prompt exists
    USER,       ///< End-user facing role
    ASSISTANT,  ///< AI-assistant persona
    ADMIN,      ///< Administrative / privileged role
    SYSTEM,     ///< Low-level system / infrastructure role
    CUSTOM      ///< Sentinel; use custom-role APIs
};

/**
 * @brief Represents a single stored system prompt.
 */
struct SystemPrompt {
    std::string    id;          ///< Internal identifier (role string by default)
    std::string    content;     ///< Prompt text; supports `{placeholder}` variables
    Role           role = Role::DEFAULT; ///< Owning role
    std::string    custom_role; ///< Set when role == Role::CUSTOM
    std::string    version;     ///< Version string (e.g. "1.0")
    bool           active = true;
    nlohmann::json metadata;    ///< Arbitrary metadata for tracking / experiments

    nlohmann::json toJson() const;
    static SystemPrompt fromJson(const nlohmann::json& j);
};

/**
 * @brief Manages system prompts with per-role override support.
 *
 * Usage:
 * @code
 * SystemPromptManager mgr;
 * mgr.setPrompt(Role::USER,
 *               "You are a helpful assistant. Database version: {version}.",
 *               "1.0");
 * mgr.setPrompt(Role::ADMIN,
 *               "You are an expert DBA with unrestricted access.",
 *               "1.0");
 *
 * std::string prompt = mgr.renderPrompt(Role::USER, {{"version", "1.5.0"}});
 * @endcode
 */
class SystemPromptManager {
public:
    SystemPromptManager() = default;

    // -------------------------------------------------------------------------
    // Standard role API
    // -------------------------------------------------------------------------

    /**
     * @brief Set (or replace) the system prompt for a built-in role.
     * @param role     Target role.
     * @param content  Prompt text.  May contain `{placeholder}` tokens.
     * @param version  Optional version string (default "1.0").
     */
    void setPrompt(Role role, const std::string& content,
                   const std::string& version = "1.0");

    /**
     * @brief Retrieve the SystemPrompt record for a built-in role.
     * @return `std::nullopt` if no prompt is registered for @p role.
     */
    std::optional<SystemPrompt> getPrompt(Role role) const;

    /**
     * @brief Return the raw content string for a built-in role.
     * @param role             Target role.
     * @param default_content  Fallback when no prompt is registered.
     * @return Prompt content or @p default_content.
     */
    std::string getPromptContent(Role role,
                                 const std::string& default_content = "") const;

    /**
     * @brief Remove the system prompt for a built-in role.
     * @return `true` if a prompt was removed, `false` if none existed.
     */
    bool removePrompt(Role role);

    // -------------------------------------------------------------------------
    // Custom role API
    // -------------------------------------------------------------------------

    /**
     * @brief Set (or replace) a system prompt for an arbitrary role name.
     * @param role_name  Arbitrary string key, e.g. "legal_reviewer".
     * @param content    Prompt text.
     * @param version    Optional version string.
     */
    void setCustomPrompt(const std::string& role_name,
                         const std::string& content,
                         const std::string& version = "1.0");

    /**
     * @brief Retrieve the SystemPrompt record for a custom role.
     * @return `std::nullopt` if no prompt is registered for @p role_name.
     */
    std::optional<SystemPrompt> getCustomPrompt(
        const std::string& role_name) const;

    /**
     * @brief Return the raw content string for a custom role.
     * @param role_name       Arbitrary string key.
     * @param default_content Fallback when no prompt is registered.
     * @return Prompt content or @p default_content.
     */
    std::string getCustomPromptContent(
        const std::string& role_name,
        const std::string& default_content = "") const;

    /**
     * @brief Remove the system prompt for a custom role.
     * @return `true` if a prompt was removed.
     */
    bool removeCustomPrompt(const std::string& role_name);

    // -------------------------------------------------------------------------
    // Listing
    // -------------------------------------------------------------------------

    /**
     * @brief Return all registered system prompts (both built-in and custom).
     */
    std::vector<SystemPrompt> listPrompts() const;

    // -------------------------------------------------------------------------
    // Rendering (context injection)
    // -------------------------------------------------------------------------

    /**
     * @brief Render the system prompt for a built-in role, substituting
     *        `{key}` tokens with values from @p context.
     *
     * Falls back to an empty string when the role has no prompt.
     *
     * @param role     Target role.
     * @param context  Variable substitution map.
     * @return Rendered prompt string.
     */
    std::string renderPrompt(
        Role role,
        const std::unordered_map<std::string, std::string>& context = {}) const;

    /**
     * @brief Render the system prompt for a custom role with variable substitution.
     * @param role_name  Custom role key.
     * @param context    Variable substitution map.
     * @return Rendered prompt string, or empty string if not found.
     */
    std::string renderCustomPrompt(
        const std::string& role_name,
        const std::unordered_map<std::string, std::string>& context = {}) const;

    // -------------------------------------------------------------------------
    // Role ↔ string conversion helpers
    // -------------------------------------------------------------------------

    static std::string roleToString(Role role);
    static Role        stringToRole(const std::string& role_str);

private:
    /// All prompts stored under their role string key.
    std::unordered_map<std::string, SystemPrompt> prompts_;
    mutable std::mutex mutex_;

    /// Substitute `{key}` tokens in @p content using @p context.
    static std::string injectContext(
        const std::string& content,
        const std::unordered_map<std::string, std::string>& context);
};

} // namespace prompt_engineering
} // namespace themis
