/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_error_handler.h                              ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-04-13 04:22:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     229                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

std::string errorCodeToString(VoiceErrorCode code);

// Voice error exception
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

std::string circuitStateToString(CircuitState state);

// Circuit breaker config
struct CircuitBreakerConfig {
    size_t failure_threshold = 5;
    size_t success_threshold = 2;
    int64_t open_duration_ms = 30000;
    int64_t half_open_probe_interval_ms = 5000;
};

// Circuit breaker for voice services
class VoiceCircuitBreaker {
public:
    explicit VoiceCircuitBreaker(const std::string& name, const CircuitBreakerConfig& config = {});

    template<typename Func>
    bool call(Func&& func) {
        if (!canCall()) return false;
        try {
            func();
            recordSuccess();
            return true;
        } catch (...) {
            recordFailure();
            return false;
        }
    }

    bool canCall();
    void recordSuccess();
    void recordFailure();
    void reset();

    CircuitState getState() const;
    std::string getName() const { return name_; }
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

    json getStats() const;

private:
    RetryConfig config_;
    std::atomic<uint64_t> total_retries_{0};
    std::atomic<uint64_t> total_failures_{0};

    void sleepMs(int64_t ms) const;
};

// Fallback strategy for graceful degradation
class VoiceFallbackStrategy {
public:
    struct FallbackResult {
        bool used_fallback = false;
        std::string fallback_type;
        std::string result;
    };

    static FallbackResult sttFallback(const std::string& error_context);
    static FallbackResult ttsFallback(const std::string& error_context);
    static FallbackResult llmFallback(const std::string& user_input);
    static FallbackResult sessionFallback(const std::string& session_id);
};

// VoiceErrorHandler: Phase 8 production component
class VoiceErrorHandler {
public:
    VoiceErrorHandler();
    ~VoiceErrorHandler() = default;

    VoiceCircuitBreaker& sttCircuit();
    VoiceCircuitBreaker& ttsCircuit();
    VoiceCircuitBreaker& llmCircuit();
    VoiceCircuitBreaker& storageCircuit();

    VoiceRetryHandler& getRetryHandler();

    json handleError(VoiceErrorCode code, const std::string& context, const std::string& details = "");

    bool isSystemHealthy() const;
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
