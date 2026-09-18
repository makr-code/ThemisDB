/**
 * @file voice_error_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.42
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Error handling and resilience for Phase 8 production readiness
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace themis { namespace voice {
using json = nlohmann::json;

// Voice-specific error types
enum class VoiceErrorCode {
    NONE = 0,
    INITIALIZATION_FAILED,
    MODEL_NOT_LOADED,
    AUDIO_PROCESSING_FAILED,
    STT_FAILED,
    TTS_FAILED,
    LLM_FAILED,
    SESSION_NOT_FOUND,
    SESSION_EXPIRED,
    CONSENT_MISSING,
    RATE_LIMIT_EXCEEDED,
    NETWORK_ERROR,
    TIMEOUT,
    STORAGE_FAILED,
    SECURITY_VIOLATION,
    UNKNOWN
};

/**
 * @brief TBD: Describe errorCodeToString.
 * @param[in] code Input parameter.
 * @return Return value.
 */
std::string errorCodeToString(VoiceErrorCode code);

// Voice error exception
/** @brief Voice error exception. */
class VoiceException : public std::runtime_error {
public:
    VoiceException(VoiceErrorCode code, const std::string& message);
    VoiceErrorCode code() const { return code_; }
private:
    VoiceErrorCode code_;
};

// Circuit breaker states
enum class CircuitState {
    CLOSED,      // Normal operation
    OPEN,        // Failing - reject requests
    HALF_OPEN    // Testing if service recovered
};

/**
 * @brief TBD: Describe circuitStateToString.
 * @param[in] state Input parameter.
 * @return Return value.
 */
std::string circuitStateToString(CircuitState state);

// Circuit breaker config
struct CircuitBreakerConfig {
    size_t failure_threshold = 5;
    size_t success_threshold = 2;
    int64_t open_duration_ms = 30000;
    int64_t half_open_probe_interval_ms = 5000;
};

// Circuit breaker for voice services
/** @brief Circuit breaker for voice services. */
class VoiceCircuitBreaker {
public:
    explicit VoiceCircuitBreaker(const std::string& name, const CircuitBreakerConfig& config = {});

    template<typename Func>
    /**
     * @brief TBD: Describe call.
     * @param[in] func Input parameter.
     * @return True on success.
     * @details Calls: canCall(), func(), recordSuccess(), recordFailure().
     */
    bool call(Func&& func) {
        if (!canCall()) {
          return false;
        }
        try {
            func();
            recordSuccess();
            return true;
        } catch (...) {
            recordFailure();
            return false;
        }
    }

    /**
     * @brief TBD: Describe canCall.
     * @return True on success.
     */
    bool canCall();
    /**
     * @brief TBD: Describe recordSuccess.
     */
    void recordSuccess();
    /**
     * @brief TBD: Describe recordFailure.
     */
    void recordFailure();
    /**
     * @brief TBD: Describe reset.
     */
    void reset();

    /**
     * @brief TBD: Describe getState.
     * @return Return value.
     */
    CircuitState getState() const;
    std::string getName() const { return name_; }
    /**
     * @brief TBD: Describe getStats.
     * @return Return value.
     */
    json getStats() const;

private:
    std::string name_;
    CircuitBreakerConfig config_;
    mutable std::mutex mutex_;

    CircuitState state_ = CircuitState::CLOSED;
    size_t failure_count_ = 0;
    size_t success_count_ = 0;
    int64_t last_failure_time_ms_ = 0;
    int64_t last_state_change_ms_ = 0;

    uint64_t total_calls_ = 0;
    uint64_t successful_calls_ = 0;
    uint64_t rejected_calls_ = 0;

    /**
     * @brief TBD: Describe nowMs.
     * @return Return value.
     */
    int64_t nowMs() const;
};

// Retry configuration with exponential backoff
struct RetryConfig {
    size_t max_attempts = 3;
    int64_t initial_delay_ms = 100;
    float backoff_multiplier = 2.0f;
    int64_t max_delay_ms = 5000;
    bool jitter = true;
    std::vector<VoiceErrorCode> retryable_errors = {
        VoiceErrorCode::NETWORK_ERROR,
        VoiceErrorCode::TIMEOUT,
        VoiceErrorCode::STORAGE_FAILED
    };
};

// Retry helper with exponential backoff
/** @brief Retry helper with exponential backoff. */
class VoiceRetryHandler {
public:
    explicit VoiceRetryHandler(const RetryConfig& config = {});

    template<typename T, typename Func>
    T executeWithRetry(Func&& func, const std::string& /*operation_name*/ = "") {
        size_t attempt = 0;
        int64_t delay = config_.initial_delay_ms;
        while (true) {
            try {
                return func();
            } catch (const VoiceException& e) {
                attempt++;
                bool is_retryable = std::find(
                    config_.retryable_errors.begin(),
                    config_.retryable_errors.end(),
                    e.code()
                ) != config_.retryable_errors.end();

                if (!is_retryable || attempt >= config_.max_attempts) {
                    total_failures_++;
                    throw;
                }
                total_retries_++;
                sleepMs(delay);
                delay = std::min(static_cast<int64_t>(delay * config_.backoff_multiplier), config_.max_delay_ms);
            }
        }
    }

