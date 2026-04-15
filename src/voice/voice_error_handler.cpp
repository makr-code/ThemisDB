/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_error_handler.cpp                            ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-15 18:51:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     307                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file voice_error_handler.cpp
 * @brief Error handling and resilience implementation (Phase 8 production readiness)
 */

#include "voice/voice_error_handler.h"
#include <chrono>
#include <thread>
#include <sstream>

namespace themis { namespace voice {

// ---- Free functions ----

std::string errorCodeToString(VoiceErrorCode code) {
    switch (code) {
        case VoiceErrorCode::NONE:                  return "NONE";
        case VoiceErrorCode::INITIALIZATION_FAILED: return "INITIALIZATION_FAILED";
        case VoiceErrorCode::MODEL_NOT_LOADED:      return "MODEL_NOT_LOADED";
        case VoiceErrorCode::AUDIO_PROCESSING_FAILED: return "AUDIO_PROCESSING_FAILED";
        case VoiceErrorCode::STT_FAILED:            return "STT_FAILED";
        case VoiceErrorCode::TTS_FAILED:            return "TTS_FAILED";
        case VoiceErrorCode::LLM_FAILED:            return "LLM_FAILED";
        case VoiceErrorCode::SESSION_NOT_FOUND:     return "SESSION_NOT_FOUND";
        case VoiceErrorCode::SESSION_EXPIRED:       return "SESSION_EXPIRED";
        case VoiceErrorCode::CONSENT_MISSING:       return "CONSENT_MISSING";
        case VoiceErrorCode::RATE_LIMIT_EXCEEDED:   return "RATE_LIMIT_EXCEEDED";
        case VoiceErrorCode::NETWORK_ERROR:         return "NETWORK_ERROR";
        case VoiceErrorCode::TIMEOUT:               return "TIMEOUT";
        case VoiceErrorCode::STORAGE_FAILED:        return "STORAGE_FAILED";
        case VoiceErrorCode::SECURITY_VIOLATION:    return "SECURITY_VIOLATION";
        default:                                    return "UNKNOWN";
    }
}

std::string circuitStateToString(CircuitState state) {
    switch (state) {
        case CircuitState::CLOSED:    return "CLOSED";
        case CircuitState::OPEN:      return "OPEN";
        case CircuitState::HALF_OPEN: return "HALF_OPEN";
        default:                      return "UNKNOWN";
    }
}

// ---- VoiceException ----

VoiceException::VoiceException(VoiceErrorCode code, const std::string& message)
    : std::runtime_error(message), code_(code) {}

// ---- VoiceCircuitBreaker ----

VoiceCircuitBreaker::VoiceCircuitBreaker(const std::string& name, const CircuitBreakerConfig& config)
    : name_(name), config_(config) {}

int64_t VoiceCircuitBreaker::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool VoiceCircuitBreaker::canCall() {
    std::lock_guard<std::mutex> lock(mutex_);
    switch (state_) {
        case CircuitState::CLOSED:
            ++total_calls_;
            return true;

        case CircuitState::OPEN: {
            // Check if open duration has passed -> transition to HALF_OPEN
            int64_t now = nowMs();
            if (now - last_state_change_ms_ >= config_.open_duration_ms) {
                state_ = CircuitState::HALF_OPEN;
                success_count_ = 0;
                last_state_change_ms_ = now;
                ++total_calls_;
                return true;
            }
            ++rejected_calls_;
            return false;
        }

        case CircuitState::HALF_OPEN:
            ++total_calls_;
            return true;
    }
    return false;
}

void VoiceCircuitBreaker::recordSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++successful_calls_;
    if (state_ == CircuitState::HALF_OPEN) {
        ++success_count_;
        if (success_count_ >= config_.success_threshold) {
            state_ = CircuitState::CLOSED;
            failure_count_ = 0;
            success_count_ = 0;
            last_state_change_ms_ = nowMs();
        }
    } else if (state_ == CircuitState::CLOSED) {
        failure_count_ = 0;
    }
}

void VoiceCircuitBreaker::recordFailure() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++failure_count_;
    last_failure_time_ms_ = nowMs();

    if (state_ == CircuitState::CLOSED) {
        if (failure_count_ >= config_.failure_threshold) {
            state_ = CircuitState::OPEN;
            last_state_change_ms_ = nowMs();
        }
    } else if (state_ == CircuitState::HALF_OPEN) {
        state_ = CircuitState::OPEN;
        last_state_change_ms_ = nowMs();
        success_count_ = 0;
    }
}

void VoiceCircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = CircuitState::CLOSED;
    failure_count_ = 0;
    success_count_ = 0;
    last_failure_time_ms_ = 0;
    last_state_change_ms_ = 0;
}

CircuitState VoiceCircuitBreaker::getState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

