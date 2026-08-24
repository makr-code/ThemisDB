// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file edition_gate.cpp
 * @brief Implementation of edition-aware access control for LLM Wiki plugin.
 *
 * @see include/llm_wiki/edition_gate.h
 */

#include "llm_wiki/edition_gate.h"

#include <cstring>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm_wiki {

// ============================================================================
// Edition detection (stub implementation)
// ============================================================================

Edition getCurrentEdition() noexcept {
    // Phase 3 implementation: check THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
    // Future: read from config, license manager, environment variables
    
    #ifdef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
        return Edition::Enterprise;
    #else
        return Edition::Community;
    #endif
}

bool isLLMWikiEnabled() noexcept {
    Edition current = getCurrentEdition();
    return current == Edition::Enterprise ||
           current == Edition::Hyperscaler ||
           current == Edition::Military;
}

bool isLLMWikiFeatureEnabled(const char* feature_name) noexcept {
    if (!isLLMWikiEnabled()) {
        return false;
    }
    
    // Phase 3: All sub-features are available if the plugin is enabled
    // Future: check against license keys and feature flags
    
    if (std::strcmp(feature_name, "llm_wiki_wikipedia") == 0) {
        // Wikipedia feature requires explicit license (future work)
        return true;  // For now, enabled if plugin is enabled
    }
    
    if (std::strcmp(feature_name, "llm_wiki_workspace") == 0) {
        // Workspace feature available in all enterprise+ editions
        return true;
    }
    
    // Unknown feature
    return false;
}

PluginStatus enforcePluginGate(const char* operation_name) noexcept {
    if (!isLLMWikiEnabled()) {
        auto logger = spdlog::get("themisdb.llm_wiki");
        if (logger) {
            logger->warn("LLM Wiki plugin operation '{}' blocked: not available in this edition",
                        operation_name);
        }
        return PluginStatus::PermissionDenied(
            std::string("LLM Wiki plugin operation '") + operation_name +
            "' is not available in this ThemisDB edition. "
            "Upgrade to enterprise, hyperscaler, or military edition to use this feature.");
    }
    return PluginStatus::Ok();
}

PluginStatus enforceFeatureGate(const char* feature_name) noexcept {
    if (!isLLMWikiFeatureEnabled(feature_name)) {
        auto logger = spdlog::get("themisdb.llm_wiki");
        if (logger) {
            logger->warn("LLM Wiki feature '{}' blocked: not available in this edition",
                        feature_name);
        }
        return PluginStatus::PermissionDenied(
            std::string("LLM Wiki feature '") + feature_name +
            "' is not available in this ThemisDB edition. "
            "Upgrade to enterprise or higher to use this feature.");
    }
    return PluginStatus::Ok();
}

} // namespace llm_wiki
} // namespace themis
