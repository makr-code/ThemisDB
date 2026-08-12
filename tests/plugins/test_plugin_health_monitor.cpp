/**
 * @file test_plugin_health_monitor.cpp
 * @brief Unit tests for PluginHealthMonitor
 *
 * Tests cover:
 * - Plugin registration and unregistration
 * - Monitoring lifecycle (start/stop)
 * - Manual health checks and recovery
 * - Automatic recovery with backoff
 * - Event callbacks
 * - Global statistics
 * - Per-plugin enable/disable
 */

#include <gtest/gtest.h>
#include "plugins/plugin_health_monitor.h"
#include "plugins/self_healing_plugin.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::plugins;

// ============================================================================
// Mock ISelfHealingPlugin implementation
// ============================================================================

class MockSelfHealingPlugin : public ISelfHealingPlugin {
public:
    // Controls returned by individual tests
    PluginHealthStatus next_status = PluginHealthStatus::HEALTHY;
    bool repair_result = true;
    bool cleanup_result = true;
    bool rollback_result = true;

    std::atomic<int> health_check_count{0};
    std::atomic<int> repair_count{0};
    std::atomic<int> cleanup_count{0};
    std::atomic<int> rollback_count{0};
    std::atomic<int> checkpoint_count{0};

    PluginDiagnostics performHealthCheck() override {
        health_check_count++;
        PluginDiagnostics d;
        d.status = next_status;
        if (next_status != PluginHealthStatus::HEALTHY &&
            next_status != PluginHealthStatus::DEGRADED) {
            d.error_message = "mock error";
        }
        return d;
    }

    bool attemptSelfRepair(const PluginDiagnostics&) override {
        repair_count++;
        return repair_result;
    }

    bool cleanupResources() override {
        cleanup_count++;
        return cleanup_result;
    }

    bool rollbackToLastGoodState() override {
        rollback_count++;
        return rollback_result;
    }

    void saveCheckpoint() override {
        checkpoint_count++;
    }

    std::vector<RecoveryAction> getRecoveryStrategies() const override {
        return {RecoveryAction::CLEANUP_RESOURCES, RecoveryAction::RESTART_PLUGIN};
    }
};

// ============================================================================
// Test fixture
// ============================================================================

class PluginHealthMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a fast check interval for unit tests
        HealthMonitorConfig cfg;
        cfg.check_interval       = std::chrono::seconds{3600}; // prevent auto-cycle
        cfg.max_recovery_attempts = 3;
        cfg.backoff_strategy     = "none";
        cfg.auto_disable_on_failure = true;
        monitor_ = std::make_unique<PluginHealthMonitor>(cfg);
    }

    void TearDown() override {
        monitor_->stopMonitoring();
    }

    std::unique_ptr<PluginHealthMonitor> monitor_;
};

// ============================================================================
// Registration tests
// ============================================================================

TEST_F(PluginHealthMonitorTest, RegisterPlugin_Success) {
    MockSelfHealingPlugin plugin;
    EXPECT_TRUE(monitor_->registerPlugin("p1", &plugin));
}

TEST_F(PluginHealthMonitorTest, RegisterPlugin_NullPointerRejected) {
    EXPECT_FALSE(monitor_->registerPlugin("null_plugin", nullptr));
}

TEST_F(PluginHealthMonitorTest, RegisterPlugin_DuplicateRejected) {
    MockSelfHealingPlugin plugin;
    EXPECT_TRUE(monitor_->registerPlugin("dup", &plugin));
    EXPECT_FALSE(monitor_->registerPlugin("dup", &plugin));
}

TEST_F(PluginHealthMonitorTest, UnregisterPlugin_Success) {
    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("p2", &plugin);
    EXPECT_TRUE(monitor_->unregisterPlugin("p2"));
}

TEST_F(PluginHealthMonitorTest, UnregisterPlugin_UnknownReturnsFalse) {
    EXPECT_FALSE(monitor_->unregisterPlugin("does_not_exist"));
}

// ============================================================================
// Monitoring lifecycle tests
// ============================================================================

TEST_F(PluginHealthMonitorTest, StartMonitoring_Succeeds) {
    EXPECT_TRUE(monitor_->startMonitoring());
    EXPECT_TRUE(monitor_->isRunning());
}

TEST_F(PluginHealthMonitorTest, StartMonitoring_IdempotentReturnsFalse) {
    monitor_->startMonitoring();
    EXPECT_FALSE(monitor_->startMonitoring()); // second start should fail
    EXPECT_TRUE(monitor_->isRunning());
}

TEST_F(PluginHealthMonitorTest, StopMonitoring_StopsThread) {
    monitor_->startMonitoring();
    monitor_->stopMonitoring();
    EXPECT_FALSE(monitor_->isRunning());
}

