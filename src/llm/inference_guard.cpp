/**
 * @file inference_guard.cpp
 * @brief Implementation of exception-safe inference context management
 */

#include "llm/inference_guard.h"
#include "llm/inference_engine_enhanced.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {
namespace llm {

// Forward declare the context structure (implementation detail)
struct InferenceContext {
    bool valid = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// InferenceGuard Implementation
// ═══════════════════════════════════════════════════════════════════════════

InferenceGuard::InferenceGuard(InferenceEngineEnhanced& engine)
    : engine_(&engine), context_(nullptr) {
    
    try {
        // Attempt to create context
        // Note: CreateContext would be a virtual method in InferenceEngineEnhanced
        // This is the RAII pattern for safe context creation
        
        if (!context_) {
            throw std::runtime_error("Failed to create inference context (null returned)");
        }
        
        spdlog::debug("InferenceGuard: successfully created context");
    } catch (const std::exception& e) {
        spdlog::error("InferenceGuard initialization failed: {}", e.what());
        // Cleanup on exception
        if (context_ && engine_) {
            try {
                Cleanup();
            } catch (...) {
                spdlog::error("InferenceGuard cleanup exception during initialization");
            }
        }
        context_ = nullptr;
        engine_ = nullptr;
        throw;  // Re-throw
    }
}

InferenceGuard::~InferenceGuard() noexcept {
    try {
        Cleanup();
    } catch (const std::exception& e) {
        spdlog::error("InferenceGuard destructor exception (suppressed): {}", e.what());
        // Suppress to maintain no-throw guarantee
    }
}

InferenceGuard::InferenceGuard(InferenceGuard&& other) noexcept
    : engine_(other.engine_), context_(other.context_) {
    other.engine_ = nullptr;
    other.context_ = nullptr;
    spdlog::debug("InferenceGuard: move constructor");
}

InferenceGuard& InferenceGuard::operator=(InferenceGuard&& other) noexcept {
    if (this != &other) {
        // Clean up existing context
        try {
            Cleanup();
        } catch (const std::exception& e) {
            spdlog::error("InferenceGuard move assignment cleanup exception: {}", e.what());
        }
        
        engine_ = other.engine_;
        context_ = other.context_;
        
        other.engine_ = nullptr;
        other.context_ = nullptr;
        
        spdlog::debug("InferenceGuard: move assignment");
    }
    return *this;
}

InferenceContext& InferenceGuard::Get() {
    if (!context_) {
        throw std::logic_error("InferenceGuard: context accessed after release or error");
    }
    if (!engine_) {
        throw std::logic_error("InferenceGuard: engine is invalid");
    }
    return *context_;
}

const InferenceContext& InferenceGuard::Get() const {
    if (!context_) {
        throw std::logic_error("InferenceGuard: context accessed after release or error");
    }
    if (!engine_) {
        throw std::logic_error("InferenceGuard: engine is invalid");
    }
    return *context_;
}

InferenceContext* InferenceGuard::Release() noexcept {
    auto temp = context_;
    context_ = nullptr;
    engine_ = nullptr;
    spdlog::debug("InferenceGuard: context released (ownership transferred to caller)");
    return temp;
}

void InferenceGuard::Cleanup() noexcept {
    if (context_ && engine_) {
        try {
            spdlog::debug("InferenceGuard: cleaning up context");
            // DestroyContext would be called here
            // engine_->DestroyContext(context_);
            context_ = nullptr;
        } catch (const std::exception& e) {
            spdlog::error("InferenceGuard cleanup exception (suppressed): {}", e.what());
            // Suppress to maintain no-throw
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TokenBufferGuard Implementation
// ═══════════════════════════════════════════════════════════════════════════

TokenBufferGuard::TokenBufferGuard(size_t capacity)
    : max_capacity_(capacity) {
    
    if (capacity == 0) {
        throw std::invalid_argument("TokenBufferGuard: capacity must be > 0");
    }
    
    try {
        tokens_.reserve(capacity);
        spdlog::debug("TokenBufferGuard: created with capacity {}", capacity);
    } catch (const std::bad_alloc& e) {
        spdlog::error("TokenBufferGuard: allocation failed: {}", e.what());
        throw std::runtime_error("Failed to allocate token buffer");
    }
}

void TokenBufferGuard::Push(int32_t token) {
    if (static_cast<int>(tokens_.size()) >= max_capacity_) {
        spdlog::error("TokenBufferGuard: overflow detected at size {}",static_cast<int>(tokens_.size()));
        throw std::overflow_error(
            "Token buffer overflow: size=" + std::to_string(tokens_.size()) +
            " capacity=" + std::to_string(max_capacity_)
        );
    }
    
    try {
        tokens_.push_back(token);
    } catch (const std::bad_alloc& e) {
        spdlog::error("TokenBufferGuard: push allocation failed: {}", e.what());
        throw std::runtime_error("Failed to push token to buffer");
    }
}

int32_t TokenBufferGuard::At(size_t index) const {
    if (index >= static_cast<int>(tokens_.size())) {
        spdlog::error("TokenBufferGuard: index {} out of range [0, {})",
                     index,static_cast<int>(tokens_.size()));
        throw std::out_of_range(
            "Token index out of range: index=" + std::to_string(index) +
            " size=" + std::to_string(tokens_.size())
        );
    }
    return tokens_[index];
}

void TokenBufferGuard::Reserve(size_t size) {
    if (size > max_capacity_) {
        throw std::invalid_argument(
            "TokenBufferGuard: reserve size exceeds capacity"
        );
    }
    
    try {
        tokens_.reserve(size);
    } catch (const std::bad_alloc& e) {
        spdlog::error("TokenBufferGuard: reserve failed: {}", e.what());
        throw std::runtime_error("Failed to reserve token buffer space");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PluginGuard Implementation
// ═══════════════════════════════════════════════════════════════════════════

PluginGuard::PluginGuard(
    const std::string& factory_name,
    const std::string& config
) : factory_name_(factory_name), initialized_(false), plugin_handle_(nullptr) {
    
    if (factory_name.empty()) {
        throw std::invalid_argument("PluginGuard: factory_name cannot be empty");
    }
    
    try {
        spdlog::debug("PluginGuard: initializing plugin '{}'", factory_name);
        
        // Virtual plugin creation would happen here
        // This is a safe pattern with cleanup on failure
        
        initialized_ = true;
        spdlog::info("PluginGuard: plugin '{}' initialized successfully", factory_name);
    } catch (const std::exception& e) {
        spdlog::error("PluginGuard initialization failed: {}", e.what());
        initialized_ = false;
        plugin_handle_ = nullptr;
        throw;
    }
}

PluginGuard::~PluginGuard() noexcept {
    try {
        if (initialized_ && plugin_handle_) {
            spdlog::debug("PluginGuard: cleaning up plugin '{}'", factory_name_);
            // Plugin cleanup would happen here
            plugin_handle_ = nullptr;
        }
        initialized_ = false;
    } catch (const std::exception& e) {
        spdlog::error("PluginGuard destructor exception (suppressed): {}", e.what());
        // Suppress to maintain no-throw
    }
}

PluginGuard::PluginGuard(PluginGuard&& other) noexcept
    : factory_name_(std::move(other.factory_name_)),
      initialized_(other.initialized_),
      plugin_handle_(other.plugin_handle_) {
    other.initialized_ = false;
    other.plugin_handle_ = nullptr;
}

PluginGuard& PluginGuard::operator=(PluginGuard&& other) noexcept {
    if (this != &other) {
        // Clean up existing
        if (initialized_ && plugin_handle_) {
            try {
                spdlog::debug("PluginGuard: move cleanup for '{}'", factory_name_);
            } catch (...) {
                spdlog::error("PluginGuard move cleanup exception suppressed");
            }
        }
        
        factory_name_ = std::move(other.factory_name_);
        initialized_ = other.initialized_;
        plugin_handle_ = other.plugin_handle_;
        
        other.initialized_ = false;
        other.plugin_handle_ = nullptr;
    }
    return *this;
}

bool PluginGuard::IsHealthy() const noexcept {
    if (!initialized_ || !plugin_handle_) {
        return false;
    }
    
    try {
        // Health check would be performed here
        // For now, just check initialization state
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace llm
} // namespace themis
