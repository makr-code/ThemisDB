/**
 * @file test_gap008_observability.cpp
 * @brief Tests for GAP-008 Observability – Prometheus Alertmanager v2 HTTP API integration
 * 
 * Tests the Alertmanager implementation including:
 * - Configuration (including retry config)
 * - Alert lifecycle (send, resolve, silence, getActiveAlerts)
 * - Disabled-mode fallback (no HTTP calls, returns success)
 * - Enabled-mode with unreachable endpoint returns error
 * 
 * Note: HealthCheck functionality is provided by existing systems:
 * - sharding::HealthCheckSystem (shard/cluster health)
 * - sharding::HealthMonitor (node health with auto-failover)
 * - server::HealthErrorService (HTTP health endpoint)
 */

#include <gtest/gtest.h>
#include "observability/alertmanager.h"
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

namespace themis {
namespace observability {
namespace test {

// ============================================================================
// Alertmanager Tests
// ============================================================================

class GAP008AlertmanagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        AlertmanagerConfig config;
        config.endpoint_url = "http://alertmanager:9093";
        config.timeout_seconds = 10;
        config.enabled = false;  // Disabled for testing – no network calls
        config.retry_count = 0;
        
        alertmanager_ = std::make_unique<DefaultAlertmanager>(config);
    }
    
    void TearDown() override {
        alertmanager_.reset();
    }
    
    std::unique_ptr<DefaultAlertmanager> alertmanager_;
};

TEST_F(GAP008AlertmanagerTest, InitializeWithConfig) {
    AlertmanagerConfig config;
    config.endpoint_url = "http://localhost:9093";
    config.enabled = false;  // Keep disabled to avoid network calls in test
    config.timeout_seconds = 15;
    config.receivers = {"email", "slack"};
    config.retry_count = 2;
    config.retry_delay_ms = 100;
    
    auto result = alertmanager_->initialize(config);
    
    // Initialization should succeed
    EXPECT_TRUE(result.has_value());
    
    // Configuration should be stored
    EXPECT_EQ(alertmanager_->getConfig().endpoint_url, "http://localhost:9093");
    EXPECT_FALSE(alertmanager_->getConfig().enabled);
    EXPECT_EQ(alertmanager_->getConfig().timeout_seconds, 15);
    EXPECT_EQ(alertmanager_->getConfig().retry_count, 2);
    EXPECT_EQ(alertmanager_->getConfig().retry_delay_ms, 100);
}

TEST_F(GAP008AlertmanagerTest, SendAlertLogsAlert) {
    Alert alert;
    alert.alert_name = "HighMemoryUsage";
    alert.alert_id = "alert_001";
    alert.severity = AlertSeverity::WARNING;
    alert.status = AlertStatus::FIRING;
    alert.message = "Memory usage above 80%";
    alert.labels["component"] = "database";
    alert.labels["instance"] = "themisdb-0";
    
    // Stub implementation logs alerts and returns success (when disabled)
    // Since alertmanager is disabled in test setup, it should succeed
    auto result = alertmanager_->sendAlert(alert);
    EXPECT_TRUE(result.has_value());
}

TEST_F(GAP008AlertmanagerTest, ResolveAlertReturnsSuccess) {
    // First create an alert
    Alert alert;
    alert.alert_id = "alert_002";
    alert.alert_name = "DatabaseDown";
    alert.severity = AlertSeverity::CRITICAL;
    alert.status = AlertStatus::FIRING;
    alert.message = "Database is not responding";
    
    alertmanager_->sendAlert(alert);
    
    // Then resolve it – disabled mode returns success without network call
    auto result = alertmanager_->resolveAlert("alert_002");
    EXPECT_TRUE(result.has_value());
}

TEST_F(GAP008AlertmanagerTest, SilenceAlertWorks) {
    Alert alert;
    alert.alert_id = "alert_003";
    alert.alert_name = "HighDiskUsage";
    alert.severity = AlertSeverity::WARNING;
    alert.status = AlertStatus::FIRING;
    
    alertmanager_->sendAlert(alert);
    
    // Silence for 60 minutes – disabled mode returns success without network call
    auto result = alertmanager_->silenceAlert("alert_003", 60);
    EXPECT_TRUE(result.has_value());
}

TEST_F(GAP008AlertmanagerTest, GetActiveAlertsReturnsVector) {
    // Send a few alerts
    for (int i = 0; i < 3; i++) {
        Alert alert;
        alert.alert_id = "alert_00" + std::to_string(i);
        alert.alert_name = "TestAlert" + std::to_string(i);
        alert.severity = AlertSeverity::INFO;
        alert.status = AlertStatus::FIRING;
        
        alertmanager_->sendAlert(alert);
    }
    
    auto alerts = alertmanager_->getActiveAlerts();
    // Should return active (firing) alerts that were sent
    EXPECT_EQ(alerts.size(), 3u);
}

TEST_F(GAP008AlertmanagerTest, ResolveRemovesFromActiveAlerts) {
    Alert alert;
    alert.alert_id = "alert_resolve_test";
    alert.alert_name = "ResolveTest";
    alert.severity = AlertSeverity::WARNING;
    alert.status = AlertStatus::FIRING;
    alertmanager_->sendAlert(alert);
    EXPECT_EQ(alertmanager_->getActiveAlerts().size(), 1u);

    alertmanager_->resolveAlert("alert_resolve_test");
    EXPECT_EQ(alertmanager_->getActiveAlerts().size(), 0u);
}

TEST_F(GAP008AlertmanagerTest, TestConnectionFailsWhenDisabled) {
    // testConnection must return an error when the alertmanager is disabled.
    auto result = alertmanager_->testConnection();
    EXPECT_FALSE(result.has_value());
}

TEST_F(GAP008AlertmanagerTest, EnabledModeReturnsErrorForUnreachableEndpoint) {
    // Set up with enabled=true but an unreachable endpoint.
    // Port 39093 is an unlikely-to-be-in-use ephemeral port for test isolation.
    AlertmanagerConfig config;
    config.endpoint_url = "http://127.0.0.1:39093";
    config.enabled = true;
    config.timeout_seconds = 1;
    config.retry_count = 0;  // no retries to keep the test fast

    DefaultAlertmanager am(config);

    Alert alert;
    alert.alert_id   = "alert_enabled";
    alert.alert_name = "EnabledTest";
    alert.severity   = AlertSeverity::WARNING;
    alert.status     = AlertStatus::FIRING;
    alert.message    = "Test alert for enabled mode";

    // Should return an error because the endpoint is not reachable
    auto result = am.sendAlert(alert);
    EXPECT_FALSE(result.has_value());
}

TEST_F(GAP008AlertmanagerTest, DefaultRetryConfigValues) {
    AlertmanagerConfig config;
    // Default-constructed config should have sensible retry defaults
    EXPECT_GE(config.retry_count, 0);
    EXPECT_GE(config.retry_delay_ms, 0);
}

}  // namespace test
}  // namespace observability
}  // namespace themis
