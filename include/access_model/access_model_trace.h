/**
 * @file access_model_trace.h
 * @brief Correlation ID and trace context propagation for access model.
 *
 * ThemisDB | File: access_model_trace.h | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 5 Implementation) | Status: Active development
 * Author: Copilot | Date: 2026-08-17
 *
 * Enables trace correlation across cache ↔ storage ↔ coordinator event chain
 * using thread-local context storage and correlation IDs.
 *
 * **Usage Pattern:**
 * ```cpp
 * // Generate correlation ID when event enters system
 * auto corr_id = TraceContextManager::generateCorrelationID();
 * 
 * // Set thread-local context
 * TraceContext ctx{.correlation_id = corr_id};
 * auto scoped = TraceContextManager::ScopedContext(ctx);
 * 
 * // All logs now use active correlation ID automatically
 * accessModelLogger().logEvent(...);
 * ```
 *
 * @see include/access_model/access_model_logging.h
 */

#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Correlation ID Type Alias
// ============================================================================

/**
 * @brief Unique identifier for correlating related events across system.
 *
 * Format: "{prefix}-{uuid}" or "{prefix}-{counter}"
 * Example: "evict-550e8400-e29b-41d4-a716-446655440000"
 */
using CorrelationID = std::string;

// ============================================================================
// § 2  Trace Context
// ============================================================================

/**
 * @brief Thread-local trace context for active operation.
 *
 * Captures:
 * - Correlation ID: Unique ID for this operation chain
 * - Parent span ID: Optional parent operation (for hierarchical tracing)
 * - Start time: When this context began
 */
struct TraceContext {
    /// Correlation ID for this operation
    CorrelationID correlation_id;
    
    /// Parent span ID (optional, for hierarchical tracing)
    std::optional<std::string> parent_span_id;
    
    /// Operation start time
    std::chrono::system_clock::time_point start_time;
    
    TraceContext()
        : correlation_id(),
          parent_span_id(std::nullopt),
          start_time(std::chrono::system_clock::now()) {}
    
    explicit TraceContext(const CorrelationID& id)
        : correlation_id(id),
          parent_span_id(std::nullopt),
          start_time(std::chrono::system_clock::now()) {}
    
    TraceContext(const CorrelationID& id, const std::string& parent_id)
        : correlation_id(id),
          parent_span_id(parent_id),
          start_time(std::chrono::system_clock::now()) {}
};

// ============================================================================
// § 3  Trace Context Manager (Thread-Local Storage)
// ============================================================================

/**
 * @brief Manages thread-local trace context for correlation ID propagation.
 *
 * Singleton providing:
 * - Correlation ID generation
 * - Thread-local context storage
 * - RAII helper for scoped context
 *
 * **Thread-safety:** Each thread maintains its own context stack.
 */
class TraceContextManager {
public:
    /**
     * @brief Generate a new correlation ID.
     *
     * Creates a unique identifier suitable for tracing.
     * Format: "{prefix}-{uuid}" where uuid is generated via random seed.
     *
     * @param prefix Optional prefix (e.g., "evict", "promo", "demotion")
     * @return Unique correlation ID
     */
    static CorrelationID generateCorrelationID(
        const std::string& prefix = "op");
    
    /**
     * @brief Set the thread-local trace context.
     *
     * Replaces any existing context in the current thread.
     * Call must be paired with clearContext() when done.
     *
     * **Prefer ScopedContext RAII helper for exception safety.**
     *
     * @param ctx Context to set
     */
    static void setContext(const TraceContext& ctx);
    
    /**
     * @brief Get the current thread-local trace context.
     *
     * Returns the most recent context set in this thread,
     * or a default empty context if none is set.
     *
     * @return Current trace context (always valid)
     */
    static TraceContext getContext();
    
    /**
     * @brief Clear the thread-local trace context.
     *
     * Resets to default context.
     * **Prefer ScopedContext RAII helper.**
     */
    static void clearContext();
    
    /**
     * @brief Get the correlation ID from the current thread-local context.
     *
     * Convenience accessor for the active correlation ID.
     *
     * @return Current correlation ID (empty string if none set)
     */
    static CorrelationID currentCorrelationID();
    
    // ========================================================================
    // § 3a  RAII Helper for Scoped Context
    // ========================================================================
    
    /**
     * @brief RAII wrapper for setting and clearing thread-local context.
     *
     * Automatically restores previous context on destruction.
     *
     * **Usage:**
     * ```cpp
     * {
     *   TraceContext ctx{TraceContextManager::generateCorrelationID()};
     *   TraceContextManager::ScopedContext guard(ctx);
     *   
     *   // All logs in this scope use ctx.correlation_id
     *   accessModelLogger().logEvent(...);
     * }  // Context automatically cleared
     * ```
     */
    class ScopedContext {
    public:
        /**
         * @brief Create scoped context.
         *
         * Saves current context and sets new one.
         *
         * @param ctx New context to activate
         */
        explicit ScopedContext(const TraceContext& ctx);
        
        /**
         * @brief Destructor: restore previous context.
         *
         * Exception-safe; will not throw.
         */
        ~ScopedContext();
        
        // Disable copy
        ScopedContext(const ScopedContext&) = delete;
        ScopedContext& operator=(const ScopedContext&) = delete;
        
        // Allow move
        ScopedContext(ScopedContext&& other) noexcept
            : previous_context_(std::move(other.previous_context_)),
              context_set_(other.context_set_) {
            other.context_set_ = false;
        }
        
        ScopedContext& operator=(ScopedContext&& other) noexcept {
            if (this != &other) {
                if (context_set_) {
                    setContext(previous_context_);
                }
                previous_context_ = std::move(other.previous_context_);
                context_set_ = other.context_set_;
                other.context_set_ = false;
            }
            return *this;
        }
    
    private:
        TraceContext previous_context_;
        bool context_set_;
    };
};

}  // namespace access_model
}  // namespace themis

