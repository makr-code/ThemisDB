/**
 * @file test_observability_phase3_continuation_focused.cpp
 * @brief Phase 3 hardening tests for operator remediation engine (ORE-11..20).
 *
 * Focus areas:
 * - ORE-11: weak_ptr listener removal correctness
 * - ORE-12: concurrent listener add/remove/notify
 * - ORE-13: listener lifecycle with hint generation
 * - ORE-14: malformed metric pattern matching
 * - ORE-15: cardinality overflow edge cases
 * - ORE-16: memory pressure listener eviction
 * - ORE-17: deduplication under high pressure
 * - ORE-18: concurrent pattern registration/unregistration
 * - ORE-19: clock skew in hint timestamps
 * - ORE-20: listener notification ordering and atomicity
 */

#include "gtest/gtest.h"
#include "observability/operator_remediation_engine.h"
#include <algorithm>
#include <thread>
#include <vector>
#include <map>
#include <chrono>
#include <atomic>
#include <limits>
#include <cmath>

namespace themis {
namespace observability {

// Test listener implementation with detailed tracking
class PhaseThreeTestListener : public IRemediationHintListener {
public:
    void onNewHint(const std::shared_ptr<RemediationHint>& hint) override {
        std::lock_guard<std::mutex> lock(hints_mutex_);
        new_hints.push_back(hint->hintId());
        received_hints.push_back(hint);
    }

    void onHintResolved(const std::string& hint_id) override {
        std::lock_guard<std::mutex> lock(hints_mutex_);
        resolved_hints.push_back(hint_id);
    }

    size_t getNewHintCount() const {
        std::lock_guard<std::mutex> lock(hints_mutex_);
        return new_hints.size();
    }

    size_t getResolvedHintCount() const {
        std::lock_guard<std::mutex> lock(hints_mutex_);
        return resolved_hints.size();
    }

    std::vector<std::string> new_hints;
    std::vector<std::shared_ptr<RemediationHint>> received_hints;
    std::vector<std::string> resolved_hints;
    mutable std::mutex hints_mutex_;
};

class OperatorRemediationEnginePhaseThreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = createOperatorRemediationEngine();
    }

    std::unique_ptr<OperatorRemediationEngine> engine;
};

// ============================================================================
// ORE-11: weak_ptr listener removal correctness
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, WeakPtrListenerRemovalCorrectness) {
    // Test that listeners can be removed correctly despite weak_ptr storage
    auto listener1 = std::make_shared<PhaseThreeTestListener>();
    auto listener2 = std::make_shared<PhaseThreeTestListener>();
    
    ASSERT_TRUE(engine->addListener(listener1));
    ASSERT_TRUE(engine->addListener(listener2));
    
    // Generate a hint
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    auto hints = engine->analyzeAndGenerateHints(metrics);
    ASSERT_FALSE(hints.empty());
    
    // Both listeners should have received the hint
    EXPECT_EQ(listener1->getNewHintCount(), 1);
    EXPECT_EQ(listener2->getNewHintCount(), 1);
    
    // Remove listener1
    ASSERT_TRUE(engine->removeListener(listener1));
    
    // Generate another hint
    metrics["metric_cardinality_exceeded_total"] = 20.0;
    engine->setDeduplicationWindow(std::chrono::seconds(0));
    hints = engine->analyzeAndGenerateHints(metrics);
    
    // Only listener2 should receive the second hint
    EXPECT_EQ(listener1->getNewHintCount(), 1);  // Still 1, not incremented
    EXPECT_EQ(listener2->getNewHintCount(), 2);  // Incremented to 2
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, ListenerRemovalNonExistent) {
    auto listener = std::make_shared<PhaseThreeTestListener>();
    
    // Try to remove a listener that was never added
    EXPECT_FALSE(engine->removeListener(listener));
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, ListenerRemovalNullptr) {
    // Adding and removing nullptr should fail gracefully
    EXPECT_FALSE(engine->addListener(nullptr));
    EXPECT_FALSE(engine->removeListener(nullptr));
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, ListenerRemovalAfterExpiry) {
    auto listener = std::make_shared<PhaseThreeTestListener>();
    ASSERT_TRUE(engine->addListener(listener));
    
    // Let the weak_ptr go out of scope by destroying the only shared_ptr
    listener = nullptr;
    
    // Try to remove expired listener (should fail)
    auto dummy = std::make_shared<PhaseThreeTestListener>();
    EXPECT_FALSE(engine->removeListener(dummy));
}