json VoiceCircuitBreaker::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    json stats;
    stats["name"]           = name_;
    stats["state"]          = circuitStateToString(state_);
    stats["failure_count"]  = failure_count_;
    stats["success_count"]  = success_count_;
    stats["total_calls"]    = total_calls_;
    stats["successful_calls"] = successful_calls_;
    stats["rejected_calls"] = rejected_calls_;
    return stats;
}

// ---- VoiceRetryHandler ----

VoiceRetryHandler::VoiceRetryHandler(const RetryConfig& config)
    : config_(config) {}

void VoiceRetryHandler::sleepMs(int64_t ms) const {
    if (ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

json VoiceRetryHandler::getStats() const {
    json stats;
    stats["total_retries"]  = total_retries_.load();
    stats["total_failures"] = total_failures_.load();
    return stats;
}

// ---- VoiceFallbackStrategy ----

VoiceFallbackStrategy::FallbackResult VoiceFallbackStrategy::sttFallback(
    [[maybe_unused]] const std::string& error_context)
{
    FallbackResult res;
    res.used_fallback  = true;
    res.fallback_type  = "stt_empty_transcript";
    res.result         = "";
    return res;
}

VoiceFallbackStrategy::FallbackResult VoiceFallbackStrategy::ttsFallback(
    [[maybe_unused]] const std::string& error_context)
{
    FallbackResult res;
    res.used_fallback  = true;
    res.fallback_type  = "tts_silent_audio";
    res.result         = "";
    return res;
}

VoiceFallbackStrategy::FallbackResult VoiceFallbackStrategy::llmFallback(
    const std::string& /*user_input*/)
{
    FallbackResult res;
    res.used_fallback  = true;
    res.fallback_type  = "llm_canned_response";
    res.result         = "I'm sorry, I'm having trouble processing your request right now. Please try again later.";
    return res;
}

VoiceFallbackStrategy::FallbackResult VoiceFallbackStrategy::sessionFallback(
    const std::string& session_id)
{
    FallbackResult res;
    res.used_fallback  = true;
    res.fallback_type  = "temporary_session";
    res.result         = "temp_" + session_id;
    return res;
}

// ---- VoiceErrorHandler ----

VoiceErrorHandler::VoiceErrorHandler()
    : stt_circuit_("stt")
    , tts_circuit_("tts")
    , llm_circuit_("llm")
    , storage_circuit_("storage")
{}

VoiceCircuitBreaker& VoiceErrorHandler::sttCircuit()     { return stt_circuit_; }
VoiceCircuitBreaker& VoiceErrorHandler::ttsCircuit()     { return tts_circuit_; }
VoiceCircuitBreaker& VoiceErrorHandler::llmCircuit()     { return llm_circuit_; }
VoiceCircuitBreaker& VoiceErrorHandler::storageCircuit() { return storage_circuit_; }
VoiceRetryHandler&   VoiceErrorHandler::getRetryHandler(){ return retry_handler_; }

json VoiceErrorHandler::handleError(
    VoiceErrorCode code, const std::string& context, const std::string& details)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ++total_errors_;

    json report;
    report["error_code"]      = errorCodeToString(code);
    report["context"]         = context;
    report["details"]         = details;
    report["total_errors"]    = total_errors_;

    // Suggest recovery action
    switch (code) {
        case VoiceErrorCode::SESSION_NOT_FOUND:
        case VoiceErrorCode::SESSION_EXPIRED:
            report["recovery_action"] = "create_new_session";
            break;
        case VoiceErrorCode::CONSENT_MISSING:
            report["recovery_action"] = "request_consent";
            break;
        case VoiceErrorCode::RATE_LIMIT_EXCEEDED:
            report["recovery_action"] = "backoff_and_retry";
            break;
        case VoiceErrorCode::NETWORK_ERROR:
        case VoiceErrorCode::TIMEOUT:
            report["recovery_action"] = "retry_with_backoff";
            break;
        case VoiceErrorCode::MODEL_NOT_LOADED:
            report["recovery_action"] = "reload_model";
            break;
        default:
            report["recovery_action"] = "log_and_continue";
            break;
    }

    return report;
}

bool VoiceErrorHandler::isSystemHealthy() const {
    return stt_circuit_.getState()     != CircuitState::OPEN &&
           tts_circuit_.getState()     != CircuitState::OPEN &&
           llm_circuit_.getState()     != CircuitState::OPEN &&
           storage_circuit_.getState() != CircuitState::OPEN;
}

json VoiceErrorHandler::getHealthStatus() const {
    json status;
    status["healthy"]          = isSystemHealthy();
    status["stt_circuit"]      = stt_circuit_.getStats();
    status["tts_circuit"]      = tts_circuit_.getStats();
    status["llm_circuit"]      = llm_circuit_.getStats();
    status["storage_circuit"]  = storage_circuit_.getStats();
    status["retry_stats"]      = retry_handler_.getStats();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        status["total_errors"] = total_errors_;
    }
    return status;
}

}} // namespace themis::voice
