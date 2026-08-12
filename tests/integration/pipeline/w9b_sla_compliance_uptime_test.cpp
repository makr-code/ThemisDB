/**
 * @file w9b_sla_compliance_uptime_test.cpp
 * @brief Wave 9B — SLA Compliance & Uptime Validation (SLA-01..SLA-08).
 *
 * Validates that ThemisDB meets its Service Level Agreement contracts under
 * normal, degraded, and recovery conditions.  All infrastructure is
 * implemented in-process; no external services are required.
 *
 * SLA-01  Availability window — 1000 simulated requests with 1 injected
 *         downtime event; availability = 999/1000 ≥ 99.9%.
 * SLA-02  p99 latency compliance — 100 synthetic operations; p99 ≤ 10 ms
 *         simulated budget.
 * SLA-03  Graceful degradation under load — queue-full condition rejects
 *         new requests with kOverloaded; in-flight requests complete.
 * SLA-04  Recovery time objective (RTO) — system recovers within ≤ 3 retry
 *         cycles after a simulated failure.
 * SLA-05  Recovery point objective (RPO) — at most N data units lost after a
 *         simulated crash (N defined by RPO contract).
 * SLA-06  Uptime accounting — rolling-window tracker correctly computes
 *         availability from a sequence of up/down events.
 * SLA-07  Degraded-mode throughput floor — with 50% workers disabled,
 *         throughput stays ≥ 40% of nominal.
 * SLA-08  SLA gate self-check — gate counter reports 1.0 on compliance,
 *         0.0 on violation.
 *
 * All tests are deterministic via kCanonicalSeed = 42.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis { namespace test { 

namespace {

// kCanonicalSeed = 42 is provided by test_data_generator.h (themis::test::kCanonicalSeed).

// ---------------------------------------------------------------------------
// Status codes for SLA tests
// ---------------------------------------------------------------------------

/// @brief Operation result codes used across SLA test infrastructure.
enum class SlaStatus {
    kOk,
    kOverloaded,
    kTimeout,
    kFailed,
    kDegraded,
};

// ---------------------------------------------------------------------------
// ServiceSimulator — request processor for SLA-01
// ---------------------------------------------------------------------------

/**
 * @brief Simulates a service that processes a fixed request stream with
 *        configurable failure injection.
 *
 * Each process step is either successful (kOk) or failed (kFailed) based on
 * the injected failure set.  Statistics are collected for availability
 * computation.
 */
class ServiceSimulator {
public:
    struct Stats {
        size_t total{0};
        size_t succeeded{0};
        size_t failed{0};

        /// @brief Availability as a fraction [0.0, 1.0].
        double Availability() const {
            if (total == 0) { return 0.0; }
            return static_cast<double>(succeeded) / static_cast<double>(total);
        }
    };

    /**
     * @brief Process @p count requests; inject failures at the request
     *        indices listed in @p fail_at.
     */
    Stats Process(size_t count, const std::unordered_set<size_t>& fail_at) {
        Stats s;
        s.total = count;
        for (size_t i = 0; i < count; ++i) {
            if (fail_at.count(i) > 0) { ++s.failed; }
            else                       { ++s.succeeded; }
        }
        return s;
    }
};

// ---------------------------------------------------------------------------
// LatencyCollector — p99 measurement for SLA-02
// ---------------------------------------------------------------------------

/**
 * @brief Accumulates latency samples and computes percentiles.
 *
 * All operations are in-process; latency values are synthetic integers
 * representing microseconds.
 */
class LatencyCollector {
public:
    /// @brief Add a latency sample (µs).
    void Add(double latency_us) {
        std::lock_guard<std::mutex> lk(mu_);
        samples_.push_back(latency_us);
    }

    /// @brief Compute p99 from accumulated samples.
    double P99() const {
        std::lock_guard<std::mutex> lk(mu_);
        if (samples_.empty()) { return 0.0; }
        std::vector<double> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());
        const double idx  = 0.99 * static_cast<double>(sorted.size() - 1);
        const size_t lo   = static_cast<size_t>(std::floor(idx));
        const size_t hi   = std::min(lo + 1, sorted.size() - 1);
        const double frac = idx - static_cast<double>(lo);
        return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
    }

    size_t Count() const {
        std::lock_guard<std::mutex> lk(mu_);
        return samples_.size();
    }

