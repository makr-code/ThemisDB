/**
 * @file test_operator_remediation_engine.cpp
 * @brief Focused regression tests for operator remediation engine (Phase 2, ORE-01..10).
 *
 * Test coverage:
 * - ORE-01: Cardinality explosion pattern detection
 * - ORE-02: Exporter unavailability pattern detection
 * - ORE-03: High latency pattern detection
 * - ORE-04: Hint deduplication
 * - ORE-05: Hint filtering by category
 * - ORE-06: Hint filtering by severity
 * - ORE-07: Listener notifications
 * - ORE-08: Custom pattern registration
 * - ORE-09: Hint lifecycle (active/resolved)
 * - ORE-10: Concurrent hint generation and resolution
 */

#include "gtest/gtest.h"
#include "observability/operator_remediation_engine.h"
#include <thread>
#include <vector>
#include <map>
#include <chrono>

namespace themis {
namespace observability {

class OperatorRemediationEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = createOperatorRemediationEngine();
    }

    std::unique_ptr<OperatorRemediationEngine> engine;
};

// Test listener for tracking hint events
class TestRemediationListener : public IRemediationHintListener {
public:
    void onNewHint(const std::shared_ptr<RemediationHint>& hint) override {
        new_hints.push_back(hint->hintId());
    }

    void onHintResolved(const std::string& hint_id) override {
        resolved_hints.push_back(hint_id);
    }

    std::vector<std::string> new_hints;
    std::vector<std::string> resolved_hints;
};

// ORE-01: Cardinality explosion pattern detection
TEST_F(OperatorRemediationEngineTest, CardinalityExplosionDetection) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    metrics["metric_series_count"] = 50000.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);

    ASSERT_FALSE(hints.empty());
    EXPECT_EQ(hints[0]->problemCategory(), ProblemCategory::CARDINALITY_EXPLOSION);
    EXPECT_TRUE(hints[0]->problemTitle().find("Cardinality") != std::string::npos);
}

TEST_F(OperatorRemediationEngineTest, CardinalityRemediationActions) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 100.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);

    ASSERT_FALSE(hints.empty());
    auto hint = hints[0];

    EXPECT_FALSE(hint->suggestedActions().empty());
    EXPECT_EQ(hint->suggestedActions().size(), 3);  // Built-in has 3 actions
    EXPECT_TRUE(hint->suggestedActions()[0].priority >= 1);
}

// ORE-02: Exporter unavailability pattern detection
TEST_F(OperatorRemediationEngineTest, ExporterUnavailabilityDetection) {
    std::map<std::string, double> metrics;
    metrics["exporter_failures_total"] = 5.0;
    metrics["exporter_errors_total"] = 3.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);

    ASSERT_FALSE(hints.empty());
    EXPECT_EQ(hints[0]->problemCategory(), ProblemCategory::EXPORTER_UNAVAILABLE);
    EXPECT_EQ(hints[0]->severity(), RemediationSeverity::CRITICAL);
}

// ORE-03: High latency pattern detection
TEST_F(OperatorRemediationEngineTest, HighLatencyDetection) {
    std::map<std::string, double> metrics;
    metrics["exporter_latency_ms_p99"] = 100.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);

    ASSERT_FALSE(hints.empty());
    EXPECT_EQ(hints[0]->problemCategory(), ProblemCategory::HIGH_LATENCY);
    EXPECT_EQ(hints[0]->severity(), RemediationSeverity::WARNING);
}

TEST_F(OperatorRemediationEngineTest, CriticalHighLatencyDetection) {
    std::map<std::string, double> metrics;
    metrics["exporter_latency_ms_p99"] = 500.0;  // Very high latency

    auto hints = engine->analyzeAndGenerateHints(metrics);

    ASSERT_FALSE(hints.empty());
    EXPECT_EQ(hints[0]->severity(), RemediationSeverity::CRITICAL);
}

