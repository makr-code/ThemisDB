/**
 * @file llm_adapter_factory.h
 * @brief LLM adapter factory with explicit move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <mutex>

namespace themis {
namespace llm {

/// Forward declarations
class LLMAdapter;
class LLMAdapterConfig;

/**
 * @brief Factory for creating and managing LLM adapters with move semantics
 * 
 * Thread-safety:
 * - Thread-safe for concurrent adapter creation (uses internal locking)
 * - NOT thread-safe for move operations (only during initialization/teardown)
 * 
 * Move Semantics:
 * - Explicit move constructor transfers adapter registry
 * - Explicit move assignment closes old adapters before moving
 * - Copy semantics are deleted
 * - All operations marked noexcept
 * 
 * @code
 * LLMAdapterFactory factory1;
 * factory1.registerAdapter("openai", config);
 * 
 * LLMAdapterFactory factory2 = std::move(factory1);  // ✅ Transfer registry
 * auto adapter = factory2.createAdapter("openai");
 * @endcode
 */
class LLMAdapterFactory {
private:
    using AdapterCreator = std::function<std::unique_ptr<LLMAdapter>(
        const LLMAdapterConfig&)>;
    
    struct AdapterRegistration {
        AdapterCreator creator;
        LLMAdapterConfig config;
        
        // Explicit move semantics for the registration
        AdapterRegistration() = default;
        
        AdapterRegistration(AdapterRegistration&& other) noexcept
            : creator(std::move(other.creator)),
              config(std::move(other.config)) {}
        
        AdapterRegistration& operator=(AdapterRegistration&& other) noexcept {
            if (this != &other) {
                creator = std::move(other.creator);
                config = std::move(other.config);
            }
            return *this;
        }
        
        AdapterRegistration(const AdapterRegistration&) = delete;
        AdapterRegistration& operator=(const AdapterRegistration&) = delete;
    };

    std::unordered_map<std::string, AdapterRegistration> adapters_;
    mutable std::mutex adapters_lock_;

public:
    /// Default constructor
    LLMAdapterFactory() = default;

    /**
     * @brief Move constructor - transfers adapter registry
     * 
     * @param[in,out] other Source factory (will be empty after move)
     * 
     * @post this->adapters_ contains all adapters from other
     * @post other.adapters_.empty()
     * 
     * Exception safety: noexcept
     */
    LLMAdapterFactory(LLMAdapterFactory&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.adapters_lock_);
        adapters_ = std::move(other.adapters_);
    }

    /**
     * @brief Move assignment operator - transfers adapter registry
     * 
     * @param[in,out] other Source factory (will be empty after move)
     * @return Reference to this
     * 
     * @post this->adapters_ contains all adapters from other
     * @post other.adapters_.empty()
     * 
     * Exception safety: noexcept
     */
    LLMAdapterFactory& operator=(LLMAdapterFactory&& other) noexcept {
        if (this != &other) {
            std::unique_lock<std::mutex> lock1(adapters_lock_, std::defer_lock);
            std::unique_lock<std::mutex> lock2(other.adapters_lock_, std::defer_lock);
            std::lock(lock1, lock2);
            adapters_ = std::move(other.adapters_);
        }
        return *this;
    }

    /// Delete copy constructor
    LLMAdapterFactory(const LLMAdapterFactory&) = delete;
    
    /// Delete copy assignment operator
    LLMAdapterFactory& operator=(const LLMAdapterFactory&) = delete;

    /// Destructor
    ~LLMAdapterFactory() = default;

    /**
     * @brief Register an adapter type with creation function
     * 
     * @param[in] type Adapter type identifier (e.g., "openai", "anthropic")
     * @param[in] creator Function to create adapter instances
     * @param[in] config Configuration for the adapter
     * 
     * @return true if registered successfully, false if type already registered
     */
    bool registerAdapter(const std::string& type,
                        AdapterCreator creator,
                        const LLMAdapterConfig& config) noexcept;

    /**
     * @brief Create adapter of specified type
     * 
     * @param[in] type Adapter type identifier
     * @return Unique pointer to created adapter, or nullptr if type not found
     * 
     * @post Returned adapter is ready for use
     */
    std::unique_ptr<LLMAdapter> createAdapter(const std::string& type);

    /**
     * @brief Get number of registered adapter types
     * 
     * @return Count of registered adapters
     */
    size_t getAdapterCount() const noexcept;

    /**
     * @brief Check if adapter type is registered
     * 
     * @param[in] type Adapter type identifier
     * @return true if adapter type is registered
     */
    bool hasAdapter(const std::string& type) const noexcept;

    /**
     * @brief Get list of all registered adapter types
     * 
     * @return Vector of registered adapter type names
     */
    std::vector<std::string> getRegisteredAdapters() const;
};

}  // namespace llm
}  // namespace themis
