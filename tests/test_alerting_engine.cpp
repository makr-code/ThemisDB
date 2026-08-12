/**
 * @file test_alerting_engine.cpp
 * @brief Unit tests for the rule-based AlertingEngine with configurable
 *        notification channels.
 *
 * Covers:
 * - Channel management (addChannel, clearChannels, channelCount)
 * - loadDefaultRules() — idempotency and expected rule set
 * - evaluateAndNotify() — fires/resolves alerts and notifies channels
 * - LogNotificationChannel — always succeeds
 * - WebhookNotificationChannel — returns error on empty URL
 * - SlackNotificationChannel — returns error on empty URL
 * - AlertingEngine sendAlert/resolveAlert lifecycle
 * - Backend forwarding (optional Alertmanager backend)
 */

#include <gtest/gtest.h>
#include "observability/alerting_engine.h"
#include <atomic>
#include <thread>

using namespace themis;
using namespace themis::observability;

// ============================================================================
// Test helpers
// ============================================================================

/**
 * A simple in-process notification channel that records every alert it receives.
 */
class RecordingChannel : public INotificationChannel {
public:
    std::string channelType() const override { return "recording"; }

    Result<void> send(const Alert& alert) override {
        received.push_back(alert);
        return {};
    }

    std::vector<Alert> received;
};

/**
 * A notification channel that always returns an error.
 */
class FailingChannel : public INotificationChannel {
public:
    std::string channelType() const override { return "failing"; }

    Result<void> send(const Alert& /*alert*/) override {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            "intentional test failure"
        });
    }
};

static AlertRule makeRule(const std::string& id,
                          const std::string& metric,
                          AlertRuleOperator op,
                          double threshold,
                          AlertSeverity sev = AlertSeverity::WARNING) {
    AlertRule r;
    r.rule_id          = id;
    r.rule_name        = "Test rule " + id;
    r.metric_name      = metric;
    r.op               = op;
    r.threshold        = threshold;
    r.severity         = sev;
    r.message_template = "Metric {metric} = {value}";
    r.enabled          = true;
    return r;
}

// ============================================================================
// Channel management tests
// ============================================================================

class AlertingEngineChannelTest : public ::testing::Test {
protected:
    AlertingEngine engine_;
};

TEST_F(AlertingEngineChannelTest, AddChannel_IncrementsCount) {
    EXPECT_EQ(engine_.channelCount(), 0u);
    engine_.addChannel(std::make_shared<RecordingChannel>());
    EXPECT_EQ(engine_.channelCount(), 1u);
    engine_.addChannel(std::make_shared<LogNotificationChannel>());
    EXPECT_EQ(engine_.channelCount(), 2u);
}

TEST_F(AlertingEngineChannelTest, AddNullptrChannel_IsIgnored) {
    engine_.addChannel(nullptr);
    EXPECT_EQ(engine_.channelCount(), 0u);
}

TEST_F(AlertingEngineChannelTest, ClearChannels_ResetsCount) {
    engine_.addChannel(std::make_shared<RecordingChannel>());
    engine_.addChannel(std::make_shared<RecordingChannel>());
    ASSERT_EQ(engine_.channelCount(), 2u);
    engine_.clearChannels();
    EXPECT_EQ(engine_.channelCount(), 0u);
}

TEST_F(AlertingEngineChannelTest, GetChannels_ReturnsCopy) {
    auto ch1 = std::make_shared<RecordingChannel>();
    auto ch2 = std::make_shared<RecordingChannel>();
    engine_.addChannel(ch1);
    engine_.addChannel(ch2);

    auto snapshot = engine_.channels();
    ASSERT_EQ(snapshot.size(), 2u);
    EXPECT_EQ(snapshot[0], ch1);
    EXPECT_EQ(snapshot[1], ch2);
}

// ============================================================================
// Default rules tests
// ============================================================================

class AlertingEngineDefaultRulesTest : public ::testing::Test {
protected:
    AlertingEngine engine_;
};

TEST_F(AlertingEngineDefaultRulesTest, LoadDefaultRules_RegistersExpectedRules) {
    engine_.loadDefaultRules();
    auto rules = engine_.ruleManager().listRules();
    EXPECT_GE(rules.size(), 8u);  // at least 8 predefined rules
}

