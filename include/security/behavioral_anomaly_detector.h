/**
 * @file behavioral_anomaly_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstdint>

namespace themis {
namespace security {

// ============================================================================
// Public types
// ============================================================================

/**
 * @brief A single observable access event fed to BehavioralAnomalyDetector.
 */
struct AccessEvent {
    std::string user_id;      ///< Authenticated identity
    std::string session_id;   ///< Logical session identifier (may equal user_id)
    std::string resource;     ///< Resource accessed (e.g. collection name or path)
    std::string action;       ///< Action performed ("read", "write", "delete", "admin", …)
    std::string client_ip;    ///< Source IP address
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
};

/**
 * @brief Threat level returned by the anomaly detector.
 */
enum class ThreatLevel {
    LOW,      ///< Normal behaviour — no anomaly detected
    MEDIUM,   ///< Mildly unusual — warrant monitoring
    HIGH,     ///< Suspicious behaviour — consider rate-limiting or re-verification
    CRITICAL  ///< Immediately revoke / block session
};

/**
 * @brief Score and explanation produced by BehavioralAnomalyDetector::scoreEvent().
 */
struct ThreatScore {
    ThreatLevel level = ThreatLevel::LOW;
    double score = 0.0;          ///< Numeric score [0.0, 1.0]; higher = more suspicious
    std::string explanation;     ///< Human-readable reason(s)
};

// ============================================================================
// IAnomalyDetector interface
// ============================================================================

/**
 * @brief Abstract anomaly-detector interface — consumers depend on this rather
 *        than on the concrete BehavioralAnomalyDetector.
 *
 * Implementations must be thread-safe.
 */
class IAnomalyDetector {
public:
    virtual ~IAnomalyDetector() = default;

    /**
     * @brief Score a new access event against the session's behavioural history.
     *
     * The event is added to the session's ring buffer before scoring.
     *
     * @param event Inbound access event.
     * @return ThreatScore with level and explanation.
     */
    [[nodiscard]] virtual ThreatScore scoreEvent(const AccessEvent& event) = 0;

    /**
     * @brief Discard all state for a given session (e.g. on logout).
     */
    virtual void clearSession(const std::string& session_id) = 0;
};

// ============================================================================
// BehavioralAnomalyDetector
// ============================================================================

/**
 * @brief Sliding-window behavioural anomaly detector.
 *
 * Maintains an in-process ring buffer (up to `max_events_per_session` events)
 * per session and scores each new event against four heuristics:
 *
 *  1. **Burst-rate detection** — request rate over a short window (default 10 s)
 *     exceeds `burst_rate_threshold` requests/second → HIGH.
 *
 *  2. **Off-hours access** — access outside configured work-hours window
 *     (24-hour UTC clock) → MEDIUM.
 *
 *  3. **Privilege escalation** — the sequence `admin`, `delete`, or `write` on
 *     a resource that the session has not previously accessed → HIGH.
 *
 *  4. **Unusual resource access** — a resource accessed for the first time by
 *     a session that has already been flagged at MEDIUM or above → MEDIUM.
 *
 * The final ThreatScore is the maximum across all triggered heuristics.
 *
 * When the ring buffer is full, the oldest event is evicted (FIFO).
 *
 * Thread safety: all public methods are thread-safe via per-session mutex.
 */
class BehavioralAnomalyDetector : public IAnomalyDetector {
public:
    /**
     * @brief Configuration for the detector.
     */
    struct Config {
        /// Maximum number of events retained per session (ring-buffer capacity).
        size_t max_events_per_session = 1000;

        /// Burst rate threshold: requests per second that triggers HIGH threat.
        double burst_rate_threshold = 50.0;

        /// Burst rate window: time window over which burst rate is measured.
        std::chrono::seconds burst_window{10};

        /// Work-hours start (UTC, inclusive). Off-hours detection is disabled
        /// when work_hours_start == work_hours_end.
        int work_hours_start_utc = 8;   ///< 08:00 UTC

        /// Work-hours end (UTC, exclusive).
        int work_hours_end_utc = 20;    ///< 20:00 UTC

        /// Privileged actions that trigger the escalation heuristic.
        std::vector<std::string> privileged_actions = {"admin", "delete", "write", "rotate"};
    };

    explicit BehavioralAnomalyDetector(const Config& config = Config{});
    ~BehavioralAnomalyDetector() override = default;

    // Non-copyable, movable
    BehavioralAnomalyDetector(const BehavioralAnomalyDetector&)            = delete;
    BehavioralAnomalyDetector& operator=(const BehavioralAnomalyDetector&) = delete;
    BehavioralAnomalyDetector(BehavioralAnomalyDetector&&) noexcept        = default;
    BehavioralAnomalyDetector& operator=(BehavioralAnomalyDetector&&) noexcept = default;

    /**
     * @brief Score a new access event against the session's behavioural history.
     *
     * Adds the event to the session ring buffer (evicting the oldest entry if
     * full), then evaluates all configured heuristics.  Returns the highest
     * threat level across all heuristics.
     *
     * Performance target: p99 ≤ 0.5 ms per call.
     */
    ThreatScore scoreEvent(const AccessEvent& event) override;

    /**
     * @brief Evict all state for the given session.
     */
    void clearSession(const std::string& session_id) override;

    /**
     * @brief Return the number of events currently buffered for a session.
     */
    size_t sessionEventCount(const std::string& session_id) const;

    const Config& getConfig() const noexcept { return config_; }

private:
    // ── Per-session state ──────────────────────────────────────────────────
    struct SessionState {
        std::deque<AccessEvent> events;  ///< Ring buffer (oldest at front)
        ThreatLevel peak_level = ThreatLevel::LOW; ///< Highest level seen so far
    };

    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SessionState> sessions_;

    // ── Heuristic evaluators ───────────────────────────────────────────────
    ThreatScore checkBurstRate(const SessionState& state,
                                const AccessEvent& event) const;
    ThreatScore checkOffHours(const AccessEvent& event) const;
    ThreatScore checkPrivilegeEscalation(const SessionState& state,
                                          const AccessEvent& event) const;
    ThreatScore checkUnusualResource(const SessionState& state,
                                      const AccessEvent& event) const;

    static ThreatScore maxScore(const ThreatScore& a, const ThreatScore& b);
    static double levelToScore(ThreatLevel lvl) noexcept;
};

} // namespace security
} // namespace themis
