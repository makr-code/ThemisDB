/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zero_trust_auth_verifier.cpp                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e8e02c9ec  2026-02-24  feat(auth): implement zero-trust continuous verification ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/zero_trust_auth_verifier.h"
#include "auth/auth_audit_logger.h"
#include "auth/session_manager.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

// ============================================================================
// Construction / Destruction
// ============================================================================

ZeroTrustAuthVerifier::ZeroTrustAuthVerifier(
    const Config& config,
    TokenVerifier token_verifier)
    : config_(config)
    , enforcer_(std::move(token_verifier))
{}

ZeroTrustAuthVerifier::~ZeroTrustAuthVerifier() {
    // Signal the monitor loop to stop and wake it so it exits promptly.
    monitor_stop_.store(true);
    monitor_cv_.notify_all();
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    // Drain any in-flight worker tasks before destroying the pool.
    if (worker_pool_) {
        worker_pool_->shutdown();
    }
}

// ============================================================================
// Network policy management
// ============================================================================

void ZeroTrustAuthVerifier::addNetworkPolicy(const security::NetworkPolicy& policy) {
    enforcer_.addNetworkPolicy(policy);
}

bool ZeroTrustAuthVerifier::removeNetworkPolicy(const std::string& policy_id) {
    return enforcer_.removeNetworkPolicy(policy_id);
}

std::vector<security::NetworkPolicy> ZeroTrustAuthVerifier::getNetworkPolicies() const {
    return enforcer_.getNetworkPolicies();
}

// ============================================================================
// Core: continuous per-request verification
// ============================================================================

ZeroTrustAuthVerifier::Decision ZeroTrustAuthVerifier::verify(const Request& req) {
    // Build the per-request context for the underlying enforcer
    security::ZeroTrustContext ctx;
    ctx.request_id = req.request_id;
    ctx.user_id    = req.user_id;
    ctx.client_ip  = req.client_ip;
    ctx.token      = req.token;
    ctx.resource   = req.resource;
    ctx.action     = req.action;
    ctx.device_id  = req.device_id;
    ctx.timestamp  = std::chrono::system_clock::now();

    // Delegate to security layer enforcer (performs token + network checks)
    security::VerificationResult zt = enforcer_.verify(ctx);

    Decision decision;
    decision.request_id       = req.request_id;
    decision.trust_score      = zt.trust_score;
    decision.identity_verified = zt.identity_verified;
    decision.network_ok       = zt.network_policy_passed;

    if (!zt.verified) {
        decision.allowed = false;
        decision.reason  = zt.reason;
        THEMIS_WARN("ZeroTrustAuth: denied user='{}' resource='{}' action='{}' "
                    "reason='{}' request='{}'",
                    req.user_id, req.resource, req.action,
                    zt.reason, req.request_id);
        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logZeroTrustDenied(req.user_id, req.resource, zt.reason, req.request_id);
        }
        return decision;
    }

    // Apply minimum trust score threshold
    if (zt.trust_score < config_.min_trust_score) {
        decision.allowed = false;
        decision.reason  = "Trust score " + std::to_string(zt.trust_score) +
                           " below minimum threshold " +
                           std::to_string(config_.min_trust_score);
        THEMIS_WARN("ZeroTrustAuth: denied (low trust score {:.2f} < {:.2f}) "
                    "user='{}' request='{}'",
                    zt.trust_score, config_.min_trust_score,
                    req.user_id, req.request_id);
        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logZeroTrustDenied(req.user_id, req.resource, decision.reason, req.request_id);
        }
        return decision;
    }

    // All checks passed
    decision.allowed = true;
    decision.reason  = "Zero-trust continuous verification passed";
    THEMIS_DEBUG("ZeroTrustAuth: allowed user='{}' resource='{}' action='{}' "
                 "trust_score={:.2f} request='{}'",
                 req.user_id, req.resource, req.action,
                 zt.trust_score, req.request_id);
    if (audit_logger_) {
        AuthAuditLogger al(audit_logger_);
        al.logZeroTrustAllowed(req.user_id, req.resource, zt.trust_score, req.request_id);
    }
    return decision;
}

// ============================================================================
// Background session monitoring (async policy re-evaluation)
// ============================================================================