TEST_F(AlertingEngineDefaultRulesTest, LoadDefaultRules_IsIdempotent) {
    engine_.loadDefaultRules();
    size_t first_count = engine_.ruleManager().ruleCount();
    engine_.loadDefaultRules();
    EXPECT_EQ(engine_.ruleManager().ruleCount(), first_count);
}

TEST_F(AlertingEngineDefaultRulesTest, DefaultRuleIds_ArePresent) {
    engine_.loadDefaultRules();

    const std::vector<std::string> expected_ids = {
        "default_cpu_high",
        "default_memory_high",
        "default_query_latency_p99",
        "default_error_rate_high",
        "default_disk_low",
        "default_query_queue_deep",
        "default_cache_miss_high",
        "default_write_amplification",
    };

    for (const auto& id : expected_ids) {
        EXPECT_TRUE(engine_.ruleManager().getRule(id).has_value())
            << "Default rule '" << id << "' is missing";
    }
}

TEST_F(AlertingEngineDefaultRulesTest, DefaultCpuRule_HasCorrectThreshold) {
    engine_.loadDefaultRules();
    auto rule = engine_.ruleManager().getRule("default_cpu_high");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->metric_name, "themis_cpu_usage_percent");
    EXPECT_EQ(rule->op, AlertRuleOperator::GREATER_THAN);
    EXPECT_DOUBLE_EQ(rule->threshold, 80.0);
    EXPECT_EQ(rule->severity, AlertSeverity::WARNING);
    EXPECT_TRUE(rule->enabled);
}

TEST_F(AlertingEngineDefaultRulesTest, DefaultDiskRule_IsLessThanOperator) {
    engine_.loadDefaultRules();
    auto rule = engine_.ruleManager().getRule("default_disk_low");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->op, AlertRuleOperator::LESS_THAN);
    EXPECT_EQ(rule->severity, AlertSeverity::CRITICAL);
}

// ============================================================================
// EvaluateAndNotify tests
// ============================================================================

class AlertingEngineEvalTest : public ::testing::Test {
protected:
    AlertingEngine engine_;
    std::shared_ptr<RecordingChannel> channel_ = std::make_shared<RecordingChannel>();

