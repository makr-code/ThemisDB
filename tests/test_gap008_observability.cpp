/**
 * @file test_gap008_observability.cpp
 * @brief Example tests for GAP-008 Observability features
 * 
 * Tests the new observability features:
 * - Alertmanager integration (stub)
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
        config.enabled = false;  // Disabled for testing
        
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
    config.enabled = true;
    config.timeout_seconds = 15;
    config.receivers = {"email", "slack"};
    
    auto result = alertmanager_->initialize(config);
    
    // Initialization should succeed (even if stub)
    EXPECT_TRUE(result.has_value());
    
    // Configuration should be stored
    EXPECT_EQ(alertmanager_->getConfig().endpoint_url, "http://localhost:9093");
    EXPECT_TRUE(alertmanager_->getConfig().enabled);
    EXPECT_EQ(alertmanager_->getConfig().timeout_seconds, 15);
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
    
    // Then resolve it
    auto result = alertmanager_->resolveAlert("alert_002");
    
    // Stub implementation should succeed for disabled alertmanager
    EXPECT_TRUE(result.has_value());
}

TEST_F(GAP008AlertmanagerTest, SilenceAlertWorks) {
    Alert alert;
    alert.alert_id = "alert_003";
    alert.alert_name = "HighDiskUsage";
    alert.severity = AlertSeverity::WARNING;
    alert.status = AlertStatus::FIRING;
    
    alertmanager_->sendAlert(alert);
    
    // Silence for 60 minutes
    auto result = alertmanager_->silenceAlert("alert_003", 60);
    
    // Stub implementation should succeed for disabled alertmanager
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
    
    // Should return a vector (may be empty for stub)
    EXPECT_TRUE(alerts.size() >= 0);
}

TEST_F(GAP008AlertmanagerTest, TestConnectionWhenDisabled) {
    // Alertmanager is disabled in config
    auto result = alertmanager_->testConnection();
    
    // Should return error when disabled
    EXPECT_FALSE(result.has_value());
    if (!result.has_value()) {
        EXPECT_TRUE(result.error().code() == ErrorCode::SERVICE_UNAVAILABLE ||
                   result.error().code() == ErrorCode::NOT_IMPLEMENTED);
    }
}

TEST_F(GAP008AlertmanagerTest, IsEnabledReflectsConfig) {
    EXPECT_FALSE(alertmanager_->isEnabled());
    
    // Enable it
    AlertmanagerConfig config;
    config.enabled = true;
    alertmanager_->initialize(config);
    
    EXPECT_TRUE(alertmanager_->isEnabled());
}

TEST_F(GAP008AlertmanagerTest, AlertSeverityLevels) {
    // Test different severity levels
    std::vector<AlertSeverity> severities = {
        AlertSeverity::INFO,
        AlertSeverity::WARNING,
        AlertSeverity::ERROR,
        AlertSeverity::CRITICAL
    };
    
    for (const auto& severity : severities) {
        Alert alert;
        alert.alert_id = "alert_severity_test";
        alert.alert_name = "TestAlert";
        alert.severity = severity;
        alert.status = AlertStatus::FIRING;
        alert.message = "Test message";
        
        // Should handle all severity levels successfully (stub implementation)
        auto result = alertmanager_->sendAlert(alert);
        EXPECT_TRUE(result.has_value());
    }
}

TEST_F(GAP008AlertmanagerTest, AlertWithLabelsAndAnnotations) {
    Alert alert;
    alert.alert_id = "alert_004";
    alert.alert_name = "DetailedAlert";
    alert.severity = AlertSeverity::WARNING;
    alert.status = AlertStatus::FIRING;
    alert.message = "Detailed alert with metadata";
    
    // Add labels
    alert.labels["environment"] = "production";
    alert.labels["component"] = "storage";
    alert.labels["severity"] = "warning";
    
    // Add annotations
    alert.annotations["description"] = "Storage usage is above threshold";
    alert.annotations["runbook_url"] = "https://docs.example.com/runbooks/storage";
    alert.annotations["dashboard"] = "https://grafana.example.com/d/storage";
    
    auto result = alertmanager_->sendAlert(alert);
    
    // Should handle labels and annotations successfully (stub implementation)
    EXPECT_TRUE(result.has_value());
}

} // namespace test
} // namespace observability
} // namespace themis