void ZeroTrustAuthVerifier::startSessionMonitoring(
    const MonitoredSession& session,
    SessionManager* session_manager)
{
    // Lazily create the worker pool and background monitor thread on the first
    // registered session — avoids thread overhead for callers that only use
    // the synchronous verify() path.
    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);

        if (!worker_pool_) {
            worker_pool_ = std::make_unique<AuthWorkerThreadPool>();
        }

        MonitorEntry entry;
        entry.session         = session;
        entry.session_manager = session_manager;
        entry.next_eval       = std::chrono::system_clock::now()
                                + config_.re_evaluation_interval;
        monitored_sessions_[session.session_id] = std::move(entry);
    }

    // Start the background loop (idempotent: only spawns once).
    if (!monitor_thread_.joinable()) {
        monitor_stop_.store(false);
        monitor_thread_ = std::thread(&ZeroTrustAuthVerifier::monitorLoop, this);
    }

    // Wake the loop in case it's sleeping past the new session's deadline.
    monitor_cv_.notify_one();

    THEMIS_DEBUG("ZeroTrustAuth: monitoring session '{}' for user '{}' (interval={}s)",
                 session.session_id, session.user_id,
                 config_.re_evaluation_interval.count());
}

void ZeroTrustAuthVerifier::stopSessionMonitoring(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    monitored_sessions_.erase(session_id);
}

size_t ZeroTrustAuthVerifier::monitoredSessionCount() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    return monitored_sessions_.size();
}

// ---------------------------------------------------------------------------
// Private: background monitoring loop
// ---------------------------------------------------------------------------

void ZeroTrustAuthVerifier::monitorLoop() {
    for (;;) {
        std::vector<MonitorEntry> to_eval;

        {
            std::unique_lock<std::mutex> lock(monitor_mutex_);

            // Find the earliest deadline across all registered sessions.
            auto now       = std::chrono::system_clock::now();
            auto next_wake = now + config_.re_evaluation_interval;

            for (const auto& [id, entry] : monitored_sessions_) {
                if (entry.next_eval < next_wake) {
                    next_wake = entry.next_eval;
                }
            }

            // Sleep until the next deadline fires, a new session is added, or
            // we're asked to stop.  wait_until returns true only when the
            // predicate (stop requested) becomes true before the deadline.
            bool stop_requested = monitor_cv_.wait_until(
                lock, next_wake,
                [this] { return monitor_stop_.load(); });

            if (stop_requested) {
                return;
            }

            // Collect sessions whose deadline has now passed.
            now = std::chrono::system_clock::now();
            for (auto& [id, entry] : monitored_sessions_) {
                if (entry.next_eval <= now) {
                    to_eval.push_back(entry);
                    // Advance the deadline so we don't re-queue until the
                    // next full interval has elapsed.
                    entry.next_eval = now + config_.re_evaluation_interval;
                }
            }
        }

        // Dispatch each due session to the worker pool — never on this thread.
        for (const auto& entry : to_eval) {
            if (monitor_stop_.load()) {
                return;
            }
            try {
                worker_pool_->submit([this, e = entry]() {
                    reEvaluateSession(e);
                });
            } catch (const std::exception& ex) {
                THEMIS_WARN("ZeroTrustAuth: failed to submit re-evaluation task for "
                            "session '{}': {}", entry.session.session_id, ex.what());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Private: single-session re-evaluation (runs on worker thread)
// ---------------------------------------------------------------------------

void ZeroTrustAuthVerifier::reEvaluateSession(const MonitorEntry& entry) {
    Request req;
    req.request_id = "re_eval_" + entry.session.session_id;
    req.user_id    = entry.session.user_id;
    req.token      = entry.session.token;
    req.client_ip  = entry.session.client_ip;
    req.resource   = entry.session.resource;
    req.action     = entry.session.action;
    req.device_id  = entry.session.device_id;

    auto decision = verify(req);

    if (!decision.allowed) {
        THEMIS_WARN("ZeroTrustAuth: re-evaluation failed for session '{}' "
                    "user='{}' reason='{}'; terminating session",
                    entry.session.session_id, entry.session.user_id,
                    decision.reason);

        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logZeroTrustReEvaluationFailed(
                entry.session.user_id,
                entry.session.session_id,
                decision.reason);
        }

        if (entry.session_manager) {
            entry.session_manager->terminateSession(entry.session.session_id);
        }

        // Remove from future monitoring — session has been revoked.
        stopSessionMonitoring(entry.session.session_id);
    }
}

} // namespace auth
} // namespace themis
