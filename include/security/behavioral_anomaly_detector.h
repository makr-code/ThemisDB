/**
 * @file behavioral_anomaly_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct AccessEvent {
    std::string user_id;      ///< Authenticated identity
    std::string session_id;   ///< Logical session identifier (may equal user_id)
    std::string resource;     ///< Resource accessed (e.g. collection name or path)
    std::string action;       ///< Action performed ("read", "write", "delete", "admin", …)
    std::string client_ip;    ///< Source IP address
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
};

enum class ThreatLevel {
    LOW,      ///< Normal behaviour — no anomaly detected
    MEDIUM,   ///< Mildly unusual — warrant monitoring
    HIGH,     ///< Suspicious behaviour — consider rate-limiting or re-verification
    CRITICAL  ///< Immediately revoke / block session
};

struct ThreatScore {
    ThreatLevel level = ThreatLevel::LOW;
    double score = 0.0;          ///< Numeric score [0.0, 1.0]; higher = more suspicious
    std::string explanation;     ///< Human-readable reason(s)
};

// ============================================================================
// IAnomalyDetector interface
// ============================================================================

class IAnomalyDetector {
public:
    /**
     * @brief IAnomaly Detector.
     * @return Return value.
     */
    virtual ~IAnomalyDetector() = default;

    [[nodiscard]] virtual ThreatScore scoreEvent(const AccessEvent& event) = 0;

    /**
     * @brief Clear Session.
     * @param[in] session_id Identifier of the session.
     */
    virtual void clearSession(const std::string& session_id) = 0;
};

// ============================================================================
// BehavioralAnomalyDetector
// ============================================================================

class BehavioralAnomalyDetector : public IAnomalyDetector {
public:
    struct Config {
        size_t max_events_per_session = 1000;

        double burst_rate_threshold = 50.0;

        std::chrono::seconds burst_window{10};

        int work_hours_start_utc = 8;   ///< 08:00 UTC

        int work_hours_end_utc = 20;    ///< 20:00 UTC

        std::vector<std::string> privileged_actions = {"admin", "delete", "write", "rotate"};
    };

    explicit BehavioralAnomalyDetector(const Config& config = Config{});
    ~BehavioralAnomalyDetector() override = default;

    // Non-copyable, movable
    BehavioralAnomalyDetector(const BehavioralAnomalyDetector&)            = delete;
    BehavioralAnomalyDetector& operator=(const BehavioralAnomalyDetector&) = delete;
    BehavioralAnomalyDetector(BehavioralAnomalyDetector&&) noexcept        noexcept = default;
    BehavioralAnomalyDetector& operator=(BehavioralAnomalyDetector&&) noexcept = default;

    ThreatScore scoreEvent(const AccessEvent& event) override;

    void clearSession(const std::string& session_id) override;

    /**
     * @brief Session Event Count.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
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

    /**
     * @brief Check Burst Rate.
     * @param[in] state Input parameter.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    ThreatScore checkBurstRate(const SessionState& state,
                                const AccessEvent& event) const;
    /**
     * @brief Check Off Hours.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    ThreatScore checkOffHours(const AccessEvent& event) const;
    /**
     * @brief Check Privilege Escalation.
     * @param[in] state Input parameter.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    ThreatScore checkPrivilegeEscalation(const SessionState& state,
                                          const AccessEvent& event) const;
    /**
     * @brief Check Unusual Resource.
     * @param[in] state Input parameter.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    ThreatScore checkUnusualResource(const SessionState& state,
                                      const AccessEvent& event) const;

    /**
     * @brief Max Score.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static ThreatScore maxScore(const ThreatScore& a, const ThreatScore& b);
    /**
     * @brief Level To Score.
     * @param[in] lvl Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static double levelToScore(ThreatLevel lvl) noexcept;
};

} // namespace security
} // namespace themis
