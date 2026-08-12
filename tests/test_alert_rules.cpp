/**
 * @file test_alert_rules.cpp
 * @brief Unit tests for the custom user-defined alert rules API (AlertRuleManager).
 *
 * Covers:
 * - CRUD operations (addRule, removeRule, getRule, updateRule, listRules)
 * - Rule evaluation (evaluateRules fires/resolves alerts via Alertmanager)
 * - Edge cases (duplicate IDs, missing metric, disabled rule, auto-generated IDs)
 */

#include <gtest/gtest.h>
#include "observability/alertmanager.h"
#include <atomic>
#include <thread>
#include <type_traits>

using namespace themis;
using namespace themis::observability;

// ============================================================================
// Helpers
// ============================================================================

static AlertRule makeRule(const std::string& id,
                          const std::string& metric,
                          AlertRuleOperator op,
                          double threshold,
                          AlertSeverity sev = AlertSeverity::WARNING) {
    AlertRule r;
    r.rule_id        = id;
    r.rule_name      = "Test rule " + id;
    r.metric_name    = metric;
    r.op             = op;
    r.threshold      = threshold;
    r.severity       = sev;
    r.message_template = "Metric {metric} value {value} crossed threshold";
    r.enabled        = true;
    return r;
}

static DefaultAlertmanager makeDisabledAlertmanager() {
    AlertmanagerConfig cfg;
    cfg.enabled       = false;
    cfg.retry_count   = 0;
    cfg.endpoint_url  = "http://localhost:9093";
    return DefaultAlertmanager(cfg);
}

// ============================================================================
// CRUD Tests
// ============================================================================

class AlertRuleManagerTest : public ::testing::Test {
protected:
    AlertRuleManager mgr_;
};

TEST_F(AlertRuleManagerTest, AddRule_Success) {
    AlertRule rule = makeRule("r1", "themis_cpu_usage", AlertRuleOperator::GREATER_THAN, 80.0);
    auto res = mgr_.addRule(rule);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(*res, "r1");
    EXPECT_EQ(mgr_.ruleCount(), 1u);
}

TEST_F(AlertRuleManagerTest, AddRule_AutoGeneratesId) {
    AlertRule rule;
    rule.metric_name = "themis_memory_bytes";
    rule.op          = AlertRuleOperator::GREATER_THAN;
    rule.threshold   = 1024.0;

    auto res = mgr_.addRule(rule);
    ASSERT_TRUE(res.has_value());
    EXPECT_FALSE(res->empty());
    EXPECT_EQ(mgr_.ruleCount(), 1u);
}

TEST_F(AlertRuleManagerTest, AddRule_DuplicateId_ReturnsError) {
    AlertRule rule = makeRule("dup", "themis_cpu_usage", AlertRuleOperator::GREATER_THAN, 90.0);
    ASSERT_TRUE(mgr_.addRule(rule).has_value());
    auto res = mgr_.addRule(rule);
    EXPECT_FALSE(res.has_value());
}

TEST_F(AlertRuleManagerTest, AddRule_EmptyMetricName_ReturnsError) {
    AlertRule rule;
    rule.rule_id      = "bad_rule";
    rule.metric_name  = "";  // invalid
    auto res = mgr_.addRule(rule);
    EXPECT_FALSE(res.has_value());
}

TEST_F(AlertRuleManagerTest, RemoveRule_Success) {
    AlertRule rule = makeRule("r_del", "themis_disk_io", AlertRuleOperator::GREATER_THAN, 100.0);
    ASSERT_TRUE(mgr_.addRule(rule).has_value());
    auto res = mgr_.removeRule("r_del");
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(mgr_.ruleCount(), 0u);
}

TEST_F(AlertRuleManagerTest, RemoveRule_NotFound_ReturnsError) {
    auto res = mgr_.removeRule("nonexistent");
    EXPECT_FALSE(res.has_value());
}

TEST_F(AlertRuleManagerTest, GetRule_Success) {
    AlertRule rule = makeRule("r_get", "themis_query_latency_p99_ms",
                              AlertRuleOperator::GREATER_THAN, 500.0,
                              AlertSeverity::CRITICAL);
    ASSERT_TRUE(mgr_.addRule(rule).has_value());

    auto res = mgr_.getRule("r_get");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->metric_name, "themis_query_latency_p99_ms");
    EXPECT_EQ(res->threshold, 500.0);
    EXPECT_EQ(res->severity, AlertSeverity::CRITICAL);
}

TEST_F(AlertRuleManagerTest, GetRule_NotFound_ReturnsError) {
    auto res = mgr_.getRule("ghost");
    EXPECT_FALSE(res.has_value());
}

