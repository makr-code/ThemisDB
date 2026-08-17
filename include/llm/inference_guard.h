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

/**
 * @brief Exception-safe RAII guard for inference context lifecycle
 * 
 * Guarantees:
 * - Strong exception safety: full initialization or exception
 * - Automatic cleanup on scope exit (normal or exception)
 * - No-throw destructor
 * - Proper ownership semantics with move support
 * 
 * @example
 * @code
 * try {
 *   InferenceGuard context_guard(engine);
 *   auto& ctx = context_guard.Get();
 *   
 *   InferenceRequest req = {...};
 *   InferenceResponse resp = engine.Infer(ctx, req);
 *   
 *   // Context auto-cleaned on exit
 * } catch (const std::exception& e) {
 *   spdlog::error("Inference failed: {}", e.what());
 *   // Context still cleaned up
 * }
 * @endcode
 */
class InferenceGuard {
public:
    /**
     * @brief Construct and initialize inference context
     * 
     * Creates a new inference context via the inference engine.
     * Strong exception safety: either fully succeeds or throws without
     * leaving partial state.
     * 
     * @param engine InferenceEngineEnhanced instance (not owned, must outlive this guard)
     * @throws std::invalid_argument if engine is null
     * @throws std::runtime_error if context creation fails
     */
    explicit InferenceGuard(InferenceEngineEnhanced& engine);
    
    /**
     * @brief Destructor: guaranteed no-throw cleanup
     * 
     * Destroys the inference context if valid.
     * Never throws, ensuring exception safety.
     */
    ~InferenceGuard() noexcept;
    
    // Prevent copying (enforce resource ownership semantics)
    InferenceGuard(const InferenceGuard&) = delete;
    InferenceGuard& operator=(const InferenceGuard&) = delete;
    
    // Allow move semantics (transfer ownership)
    /**
     * @brief Move constructor: transfers context ownership
     */
    InferenceGuard(InferenceGuard&& other) noexcept;
    
    /**
     * @brief Move assignment: transfers context ownership
     */
    InferenceGuard& operator=(InferenceGuard&& other) noexcept;
    
    /**
     * @brief Get mutable reference to inference context
     * 
     * @return Reference to managed InferenceContext
     * @throws std::logic_error if context is invalid or released
     */
    InferenceContext& Get();
    
    /**
     * @brief Get const reference to inference context
     * 
     * @return Const reference to managed InferenceContext
     * @throws std::logic_error if context is invalid or released
     */
    const InferenceContext& Get() const;
    
    /**
     * @brief Get pointer to inference context
     * 
     * @return Pointer to InferenceContext or nullptr if invalid/released
     */
    InferenceContext* GetPtr() noexcept { return context_; }
    const InferenceContext* GetPtr() const noexcept { return context_; }
    
    /**
     * @brief Check if context is valid and ready to use
     * 
     * @return true if context is valid, false if released or error
     */
    bool IsValid() const noexcept { return context_ != nullptr && engine_ != nullptr; }
    
    /**
     * @brief Release ownership of context (caller becomes responsible for cleanup)
     * 
     * After this call, the guard no longer manages the context.
     * Caller must ensure proper cleanup via engine->DestroyContext()
     * 
     * @return Pointer to InferenceContext (caller owns cleanup responsibility)
     */
    InferenceContext* Release() noexcept;

private:
    InferenceEngineEnhanced* engine_;  ///< Engine reference (not owned)
    InferenceContext* context_;        ///< Managed context (owned by engine)
    
    /**
     * @brief Cleanup helper (guaranteed no-throw)
     */
    void Cleanup() noexcept;
};

/**
 * @brief RAII guard for exception-safe token buffer management
 * 
 * Provides automatic bounds checking and overflow detection for token sequences.
 */
class TokenBufferGuard {
public:
    /**
     * @brief Construct token buffer with capacity
     * 
     * @param capacity Maximum number of tokens
     * @throws std::invalid_argument if capacity is 0
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
     * @brief Add token to buffer with bounds checking
     * 
     * @param token Token value to add
     * @throws std::overflow_error if buffer capacity exceeded
     */
    void Push(int32_t token);
    
    /**
     * @brief Get token at index with bounds checking
     * 
     * @param index Position in buffer
     * @return Token value
     * @throws std::out_of_range if index is invalid
     */
    int32_t At(size_t index) const;
    
    /**
     * @brief Get number of tokens in buffer
     */
    size_t Size() const noexcept { return tokens_.size(); }
    
    /**
     * @brief Get buffer capacity
     */
    size_t Capacity() const noexcept { return max_capacity_; }
    
    /**
     * @brief Get remaining capacity
     */
    size_t Remaining() const noexcept { 
        return max_capacity_ > tokens_.size() ? max_capacity_ - tokens_.size() : 0;
    }
    
    /**
     * @brief Check if buffer is full
     */
    bool IsFull() const noexcept { return tokens_.size() >= max_capacity_; }
    
    /**
     * @brief Get token data for reading
     */
    const int32_t* Data() const noexcept { 
        return tokens_.empty() ? nullptr : tokens_.data();
    }
    
    /**
     * @brief Clear all tokens
     */
    void Clear() noexcept { tokens_.clear(); }
    
    /**
     * @brief Pre-allocate tokens (may throw std::bad_alloc)
     */
    void Reserve(size_t size);

private:
    std::vector<int32_t> tokens_;
    size_t max_capacity_;
};

/**
 * @brief RAII guard for exception-safe plugin lifecycle
 * 
 * Manages plugin initialization, health checks, and cleanup.
 */
class PluginGuard {
public:
    /**
     * @brief Construct and initialize plugin
     * 
     * @param factory_name Name of plugin factory
     * @param config Configuration JSON (optional)
     * @throws std::invalid_argument if factory_name is empty
     * @throws std::runtime_error if plugin creation/initialization fails
     * @throws std::logic_error if plugin health check fails
     */
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
     * @brief Check if plugin is healthy and ready
     */
    bool IsHealthy() const noexcept;
    
    /**
     * @brief Get plugin name
     */
    const std::string& GetName() const noexcept { return factory_name_; }
    
    /**
     * @brief Check plugin initialization status
     */
    bool IsInitialized() const noexcept { return initialized_; }

private:
    std::string factory_name_;
    bool initialized_;
    void* plugin_handle_;  // Opaque pointer to actual plugin
};

} // namespace llm
} // namespace themis