    void SetUp() override {
        engine_.addChannel(channel_);
    }
};

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_NoRules_ReturnsZero) {
    std::map<std::string, double> metrics{{"themis_cpu_usage_percent", 99.0}};
    EXPECT_EQ(engine_.evaluateAndNotify(metrics), 0);
    EXPECT_TRUE(channel_->received.empty());
}

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_ConditionMet_NotifiesChannel) {
    (void)engine_.ruleManager().addRule(
        makeRule("cpu_rule", "themis_cpu_usage_percent",
                 AlertRuleOperator::GREATER_THAN, 80.0));

    std::map<std::string, double> metrics{{"themis_cpu_usage_percent", 90.0}};
    int fired = engine_.evaluateAndNotify(metrics);

    EXPECT_EQ(fired, 1);
    ASSERT_EQ(channel_->received.size(), 1u);
    EXPECT_EQ(channel_->received[0].alert_name, "Test rule cpu_rule");
    EXPECT_EQ(channel_->received[0].status, AlertStatus::FIRING);
    EXPECT_EQ(channel_->received[0].severity, AlertSeverity::WARNING);
}

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_ConditionNotMet_NoNotification) {
    (void)engine_.ruleManager().addRule(
        makeRule("cpu_rule", "themis_cpu_usage_percent",
                 AlertRuleOperator::GREATER_THAN, 80.0));

    std::map<std::string, double> metrics{{"themis_cpu_usage_percent", 50.0}};
    int fired = engine_.evaluateAndNotify(metrics);

    EXPECT_EQ(fired, 0);
    EXPECT_TRUE(channel_->received.empty());
}

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_AlertResolved_NotifiesChannel) {
    (void)engine_.ruleManager().addRule(
        makeRule("cpu_rule", "themis_cpu_usage_percent",
                 AlertRuleOperator::GREATER_THAN, 80.0));

    // Fire the alert
    (void)engine_.evaluateAndNotify({{"themis_cpu_usage_percent", 90.0}});
    ASSERT_EQ(channel_->received.size(), 1u);

    // Clear and resolve
    channel_->received.clear();
    int still_firing = engine_.evaluateAndNotify({{"themis_cpu_usage_percent", 50.0}});
    EXPECT_EQ(still_firing, 0);

    // The channel should have received a RESOLVED notification
    ASSERT_EQ(channel_->received.size(), 1u);
    EXPECT_EQ(channel_->received[0].status, AlertStatus::RESOLVED);
}

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_MultipleChannels_AllNotified) {
    auto second_channel = std::make_shared<RecordingChannel>();
    engine_.addChannel(second_channel);

    (void)engine_.ruleManager().addRule(
        makeRule("r1", "m", AlertRuleOperator::GREATER_THAN, 0.0));

    (void)engine_.evaluateAndNotify({{"m", 1.0}});

    EXPECT_EQ(channel_->received.size(), 1u);
    EXPECT_EQ(second_channel->received.size(), 1u);
}

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_FailingChannel_DoesNotAbort) {
    engine_.addChannel(std::make_shared<FailingChannel>());
    engine_.addChannel(std::make_shared<RecordingChannel>());  // should still receive

    (void)engine_.ruleManager().addRule(
        makeRule("r1", "m", AlertRuleOperator::GREATER_THAN, 0.0));

    // Should not throw even though FailingChannel returns an error.
    EXPECT_NO_THROW(engine_.evaluateAndNotify({{"m", 1.0}}));

    // The recording channel (registered after the failing one) must still receive.
    auto snap = engine_.channels();
    for (const auto& ch : snap) {
        if (ch->channelType() == "recording") {
            auto* rec = dynamic_cast<RecordingChannel*>(ch.get());
            if (rec) {
                EXPECT_FALSE(rec->received.empty());
            }
        }
    }
}

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_DefaultRules_CpuThreshold) {
    engine_.loadDefaultRules();

    // CPU at 85% — above default threshold of 80%
    int fired = engine_.evaluateAndNotify({
        {"themis_cpu_usage_percent", 85.0},
        {"themis_memory_usage_percent", 50.0},
        {"themis_disk_free_percent", 50.0}
    });

    EXPECT_GE(fired, 1);

    bool found_cpu_alert = false;
    for (const auto& alert : channel_->received) {
        if (alert.labels.count("rule_id") &&
            alert.labels.at("rule_id") == "default_cpu_high") {
            found_cpu_alert = true;
            EXPECT_EQ(alert.severity, AlertSeverity::WARNING);
            EXPECT_EQ(alert.status,   AlertStatus::FIRING);
        }
    }
    EXPECT_TRUE(found_cpu_alert);
}

