/**
 * @file zero_trust_auth_verifier.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/zero_trust_auth_verifier.h"
#include "auth/auth_audit_logger.h"
#include "auth/auth_worker_thread_pool.h"
#include "auth/session_manager.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

// ============================================================================
// Construction / Destruction
// ============================================================================

ZeroTrustAuthVerifier::ZeroTrustAuthVerifier()
    : ZeroTrustAuthVerifier(Config{}, nullptr)
{}

ZeroTrustAuthVerifier::ZeroTrustAuthVerifier(TokenVerifier token_verifier)
    : ZeroTrustAuthVerifier(Config{}, std::move(token_verifier))
{}

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
    bool needs_spawn = false;

    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);

        // Lazily create the worker pool on the first registered session.
        if (!worker_pool_) {
            worker_pool_ = std::make_unique<AuthWorkerThreadPool>();
        }

        MonitorEntry entry;
        entry.session         = session;
        entry.session_manager = session_manager;
        entry.next_eval       = std::chrono::steady_clock::now()
                                + config_.re_evaluation_interval;
        monitored_sessions_[session.session_id] = std::move(entry);

        // Increment the generation counter so the sleeping monitor loop can
        // detect the schedule change and re-compute its next wake deadline.
        ++schedule_generation_;

        // Determine whether a (re-)spawn is needed.  Hold the mutex throughout so
        // concurrent callers cannot double-spawn.
        //   thread_exited      — the thread ran but stopped because sessions
        //                        became empty (monitor_stop_ was set by
        //                        stopSessionMonitoring when the map drained).
        //   thread_never_started — initial state: default-constructed thread.
        const bool thread_exited        = monitor_stop_.load();
        const bool thread_never_started = !monitor_thread_.joinable();
        if (thread_exited || thread_never_started) {
            monitor_stop_.store(false);
            needs_spawn = true;
        }
    }

    if (needs_spawn) {
        // Join the previous thread if it finished but was never joined.
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        monitor_thread_ = std::thread(&ZeroTrustAuthVerifier::monitorLoop, this);
    } else {
        // Wake a sleeping loop so it can re-compute the earliest deadline.
        monitor_cv_.notify_one();
    }

    THEMIS_DEBUG("ZeroTrustAuth: monitoring session '{}' for user '{}' (interval={}ms)",
                 session.session_id, session.user_id,
                 config_.re_evaluation_interval.count());
}

void ZeroTrustAuthVerifier::stopSessionMonitoring(const std::string& session_id) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        monitored_sessions_.erase(session_id);

        // When all sessions are removed, stop the background thread and pool
        // to avoid holding idle threads indefinitely.
        if (monitored_sessions_.empty()) {
            monitor_stop_.store(true);
        }
    }

    if (monitor_stop_.load()) {
        monitor_cv_.notify_all();
    }
}

size_t ZeroTrustAuthVerifier::monitoredSessionCount() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    return static_cast<int>(monitored_sessions_.size());
}

// ---------------------------------------------------------------------------
// Private: background monitoring loop
// ---------------------------------------------------------------------------

void ZeroTrustAuthVerifier::monitorLoop() {
    for (;;) {
        std::vector<MonitorEntry> to_eval;

        {
            std::unique_lock<std::mutex> lock(monitor_mutex_);

            // Exit immediately if explicitly stopped or nothing to monitor.
            if (monitor_stop_.load() || monitored_sessions_.empty()) {
                return;
            }

            // Find the earliest deadline across all registered sessions.
            auto now       = std::chrono::steady_clock::now();
            auto next_wake = now + config_.re_evaluation_interval;

            for (const auto& [id, entry] : monitored_sessions_) {
                if (entry.next_eval < next_wake) {
                    next_wake = entry.next_eval;
                }
            }

            // Snapshot the generation before sleeping so the predicate can
            // detect when a new session is added (which bumps the generation)
            // and re-compute the earliest deadline without sleeping until the
            // old `next_wake`.
            const auto schedule_gen = schedule_generation_.load();

            // Sleep until the deadline, a stop is requested, the session map
            // becomes empty, or a new session is registered.
            monitor_cv_.wait_until(
                lock, next_wake,
                [this, schedule_gen] {
                    return monitor_stop_.load()
                        || monitored_sessions_.empty()
                    || schedule_generation_.load() != schedule_gen;
                });

            if (monitor_stop_.load() || monitored_sessions_.empty()) {
                return;
            }

            // If the generation changed (new session added), loop back to
            // re-compute the earliest wake deadline before dispatching.
            if (schedule_generation_.load() != schedule_gen) {
                continue;
            }

            // Collect sessions whose deadline has now passed.
            now = std::chrono::steady_clock::now();
            for (auto& [id, entry] : monitored_sessions_) {
                if (entry.next_eval <= now) {
                    to_eval.push_back(entry);
                    // Advance the deadline so we do not re-queue until the
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
    // Guard against stale work items: verify the session is still registered
    // before taking any action (it may have been stopped after being queued).
    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        if (monitored_sessions_.find(entry.session.session_id) ==
                monitored_sessions_.end()) {
            return; // Session was already stopped; nothing to do.
        }
    }

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
