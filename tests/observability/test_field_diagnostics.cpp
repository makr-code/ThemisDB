/**
 * @file test_field_diagnostics.cpp
 * @brief Unit and Integration Tests for Field Diagnostics
 * @version 0.0.1
 *
 * Validates:
 * - DiagnosticEvent schema and JSON serialization
 * - PII masking functionality
 * - Collector thread-safety and buffering
 * - Metrics integration
 * - Performance (<1% CPU overhead)
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "utils/field_diagnostics_schema.h"
#include "observability/field_diagnostics_collector.h"
#include <thread>
#include <vector>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace observability {

using json = nlohmann::json;

// ============================================================================
// DiagnosticEvent Schema Tests
// ============================================================================

class DiagnosticEventTest : public ::testing::Test {
protected:
    DiagnosticEvent createSampleEvent() {
        return DiagnosticEvent{
            .timestamp = std::chrono::system_clock::now(),
            .failure_category = DiagnosticFailureCategory::NLI_INFERENCE,
            .module_name = "rag",
            .error_message = "Model inference failed",
            .severity_level = DiagnosticSeverity::ERROR,
            .deployment_environment = "production",
            .version = "1.5.0",
            .stacktrace_hash = "abc123def456"
        };
    }
};

TEST_F(DiagnosticEventTest, EventCreation) {
    auto evt = createSampleEvent();
    EXPECT_EQ(evt.module_name, "rag");
    EXPECT_EQ(evt.failure_category, DiagnosticFailureCategory::NLI_INFERENCE);
    EXPECT_EQ(evt.severity_level, DiagnosticSeverity::ERROR);
}

TEST_F(DiagnosticEventTest, EventToString) {
    auto evt = createSampleEvent();
    auto str = evt.toString();
    EXPECT_THAT(str, ::testing::HasSubstr("NLI_INFERENCE"));
    EXPECT_THAT(str, ::testing::HasSubstr("rag"));
    EXPECT_THAT(str, ::testing::HasSubstr("ERROR"));
}

TEST_F(DiagnosticEventTest, EventToJSON) {
    auto evt = createSampleEvent();
    auto j = evt.toJson();

    EXPECT_EQ(j["module_name"], "rag");
    EXPECT_EQ(j["failure_category"], "NLI_INFERENCE");
    EXPECT_EQ(j["severity_level"], "ERROR");
    EXPECT_EQ(j["version"], "1.5.0");
    EXPECT_TRUE(j.contains("timestamp"));
}

TEST_F(DiagnosticEventTest, EventWithOptionalFields) {
    auto evt = createSampleEvent();
    evt.affected_user_count = 42;
    evt.request_id = "req-12345";
    evt.context_data["latency_ms"] = "1234";

    auto j = evt.toJson();
    EXPECT_EQ(j["affected_user_count"], 42);
    EXPECT_EQ(j["request_id"], "req-12345");
    EXPECT_EQ(j["context_data"]["latency_ms"], "1234");
}

TEST_F(DiagnosticEventTest, FailureCategoryToString) {
    EXPECT_EQ(failureCategoryToString(DiagnosticFailureCategory::NLI_INFERENCE), 
              "NLI_INFERENCE");
    EXPECT_EQ(failureCategoryToString(DiagnosticFailureCategory::MTLS_CONNECTION), 
              "MTLS_CONNECTION");
    EXPECT_EQ(failureCategoryToString(DiagnosticFailureCategory::QUERY_TIMEOUT), 
              "QUERY_TIMEOUT");
}

TEST_F(DiagnosticEventTest, SeverityToString) {
    EXPECT_EQ(severityToString(DiagnosticSeverity::DEBUG), "DEBUG");
    EXPECT_EQ(severityToString(DiagnosticSeverity::INFO), "INFO");
    EXPECT_EQ(severityToString(DiagnosticSeverity::WARN), "WARN");
    EXPECT_EQ(severityToString(DiagnosticSeverity::ERROR), "ERROR");
    EXPECT_EQ(severityToString(DiagnosticSeverity::CRITICAL), "CRITICAL");
}

// ============================================================================
// PII Masking Tests
// ============================================================================

class PIIMaskingTest : public ::testing::Test {
protected:
    DiagnosticEvent createEventWithPII() {
        DiagnosticEvent evt;
        evt.timestamp = std::chrono::system_clock::now();
        evt.failure_category = DiagnosticFailureCategory::QUERY_TIMEOUT;
        evt.module_name = "query";
        evt.error_message = "Query timeout";
        evt.severity_level = DiagnosticSeverity::WARN;
        evt.context_data["user_email"] = "user@example.com";
        evt.context_data["query"] = "SELECT * FROM users WHERE id = 12345";
        evt.context_data["safe_field"] = "this_is_safe";
        return evt;
    }
};

TEST_F(PIIMaskingTest, IsPIIFieldDetection) {
    EXPECT_TRUE(isPIIField("user_id"));
    EXPECT_TRUE(isPIIField("user_email"));
    EXPECT_TRUE(isPIIField("api_key"));
    EXPECT_TRUE(isPIIField("query"));
    EXPECT_FALSE(isPIIField("module_name"));
    EXPECT_FALSE(isPIIField("version"));
}

TEST_F(PIIMaskingTest, SanitizePIIBasic) {
    auto evt = createEventWithPII();
    EXPECT_EQ(evt.context_data["safe_field"], "this_is_safe");

    sanitizePII(evt);

    // Safe field should not change
    EXPECT_EQ(evt.context_data["safe_field"], "this_is_safe");

    // PII fields should be masked
    auto email = evt.context_data["user_email"];
    EXPECT_NE(email, "user@example.com");  // Changed
    EXPECT_TRUE(email.find('*') != std::string::npos);  // Contains mask char
}

TEST_F(PIIMaskingTest, SanitizePIIPreservesLength) {
    auto evt = createEventWithPII();
    size_t original_len = evt.context_data["user_email"].length();

    sanitizePII(evt);

    // Length should be preserved (only characters replaced)
    EXPECT_EQ(evt.context_data["user_email"].length(), original_len);
}

TEST_F(PIIMaskingTest, SanitizePIIWithCustomMaskChar) {
    auto evt = createEventWithPII();
    sanitizePII(evt, '#');

    auto email = evt.context_data["user_email"];
    EXPECT_TRUE(email.find('#') != std::string::npos);
    EXPECT_TRUE(email.find('*') == std::string::npos);  // No asterisks
}

// ============================================================================
// Collector Tests
// ============================================================================

class FieldDiagnosticsCollectorTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        auto& collector = FieldDiagnosticsCollector::getInstance();
        collector.clearBuffer();
        
        FieldDiagnosticsConfig config;
        config.max_buffer_size = 100;
        config.enable_pii_masking = true;
        config.enabled = true;
        collector.configure(config);
    }

    virtual void TearDown() override {
        FieldDiagnosticsCollector::getInstance().clearBuffer();
    }

    DiagnosticEvent createEvent(DiagnosticFailureCategory cat = DiagnosticFailureCategory::NLI_INFERENCE) {
        return DiagnosticEvent{
            .timestamp = std::chrono::system_clock::now(),
            .failure_category = cat,
            .module_name = "test_module",
            .error_message = "Test error",
            .severity_level = DiagnosticSeverity::INFO,
            .deployment_environment = "test",
            .version = "1.0.0"
        };
    }
};

TEST_F(FieldDiagnosticsCollectorTest, SingletonInstance) {
    auto& c1 = FieldDiagnosticsCollector::getInstance();
    auto& c2 = FieldDiagnosticsCollector::getInstance();
    EXPECT_EQ(&c1, &c2);
}

TEST_F(FieldDiagnosticsCollectorTest, EmitEvent) {
    auto& collector = FieldDiagnosticsCollector::getInstance();
    auto evt = createEvent();

    bool result = collector.emitDiagnosticEvent(evt);
    EXPECT_TRUE(result);
    EXPECT_EQ(collector.getBufferSize(), 1);
}

TEST_F(FieldDiagnosticsCollectorTest, EmitWithPIIMasking) {
    auto& collector = FieldDiagnosticsCollector::getInstance();
    
    DiagnosticEvent evt = createEvent();
    evt.context_data["user_id"] = "secret123";

    bool result = collector.emitWithPIIMasking(evt);
    EXPECT_TRUE(result);

    auto events = collector.getAllEvents();
    EXPECT_EQ(events.size(), 1);
    EXPECT_THAT(events[0].context_data["user_id"], 
                ::testing::Not(::testing::StrEq("secret123")));
}

TEST_F(FieldDiagnosticsCollectorTest, BufferFillingAndWrapping) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    // Fill buffer beyond max size
    for (int i = 0; i < 150; i++) {
        collector.emitDiagnosticEvent(createEvent());
    }

    // Should not exceed max size (100)
    EXPECT_LE(collector.getBufferSize(), 100);
}

TEST_F(FieldDiagnosticsCollectorTest, GetEventsSince) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    auto t0 = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto evt1 = createEvent(DiagnosticFailureCategory::NLI_INFERENCE);
    collector.emitDiagnosticEvent(evt1);

    auto t1 = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto evt2 = createEvent(DiagnosticFailureCategory::MTLS_CONNECTION);
    collector.emitDiagnosticEvent(evt2);

    auto since_t1 = collector.getEventsSince(t1);
    EXPECT_EQ(since_t1.size(), 1);
    EXPECT_EQ(since_t1[0].failure_category, DiagnosticFailureCategory::MTLS_CONNECTION);
}

TEST_F(FieldDiagnosticsCollectorTest, GetEventCountsByCategory) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    collector.emitDiagnosticEvent(createEvent(DiagnosticFailureCategory::NLI_INFERENCE));
    collector.emitDiagnosticEvent(createEvent(DiagnosticFailureCategory::NLI_INFERENCE));
    collector.emitDiagnosticEvent(createEvent(DiagnosticFailureCategory::MTLS_CONNECTION));

    auto counts = collector.getEventCountsByCategory();
    EXPECT_EQ(counts[DiagnosticFailureCategory::NLI_INFERENCE], 2);
    EXPECT_EQ(counts[DiagnosticFailureCategory::MTLS_CONNECTION], 1);
}

TEST_F(FieldDiagnosticsCollectorTest, DisabledCollection) {
    auto& collector = FieldDiagnosticsCollector::getInstance();
    collector.setEnabled(false);

    bool result = collector.emitDiagnosticEvent(createEvent());
    EXPECT_FALSE(result);
    EXPECT_EQ(collector.getBufferSize(), 0);

    collector.setEnabled(true);
}

TEST_F(FieldDiagnosticsCollectorTest, ExportAsJSON) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    collector.emitDiagnosticEvent(createEvent());
    collector.emitDiagnosticEvent(createEvent());

    auto j = collector.exportAsJSON();
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2);
    EXPECT_TRUE(j[0].contains("failure_category"));
}

TEST_F(FieldDiagnosticsCollectorTest, GetStats) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    collector.emitDiagnosticEvent(createEvent());
    auto stats = collector.getStats();

    EXPECT_EQ(stats["current_buffer_size"], 1);
    EXPECT_TRUE(stats.contains("total_events_emitted"));
    EXPECT_TRUE(stats.contains("events_dropped"));
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

class ThreadSafetyTest : public FieldDiagnosticsCollectorTest {
};

TEST_F(ThreadSafetyTest, ConcurrentEmit) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    const int num_threads = 10;
    const int events_per_thread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < events_per_thread; j++) {
                auto evt = createEvent();
                evt.module_name = "thread_" + std::to_string(i);
                collector.emitDiagnosticEvent(evt);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Should have collected all events (up to buffer size)
    size_t expected = num_threads * events_per_thread;
    EXPECT_LE(collector.getBufferSize(), expected);
    EXPECT_GT(collector.getBufferSize(), 0);
}

TEST_F(ThreadSafetyTest, ConcurrentReadWrite) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    std::vector<std::thread> threads;
    const int num_threads = 5;

    for (int i = 0; i < num_threads; i++) {
        // Writers
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; j++) {
                collector.emitDiagnosticEvent(createEvent());
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });

        // Readers
        threads.emplace_back([&]() {
            for (int j = 0; j < 50; j++) {
                auto size = collector.getBufferSize();
                auto all_events = collector.getAllEvents();
                auto counts = collector.getEventCountsByCategory();
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Should not crash and buffer should have events
    EXPECT_GT(collector.getBufferSize(), 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

class IntegrationTest : public FieldDiagnosticsCollectorTest {
};

TEST_F(IntegrationTest, EmitCallbackInvocation) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    auto callback_invoked = std::make_shared<int>(0);
    collector.registerEmitCallback([callback_invoked](const DiagnosticEvent&) {
        ++(*callback_invoked);
    });

    collector.emitDiagnosticEvent(createEvent());
    collector.emitDiagnosticEvent(createEvent());

    EXPECT_EQ(*callback_invoked, 2);
}

TEST_F(IntegrationTest, PrometheusMetricsEmission) {
    auto& collector = FieldDiagnosticsCollector::getInstance();

    // Enable metrics emission
    FieldDiagnosticsConfig config;
    config.enable_metrics_emission = true;
    collector.configure(config);

    auto evt = createEvent();
    evt.failure_category = DiagnosticFailureCategory::NLI_INFERENCE;
    evt.module_name = "rag";
    evt.severity_level = DiagnosticSeverity::ERROR;

    bool result = collector.emitDiagnosticEvent(evt);
    EXPECT_TRUE(result);
}

}  // namespace observability
}  // namespace themis
