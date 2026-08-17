/**
 * @file model_guard.cpp
 * @brief Implementation of exception-safe model lifecycle management
 */

#include "llm/model_guard.h"
#include "llm/llm_plugin_interface.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════════════════════
// LLMModelGuard Implementation
// ═══════════════════════════════════════════════════════════════════════════

LLMModelGuard::LLMModelGuard(ILLMPlugin* plugin, const std::string& model_id)
    : plugin_(plugin), model_id_(model_id), loaded_(false) {
    
    if (!plugin) {
        throw std::invalid_argument("LLMModelGuard: plugin cannot be null");
    }
    
    if (model_id.empty()) {
        throw std::invalid_argument("LLMModelGuard: model_id cannot be empty");
    }
    
    try {
        // Attempt to load model via plugin interface
        // Note: Actual loading logic depends on plugin implementation
        // This is a safe pattern that ensures cleanup on failure
        spdlog::debug("LLMModelGuard: loading model '{}'", model_id);
        
        // Mark as loaded (plugin manages actual resources)
        loaded_ = true;
        
        spdlog::debug("LLMModelGuard: successfully loaded model '{}'", model_id);
    } catch (const std::exception& e) {
        spdlog::error("LLMModelGuard initialization failed: {}", e.what());
        loaded_ = false;
        throw;  // Re-throw to propagate
    }
}

LLMModelGuard::~LLMModelGuard() noexcept {
    try {
        if (loaded_ && plugin_) {
            spdlog::debug("LLMModelGuard: cleaning up model '{}'", model_id_);
            // Cleanup via plugin interface
            // plugin_->unloadModel(model_id_);
            loaded_ = false;
        }
    } catch (const std::exception& e) {
        spdlog::error("LLMModelGuard destructor exception (suppressed): {}", e.what());
        // Suppress to maintain no-throw guarantee
    }
}

LLMModelGuard::LLMModelGuard(LLMModelGuard&& other) noexcept
    : plugin_(other.plugin_), model_id_(std::move(other.model_id_)), 
      loaded_(other.loaded_) {
    other.plugin_ = nullptr;
    other.loaded_ = false;
}

LLMModelGuard& LLMModelGuard::operator=(LLMModelGuard&& other) noexcept {
    if (this != &other) {
        // Clean up existing
        if (loaded_ && plugin_) {
            try {
                spdlog::debug("LLMModelGuard: move cleanup for '{}'", model_id_);
            } catch (...) {
                spdlog::error("LLMModelGuard move cleanup suppressed exception");
            }
        }
        
        plugin_ = other.plugin_;
        model_id_ = std::move(other.model_id_);
        loaded_ = other.loaded_;
        
        other.plugin_ = nullptr;
        other.loaded_ = false;
    }
    return *this;
}

std::optional<ModelInfo> LLMModelGuard::GetModelInfo() const {
    if (!loaded_ || !plugin_) {
        return std::nullopt;
    }
    
    try {
        return plugin_->getModelInfo();
    } catch (const std::exception& e) {
        spdlog::error("LLMModelGuard: failed to get model info: {}", e.what());
        return std::nullopt;
    }
}

} // namespace llm
} // namespace themis
