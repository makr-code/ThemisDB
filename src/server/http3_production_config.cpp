/*
 * ThemisDB | File: http3_production_config.cpp | Version: 0.0.13 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 109
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=34 | delta=31 | status=divergent
 * External Severity (v3): C=6, H=27, M=1
 * PR: #4168 feat(server): HTTP/3 Production Readiness â€” BBR, connection migra... (2026-03-13T09:25:20Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "server/http3_production_config.h"

namespace themis {
namespace server {

// ============================================================================
// Http3FallbackManager
// ============================================================================

Http3FallbackManager::Http3FallbackManager(const Http3ProductionConfig& cfg)
    : cfg_(cfg)
{
}

void Http3FallbackManager::recordQuicFailure(const std::string& client_ip) {
    if (!cfg_.enable_http2_fallback) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto& state = clients_[client_ip];
    ++state.failure_count;

    if (!state.in_fallback &&
        state.failure_count >= cfg_.fallback_failure_threshold) {
        state.in_fallback     = true;
        state.fallback_until  = std::chrono::steady_clock::now() +
                                std::chrono::seconds(cfg_.fallback_recovery_secs);
    }
}

void Http3FallbackManager::recordQuicSuccess(const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(client_ip);
    if (it != clients_.end()) {
        it->second.failure_count = 0;
        it->second.in_fallback   = false;
    }
}

bool Http3FallbackManager::shouldFallbackToHttp2(const std::string& client_ip) const {
    if (!cfg_.enable_http2_fallback) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(client_ip);
    if (it == clients_.end()) {
        return false;
    }

    const auto& state = it->second;
    if (!state.in_fallback) {
        return false;
    }

    // Check if recovery window has expired
    if (std::chrono::steady_clock::now() >= state.fallback_until) {
        // Expired – allow QUIC again (side-effect free; purgeExpired clears it)
        return false;
    }

    return true;
}

std::string Http3FallbackManager::altSvcValue(uint16_t h3_port,
                                               const std::string& client_ip) const {
    if (shouldFallbackToHttp2(client_ip)) {
        return {}; // Suppress Alt-Svc to prevent the client from retrying QUIC
    }
    // e.g. "h3=\":443\"; ma=86400"
    return "h3=\":" + std::to_string(h3_port) + "\"; ma=86400";
}

void Http3FallbackManager::purgeExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    for (auto it = clients_.begin(); it != clients_.end(); ) {
        auto& state = it->second;
        if (state.in_fallback && now >= state.fallback_until) {
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t Http3FallbackManager::fallbackClientCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    auto now = std::chrono::steady_clock::now();
    for (const auto& [ip, state] : clients_) {
        if (state.in_fallback && now < state.fallback_until) {
            ++count;
        }
    }
    return count;
}

} // namespace server
} // namespace themis
