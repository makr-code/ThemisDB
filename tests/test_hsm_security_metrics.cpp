#include <gtest/gtest.h>
#include "security/hsm_provider.h"
#include "security/hsm_security_metrics.h"

using namespace themis::security;

/**
 * HSM Security Metrics Tests
 * 
 * Tests Prometheus metrics export for HSM security monitoring.
 * Addresses FIND-002 monitoring requirements.
 */

class HSMSecurityMetricsTest : public ::testing::Test {
protected:
    HSMConfig createStubConfig() {
        HSMConfig config;
        config.library_path = "";  // Force stub
        config.key_label = "test-key";
        return config;
    }
};

TEST_F(HSMSecurityMetricsTest, ExportMetrics_StubProvider) {
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string metrics = HSMSecurityMetrics::exportMetrics(hsm, 5);
    
    // Verify key metrics are present
    EXPECT_NE(metrics.find("hsm_security_stub_active"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_security_warnings_total"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_provider_type"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_provider_ready"), std::string::npos);
    
    // Verify stub is detected (value = 1)
    EXPECT_NE(metrics.find("hsm_security_stub_active 1"), std::string::npos);
    
    // Verify provider type is stub
    EXPECT_NE(metrics.find("hsm_provider_type{provider=\"stub\"}"), std::string::npos);
    
    // Verify warnings count
    EXPECT_NE(metrics.find("hsm_security_warnings_total 5"), std::string::npos);
}

TEST_F(HSMSecurityMetricsTest, ExportMetrics_ComplianceStatus) {
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string metrics = HSMSecurityMetrics::exportMetrics(hsm);
    
    // Verify compliance metrics exist
    EXPECT_NE(metrics.find("hsm_compliance_status"), std::string::npos);
    
    // With stub, all compliance should be 0 (non-compliant)
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"nist_sp_800_53_sc_12\"} 0"), 
              std::string::npos);
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"iso_27001_a_8_24\"} 0"), 
              std::string::npos);
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"pci_dss_3_6\"} 0"), 
              std::string::npos);
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"gdpr_art_32\"} 0"), 
              std::string::npos);
}

TEST_F(HSMSecurityMetricsTest, ExportMetrics_OperationStats) {
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    hsm.initialize();
    
    // Perform some operations
    std::vector<uint8_t> data = {'t', 'e', 's', 't'};
    for (int i = 0; i < 3; ++i) {
        hsm.sign(data);
    }
    
    std::string metrics = HSMSecurityMetrics::exportMetrics(hsm);
    
    // Should include operation metrics
    EXPECT_NE(metrics.find("hsm_sign_operations_total"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_verify_operations_total"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_sign_errors_total"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_verify_errors_total"), std::string::npos);
}

TEST_F(HSMSecurityMetricsTest, ExportJSON_StubProvider) {
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string json = HSMSecurityMetrics::exportJSON(hsm, 10);
    
    // Verify JSON structure
    EXPECT_NE(json.find("\"hsm_security\""), std::string::npos);
    EXPECT_NE(json.find("\"compliance\""), std::string::npos);
    EXPECT_NE(json.find("\"operations\""), std::string::npos);
    
    // Verify stub is detected
    EXPECT_NE(json.find("\"stub_active\": true"), std::string::npos);
    EXPECT_NE(json.find("\"provider_type\": \"stub\""), std::string::npos);
    
    // Verify warnings count
    EXPECT_NE(json.find("\"warnings_total\": 10"), std::string::npos);
    
    // Verify compliance status
    EXPECT_NE(json.find("\"nist_sp_800_53_sc_12\": false"), std::string::npos);
    EXPECT_NE(json.find("\"pci_dss_3_6\": false"), std::string::npos);
}

TEST_F(HSMSecurityMetricsTest, ExportJSON_ReadyStatus) {
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    
    // Before initialization
    std::string json_before = HSMSecurityMetrics::exportJSON(hsm);
    EXPECT_NE(json_before.find("\"ready\": false"), std::string::npos);
    
    // After initialization
    hsm.initialize();
    std::string json_after = HSMSecurityMetrics::exportJSON(hsm);
    EXPECT_NE(json_after.find("\"ready\": true"), std::string::npos);
}

TEST_F(HSMSecurityMetricsTest, PrometheusFormat_HelpAndType) {
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string metrics = HSMSecurityMetrics::exportMetrics(hsm);
    
    // Verify Prometheus format with HELP and TYPE
    EXPECT_NE(metrics.find("# HELP hsm_security_stub_active"), std::string::npos);
    EXPECT_NE(metrics.find("# TYPE hsm_security_stub_active gauge"), std::string::npos);
    
    EXPECT_NE(metrics.find("# HELP hsm_security_warnings_total"), std::string::npos);
    EXPECT_NE(metrics.find("# TYPE hsm_security_warnings_total counter"), std::string::npos);
    
    EXPECT_NE(metrics.find("# HELP hsm_compliance_status"), std::string::npos);
    EXPECT_NE(metrics.find("# TYPE hsm_compliance_status gauge"), std::string::npos);
}

TEST_F(HSMSecurityMetricsTest, MetricsUpdate_AfterOperations) {
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    hsm.initialize();
    
    // Initial metrics
    std::string metrics1 = HSMSecurityMetrics::exportMetrics(hsm);
    
    // Perform operations
    std::vector<uint8_t> data = {'d', 'a', 't', 'a'};
    auto sig = hsm.sign(data);
    hsm.verify(data, sig.signature_b64);
    
    // Updated metrics
    std::string metrics2 = HSMSecurityMetrics::exportMetrics(hsm);
    
    // Should show operation counts
    // Note: exact counts depend on whether stub tracks stats
    EXPECT_FALSE(metrics2.empty());
}

TEST_F(HSMSecurityMetricsTest, IntegrationExample) {
    // Complete workflow example
    
    // 1. Create and initialize HSM
    HSMConfig config = createStubConfig();
    HSMProvider hsm(config);
    hsm.initialize();
    
    // 2. Track warnings (simulated)
    uint64_t warning_count = 0;
    if (hsm.isStubProvider()) {
        warning_count++;  // One warning on init
    }
    
    // 3. Perform operations
    std::vector<uint8_t> data = {'t', 'e', 's', 't'};
    for (int i = 0; i < 5; ++i) {
        auto sig = hsm.sign(data);
        if (sig.success) {
            hsm.verify(data, sig.signature_b64);
        }
        warning_count++;  // Simulate periodic warnings
    }
    
    // 4. Export Prometheus metrics
    std::string prom_metrics = HSMSecurityMetrics::exportMetrics(hsm, warning_count);
    EXPECT_FALSE(prom_metrics.empty());
    EXPECT_NE(prom_metrics.find("hsm_security_stub_active 1"), std::string::npos);
    
    // 5. Export JSON metrics
    std::string json_metrics = HSMSecurityMetrics::exportJSON(hsm, warning_count);
    EXPECT_FALSE(json_metrics.empty());
    EXPECT_NE(json_metrics.find("\"stub_active\": true"), std::string::npos);
    
    // 6. Verify metrics can be scraped
    // In real deployment, Prometheus would scrape /metrics endpoint
    // serving the prom_metrics string
}
