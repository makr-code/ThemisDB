// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file edition_gate.h
 * @brief Edition-aware access control for LLM Wiki plugin.
 *
 * Enforces that the LLM Wiki plugin and certain features are only available
 * in enterprise, hyperscaler, and military editions. Community and minimal
 * builds return `PermissionDenied` for plugin operations.
 *
 * ## Compile-time gating
 *
 * Define `THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED` when building enterprise+ editions:
 *
 * @code
 *   cmake -DTHEMISDB_LLM_WIKI_ENTERPRISE_ENABLED=ON <build_dir>
 * @endcode
 *
 * ## Feature-level gating
 *
 * Sub-features like `"llm_wiki_wikipedia"` (Wikipedia dump ingestion) require
 * explicit license keys in addition to edition checks.
 *
 * ## Runtime behavior
 *
 * - Community/minimal: All plugin calls return `Status::PermissionDenied()`
 * - Enterprise/hyperscaler/military: Full plugin functionality
 * - Wikipedia sub-feature: Additional license key check
 *
 * @version 1.0.0 (Phase 3 hardening)
 */

#pragma once

#include "llm_wiki/llm_wiki_plugin_interface.h"

namespace themis {
namespace llm_wiki {

// Import Status from the plugin interface namespace so it resolves within
// this namespace without full qualification.
using ::themis::plugins::llm_wiki::Status;

// ============================================================================
// Edition enumeration
// ============================================================================

/**
 * @brief ThemisDB edition level.
 */
enum class Edition {
    Community,      ///< Open-source community edition
    Minimal,        ///< Minimal stripped-down edition
    Enterprise,     ///< Enterprise edition
    Hyperscaler,    ///< Hyperscaler edition
    Military,       ///< Military-grade edition
};

// ============================================================================
// Edition detection and gating
// ============================================================================

/**
 * @brief Detect the current ThemisDB edition at runtime.
 *
 * Checks compile-time defines and runtime configuration to determine
 * which edition is active.
 *
 * @return Current edition.
 */
[[nodiscard]] Edition getCurrentEdition() noexcept;

/**
 * @brief Check if the LLM Wiki plugin is enabled in the current edition.
 *
 * The plugin is available in enterprise, hyperscaler, and military editions.
 * Returns false for community and minimal.
 *
 * @return True if plugin is available; false otherwise.
 */
[[nodiscard]] bool isLLMWikiEnabled() noexcept;

/**
 * @brief Check if a specific LLM Wiki sub-feature is enabled.
 *
 * Sub-features include:
 *  - "llm_wiki_wikipedia" — Wikipedia dump ingestion (enterprise+ only)
 *  - "llm_wiki_workspace" — Persistent workspace (enterprise+ only)
 *  - (Future) multi-tenant, RBAC, quality evaluation
 *
 * @param feature_name  Name of the sub-feature (case-sensitive).
 * @return              True if available; false otherwise.
 */
[[nodiscard]] bool isLLMWikiFeatureEnabled(const char* feature_name) noexcept;

/**
 * @brief Enforce edition gate for a plugin operation.
 *
 * If the plugin is not enabled in the current edition, returns
 * `Status::PermissionDenied()`. Otherwise, returns `Status::Ok()`.
 *
 * Use this in all public API entry points.
 *
 * @param operation_name  Name of the operation (for error message).
 * @return                Status indicating whether access is allowed.
 */
[[nodiscard]] Status enforcePluginGate(const char* operation_name) noexcept;

/**
 * @brief Enforce edition gate for a specific LLM Wiki sub-feature.
 *
 * If the sub-feature is not available in the current edition, returns
 * `Status::PermissionDenied()`. Otherwise, returns `Status::Ok()`.
 *
 * @param feature_name  Name of the sub-feature (e.g., "llm_wiki_wikipedia").
 * @return              Status indicating whether access is allowed.
 */
[[nodiscard]] Status enforceFeatureGate(const char* feature_name) noexcept;

// ============================================================================
// Inline implementation for compile-time gating
// ============================================================================

/**
 * @brief Compile-time check: is LLM Wiki enabled?
 *
 * Evaluates to true only if THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED is defined.
 */
#ifdef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
    constexpr bool kLLMWikiCompileTimeEnabled = true;
#else
    constexpr bool kLLMWikiCompileTimeEnabled = false;
#endif

/**
 * @brief Compile-time check: is Wikipedia sub-feature enabled?
 *
 * Evaluates to true only if THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED is defined.
 */
#ifdef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
    constexpr bool kLLMWikiWikipediaCompileTimeEnabled = true;
#else
    constexpr bool kLLMWikiWikipediaCompileTimeEnabled = false;
#endif

} // namespace llm_wiki
} // namespace themis