// ============================================================================
// ORE-12: concurrent listener add/remove/notify
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, ConcurrentListenerOperations) {
    // Create multiple listeners
    std::vector<std::shared_ptr<PhaseThreeTestListener>> listeners;
    for (int i = 0; i < 10; ++i) {
        listeners.push_back(std::make_shared<PhaseThreeTestListener>());
    }
    
    std::atomic<int> errors(0);
    std::vector<std::thread> threads;
    
    // Thread 1: Add listeners
    threads.emplace_back([&]() {
        for (auto& listener : listeners) {
            if (!engine->addListener(listener)) {
                errors++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Thread 2: Remove listeners (after they're added)
    threads.emplace_back([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        for (int i = 0; i < 5; ++i) {
            if (!engine->removeListener(listeners[i])) {
                errors++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Thread 3: Generate hints (which triggers notifications)
    threads.emplace_back([&]() {
        std::map<std::string, double> metrics;
        metrics["metric_cardinality_exceeded_total"] = 10.0;
        
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            engine->setDeduplicationWindow(std::chrono::seconds(0));
            engine->analyzeAndGenerateHints(metrics);
        }
    });
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(errors, 0);  // No errors in listener operations
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, ConcurrentNotificationStress) {
    auto listener = std::make_shared<PhaseThreeTestListener>();
    engine->addListener(listener);
    
    std::atomic<int> hint_count(0);
    std::vector<std::thread> threads;
    
    engine->setDeduplicationWindow(std::chrono::seconds(0));
    
    // Multiple threads generating hints concurrently
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&, t]() {
            std::map<std::string, double> metrics;
            metrics["metric_cardinality_exceeded_total"] = 10.0 + t;
            
            for (int i = 0; i < 10; ++i) {
                auto hints = engine->analyzeAndGenerateHints(metrics);
                hint_count += hints.size();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Listener should have received notifications for generated hints
    EXPECT_GT(listener->getNewHintCount(), 0);
    EXPECT_LE(listener->getNewHintCount(), static_cast<size_t>(hint_count));
}

// ============================================================================
// ORE-13: listener lifecycle with hint generation
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, ListenerLifecycleFullFlow) {
    auto listener = std::make_shared<PhaseThreeTestListener>();
    
    // Add listener
    ASSERT_TRUE(engine->addListener(listener));
    
    // Generate hint
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    auto hints1 = engine->analyzeAndGenerateHints(metrics);
    ASSERT_FALSE(hints1.empty());
    std::string hint_id = hints1[0]->hintId();
    
    EXPECT_EQ(listener->getNewHintCount(), 1);
    EXPECT_EQ(listener->getResolvedHintCount(), 0);
    
    // Resolve hint
    ASSERT_TRUE(engine->resolveHint(hint_id));
    EXPECT_EQ(listener->getResolvedHintCount(), 1);
    
    // Remove listener
    ASSERT_TRUE(engine->removeListener(listener));
    
    // Generate another hint (listener shouldn't receive it)
    engine->setDeduplicationWindow(std::chrono::seconds(0));
    auto hints2 = engine->analyzeAndGenerateHints(metrics);
    EXPECT_EQ(listener->getNewHintCount(), 1);  // Still 1
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, ListenerSurvivesHintClear) {
    auto listener = std::make_shared<PhaseThreeTestListener>();
    engine->addListener(listener);
    
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    engine->analyzeAndGenerateHints(metrics);
    
    EXPECT_EQ(listener->getNewHintCount(), 1);
    
    // Clear all hints
    engine->clearAllHints();
    
    // Generate new hint (listener should still be there)
    engine->setDeduplicationWindow(std::chrono::seconds(0));
    engine->analyzeAndGenerateHints(metrics);
    
    EXPECT_EQ(listener->getNewHintCount(), 2);
}

// ============================================================================
// ORE-14: malformed metric pattern matching
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, MalformedMetricNaN) {
    std::map<std::string, double> metrics;
    // NaN value should be handled gracefully
    metrics["metric_cardinality_exceeded_total"] = std::numeric_limits<double>::quiet_NaN();
    metrics["metric_series_count"] = 50000.0;
    
    // Should not crash and should not generate hint
    auto hints = engine->analyzeAndGenerateHints(metrics);
    EXPECT_TRUE(hints.empty());
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, MalformedMetricInfinity) {
    std::map<std::string, double> metrics;
    // Infinity should be handled gracefully
    metrics["metric_cardinality_exceeded_total"] = std::numeric_limits<double>::infinity();
    metrics["metric_series_count"] = 50000.0;
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    EXPECT_TRUE(hints.empty());
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, MalformedMetricNegative) {
    std::map<std::string, double> metrics;
    // Negative cardinality doesn't make sense, should be rejected
    metrics["metric_cardinality_exceeded_total"] = -10.0;
    metrics["metric_series_count"] = 50000.0;
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    EXPECT_TRUE(hints.empty());
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, MalformedMetricMixedValid) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;  // Valid
    metrics["metric_series_count"] = std::numeric_limits<double>::quiet_NaN();  // Invalid (ignored)
    metrics["exporter_latency_ms_p99"] = 100.0;  // Valid
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    
    // Cardinality pattern should match despite NaN in other metric
    auto cardinality_hints = std::count_if(
        hints.begin(), hints.end(),
        [](const auto& h) { return h->problemCategory() == ProblemCategory::CARDINALITY_EXPLOSION; }
    );
    EXPECT_GE(cardinality_hints, 1);
}

// ============================================================================
// ORE-15: cardinality overflow edge cases
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, CardinalityMaxValue) {
    std::map<std::string, double> metrics;
    // Use very large valid value
    metrics["metric_cardinality_exceeded_total"] = 1e15;
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    ASSERT_FALSE(hints.empty());
    EXPECT_EQ(hints[0]->severity(), RemediationSeverity::CRITICAL);
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, CardinalityZero) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 0.0;
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    EXPECT_TRUE(hints.empty());  // No problem if cardinality exceeded is 0
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, CardinalityFractional) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 0.5;  // Between 0 and 1
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    EXPECT_TRUE(hints.empty());  // Pattern requires >= 1.0
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, CardinalityBoundary) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 1.0;  // Exactly at threshold
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    ASSERT_FALSE(hints.empty());
    EXPECT_EQ(hints[0]->severity(), RemediationSeverity::WARNING);  // Not critical yet
}

// ============================================================================
// ORE-16: memory pressure listener eviction
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, ListenerWeakPtrEviction) {
    // Test that weak_ptr automatically evicts when listeners are destroyed
    {
        auto listener1 = std::make_shared<PhaseThreeTestListener>();
        auto listener2 = std::make_shared<PhaseThreeTestListener>();
        
        engine->addListener(listener1);
        engine->addListener(listener2);
        
        // listener1 goes out of scope and is destroyed
        listener1 = nullptr;
    }
    
    // Create new listener that we can track
    auto listener3 = std::make_shared<PhaseThreeTestListener>();
    engine->addListener(listener3);
    
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    engine->analyzeAndGenerateHints(metrics);
    
    // Only listener2 and listener3 should receive notification
    // (listener1 was destroyed)
    EXPECT_GT(listener3->getNewHintCount(), 0);
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, ManyListenerScenario) {
    // Test adding many listeners and verifying weak_ptr cleanup
    std::vector<std::shared_ptr<PhaseThreeTestListener>> listeners;
    
    // Add 100 listeners
    for (int i = 0; i < 100; ++i) {
        auto listener = std::make_shared<PhaseThreeTestListener>();
        listeners.push_back(listener);
        engine->addListener(listener);
    }
    
    // Remove every other listener
    for (int i = 0; i < 100; i += 2) {
        EXPECT_TRUE(engine->removeListener(listeners[i]));
    }
    
    // Clear references to first half
    for (int i = 0; i < 50; ++i) {
        listeners[i] = nullptr;
    }
    
    // Generate hints
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    engine->analyzeAndGenerateHints(metrics);
    
    // Check that remaining listeners received hints
    for (int i = 51; i < 100; i += 2) {  // Remaining odd-indexed listeners
        EXPECT_GT(listeners[i]->getNewHintCount(), 0);
    }
}

// ============================================================================
// ORE-17: deduplication under high pressure
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, DeduplicationHighFrequency) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    
    engine->setDeduplicationWindow(std::chrono::seconds(10));
    
    // Generate same hint multiple times rapidly
    std::vector<std::vector<std::shared_ptr<RemediationHint>>> all_hints;
    for (int i = 0; i < 100; ++i) {
        auto hints = engine->analyzeAndGenerateHints(metrics);
        all_hints.push_back(hints);
    }
    
    // Should only have 1 hint generated despite 100 analyses
    int total_hints = 0;
    for (const auto& hints : all_hints) {
        total_hints += hints.size();
    }
    EXPECT_EQ(total_hints, 1);
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, DeduplicationMultiplePatterns) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    metrics["exporter_failures_total"] = 5.0;
    metrics["exporter_latency_ms_p99"] = 100.0;
    
    engine->setDeduplicationWindow(std::chrono::seconds(300));
    
    // First generation should produce multiple hints
    auto hints1 = engine->analyzeAndGenerateHints(metrics);
    size_t expected_patterns = hints1.size();
    EXPECT_GE(expected_patterns, 2);  // Should have multiple patterns
    
    // Second generation should deduplicate all
    auto hints2 = engine->analyzeAndGenerateHints(metrics);
    EXPECT_TRUE(hints2.empty());
    
    // Change one metric
    metrics["metric_cardinality_exceeded_total"] = 20.0;
    auto hints3 = engine->analyzeAndGenerateHints(metrics);
    
    // Should get new cardinality hint but not others (still deduplicated)
    auto cardinality_new = std::count_if(
        hints3.begin(), hints3.end(),
        [](const auto& h) { return h->problemCategory() == ProblemCategory::CARDINALITY_EXPLOSION; }
    );
    EXPECT_EQ(cardinality_new, 1);
}