// ORE-04: Hint deduplication
TEST_F(OperatorRemediationEngineTest, HintDeduplication) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;

    // Generate same hint multiple times within deduplication window
    auto hints1 = engine->analyzeAndGenerateHints(metrics);
    auto hints2 = engine->analyzeAndGenerateHints(metrics);

    // Second generation should produce no new hints (deduplicated)
    EXPECT_FALSE(hints1.empty());
    EXPECT_TRUE(hints2.empty());  // Should be filtered out
}

TEST_F(OperatorRemediationEngineTest, DeduplicationWindowExpiry) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;

    // Generate hint
    auto hints1 = engine->analyzeAndGenerateHints(metrics);
    ASSERT_FALSE(hints1.empty());
    std::string hint_id = hints1[0]->hintId();

    // Set very short deduplication window
    engine->setDeduplicationWindow(std::chrono::seconds(0));

    // Wait a bit and try again
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Should generate new hint after window expires
    auto hints2 = engine->analyzeAndGenerateHints(metrics);
    EXPECT_FALSE(hints2.empty());
    EXPECT_NE(hints2[0]->hintId(), hint_id);
}

// ORE-05: Hint filtering by category
TEST_F(OperatorRemediationEngineTest, HintFilteringByCategory) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;
    metrics["exporter_failures_total"] = 5.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);

    auto cardinality_hints = engine->getHintsByCategory(ProblemCategory::CARDINALITY_EXPLOSION);
    auto exporter_hints = engine->getHintsByCategory(ProblemCategory::EXPORTER_UNAVAILABLE);

    EXPECT_TRUE(std::any_of(
        cardinality_hints.begin(), cardinality_hints.end(),
        [](const auto& h) { return h->problemCategory() == ProblemCategory::CARDINALITY_EXPLOSION; }
    ));

    EXPECT_TRUE(std::any_of(
        exporter_hints.begin(), exporter_hints.end(),
        [](const auto& h) { return h->problemCategory() == ProblemCategory::EXPORTER_UNAVAILABLE; }
    ));
}

// ORE-06: Hint filtering by severity
TEST_F(OperatorRemediationEngineTest, HintFilteringBySeverity) {
    std::map<std::string, double> metrics;
    metrics["exporter_failures_total"] = 5.0;  // Critical

    auto hints = engine->analyzeAndGenerateHints(metrics);

    auto critical_hints = engine->getHintsBySeverity(RemediationSeverity::CRITICAL);
    EXPECT_FALSE(critical_hints.empty());

    // All critical hints should actually be critical or higher
    for (const auto& hint : critical_hints) {
        EXPECT_GE(static_cast<int>(hint->severity()), static_cast<int>(RemediationSeverity::CRITICAL));
    }
}

// ORE-07: Listener notifications
TEST_F(OperatorRemediationEngineTest, ListenerNotifications) {
    auto listener = std::make_shared<TestRemediationListener>();
    engine->addListener(listener);

    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);

    EXPECT_FALSE(listener->new_hints.empty());
    EXPECT_EQ(listener->new_hints.size(), hints.size());
}

TEST_F(OperatorRemediationEngineTest, ListenerHintResolution) {
    auto listener = std::make_shared<TestRemediationListener>();
    engine->addListener(listener);

    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);
    ASSERT_FALSE(hints.empty());

    std::string hint_id = hints[0]->hintId();

    engine->resolveHint(hint_id);

    EXPECT_TRUE(std::find(listener->resolved_hints.begin(),
                         listener->resolved_hints.end(),
                         hint_id) != listener->resolved_hints.end());
}