TEST_F(AlertRuleManagerTest, UpdateRule_Success) {
    AlertRule rule = makeRule("r_upd", "themis_cpu_usage", AlertRuleOperator::GREATER_THAN, 70.0);
    ASSERT_TRUE(mgr_.addRule(rule).has_value());

    rule.threshold = 95.0;
    rule.severity  = AlertSeverity::CRITICAL;
    auto res = mgr_.updateRule(rule);
    EXPECT_TRUE(res.has_value());

    auto fetched = mgr_.getRule("r_upd");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_DOUBLE_EQ(fetched->threshold, 95.0);
    EXPECT_EQ(fetched->severity, AlertSeverity::CRITICAL);
}

TEST_F(AlertRuleManagerTest, UpdateRule_NotFound_ReturnsError) {
    AlertRule rule = makeRule("nope", "themis_cpu_usage", AlertRuleOperator::GREATER_THAN, 80.0);
    auto res = mgr_.updateRule(rule);
    EXPECT_FALSE(res.has_value());
}

TEST_F(AlertRuleManagerTest, UpdateRule_EmptyMetricName_ReturnsError) {
    AlertRule rule = makeRule("r_empty_metric", "themis_cpu_usage",
                              AlertRuleOperator::GREATER_THAN, 80.0);
    ASSERT_TRUE(mgr_.addRule(rule).has_value());

    rule.metric_name = "";
    auto res = mgr_.updateRule(rule);
    EXPECT_FALSE(res.has_value());
}

TEST_F(AlertRuleManagerTest, ListRules_ReturnsAll) {
    (void)mgr_.addRule(makeRule("l1", "m1", AlertRuleOperator::GREATER_THAN, 1.0));
    (void)mgr_.addRule(makeRule("l2", "m2", AlertRuleOperator::LESS_THAN, 2.0));
    (void)mgr_.addRule(makeRule("l3", "m3", AlertRuleOperator::EQUAL, 3.0));

    auto rules = mgr_.listRules();
    EXPECT_EQ(rules.size(), 3u);
}

TEST_F(AlertRuleManagerTest, ClearRules_RemovesAll) {
    (void)mgr_.addRule(makeRule("c1", "m1", AlertRuleOperator::GREATER_THAN, 1.0));
    (void)mgr_.addRule(makeRule("c2", "m2", AlertRuleOperator::LESS_THAN, 2.0));
    ASSERT_EQ(mgr_.ruleCount(), 2u);

    (void)mgr_.clearRules();
    EXPECT_EQ(mgr_.ruleCount(), 0u);
    EXPECT_TRUE(mgr_.listRules().empty());
}

// ============================================================================
// Rule Evaluation Tests
// ============================================================================

class AlertRuleEvalTest : public ::testing::Test {
protected:
    AlertRuleManager mgr_;
    DefaultAlertmanager am_{makeDisabledAlertmanager()};
};

TEST_F(AlertRuleEvalTest, EvaluateRules_NoRules_ReturnsZero) {
    std::map<std::string, double> metrics{{"themis_cpu", 99.0}};
    EXPECT_EQ(mgr_.evaluateRules(metrics, am_), 0);
}

TEST_F(AlertRuleEvalTest, EvaluateRules_ConditionMet_FiresAlert) {
    (void)mgr_.addRule(makeRule("cpu_high", "themis_cpu",
                                AlertRuleOperator::GREATER_THAN, 80.0));

    std::map<std::string, double> metrics{{"themis_cpu", 90.0}};
    int triggered = mgr_.evaluateRules(metrics, am_);
    EXPECT_EQ(triggered, 1);
}

TEST_F(AlertRuleEvalTest, EvaluateRules_ConditionNotMet_DoesNotFire) {
    (void)mgr_.addRule(makeRule("cpu_high", "themis_cpu",
                                AlertRuleOperator::GREATER_THAN, 80.0));

    std::map<std::string, double> metrics{{"themis_cpu", 50.0}};
    int triggered = mgr_.evaluateRules(metrics, am_);
    EXPECT_EQ(triggered, 0);
}

TEST_F(AlertRuleEvalTest, EvaluateRules_DisabledRule_IsSkipped) {
    AlertRule rule = makeRule("disabled_rule", "themis_cpu",
                              AlertRuleOperator::GREATER_THAN, 0.0);
    rule.enabled = false;
    (void)mgr_.addRule(rule);

    std::map<std::string, double> metrics{{"themis_cpu", 999.0}};
    int triggered = mgr_.evaluateRules(metrics, am_);
    EXPECT_EQ(triggered, 0);
}

