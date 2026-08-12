/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB — BehavioralAnomalyDetector Tests (Phase 3.3)              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "security/behavioral_anomaly_detector.h"
#include <chrono>
#include <thread>
#include <ctime>

using namespace themis::security;
using Clock = std::chrono::system_clock;

// Portable timegm: interprets struct tm as UTC and returns time_t.
// timegm is POSIX (Linux/macOS) but not available on MSVC.
static time_t portable_timegm(std::tm* t) {
#if defined(_WIN32)
    return _mkgmtime(t);
#else
    return timegm(t);
#endif
}

// ─── Helpers ────────────────────────────────────────────────────────────────

static AccessEvent makeEvent(const std::string& user,
                              const std::string& session,
                              const std::string& resource,
                              const std::string& action,
                              Clock::time_point ts = Clock::now()) {
    AccessEvent e;
    e.user_id    = user;
    e.session_id = session;
    e.resource   = resource;
    e.action     = action;
    e.client_ip  = "127.0.0.1";
    e.timestamp  = ts;
    return e;
}

// ─── Normal baseline ────────────────────────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, NormalBaselineIsLow) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.work_hours_start_utc = cfg.work_hours_end_utc = 0; // disable off-hours heuristic
    BehavioralAnomalyDetector det(cfg);

    auto res = det.scoreEvent(makeEvent("alice", "s1", "collection_a", "read"));
    EXPECT_EQ(res.level, ThreatLevel::LOW);
    EXPECT_DOUBLE_EQ(res.score, 0.0);
}

// ─── Burst-rate detection ───────────────────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, BurstRateDetectedAboveThreshold) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.burst_rate_threshold = 5.0;  // 5 req/s
    cfg.burst_window = std::chrono::seconds{1};
    BehavioralAnomalyDetector det(cfg);

    auto now = Clock::now();
    // Push 10 events within 1-second window → 10 req/s > 5 req/s threshold
    ThreatScore res;
    for (int i = 0; i < 10; ++i) {
        res = det.scoreEvent(makeEvent("alice", "s1", "res", "read", now));
    }
    EXPECT_EQ(res.level, ThreatLevel::HIGH);
    EXPECT_GT(res.score, 0.5);
}

TEST(BehavioralAnomalyDetectorTest, BurstRateNotTriggeredBelowThreshold) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.burst_rate_threshold = 100.0;  // high threshold
    cfg.burst_window = std::chrono::seconds{10};
    cfg.work_hours_start_utc = cfg.work_hours_end_utc = 0; // isolate burst-rate behavior
    BehavioralAnomalyDetector det(cfg);

    auto now = Clock::now();
    ThreatScore res;
    for (int i = 0; i < 5; ++i) {
        res = det.scoreEvent(makeEvent("alice", "s2", "res", "read", now));
    }
    EXPECT_EQ(res.level, ThreatLevel::LOW);
}

// ─── Off-hours detection ────────────────────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, OffHoursDetected) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.work_hours_start_utc = 8;
    cfg.work_hours_end_utc   = 20;
    // Disable other heuristics to isolate
    cfg.burst_rate_threshold = 1e9;
    BehavioralAnomalyDetector det(cfg);

    // Manufacture a timestamp at 02:00 UTC (off-hours)
    // Use 2026-01-01 02:00 UTC
    std::tm t{};
    t.tm_year = 126; t.tm_mon = 0; t.tm_mday = 1;
    t.tm_hour = 2; t.tm_min = 0; t.tm_sec = 0;
    auto off_hours_ts = Clock::from_time_t(portable_timegm(&t));

    auto res = det.scoreEvent(makeEvent("bob", "s3", "secret", "read", off_hours_ts));
    EXPECT_EQ(res.level, ThreatLevel::MEDIUM);
    EXPECT_NE(res.explanation.find("Off-hours"), std::string::npos);
}

TEST(BehavioralAnomalyDetectorTest, InHoursNotFlagged) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.work_hours_start_utc = 8;
    cfg.work_hours_end_utc   = 20;
    cfg.burst_rate_threshold = 1e9;
    BehavioralAnomalyDetector det(cfg);

    // 14:00 UTC — within work hours
    std::tm t{};
    t.tm_year = 126; t.tm_mon = 0; t.tm_mday = 1;
    t.tm_hour = 14; t.tm_min = 0; t.tm_sec = 0;
    auto in_hours_ts = Clock::from_time_t(portable_timegm(&t));

    auto res = det.scoreEvent(makeEvent("carol", "s4", "data", "read", in_hours_ts));
    EXPECT_EQ(res.level, ThreatLevel::LOW);
}