// ORE-08: Custom pattern registration
TEST_F(OperatorRemediationEngineTest, CustomPatternRegistration) {
    class CustomPattern : public RemediationPattern {
    public:
        std::shared_ptr<RemediationHint> match(
            const std::map<std::string, double>& metrics) override {

            auto it = metrics.find("custom_metric");
            if (it == metrics.end() || it->second < 1.0) {
                return nullptr;
            }

            auto hint = std::make_shared<RemediationHint>();
            hint->category_ = ProblemCategory::UNKNOWN;
            hint->title_ = "Custom Problem Detected";
            hint->description_ = "This is a custom test pattern";
            hint->severity_ = RemediationSeverity::INFO;
            hint->confidence_score_ = 0.9;
            hint->generated_at_ = std::chrono::system_clock::now();
            hint->detection_window_ = std::chrono::seconds(60);
            hint->hint_id_ = "custom-hint-1";

            return hint;
        }

        std::string patternName() const override {
            return "custom_pattern";
        }

        ProblemCategory problemCategory() const override {
            return ProblemCategory::UNKNOWN;
        }
    };

    EXPECT_TRUE(engine->registerPattern(std::make_unique<CustomPattern>()));

    std::map<std::string, double> metrics;
    metrics["custom_metric"] = 1.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);

    auto custom_hints = std::find_if(
        hints.begin(), hints.end(),
        [](const auto& h) { return h->problemTitle().find("Custom") != std::string::npos; }
    );

    EXPECT_NE(custom_hints, hints.end());
}

// ORE-09: Hint lifecycle (active/resolved)
TEST_F(OperatorRemediationEngineTest, HintLifecycle) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;

    auto hints = engine->analyzeAndGenerateHints(metrics);
    ASSERT_FALSE(hints.empty());
    std::string hint_id = hints[0]->hintId();

    // Hint should be active
    auto active_hints = engine->getActiveHints();
    EXPECT_TRUE(std::any_of(
        active_hints.begin(), active_hints.end(),
        [&hint_id](const auto& h) { return h->hintId() == hint_id; }
    ));

    // Resolve the hint
    EXPECT_TRUE(engine->resolveHint(hint_id));

    // Hint should no longer be active
    auto resolved_active_hints = engine->getActiveHints();
    EXPECT_FALSE(std::any_of(
        resolved_active_hints.begin(), resolved_active_hints.end(),
        [&hint_id](const auto& h) { return h->hintId() == hint_id; }
    ));
}

// ORE-10: Concurrent hint generation and resolution
TEST_F(OperatorRemediationEngineTest, ConcurrentHintOperations) {
    std::vector<std::thread> threads;
    std::vector<std::string> hint_ids;
    std::mutex ids_mutex;

    // Generate hints concurrently
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([this, t, &hint_ids, &ids_mutex]() {
            std::map<std::string, double> metrics;
            // Each thread generates a different metric to avoid deduplication
            metrics["metric_cardinality_exceeded_total"] = static_cast<double>(t + 1);

            auto hints = engine->analyzeAndGenerateHints(metrics);
            if (!hints.empty()) {
                std::lock_guard<std::mutex> lock(ids_mutex);
                hint_ids.push_back(hints[0]->hintId());
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto active_hints = engine->getActiveHints();
    EXPECT_GE(active_hints.size(), 1);  // At least one hint generated

    // Resolve hints concurrently
    threads.clear();
    for (const auto& hint_id : hint_ids) {
        threads.emplace_back([this, &hint_id]() {
            engine->resolveHint(hint_id);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_F(OperatorRemediationEngineTest, HintGenerationToggle) {
    std::map<std::string, double> metrics;
    metrics["metric_cardinality_exceeded_total"] = 10.0;

    // Initially enabled
    EXPECT_TRUE(engine->isHintGenerationEnabled());

    // Disable hint generation
    engine->setHintGenerationEnabled(false);

    auto hints = engine->analyzeAndGenerateHints(metrics);
    EXPECT_TRUE(hints.empty());

    // Re-enable
    engine->setHintGenerationEnabled(true);

    hints = engine->analyzeAndGenerateHints(metrics);
    EXPECT_FALSE(hints.empty());
}

} // namespace observability
} // namespace themis