TEST_F(AlertRuleEvalTest, EvaluateRules_MissingMetric_DoesNotFire) {
    (void)mgr_.addRule(makeRule("latency_rule", "themis_latency_p99",
                                AlertRuleOperator::GREATER_THAN, 500.0));

    // Snapshot does not contain the required metric
    std::map<std::string, double> metrics{{"themis_cpu", 90.0}};
    int triggered = mgr_.evaluateRules(metrics, am_);
    EXPECT_EQ(triggered, 0);
}

TEST_F(AlertRuleEvalTest, EvaluateRules_AlertResolvedWhenConditionClears) {
    (void)mgr_.addRule(makeRule("cpu_rule", "themis_cpu",
                                AlertRuleOperator::GREATER_THAN, 80.0));

    // First pass: condition is met → alert fires
    {
        std::map<std::string, double> metrics{{"themis_cpu", 95.0}};
        EXPECT_EQ(mgr_.evaluateRules(metrics, am_), 1);
    }
    // Second pass: condition still met → already active, still counted
    {
        std::map<std::string, double> metrics{{"themis_cpu", 85.0}};
        EXPECT_EQ(mgr_.evaluateRules(metrics, am_), 1);
    }
    // Third pass: condition no longer met → alert should be resolved
    {
        std::map<std::string, double> metrics{{"themis_cpu", 60.0}};
        EXPECT_EQ(mgr_.evaluateRules(metrics, am_), 0);
    }
    // Fourth pass: condition met again → fresh alert fires
    {
        std::map<std::string, double> metrics{{"themis_cpu", 90.0}};
        EXPECT_EQ(mgr_.evaluateRules(metrics, am_), 1);
    }
}

TEST_F(AlertRuleEvalTest, EvaluateRules_MultipleRules) {
    (void)mgr_.addRule(makeRule("r1", "themis_cpu",
                                AlertRuleOperator::GREATER_THAN, 80.0));
    (void)mgr_.addRule(makeRule("r2", "themis_memory_mb",
                                AlertRuleOperator::GREATER_THAN, 8192.0));
    (void)mgr_.addRule(makeRule("r3", "themis_disk_free_pct",
                                AlertRuleOperator::LESS_THAN, 10.0));

    std::map<std::string, double> metrics{
        {"themis_cpu",        90.0},  // triggers r1
        {"themis_memory_mb",  4096.0}, // does NOT trigger r2
        {"themis_disk_free_pct", 5.0}  // triggers r3
    };
    EXPECT_EQ(mgr_.evaluateRules(metrics, am_), 2);
}

// ============================================================================
// Operator Coverage Tests
// ============================================================================

class AlertRuleOperatorTest : public ::testing::Test {
protected:
    AlertRuleManager mgr_;
    DefaultAlertmanager am_{makeDisabledAlertmanager()};
};

TEST_F(AlertRuleOperatorTest, GreaterThan) {
    (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::GREATER_THAN, 10.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 11.0}}, am_), 1);
    (void)mgr_.clearRules(); (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::GREATER_THAN, 10.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 10.0}}, am_), 0);
}

TEST_F(AlertRuleOperatorTest, GreaterThanOrEqual) {
    (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::GREATER_THAN_OR_EQUAL, 10.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 10.0}}, am_), 1);
    (void)mgr_.clearRules(); (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::GREATER_THAN_OR_EQUAL, 10.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 9.0}}, am_), 0);
}

TEST_F(AlertRuleOperatorTest, LessThan) {
    (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::LESS_THAN, 5.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 3.0}}, am_), 1);
    (void)mgr_.clearRules(); (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::LESS_THAN, 5.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 5.0}}, am_), 0);
}

TEST_F(AlertRuleOperatorTest, LessThanOrEqual) {
    (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::LESS_THAN_OR_EQUAL, 5.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 5.0}}, am_), 1);
    (void)mgr_.clearRules(); (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::LESS_THAN_OR_EQUAL, 5.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 6.0}}, am_), 0);
}

TEST_F(AlertRuleOperatorTest, Equal) {
    (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::EQUAL, 42.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 42.0}}, am_), 1);
    (void)mgr_.clearRules(); (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::EQUAL, 42.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 43.0}}, am_), 0);
}

TEST_F(AlertRuleOperatorTest, NotEqual) {
    (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::NOT_EQUAL, 0.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 1.0}}, am_), 1);
    (void)mgr_.clearRules(); (void)mgr_.addRule(makeRule("r", "m", AlertRuleOperator::NOT_EQUAL, 0.0));
    EXPECT_EQ(mgr_.evaluateRules({{"m", 0.0}}, am_), 0);
}

// ============================================================================
// Message Template Tests
// ============================================================================

