/**
 * @file inference_guard.h
 * @brief Exception-safe RAII guard for inference context lifecycle
 * @version 1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * 
 * Provides exception-safe inference context management with proper
 * resource cleanup, ownership transfer, and state tracking.
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <utility>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// Forward declarations
class InferenceEngineEnhanced;
struct InferenceContext;
struct InferenceRequest;
struct InferenceResponse;

class InferenceGuard {
public:
    /**
     * @brief Inference Guard.
     * @param[in,out] engine Input/output parameter.
     * @return Return value.
     */
    explicit InferenceGuard(InferenceEngineEnhanced& engine);
    
    ~InferenceGuard() noexcept;
    
    // Prevent copying (enforce resource ownership semantics)
    InferenceGuard(const InferenceGuard&) = delete;
    InferenceGuard& operator=(const InferenceGuard&) = delete;
    
    // Allow move semantics (transfer ownership)
    InferenceGuard(InferenceGuard&& other) noexcept;
    
    InferenceGuard& operator=(InferenceGuard&& other) noexcept;
    
    /**
     * @brief Get.
     * @return Return value.
     */
    InferenceContext& Get();
    
    /**
     * @brief Get.
     * @return Return value.
     */
    const InferenceContext& Get() const;
    
    InferenceContext* GetPtr() noexcept { return context_; }
    const InferenceContext* GetPtr() const noexcept { return context_; }
    
    bool IsValid() const noexcept { return context_ != nullptr && engine_ != nullptr; }
    
    /**
     * @brief Release.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    InferenceContext* Release() noexcept;

private:
    InferenceEngineEnhanced* engine_;  ///< Engine reference (not owned)
    InferenceContext* context_;        ///< Managed context (owned by engine)
    
    /**
     * @brief Cleanup.
     * @note Exception safety: noexcept.
     */
    void Cleanup() noexcept;
};

class TokenBufferGuard {
public:
    /**
     * @brief Token Buffer Guard.
     * @param[in] capacity Input parameter.
     * @return Return value.
     */
    explicit TokenBufferGuard(size_t capacity);
    
    ~TokenBufferGuard() noexcept = default;
    
    // Prevent copying
    TokenBufferGuard(const TokenBufferGuard&) = delete;
    TokenBufferGuard& operator=(const TokenBufferGuard&) = delete;
    
    // Allow move
    TokenBufferGuard(TokenBufferGuard&&) noexcept = default;
    TokenBufferGuard& operator=(TokenBufferGuard&&) noexcept = default;
    
    /**
     * @brief Push.
     * @param[in] token Input parameter.
     */
    void Push(int32_t token);
    
    /**
     * @brief At.
     * @param[in] index Input parameter.
     * @return Return value.
     */
    int32_t At(size_t index) const;
    
    size_t Size() const noexcept { return tokens_.size(); }
    
    size_t Capacity() const noexcept { return max_capacity_; }
    
    size_t Remaining() const noexcept { 
        return max_capacity_ > tokens_.size() ? max_capacity_ - tokens_.size() : 0;
    }
    
    bool IsFull() const noexcept { return tokens_.size() >= max_capacity_; }
    
    const int32_t* Data() const noexcept { 
        return tokens_.empty() ? nullptr : tokens_.data();
    }
    
    void Clear() noexcept { tokens_.clear(); }
    
    /**
     * @brief Reserve.
     * @param[in] size Input parameter.
     */
    void Reserve(size_t size);

private:
    std::vector<int32_t> tokens_;
    size_t max_capacity_;
};

class PluginGuard {
public:
    explicit PluginGuard(
        const std::string& factory_name,
        const std::string& config = ""
    );
    
    ~PluginGuard() noexcept;
    
    // Prevent copying
    PluginGuard(const PluginGuard&) = delete;
    PluginGuard& operator=(const PluginGuard&) = delete;
    
    // Allow move
    PluginGuard(PluginGuard&&) noexcept;
    PluginGuard& operator=(PluginGuard&&) noexcept;
    
    /**
     * @brief Is Healthy.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool IsHealthy() const noexcept;
    
    const std::string& GetName() const noexcept { return factory_name_; }
    
    bool IsInitialized() const noexcept { return initialized_; }

private:
    std::string factory_name_;
    bool initialized_;
    void* plugin_handle_;  // Opaque pointer to actual plugin
};

} // namespace llm
} // namespace themis
