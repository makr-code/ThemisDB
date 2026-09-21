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

using CorrelationID = std::string;

// ============================================================================
// § 2  Trace Context
// ============================================================================

struct TraceContext {
    CorrelationID correlation_id;
    
    std::optional<std::string> parent_span_id;
    
    std::chrono::system_clock::time_point start_time;
    
    /// Default constructor: generates an empty correlation ID and records start time.
    TraceContext()
        : correlation_id(),
          parent_span_id(std::nullopt),
          start_time(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Construct a trace context with a known correlation ID.
     * @param[in] id Correlation ID string to associate with this trace context.
     */
    explicit TraceContext(const CorrelationID& id)
        : correlation_id(id),
          parent_span_id(std::nullopt),
          start_time(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Construct a trace context with a correlation ID and a parent span reference.
     * @param[in] id        Correlation ID string for this context.
     * @param[in] parent_id Parent span ID for distributed-trace hierarchy linkage.
     */
    TraceContext(const CorrelationID& id, const std::string& parent_id)
        : correlation_id(id),
          parent_span_id(parent_id),
          start_time(std::chrono::system_clock::now()) {}
};

// ============================================================================
// § 3  Trace Context Manager (Thread-Local Storage)
// ============================================================================

class TraceContextManager {
public:
    static CorrelationID generateCorrelationID(
        const std::string& prefix = "op");
    
    /**
     * @brief Set Context.
     * @param[in] ctx Input parameter.
     */
    static void setContext(const TraceContext& ctx);
    
    /**
     * @brief Get Context.
     * @return Return value.
     */
    static TraceContext getContext();
    
    /**
     * @brief Clear Context.
     */
    static void clearContext();
    
    /**
     * @brief Current Correlation ID.
     * @return Return value.
     */
    static CorrelationID currentCorrelationID();
    
    // ========================================================================
    // § 3a  RAII Helper for Scoped Context
    // ========================================================================
    
    class ScopedContext {
    public:
        /**
         * @brief Activate @p ctx as the thread-local trace context for the duration of this object.
         *        The previous context is saved and restored on destruction.
         * @param[in] ctx TraceContext to push onto the thread-local stack.
         */
        explicit ScopedContext(const TraceContext& ctx);
        
        /// Restore the previous thread-local trace context.
        ~ScopedContext();
        
        // Disable copy
        /// @cond INTERNAL
        ScopedContext(const ScopedContext&) = delete;
        ScopedContext& operator=(const ScopedContext&) = delete;
        /// @endcond
        
        /**
         * @brief Move-construct, transferring ownership of the saved context.
         * @param[in] other Source ScopedContext; left in a no-restore state after move.
         */
        ScopedContext(ScopedContext&& other) noexcept
            : previous_context_(std::move(other.previous_context_)),
              context_set_(other.context_set_) {
            other.context_set_ = false;
        }
        
        /**
         * @brief Move-assign, restoring any currently active context before taking ownership.
         * @param[in] other Source ScopedContext; left in a no-restore state after move.
         * @return Reference to this ScopedContext.
         */
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