TEST(AlertRuleMessageTemplateTest, EvaluateRules_FiresWithCorrectMessage) {
    AlertRuleManager mgr;
    DefaultAlertmanager am(makeDisabledAlertmanager());

    AlertRule rule = makeRule("msg_rule", "themis_cpu",
                              AlertRuleOperator::GREATER_THAN, 80.0);
    rule.message_template = "CPU {metric} is at {value} percent";
    (void)mgr.addRule(rule);

    std::map<std::string, double> metrics{{"themis_cpu", 90.0}};
    int triggered = mgr.evaluateRules(metrics, am);
    EXPECT_EQ(triggered, 1);

    // The alert was fired; check that the active alert list has an entry.
    auto alerts = am.getActiveAlerts();
    ASSERT_EQ(alerts.size(), 1u);
    EXPECT_NE(alerts[0].message.find("themis_cpu"), std::string::npos);
    EXPECT_NE(alerts[0].message.find("90"), std::string::npos);
}

TEST(AlertRuleMessageTemplateTest, DefaultMessage_WhenTemplateEmpty) {
    AlertRuleManager mgr;
    DefaultAlertmanager am(makeDisabledAlertmanager());

    AlertRule rule = makeRule("no_tmpl", "themis_mem", AlertRuleOperator::GREATER_THAN, 100.0);
    rule.message_template = "";
    (void)mgr.addRule(rule);

    std::map<std::string, double> metrics{{"themis_mem", 200.0}};
    EXPECT_EQ(mgr.evaluateRules(metrics, am), 1);

    auto alerts = am.getActiveAlerts();
    ASSERT_EQ(alerts.size(), 1u);
    EXPECT_FALSE(alerts[0].message.empty());
}

// ============================================================================
// Labels & Annotations Tests
// ============================================================================

TEST(AlertRuleLabelTest, LabelsAttachedToFiredAlert) {
    AlertRuleManager mgr;
    DefaultAlertmanager am(makeDisabledAlertmanager());

    AlertRule rule = makeRule("label_rule", "themis_cpu",
                              AlertRuleOperator::GREATER_THAN, 80.0);
    rule.labels["component"] = "database";
    rule.labels["env"]       = "production";
    (void)mgr.addRule(rule);

    std::map<std::string, double> metrics{{"themis_cpu", 90.0}};
    mgr.evaluateRules(metrics, am);

    auto alerts = am.getActiveAlerts();
    ASSERT_EQ(alerts.size(), 1u);
    EXPECT_EQ(alerts[0].labels.at("component"), "database");
    EXPECT_EQ(alerts[0].labels.at("env"), "production");
    EXPECT_EQ(alerts[0].labels.at("metric_name"), "themis_cpu");
    EXPECT_EQ(alerts[0].labels.at("rule_id"), "label_rule");
}

// ============================================================================
// Compile-time check: AlertRuleManager must not be movable (std::mutex member)
// ============================================================================
static_assert(!std::is_move_constructible<AlertRuleManager>::value,
              "AlertRuleManager must not be move-constructible (contains std::mutex)");
static_assert(!std::is_move_assignable<AlertRuleManager>::value,
              "AlertRuleManager must not be move-assignable (contains std::mutex)");

// ============================================================================
// Concurrency Tests
// ============================================================================

TEST(AlertRuleConcurrencyTest, CrudDuringEvaluateRules_DoesNotDeadlock) {
    // Verify that addRule/removeRule can run concurrently with evaluateRules
    // without deadlock or data corruption.
    AlertRuleManager mgr;
    DefaultAlertmanager am(makeDisabledAlertmanager());

    // Seed some initial rules
    for (int i = 0; i < 5; ++i) {
        (void)mgr.addRule(makeRule("pre_" + std::to_string(i), "themis_cpu",
                       AlertRuleOperator::GREATER_THAN, 80.0));
    }

    std::atomic<bool> done{false};
    std::map<std::string, double> hot_metrics{{"themis_cpu", 99.0}};

    // Thread A: evaluate rules until CRUD thread signals completion
    std::thread eval_thread([&] {
        while (!done.load(std::memory_order_acquire)) {
            mgr.evaluateRules(hot_metrics, am);
        }
    });

    // Thread B: add and remove rules concurrently
    std::thread crud_thread([&] {
        for (int i = 0; i < 10; ++i) {
            std::string id = "concurrent_" + std::to_string(i);
            (void)mgr.addRule(makeRule(id, "themis_cpu",
                                       AlertRuleOperator::GREATER_THAN, 50.0));
            (void)mgr.removeRule(id);
        }
        done = true;
    });

    eval_thread.join();
    crud_thread.join();

    // No assertion beyond "did not deadlock or crash"
    SUCCEED();
}