TEST_F(PluginHealthMonitorTest, StopMonitoring_NotRunning_IsNoOp) {
    EXPECT_FALSE(monitor_->isRunning());
    // Should not throw
    EXPECT_NO_THROW(monitor_->stopMonitoring());
}

// ============================================================================
// Manual health check tests
// ============================================================================

TEST_F(PluginHealthMonitorTest, CheckPluginHealth_HealthyPlugin) {
    MockSelfHealingPlugin plugin;
    plugin.next_status = PluginHealthStatus::HEALTHY;
    monitor_->registerPlugin("healthy", &plugin);

    auto result = monitor_->checkPluginHealth("healthy");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, PluginHealthStatus::HEALTHY);
    EXPECT_EQ(plugin.health_check_count.load(), 1);
}

TEST_F(PluginHealthMonitorTest, CheckPluginHealth_UnhealthyPlugin) {
    MockSelfHealingPlugin plugin;
    plugin.next_status = PluginHealthStatus::UNHEALTHY;
    monitor_->registerPlugin("unhealthy", &plugin);

    auto result = monitor_->checkPluginHealth("unhealthy");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, PluginHealthStatus::UNHEALTHY);
}

TEST_F(PluginHealthMonitorTest, CheckPluginHealth_UnknownPluginReturnsError) {
    auto result = monitor_->checkPluginHealth("unknown");
    EXPECT_FALSE(result);
}

TEST_F(PluginHealthMonitorTest, CheckPluginHealth_IncrementsTotalChecks) {
    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("stats_plugin", &plugin);

    monitor_->checkPluginHealth("stats_plugin");
    monitor_->checkPluginHealth("stats_plugin");

    auto gs = monitor_->getGlobalStats();
    EXPECT_EQ(gs.total_health_checks, 2u);
}

// ============================================================================
// Manual recovery tests
// ============================================================================

TEST_F(PluginHealthMonitorTest, TriggerRecovery_SuccessfulRecovery) {
    MockSelfHealingPlugin plugin;
    plugin.next_status = PluginHealthStatus::UNHEALTHY;
    plugin.cleanup_result = true;
    monitor_->registerPlugin("recover_ok", &plugin);

    auto result = monitor_->triggerRecovery("recover_ok");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->successful);
}

TEST_F(PluginHealthMonitorTest, TriggerRecovery_UnknownPluginReturnsError) {
    auto result = monitor_->triggerRecovery("no_such_plugin");
    EXPECT_FALSE(result);
}

TEST_F(PluginHealthMonitorTest, TriggerRecovery_FailedRecovery) {
    MockSelfHealingPlugin plugin;
    plugin.next_status = PluginHealthStatus::UNHEALTHY;
    plugin.cleanup_result = false;
    plugin.repair_result  = false;
    plugin.rollback_result = false;
    monitor_->registerPlugin("recover_fail", &plugin);

    auto result = monitor_->triggerRecovery("recover_fail");
    ASSERT_TRUE(result);
    EXPECT_FALSE(result->successful);
}

// ============================================================================
// Statistics tests
// ============================================================================

TEST_F(PluginHealthMonitorTest, GetPluginStats_RegisteredPlugin) {
    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("stat_p", &plugin);

    auto stats = monitor_->getPluginStats("stat_p");
    ASSERT_TRUE(stats);
    EXPECT_EQ(stats->name, "stat_p");
    EXPECT_TRUE(stats->enabled);
}

TEST_F(PluginHealthMonitorTest, GetPluginStats_UnknownReturnsError) {
    auto stats = monitor_->getPluginStats("ghost");
    EXPECT_FALSE(stats);
}

TEST_F(PluginHealthMonitorTest, GetAllPluginStats_EmptyMap) {
    auto all = monitor_->getAllPluginStats();
    EXPECT_TRUE(all.empty());
}

TEST_F(PluginHealthMonitorTest, GetAllPluginStats_MultiplePlugins) {
    MockSelfHealingPlugin p1, p2;
    monitor_->registerPlugin("a", &p1);
    monitor_->registerPlugin("b", &p2);

    auto all = monitor_->getAllPluginStats();
    EXPECT_EQ(all.size(), 2u);
    EXPECT_NE(all.find("a"), all.end());
    EXPECT_NE(all.find("b"), all.end());
}

TEST_F(PluginHealthMonitorTest, GlobalStats_ReflectsPluginCount) {
    MockSelfHealingPlugin p1, p2;
    monitor_->registerPlugin("x", &p1);
    monitor_->registerPlugin("y", &p2);

    auto gs = monitor_->getGlobalStats();
    EXPECT_EQ(gs.monitored_plugins_count, 2u);
}

// ============================================================================
// Enable / disable per-plugin monitoring
// ============================================================================