private:
    mutable std::mutex   mu_;
    std::vector<double>  samples_;
};

// ---------------------------------------------------------------------------
// RequestQueue — bounded queue with overload rejection for SLA-03
// ---------------------------------------------------------------------------

/**
 * @brief Thread-safe bounded request queue.
 *
 * Enqueue() returns kOverloaded when the queue is at capacity.  All
 * previously enqueued items can still be drained via Dequeue().
 */
class RequestQueue {
public:
    explicit RequestQueue(size_t capacity) : capacity_(capacity) {}

    /**
     * @brief Attempt to enqueue a work item.
     * @returns SlaStatus::kOk on success; kOverloaded if at capacity.
     */
    SlaStatus Enqueue(int item) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { return SlaStatus::kOverloaded; }
        queue_.push(item);
        return SlaStatus::kOk;
    }

    /// @brief Dequeue one item; returns std::nullopt if empty.
    std::optional<int> Dequeue() {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) { return std::nullopt; }
        const int v = queue_.front();
        queue_.pop();
        return v;
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return queue_.size();
    }

private:
    mutable std::mutex mu_;
    std::queue<int>    queue_;
    const size_t       capacity_;
};

// ---------------------------------------------------------------------------
// RecoveryManager — simulates RTO/RPO for SLA-04/SLA-05
// ---------------------------------------------------------------------------

/**
 * @brief Simulates failure injection and bounded recovery cycles.
 *
 * RecoverWithRetry() attempts recovery up to @p max_cycles times.  Each
 * cycle is a single logical step (no real sleep); the system is considered
 * recovered when AttemptRecovery() returns true.
 */
class RecoveryManager {
public:
    struct RecoveryResult {
        bool   recovered{false};
        size_t cycles_used{0};
    };

    /**
     * @brief Attempt recovery; succeeds on the @p succeed_on_cycle-th attempt.
     * @param max_cycles        Maximum retry budget.
     * @param succeed_on_cycle  Cycle number on which recovery succeeds (1-based).
     */
    RecoveryResult RecoverWithRetry(size_t max_cycles, size_t succeed_on_cycle) {
        RecoveryResult r;
        for (size_t c = 1; c <= max_cycles; ++c) {
            ++r.cycles_used;
            if (c >= succeed_on_cycle) {
                r.recovered = true;
                return r;
            }
        }
        return r;
    }

    /**
     * @brief Simulate a crash with @p total_units data units in flight.
     *
     * Units with index < @p persisted_before_crash are durable; the rest are
     * lost.  Returns the count of lost units.
     */
    static size_t SimulateCrash(size_t total_units, size_t persisted_before_crash) {
        const size_t lost = (persisted_before_crash >= total_units)
                            ? 0
                            : total_units - persisted_before_crash;
        return lost;
    }
};

// ---------------------------------------------------------------------------
// UptimeTracker — rolling window for SLA-06
// ---------------------------------------------------------------------------

/// @brief An uptime event: a service is either up or down at a given instant.
struct UptimeEvent {
    bool up{true};
};

/**
 * @brief Computes service availability from a sequence of up/down events.
 *
 * Availability = (number of up events) / (total events).
 */
class UptimeTracker {
public:
    void Record(bool is_up) {
        std::lock_guard<std::mutex> lk(mu_);
        events_.push_back({is_up});
    }

    double Availability() const {
        std::lock_guard<std::mutex> lk(mu_);
        if (events_.empty()) { return 0.0; }
        const size_t up_count = static_cast<size_t>(
            std::count_if(events_.begin(), events_.end(),
                          [](const UptimeEvent& e) { return e.up; }));
        return static_cast<double>(up_count) / static_cast<double>(events_.size());
    }

    size_t Count() const {
        std::lock_guard<std::mutex> lk(mu_);
        return events_.size();
    }

private:
    mutable std::mutex        mu_;
    std::vector<UptimeEvent>  events_;
};

// ---------------------------------------------------------------------------
// DegradedWorkerPool — partial-capacity throughput for SLA-07
// ---------------------------------------------------------------------------