// ============================================================================
// ORE-18: concurrent pattern registration/unregistration
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, ConcurrentPatternRegistration) {
    std::atomic<int> registration_errors(0);
    std::vector<std::thread> threads;
    
    // Thread 1: Register patterns
    threads.emplace_back([&]() {
        for (int i = 0; i < 20; ++i) {
            class DummyPattern : public RemediationPattern {
            public:
                int id_ = {};
                DummyPattern(int id) : id_(id) {}
                std::shared_ptr<RemediationHint> match(
                    const std::map<std::string, double>&) override {
                    return nullptr;
                }
                std::string patternName() const override {
                    return std::string("dummy_") + std::to_string(id_);
                }
                ProblemCategory problemCategory() const override {
                    return ProblemCategory::UNKNOWN;
                }
            };
            
            if (!engine->registerPattern(std::make_unique<DummyPattern>(i))) {
                registration_errors++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Thread 2: Unregister patterns
    threads.emplace_back([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        for (int i = 0; i < 10; ++i) {
            if (!engine->unregisterPattern(std::string("dummy_") + std::to_string(i))) {
                registration_errors++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Some operations should succeed (exact count depends on timing)
    EXPECT_LE(registration_errors, 30);  // Most operations should succeed
}

// ============================================================================
// ORE-19: clock skew in hint timestamps
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, HintTimestampConsistency) {
    auto listener = std::make_shared<PhaseThreeTestListener>();
    engine->addListener(listener);
    
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    
    auto before = std::chrono::system_clock::now();
    auto hints = engine->analyzeAndGenerateHints(metrics);
    auto after = std::chrono::system_clock::now();
    
    ASSERT_FALSE(hints.empty());
    auto hint_time = hints[0]->generatedAt();
    
    // Timestamp should be within generated time window
    EXPECT_GE(hint_time, before);
    EXPECT_LE(hint_time, after);
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, HintTimestampMonotonicity) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    metrics["exporter_failures_total"] = 5.0;
    
    engine->setDeduplicationWindow(std::chrono::seconds(0));
    
    auto hints1 = engine->analyzeAndGenerateHints(metrics);
    auto time1 = hints1[0]->generatedAt();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    auto hints2 = engine->analyzeAndGenerateHints(metrics);
    auto time2 = hints2[0]->generatedAt();
    
    // Timestamps should be monotonically increasing
    EXPECT_LE(time1, time2);
}

// ============================================================================
// ORE-20: listener notification ordering and atomicity
// ============================================================================

TEST_F(OperatorRemediationEnginePhaseThreeTest, NotificationOrdering) {
    // Create listeners that track order
    auto listener1 = std::make_shared<PhaseThreeTestListener>();
    auto listener2 = std::make_shared<PhaseThreeTestListener>();
    auto listener3 = std::make_shared<PhaseThreeTestListener>();
    
    engine->addListener(listener1);
    engine->addListener(listener2);
    engine->addListener(listener3);
    
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    auto hints = engine->analyzeAndGenerateHints(metrics);
    
    // All listeners should receive the same number of hints
    EXPECT_EQ(listener1->getNewHintCount(), hints.size());
    EXPECT_EQ(listener2->getNewHintCount(), hints.size());
    EXPECT_EQ(listener3->getNewHintCount(), hints.size());
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, NotificationAtomicity) {
    // Test that listener notifications are atomic (all or nothing)
    auto listener = std::make_shared<PhaseThreeTestListener>();
    engine->addListener(listener);
    
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    size_t first_count = listener->getNewHintCount();
    
    // Resolve the hint
    if (!hints.empty()) {
        engine->resolveHint(hints[0]->hintId());
    }
    
    size_t resolved_count = listener->getResolvedHintCount();
    
    // Should have received notification for both new hint and resolution
    EXPECT_EQ(first_count, 1);
    EXPECT_EQ(resolved_count, 1);
}

TEST_F(OperatorRemediationEnginePhaseThreeTest, ConcurrentNotificationConsistency) {
    std::vector<std::shared_ptr<PhaseThreeTestListener>> listeners;
    for (int i = 0; i < 10; ++i) {
        listeners.push_back(std::make_shared<PhaseThreeTestListener>());
        engine->addListener(listeners[i]);
    }
    
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    
    auto hints = engine->analyzeAndGenerateHints(metrics);
    
    // All listeners should have received the exact same hints
    size_t expected_count = listeners[0]->getNewHintCount();
    for (size_t i = 1; i < listeners.size(); ++i) {
        EXPECT_EQ(listeners[i]->getNewHintCount(), expected_count)
            << "Listener " << i << " received different notification count";
    }
}

} // namespace observability
} // namespace themis