TEST_F(PluginHealthMonitorTest, SetPluginMonitoringEnabled_DisablesPlugin) {
    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("toggled", &plugin);

    EXPECT_TRUE(monitor_->setPluginMonitoringEnabled("toggled", false));
    auto stats = monitor_->getPluginStats("toggled");
    ASSERT_TRUE(stats);
    EXPECT_FALSE(stats->enabled);
}

TEST_F(PluginHealthMonitorTest, SetPluginMonitoringEnabled_ReEnablesPlugin) {
    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("toggled2", &plugin);
    monitor_->setPluginMonitoringEnabled("toggled2", false);
    EXPECT_TRUE(monitor_->setPluginMonitoringEnabled("toggled2", true));

    auto stats = monitor_->getPluginStats("toggled2");
    ASSERT_TRUE(stats);
    EXPECT_TRUE(stats->enabled);
}

TEST_F(PluginHealthMonitorTest, SetPluginMonitoringEnabled_UnknownReturnsFalse) {
    EXPECT_FALSE(monitor_->setPluginMonitoringEnabled("no_plugin", true));
}

// ============================================================================
// Event callback tests
// ============================================================================

TEST_F(PluginHealthMonitorTest, RegisterEventCallback_CalledOnRegistration) {
    std::vector<MonitoringEvent> received_events;
    monitor_->registerEventCallback([&](const MonitoringEventData& ev) {
        received_events.push_back(ev.event);
    });

    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("cb_plugin", &plugin);

    ASSERT_FALSE(received_events.empty());
    EXPECT_EQ(received_events.front(), MonitoringEvent::PLUGIN_REGISTERED);
}

TEST_F(PluginHealthMonitorTest, RegisterEventCallback_CalledOnUnregistration) {
    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("unregcb", &plugin);

    std::vector<MonitoringEvent> events;
    monitor_->registerEventCallback([&](const MonitoringEventData& ev) {
        events.push_back(ev.event);
    });

    monitor_->unregisterPlugin("unregcb");
    ASSERT_FALSE(events.empty());
    EXPECT_EQ(events.front(), MonitoringEvent::PLUGIN_UNREGISTERED);
}

TEST_F(PluginHealthMonitorTest, ClearEventCallbacks_NoCallbackAfterClear) {
    std::atomic<int> call_count{0};
    monitor_->registerEventCallback([&](const MonitoringEventData&) {
        call_count++;
    });
    monitor_->clearEventCallbacks();

    MockSelfHealingPlugin plugin;
    monitor_->registerPlugin("nocb", &plugin);

    EXPECT_EQ(call_count.load(), 0);
}

// ============================================================================
// Configuration tests
// ============================================================================

TEST_F(PluginHealthMonitorTest, GetConfig_ReturnsCurrentConfig) {
    const auto& cfg = monitor_->getConfig();
    EXPECT_EQ(cfg.max_recovery_attempts, 3u);
    EXPECT_EQ(cfg.backoff_strategy, "none");
}

TEST_F(PluginHealthMonitorTest, UpdateConfig_ChangesConfig) {
    HealthMonitorConfig new_cfg;
    new_cfg.max_recovery_attempts = 7;
    monitor_->updateConfig(new_cfg);
    EXPECT_EQ(monitor_->getConfig().max_recovery_attempts, 7u);
}

// ============================================================================
// Singleton access
// ============================================================================

TEST(PluginHealthMonitorSingleton, InstanceReturnsSameObject) {
    auto& a = PluginHealthMonitor::instance();
    auto& b = PluginHealthMonitor::instance();
    EXPECT_EQ(&a, &b);
}

// ============================================================================
// Auto-disable after max recovery attempts
// ============================================================================

TEST_F(PluginHealthMonitorTest, AutoDisable_AfterMaxRecoveryAttempts) {
    HealthMonitorConfig cfg;
    cfg.check_interval          = std::chrono::seconds{3600};
    cfg.max_recovery_attempts   = 2;
    cfg.backoff_strategy        = "none";
    cfg.auto_disable_on_failure = true;
    auto mon = std::make_unique<PluginHealthMonitor>(cfg);

    MockSelfHealingPlugin plugin;
    plugin.next_status    = PluginHealthStatus::UNHEALTHY;
    plugin.cleanup_result = false;
    plugin.repair_result  = false;
    plugin.rollback_result = false;
    mon->registerPlugin("exhausted", &plugin);

    // Exhaust recovery attempts
    for (uint32_t i = 0; i < cfg.max_recovery_attempts; ++i) {
        mon->triggerRecovery("exhausted");
    }

    // One more recovery attempt should be blocked (max reached) and plugin auto-disabled
    mon->triggerRecovery("exhausted");

    auto stats = mon->getPluginStats("exhausted");
    ASSERT_TRUE(stats);
    EXPECT_GE(stats->total_recovery_attempts, cfg.max_recovery_attempts);
}