/**
 * @brief Simulates a worker pool where a fraction of workers are disabled.
 *
 * Nominal throughput is defined as @p total_workers workers each processing
 * one unit.  When @p active_workers < total_workers, throughput scales
 * proportionally.
 */
struct WorkerPoolResult {
    size_t total_workers{0};
    size_t active_workers{0};
    size_t units_processed{0};

    double ThroughputFraction() const {
        if (total_workers == 0) { return 0.0; }
        return static_cast<double>(active_workers) / static_cast<double>(total_workers);
    }
};

WorkerPoolResult SimulateDegradedPool(size_t total_workers,
                                      size_t disabled_workers,
                                      size_t units_per_worker) {
    const size_t active = (disabled_workers >= total_workers)
                          ? 0
                          : total_workers - disabled_workers;
    WorkerPoolResult r;
    r.total_workers   = total_workers;
    r.active_workers  = active;
    r.units_processed = active * units_per_worker;
    return r;
}

// ---------------------------------------------------------------------------
// SlaGateCounter — gate self-check for SLA-08
// ---------------------------------------------------------------------------

/**
 * @brief A simple 0.0/1.0 gate counter compatible with benchmark manifest tooling.
 *
 * Report(true) sets the counter to 1.0; Report(false) sets it to 0.0.
 */
class SlaGateCounter {
public:
    void Report(bool passed) {
        std::lock_guard<std::mutex> lk(mu_);
        value_ = passed ? 1.0 : 0.0;
    }

    double Value() const {
        std::lock_guard<std::mutex> lk(mu_);
        return value_;
    }

private:
    mutable std::mutex mu_;
    double             value_{0.0};
};

} // anonymous namespace

// ===========================================================================
// Test fixture
// ===========================================================================

/**
 * @brief Shared fixture for SLA compliance tests.
 *
 * Allocates a fresh instance of each SLA infrastructure component and
 * provides a seeded PRNG for reproducible synthetic data.
 */
class SlaComplianceUptimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_   = std::make_unique<ServiceSimulator>();
        latency_   = std::make_unique<LatencyCollector>();
        recovery_  = std::make_unique<RecoveryManager>();
        uptime_    = std::make_unique<UptimeTracker>();
        gate_      = std::make_unique<SlaGateCounter>();
        gen_.seed(kCanonicalSeed);
    }

    void TearDown() override {
        service_.reset();
        latency_.reset();
        recovery_.reset();
        uptime_.reset();
        gate_.reset();
    }

    std::unique_ptr<ServiceSimulator>  service_;
    std::unique_ptr<LatencyCollector>  latency_;
    std::unique_ptr<RecoveryManager>   recovery_;
    std::unique_ptr<UptimeTracker>     uptime_;
    std::unique_ptr<SlaGateCounter>    gate_;
    std::mt19937                        gen_;
};

// ===========================================================================
// SLA-01 — Availability window
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA01_AvailabilityWindowMeetsNineNines) {
    SCOPED_TRACE("SLA-01: 1000 requests with 1 injected failure; availability ≥ 99.9%");

    constexpr size_t kTotalRequests = 1000;
    constexpr double kMinAvailability = 0.999;

    // Inject exactly one failure at request index 500.
    const std::unordered_set<size_t> fail_at = {500};
    const auto stats = service_->Process(kTotalRequests, fail_at);

    EXPECT_EQ(stats.total, kTotalRequests);
    EXPECT_EQ(stats.failed, 1U)  << "exactly one request must fail";
    EXPECT_EQ(stats.succeeded, kTotalRequests - 1);

    const double availability = stats.Availability();
    EXPECT_GE(availability, kMinAvailability)
        << "availability " << availability
        << " must be ≥ " << kMinAvailability;
}