    /**
     * @brief TBD: Describe getStats.
     * @return Return value.
     */
    json getStats() const;

private:
    RetryConfig config_;
    std::atomic<uint64_t> total_retries_{0};
    std::atomic<uint64_t> total_failures_{0};

    /**
     * @brief TBD: Describe sleepMs.
     * @param[in] ms Input parameter.
     */
    void sleepMs(int64_t ms) const;
};

// Fallback strategy for graceful degradation
/** @brief Fallback strategy for graceful degradation. */
class VoiceFallbackStrategy {
public:
    struct FallbackResult {
        bool used_fallback = false;
        std::string fallback_type;
        std::string result;
    };

    /**
     * @brief TBD: Describe sttFallback.
     * @param[in] error_context Input parameter.
     * @return Return value.
     */
    static FallbackResult sttFallback(const std::string& error_context);
    /**
     * @brief TBD: Describe ttsFallback.
     * @param[in] error_context Input parameter.
     * @return Return value.
     */
    static FallbackResult ttsFallback(const std::string& error_context);
    /**
     * @brief TBD: Describe llmFallback.
     * @param[in] user_input Input parameter.
     * @return Return value.
     */
    static FallbackResult llmFallback(const std::string& user_input);
    /**
     * @brief TBD: Describe sessionFallback.
     * @param[in] session_id Input parameter.
     * @return Return value.
     */
    static FallbackResult sessionFallback(const std::string& session_id);
};

// Error context for Phase 3 diagnostics and audit
struct ErrorContext {
    VoiceErrorCode error_code = VoiceErrorCode::NONE;
    int64_t timestamp_ms = 0;
    std::string cause;                  // e.g., "LLM timeout after 30s"
    std::string recovery_action;        // e.g., "please try again"
    std::string user_id;
    std::string session_id;
    std::string action;                 // e.g., "STT processing"
    std::string auth_token_masked;      // e.g., "token_****..."
    json audit_context;
    
    // Serialize to JSON for logging (no sensitive data)
    json toJson() const {
        return json{
            {"error_code", errorCodeToString(error_code)},
            {"timestamp_ms", timestamp_ms},
            {"cause", cause},
            {"recovery_action", recovery_action},
            {"user_id", user_id},
            {"session_id", session_id},
            {"action", action},
            {"auth_token_masked", auth_token_masked},
            {"audit_context", audit_context}
        };
    }
};

// VoiceErrorHandler: Phase 8 production component
/** @brief VoiceErrorHandler: Phase 8 production component. */
class VoiceErrorHandler {
public:
    VoiceErrorHandler();
    ~VoiceErrorHandler() = default;

    /**
     * @brief TBD: Describe sttCircuit.
     * @return Return value.
     */
    VoiceCircuitBreaker& sttCircuit();
    /**
     * @brief TBD: Describe ttsCircuit.
     * @return Return value.
     */
    VoiceCircuitBreaker& ttsCircuit();
    /**
     * @brief TBD: Describe llmCircuit.
     * @return Return value.
     */
    VoiceCircuitBreaker& llmCircuit();
    /**
     * @brief TBD: Describe storageCircuit.
     * @return Return value.
     */
    VoiceCircuitBreaker& storageCircuit();

    /**
     * @brief TBD: Describe getRetryHandler.
     * @return Return value.
     */
    VoiceRetryHandler& getRetryHandler();

    json handleError(VoiceErrorCode code, const std::string& context, const std::string& details = "");
    
    /**
     * @brief Phase 3: Error context with diagnostics
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    json createErrorContext(const ErrorContext& ctx);
    
    /**
     * @brief Phase 3: Log error with sanitized context (no credentials)
     * @param[in] ctx Input parameter.
     */
    void logErrorWithContext(const ErrorContext& ctx);

    /**
     * @brief TBD: Describe isSystemHealthy.
     * @return True on success.
     */
    bool isSystemHealthy() const;
    /**
     * @brief TBD: Describe getHealthStatus.
     * @return Return value.
     */
    json getHealthStatus() const;

private:
    VoiceCircuitBreaker stt_circuit_{"stt"};
    VoiceCircuitBreaker tts_circuit_{"tts"};
    VoiceCircuitBreaker llm_circuit_{"llm"};
    VoiceCircuitBreaker storage_circuit_{"storage"};
    VoiceRetryHandler retry_handler_;
    mutable std::mutex mutex_;
    uint64_t total_errors_ = 0;
};

}} // namespace themis::voice