TEST_F(AlertingEngineEvalTest, EvaluateAndNotify_DefaultRules_DiskCritical) {
    engine_.loadDefaultRules();

    int fired = engine_.evaluateAndNotify({
        {"themis_disk_free_percent", 5.0}  // below 10% threshold
    });

    EXPECT_GE(fired, 1);

    bool found = false;
    for (const auto& alert : channel_->received) {
        if (alert.labels.count("rule_id") &&
            alert.labels.at("rule_id") == "default_disk_low") {
            found = true;
            EXPECT_EQ(alert.severity, AlertSeverity::CRITICAL);
        }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// sendAlert / resolveAlert lifecycle
// ============================================================================

TEST(AlertingEngineSendTest, SendAlert_PopulatesActiveAlerts) {
    AlertingEngine engine;
    auto rec = std::make_shared<RecordingChannel>();
    engine.addChannel(rec);

    Alert a;
    a.alert_name = "TestAlert";
    a.alert_id   = "test-001";
    a.severity   = AlertSeverity::ERROR;
    a.status     = AlertStatus::FIRING;
    a.message    = "test message";

    auto res = engine.sendAlert(a);
    EXPECT_TRUE(res.has_value());

    auto active = engine.getActiveAlerts();
    ASSERT_EQ(active.size(), 1u);
    EXPECT_EQ(active[0].alert_id, "test-001");

    ASSERT_EQ(rec->received.size(), 1u);
    EXPECT_EQ(rec->received[0].status, AlertStatus::FIRING);
}

TEST(AlertingEngineSendTest, ResolveAlert_RemovesFromActiveAndNotifiesChannel) {
    AlertingEngine engine;
    auto rec = std::make_shared<RecordingChannel>();
    engine.addChannel(rec);

    Alert a;
    a.alert_name = "TestAlert";
    a.alert_id   = "test-002";
    a.severity   = AlertSeverity::WARNING;
    a.status     = AlertStatus::FIRING;
    a.message    = "firing";
    (void)engine.sendAlert(a);

    rec->received.clear();
    (void)engine.resolveAlert("test-002");

    // Active list should be empty after resolution.
    EXPECT_TRUE(engine.getActiveAlerts().empty());

    // Channel should have received the RESOLVED notification.
    ASSERT_EQ(rec->received.size(), 1u);
    EXPECT_EQ(rec->received[0].status, AlertStatus::RESOLVED);
}

TEST(AlertingEngineSendTest, ResolveAlert_NonexistentId_IsNoOp) {
    AlertingEngine engine;
    auto res = engine.resolveAlert("does-not-exist");
    EXPECT_TRUE(res.has_value());
}

// ============================================================================
// Backend forwarding
// ============================================================================

namespace {
class MockAlertmanager : public Alertmanager {
public:
    int send_count    = 0;
    int resolve_count = 0;
    int silence_count = 0;

    Result<void> sendAlert(const Alert& /*alert*/) override {
        ++send_count;
        return {};
    }
    Result<void> resolveAlert(const std::string& /*id*/) override {
        ++resolve_count;
        return {};
    }
    Result<void> silenceAlert(const std::string& /*id*/, int /*minutes*/) override {
        ++silence_count;
        return {};
    }
};
} // namespace

TEST(AlertingEngineBackendTest, SendAlert_ForwardsToBackend) {
    auto mock = std::make_shared<MockAlertmanager>();
    AlertingEngine engine(mock);

    Alert a;
    a.alert_name = "BackendTest";
    a.alert_id   = "bk-001";
    a.severity   = AlertSeverity::INFO;
    a.status     = AlertStatus::FIRING;
    (void)engine.sendAlert(a);

    EXPECT_EQ(mock->send_count, 1);
}

TEST(AlertingEngineBackendTest, ResolveAlert_ForwardsToBackend) {
    auto mock = std::make_shared<MockAlertmanager>();
    AlertingEngine engine(mock);

    Alert a;
    a.alert_id = "bk-002";
    a.status   = AlertStatus::FIRING;
    (void)engine.sendAlert(a);

    (void)engine.resolveAlert("bk-002");
    EXPECT_EQ(mock->resolve_count, 1);
}

TEST(AlertingEngineBackendTest, TestConnection_NoBackend_ReturnsOk) {
    AlertingEngine engine;
    EXPECT_TRUE(engine.testConnection().has_value());
}

TEST(AlertingEngineBackendTest, TestConnection_WithBackend_DelegatesToBackend) {
    // A backend whose testConnection always returns an error.
    class FailBackend : public Alertmanager {
    public:
        Result<void> testConnection() override {
            return tl::unexpected(Error{
                errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
                "no backend"
            });
        }
    };

    AlertingEngine engine(std::make_shared<FailBackend>());
    EXPECT_FALSE(engine.testConnection().has_value());
}

// ============================================================================
// LogNotificationChannel — basic smoke test
// ============================================================================

TEST(LogNotificationChannelTest, ChannelType) {
    LogNotificationChannel ch;
    EXPECT_EQ(ch.channelType(), "log");
}

TEST(LogNotificationChannelTest, Send_AlwaysSucceeds) {
    LogNotificationChannel ch;
    Alert a;
    a.alert_name = "LogTest";
    a.severity   = AlertSeverity::CRITICAL;
    a.status     = AlertStatus::FIRING;
    a.message    = "critical log test";
    EXPECT_TRUE(ch.send(a).has_value());
}

// ============================================================================
// WebhookNotificationChannel
// ============================================================================

TEST(WebhookNotificationChannelTest, ChannelType) {
    WebhookChannelConfig cfg;
    cfg.url = "http://localhost:9999";
    WebhookNotificationChannel ch(cfg);
    EXPECT_EQ(ch.channelType(), "webhook");
}

TEST(WebhookNotificationChannelTest, EmptyUrl_ReturnsError) {
    WebhookChannelConfig cfg;  // url is empty by default
    WebhookNotificationChannel ch(cfg);
    Alert a;
    a.alert_name = "WebhookTest";
    a.status     = AlertStatus::FIRING;
    auto res = ch.send(a);
    EXPECT_FALSE(res.has_value());
}

// ============================================================================
// SlackNotificationChannel
// ============================================================================

TEST(SlackNotificationChannelTest, ChannelType) {
    SlackChannelConfig cfg;
    cfg.webhook_url = "https://hooks.slack.com/test";
    SlackNotificationChannel ch(cfg);
    EXPECT_EQ(ch.channelType(), "slack");
}

TEST(SlackNotificationChannelTest, EmptyWebhookUrl_ReturnsError) {
    SlackChannelConfig cfg;  // webhook_url is empty
    SlackNotificationChannel ch(cfg);
    Alert a;
    a.alert_name = "SlackTest";
    a.status     = AlertStatus::FIRING;
    auto res = ch.send(a);
    EXPECT_FALSE(res.has_value());
}

// ============================================================================
// silenceAlert
// ============================================================================

TEST(AlertingEngineSilenceTest, SilenceAlert_MarksAlertSilenced) {
    AlertingEngine engine;
    auto rec = std::make_shared<RecordingChannel>();
    engine.addChannel(rec);

    Alert a;
    a.alert_name = "SilenceTest";
    a.alert_id   = "sl-001";
    a.severity   = AlertSeverity::WARNING;
    a.status     = AlertStatus::FIRING;
    a.message    = "needs silencing";
    (void)engine.sendAlert(a);

    // Silence the alert — should mark it silenced in active list.
    auto res = engine.silenceAlert("sl-001", 30);
    EXPECT_TRUE(res.has_value());

    auto active = engine.getActiveAlerts();
    ASSERT_EQ(active.size(), 1u);
    EXPECT_EQ(active[0].status, AlertStatus::SILENCED);
}

TEST(AlertingEngineSilenceTest, SilenceAlert_NonexistentId_IsNoOp) {
    AlertingEngine engine;
    auto res = engine.silenceAlert("ghost", 10);
    EXPECT_TRUE(res.has_value());
}

TEST(AlertingEngineSilenceTest, SilenceAlert_ForwardsToBackend) {
    auto mock = std::make_shared<MockAlertmanager>();
    AlertingEngine engine(mock);

    Alert a;
    a.alert_id = "sl-002";
    a.status   = AlertStatus::FIRING;
    (void)engine.sendAlert(a);

    auto res = engine.silenceAlert("sl-002", 15);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(mock->silence_count, 1);
}

// ============================================================================
// Concurrency: addChannel + evaluateAndNotify
// ============================================================================

TEST(AlertingEngineConcurrencyTest, AddChannelDuringEvaluate_NoDeadlock) {
    AlertingEngine engine;
    (void)engine.ruleManager().addRule(
        makeRule("conc", "m", AlertRuleOperator::GREATER_THAN, 0.0));

    std::atomic<bool> done{false};
    std::map<std::string, double> metrics{{"m", 1.0}};

    std::thread eval_thread([&] {
        while (!done.load(std::memory_order_acquire)) {
            engine.evaluateAndNotify(metrics);
        }
    });

    std::thread add_thread([&] {
        for (int i = 0; i < 10; ++i) {
            engine.addChannel(std::make_shared<LogNotificationChannel>());
            engine.clearChannels();
        }
        done.store(true, std::memory_order_release);
    });

    eval_thread.join();
    add_thread.join();

    SUCCEED();
}