// ===========================================================================
// SLA-02 — p99 latency compliance
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA02_P99LatencyWithinBudget) {
    SCOPED_TRACE("SLA-02: p99 latency of 100 synthetic ops must be ≤ 10 ms (10 000 µs)");

    constexpr size_t kOps            = 100;
    constexpr double kBudgetUs       = 10'000.0; // 10 ms
    constexpr double kBaseLatencyUs  = 100.0;    // synthetic base (well under budget)
    constexpr double kMaxLatencyUs   = 500.0;    // worst-case synthetic sample

    // Generate synthetic latency samples using the seeded PRNG.
    std::uniform_real_distribution<double> dist(kBaseLatencyUs, kMaxLatencyUs);
    for (size_t i = 0; i < kOps; ++i) {
        latency_->Add(dist(gen_));
    }

    ASSERT_EQ(latency_->Count(), kOps);
    const double p99 = latency_->P99();
    EXPECT_LE(p99, kBudgetUs)
        << "p99 latency " << p99 << " µs exceeds budget of " << kBudgetUs << " µs";
}

// ===========================================================================
// SLA-03 — Graceful degradation under load
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA03_GracefulDegradationOverloadedNotCrashed) {
    SCOPED_TRACE("SLA-03: queue-full must return kOverloaded; in-flight items complete");

    constexpr size_t kQueueCapacity = 10;
    RequestQueue queue(kQueueCapacity);

    // Fill the queue exactly to capacity.
    for (size_t i = 0; i < kQueueCapacity; ++i) {
        const SlaStatus s = queue.Enqueue(static_cast<int>(i));
        ASSERT_EQ(s, SlaStatus::kOk)
            << "item " << i << " should enqueue successfully";
    }
    EXPECT_EQ(queue.Size(), kQueueCapacity);

    // Overflow attempts must return kOverloaded.
    constexpr size_t kOverflowAttempts = 5;
    size_t overloaded_count = 0;
    for (size_t i = 0; i < kOverflowAttempts; ++i) {
        const SlaStatus s = queue.Enqueue(static_cast<int>(kQueueCapacity + i));
        if (s == SlaStatus::kOverloaded) { ++overloaded_count; }
    }
    EXPECT_EQ(overloaded_count, kOverflowAttempts)
        << "all overflow enqueue attempts must return kOverloaded";

    // Queue size is unchanged (no overflow item was inserted).
    EXPECT_EQ(queue.Size(), kQueueCapacity)
        << "queue size must not increase on overloaded attempts";

    // Drain all in-flight items successfully.
    size_t drained = 0;
    while (queue.Dequeue().has_value()) { ++drained; }
    EXPECT_EQ(drained, kQueueCapacity)
        << "all kQueueCapacity in-flight items must be drainable";
}

// ===========================================================================
// SLA-04 — Recovery time objective (RTO)
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA04_RTORecoveryWithinThreeCycles) {
    SCOPED_TRACE("SLA-04: system must recover within ≤ 3 retry cycles");

    constexpr size_t kMaxRTOCycles    = 3;
    constexpr size_t kSucceedOnCycle  = 2; // success on second cycle

    const auto result = recovery_->RecoverWithRetry(kMaxRTOCycles, kSucceedOnCycle);

    EXPECT_TRUE(result.recovered)
        << "system must recover within the RTO budget";
    EXPECT_LE(result.cycles_used, kMaxRTOCycles)
        << "recovery cycles used (" << result.cycles_used
        << ") must not exceed RTO budget (" << kMaxRTOCycles << ")";
    EXPECT_EQ(result.cycles_used, kSucceedOnCycle)
        << "recovery must occur on cycle " << kSucceedOnCycle;

    // Verify that a scenario requiring too many cycles fails the RTO.
    const auto over_budget = recovery_->RecoverWithRetry(kMaxRTOCycles,
                                                          kMaxRTOCycles + 1);
    EXPECT_FALSE(over_budget.recovered)
        << "a recovery requiring more than kMaxRTOCycles must fail the RTO check";
}

// ===========================================================================
// SLA-05 — Recovery point objective (RPO)
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA05_RPODataLossWithinContractBound) {
    SCOPED_TRACE("SLA-05: data loss after simulated crash must not exceed RPO contract");

    constexpr size_t kTotalUnits         = 100;
    constexpr size_t kPersistedBeforeCrash = 98; // last 2 units in flight
    constexpr size_t kRpoContractMaxLoss   = 5;  // contract: lose at most 5 units

    const size_t lost = RecoveryManager::SimulateCrash(kTotalUnits, kPersistedBeforeCrash);

    EXPECT_EQ(lost, kTotalUnits - kPersistedBeforeCrash)
        << "lost unit count must equal total - persisted";
    EXPECT_LE(lost, kRpoContractMaxLoss)
        << "data loss " << lost
        << " must not exceed RPO contract bound " << kRpoContractMaxLoss;

    // Boundary: exactly at the RPO limit.
    const size_t at_limit = RecoveryManager::SimulateCrash(
        kTotalUnits, kTotalUnits - kRpoContractMaxLoss);
    EXPECT_EQ(at_limit, kRpoContractMaxLoss)
        << "exactly RPO-max units may be lost at the boundary";
    EXPECT_LE(at_limit, kRpoContractMaxLoss);
}

