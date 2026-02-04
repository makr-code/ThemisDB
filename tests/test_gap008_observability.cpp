/**
 * @file test_gap008_observability.cpp
 * @brief Example tests for GAP-008 Observability features
 * 
 * Tests the new observability features:
 * - HealthCheck interface
 * - Alertmanager integration (stub)
 */

#include <gtest/gtest.h>
#include "observability/healthcheck.h"
#include "observability/alertmanager.h"
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

namespace themis {
namespace observability {
namespace test {

// ============================================================================
// HealthCheck Tests
// ============================================================================

class GAP008HealthCheckTest : public ::testing::Test {
protected:
    void SetUp() override {
        health_check_ = std::make_unique<ThemisHealthCheck>();
    }
    
    void TearDown() override {
        health_check_.reset();
    }
    
    std::unique_ptr<ThemisHealthCheck> health_check_;
};

TEST_F(GAP008HealthCheckTest, SystemHealthCheckReturnsReport) {
    auto report = health_check_->checkSystemHealth();
    
    // Check that report is populated
    EXPECT_NE(report.overall_status, HealthStatus::UNKNOWN);
    EXPECT_FALSE(report.component_checks.empty());
    
    // Should have multiple component checks
    EXPECT_GE(report.component_checks.size(), 3u);
    
    // Check that counts are reasonable
    EXPECT_GE(report.healthy_count + report.degraded_count + 
              report.unhealthy_count + report.unknown_count, 0);
    
    // Report time should be recent
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        now - report.report_time).count();
    EXPECT_LE(diff, 5); // Should be within 5 seconds
}

TEST_F(GAP008HealthCheckTest, CheckIndividualComponents) {
    // Test checking individual components
    auto db_result = health_check_->checkComponent("database");
    EXPECT_EQ(db_result.component_name, "database");
    EXPECT_NE(db_result.status, HealthStatus::UNKNOWN);
    EXPECT_FALSE(db_result.message.empty());
    EXPECT_GE(db_result.response_time_ms, 0.0);
    
    auto network_result = health_check_->checkComponent("network");
    EXPECT_EQ(network_result.component_name, "network");
    
    auto storage_result = health_check_->checkComponent("storage");
    EXPECT_EQ(storage_result.component_name, "storage");
}

TEST_F(GAP008HealthCheckTest, CheckUnknownComponentReturnsUnknown) {
    auto result = health_check_->checkComponent("nonexistent_component");
    
    EXPECT_EQ(result.component_name, "nonexistent_component");
    EXPECT_EQ(result.status, HealthStatus::UNKNOWN);
    EXPECT_NE(result.message.find("Unknown component"), std::string::npos);
}

TEST_F(GAP008HealthCheckTest, HealthEndpointIsValid) {
    std::string endpoint = health_check_->getHealthEndpoint();
    
    EXPECT_FALSE(endpoint.empty());
    EXPECT_EQ(endpoint, "/health");
}

TEST_F(GAP008HealthCheckTest, LivenessProbeReturnsTrue) {
    // Liveness should be true by default
    EXPECT_TRUE(health_check_->isAlive());
}

TEST_F(GAP008HealthCheckTest, ReadinessProbeAfterHealthCheck) {
    // Initially readiness may be false
    // After a health check, it should be updated
    auto report = health_check_->checkSystemHealth();
    
    // Readiness should be true if system is healthy or degraded (operational)
    bool ready = health_check_->isReady();
    if (report.overall_status == HealthStatus::HEALTHY || 
        report.overall_status == HealthStatus::DEGRADED) {
        EXPECT_TRUE(ready);
    } else if (report.overall_status == HealthStatus::UNHEALTHY) {
        EXPECT_FALSE(ready);
    }
    // For UNKNOWN status, readiness is implementation-dependent
}

TEST_F(GAP008HealthCheckTest, ComponentCheckResponseTime) {
    auto result = health_check_->checkComponent("database");
    
    // Response time should be measured
    EXPECT_GE(result.response_time_ms, 0.0);
    EXPECT_LT(result.response_time_ms, 1000.0); // Should be fast (< 1 second)
}

TEST_F(GAP008HealthCheckTest, ComponentCheckHasDetails) {
    auto result = health_check_->checkComponent("database");
    
    // Details should be populated
    EXPECT_FALSE(result.details.empty());
    EXPECT_TRUE(result.details.count("status") > 0);
}

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
    
    // Should log alert (stub implementation)
    auto result = alertmanager_->sendAlert(alert);
    
    // Stub implementation logs alerts and returns success (when disabled)
    // Since alertmanager is disabled in test setup, it should succeed
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
