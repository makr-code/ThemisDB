// THEMIS_GAP_STATS: gaps=14 unimpl=0 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            http3_production_config.cpp                        ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:50:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     123                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7d190644e6  2026-03-13  feat(server): HTTP/3 production readiness - congestion co... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