// ─── Privilege escalation ───────────────────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, PrivilegeEscalationOnNewResource) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.burst_rate_threshold = 1e9;
    cfg.work_hours_start_utc = cfg.work_hours_end_utc = 0; // disable off-hours
    BehavioralAnomalyDetector det(cfg);

    // First event: admin action on a resource never seen before
    auto res = det.scoreEvent(makeEvent("eve", "s5", "admin_panel", "admin"));
    EXPECT_EQ(res.level, ThreatLevel::HIGH);
    EXPECT_NE(res.explanation.find("Privileged"), std::string::npos);
}

TEST(BehavioralAnomalyDetectorTest, PrivilegeActionOnKnownResourceIsOk) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.burst_rate_threshold = 1e9;
    cfg.work_hours_start_utc = cfg.work_hours_end_utc = 0;
    BehavioralAnomalyDetector det(cfg);

    // First read access to establish the resource in history
    det.scoreEvent(makeEvent("dave", "s6", "collection_a", "read"));
    // Now a write on the same resource should not trigger escalation
    auto res = det.scoreEvent(makeEvent("dave", "s6", "collection_a", "write"));
    EXPECT_EQ(res.level, ThreatLevel::LOW);
}

// ─── Different user / session isolation ─────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, DifferentSessionsAreIsolated) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.burst_rate_threshold = 2.0;
    cfg.burst_window = std::chrono::seconds{1};
    cfg.work_hours_start_utc = cfg.work_hours_end_utc = 0;
    BehavioralAnomalyDetector det(cfg);

    auto now = Clock::now();
    // session_a gets a burst (should flag)
    for (int i = 0; i < 5; ++i)
        det.scoreEvent(makeEvent("user_a", "session_a", "r", "read", now));

    // session_b has no events — should not be flagged
    auto res_b = det.scoreEvent(makeEvent("user_b", "session_b", "r", "read", now));
    EXPECT_EQ(res_b.level, ThreatLevel::LOW);
}

// ─── Ring-buffer eviction ───────────────────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, RingBufferEvictsOldestWhenFull) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.max_events_per_session = 5;
    cfg.burst_rate_threshold = 1e9;
    cfg.work_hours_start_utc = cfg.work_hours_end_utc = 0;
    BehavioralAnomalyDetector det(cfg);

    // Fill the buffer to capacity + 1
    for (int i = 0; i < 6; ++i)
        det.scoreEvent(makeEvent("frank", "s7", "r", "read"));

    EXPECT_EQ(det.sessionEventCount("s7"), 5u);
}

// ─── clearSession ────────────────────────────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, ClearSessionRemovesHistory) {
    BehavioralAnomalyDetector det;
    det.scoreEvent(makeEvent("grace", "s8", "r", "read"));
    EXPECT_EQ(det.sessionEventCount("s8"), 1u);

    det.clearSession("s8");
    EXPECT_EQ(det.sessionEventCount("s8"), 0u);
}

// ─── Performance ─────────────────────────────────────────────────────────────

TEST(BehavioralAnomalyDetectorTest, ScoreEventMeetsPerformanceTarget) {
    BehavioralAnomalyDetector det;
    constexpr int kRuns = 10000;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kRuns; ++i) {
        det.scoreEvent(makeEvent("perf", "s_perf", "res", "read"));
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avg_ms = elapsed_ms / kRuns;
    // p99 target: ≤ 0.5 ms; average should be well below that
    EXPECT_LT(avg_ms, 0.5) << "avg scoreEvent time " << avg_ms
                            << " ms exceeds 0.5 ms target";
}

// ─── Unusual resource (requires prior flagging) ─────────────────────────────

TEST(BehavioralAnomalyDetectorTest, UnusualResourceFlaggedAfterPriorAlert) {
    BehavioralAnomalyDetector::Config cfg;
    cfg.burst_rate_threshold = 1e9;
    // Keep off-hours enabled at a time we control
    cfg.work_hours_start_utc = 8;
    cfg.work_hours_end_utc   = 20;
    BehavioralAnomalyDetector det(cfg);

    // First event at off-hours → MEDIUM (sets peak_level to MEDIUM)
    std::tm t{};
    t.tm_year = 126; t.tm_mon = 0; t.tm_mday = 1;
    t.tm_hour = 2; t.tm_min = 0; t.tm_sec = 0;
    auto off_ts = Clock::from_time_t(portable_timegm(&t));
    det.scoreEvent(makeEvent("hank", "s9", "known_resource", "read", off_ts));

    // Now access a brand-new resource in normal hours → MEDIUM (unusual resource)
    std::tm t2 = t; t2.tm_hour = 12;
    auto normal_ts = Clock::from_time_t(portable_timegm(&t2));
    auto res = det.scoreEvent(makeEvent("hank", "s9", "new_resource", "read", normal_ts));
    EXPECT_GE(static_cast<int>(res.level), static_cast<int>(ThreatLevel::MEDIUM));
}