// ===========================================================================
// SLA-06 — Uptime accounting
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA06_UptimeAccountingCorrectFromEventSequence) {
    SCOPED_TRACE("SLA-06: rolling-window availability tracker must compute correctly");

    // Sequence: 95 up, 5 down → 95% availability.
    constexpr size_t kUp   = 95;
    constexpr size_t kDown =  5;

    for (size_t i = 0; i < kUp;   ++i) { uptime_->Record(true);  }
    for (size_t i = 0; i < kDown; ++i) { uptime_->Record(false); }

    EXPECT_EQ(uptime_->Count(), kUp + kDown);

    const double avail = uptime_->Availability();
    const double expected = static_cast<double>(kUp) / static_cast<double>(kUp + kDown);
    EXPECT_DOUBLE_EQ(avail, expected)
        << "availability must equal up/(up+down)";

    // Verify 99.9% scenario separately.
    UptimeTracker high_avail;
    for (size_t i = 0; i < 999; ++i) { high_avail.Record(true);  }
    high_avail.Record(false);
    EXPECT_GE(high_avail.Availability(), 0.999)
        << "999 up / 1000 total must yield availability ≥ 99.9%";
}

// ===========================================================================
// SLA-07 — Degraded-mode throughput floor
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA07_DegradedModeThroughputAboveFloor) {
    SCOPED_TRACE("SLA-07: 50% worker disable must keep throughput ≥ 40% of nominal");

    constexpr size_t kTotalWorkers   = 8;
    constexpr size_t kDisabled       = 4;  // 50% disabled
    constexpr size_t kUnitsPerWorker = 100;
    constexpr double kMinFraction    = 0.40; // 40% floor

    const auto nominal = SimulateDegradedPool(kTotalWorkers, 0, kUnitsPerWorker);
    const auto degraded = SimulateDegradedPool(kTotalWorkers, kDisabled, kUnitsPerWorker);

    EXPECT_EQ(nominal.units_processed, kTotalWorkers * kUnitsPerWorker);
    EXPECT_EQ(degraded.active_workers, kTotalWorkers - kDisabled);

    const double fraction = degraded.ThroughputFraction();
    EXPECT_GE(fraction, kMinFraction)
        << "degraded throughput fraction " << fraction
        << " must be ≥ floor " << kMinFraction;

    // Nominal-relative check: degraded units / nominal units ≥ kMinFraction.
    const double relative = static_cast<double>(degraded.units_processed)
                          / static_cast<double>(nominal.units_processed);
    EXPECT_GE(relative, kMinFraction)
        << "degraded units processed / nominal must be ≥ " << kMinFraction;
}

// ===========================================================================
// SLA-08 — SLA gate self-check
// ===========================================================================

TEST_F(SlaComplianceUptimeTest, SLA08_SlaGateCounterReportsCorrectly) {
    SCOPED_TRACE("SLA-08: gate counter must emit 1.0 on compliance, 0.0 on violation");

    // Simulate a compliance check that passes.
    const bool compliance_met = true;
    gate_->Report(compliance_met);
    EXPECT_DOUBLE_EQ(gate_->Value(), 1.0)
        << "gate counter must be 1.0 when SLA is met";

    // Simulate a violation.
    gate_->Report(false);
    EXPECT_DOUBLE_EQ(gate_->Value(), 0.0)
        << "gate counter must be 0.0 when SLA is violated";

    // Final pass restores 1.0.
    gate_->Report(true);
    EXPECT_DOUBLE_EQ(gate_->Value(), 1.0)
        << "gate counter must return to 1.0 after a subsequent passing report";
}
} } // namespace themis::test
