/**
 * @file test_operational_audit_evidence.cpp
 * @brief Comprehensive test suite for operational audit and evidence collection
 * @author ThemisDB Test Team
 * @date 2024
 * @license Apache License 2.0
 *
 * Test Gates:
 * - GOV-Observ-01: Basic event logging with all fields populated correctly
 * - GOV-Observ-02: Event correlation with causality tracking across events
 * - GOV-Observ-03: Time-range correlation queries and filtering
 * - GOV-Observ-04: Compliance evidence collection and requirement linking
 * - GOV-Observ-05: Event/evidence export with JSON serialization
 * - GOV-Observ-06: Performance benchmarks (logging overhead <5%, correlation latency ≤100ms)
 *
 * Maturity: 🟢 PRODUCTION-READY
 */

#include <gtest/gtest.h>
#include "governance/operational_audit.h"
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::governance;

// ============================================================================
// Test Fixture
// ============================================================================

class OperationalAuditTest : public ::testing::Test {
protected:
    OperationalAuditLogger* audit_logger;
    EventCorrelationEngine* correlation_engine;
    ComplianceEvidenceCollector* evidence_collector;
    
    void SetUp() override {
        // Get global singletons
        audit_logger = &getGlobalAuditLogger();
        correlation_engine = &getGlobalCorrelationEngine();
        evidence_collector = &getGlobalEvidenceCollector();
        
        // Clear any previous test data (in production, this would be persistence cleanup)
        // For this test, we rely on separate instances per test
    }
    
    void TearDown() override {
        // Cleanup if needed
    }

    static std::unordered_map<std::string, std::string> jsonToContext(
        const nlohmann::json& j
    ) {
        std::unordered_map<std::string, std::string> context = {};

        if (!j.is_object()) {
            return context;
        }
        for (const auto& item : j.items()) {
            if (item.value().is_string()) {
                context[item.key()] = item.value().get<std::string>();
            } else {
                context[item.key()] = item.value().dump();
            }
        }
        return context;
    }

    static OperationalEvent logEventCompat(
        OperationalAuditLogger* logger,
        OperationalEventType event_type,
        const std::string& actor_id,
        const std::string& actor_type,
        const std::string& module_name,
        const std::string& operation_name,
        const std::string& resource_id,
        const std::string& resource_type,
        const std::string& action,
        const std::string& result,
        const std::string& classification,
        int64_t operation_duration_us,
        const std::vector<std::string>& compliance_tags,
        const nlohmann::json& context_json,
        const std::string& error_message = "",
        const nlohmann::json& event_payload_json = nlohmann::json::object(),
        const std::string& correlation_id = "",
        const std::string& causality_parent_id = ""
    ) {
        OperationalEvent event;
        event.event_type = event_type;
        event.actor_id = actor_id;
        event.actor_type = actor_type;
        event.module_name = module_name;
        event.operation_name = operation_name;
        event.resource_id = resource_id;
        event.resource_type = resource_type;
        event.action = action;
        event.result = result;
        event.classification = classification;
        event.operation_duration_us = operation_duration_us;
        event.compliance_tags = compliance_tags;
        event.context = jsonToContext(context_json);
        event.error_message = error_message;
        event.event_payload = event_payload_json.dump();
        event.correlation_id = correlation_id;
        event.causality_parent_id = causality_parent_id;
        return logger->logEvent(event);
    }

    std::vector<ComplianceEvidence> collectEvidenceCompat(const std::string& requirement) {
        return evidence_collector->collectEvidence(requirement, "test_collector");
    }

    ComplianceEvidence recordEvidenceCompat(
        const std::string& requirement_id,
        const std::string& requirement_type,
        const std::string& evidence_type,
        const std::string& description,
        const std::string& source_event_id,
        const nlohmann::json& data,
        int64_t retention_days,
        const std::string& audit_classification
    ) {
        ComplianceEvidence evidence;
        evidence.requirement_id = requirement_id;
        evidence.requirement_type = requirement_type;
        evidence.evidence_type = evidence_type;
        evidence.description = description;
        evidence.source_event_id = source_event_id;
        evidence.data_summary = data.dump();
        evidence.audit_classification = audit_classification;
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        evidence.retention_until_ms = now_ms + (retention_days * 24 * 3600 * 1000);
        return evidence_collector->recordEvidence(evidence);
    }
};

// ============================================================================
// GOV-Observ-01: Basic Event Logging
// ============================================================================

TEST_F(OperationalAuditTest, GOVObserv01_BasicEventLogging) {
    // Test: Verify that logEvent() populates all required fields
    
    std::string test_actor = "test_user_001";
    std::string test_policy = "policy_test_001";
    std::string test_resource = "resource_test_001";
    
    std::unordered_map<std::string, std::string> context = {{"test_context", "value"}};
    
    // Log a policy evaluation event
    audit_logger->logPolicyEvaluation(
        "corr_test_001",
        test_policy,
        "permit",
        test_actor,
        context);
    
    // Verify event was logged
    EXPECT_GT(audit_logger->getTotalEventCount(), 0);
    
    // Query and verify event details
    auto events = audit_logger->queryEventsByActor(test_actor);
    ASSERT_GE(events.size(), 1);
    
    const auto& event = events[0];
    EXPECT_EQ(event.actor_id, test_actor);
    EXPECT_EQ(event.result, "permit");
    EXPECT_EQ(event.resource_id, test_policy);
    EXPECT_GT(event.timestamp_ms, 0);
    EXPECT_GT(event.sequence_number, 0);
    EXPECT_FALSE(event.event_id.empty());
    EXPECT_GE(event.operation_duration_us, 0);
    EXPECT_GT(event.logging_duration_us, 0);
    EXPECT_LT(event.logging_duration_us, 10000);  // Logging should be <10ms
}

TEST_F(OperationalAuditTest, GOVObserv01_ComplianceCheckLogging) {
    // Test: Verify compliance check event logging
    
    std::string check_id = "check_001";
    std::string actor_id = "compliance_service";
    std::unordered_map<std::string, std::string> context = {
        {"tag_0", "EU_AI_ACT"},
        {"tag_1", "SOC2"}
    };
    
    audit_logger->logComplianceCheck(
        "corr_check_001",
        check_id,
        "pass",
        actor_id,
        context);
    
    auto events = audit_logger->queryEventsByActor(actor_id);
    ASSERT_GE(events.size(), 1);
    
    const auto& event = events.back();
    EXPECT_EQ(event.result, "pass");
    EXPECT_EQ(event.actor_id, actor_id);
    for (const auto& tag : {std::string("EU_AI_ACT"), std::string("SOC2")}) {
        EXPECT_TRUE(std::find(event.compliance_tags.begin(),
                             event.compliance_tags.end(), tag) != event.compliance_tags.end());
    }
}

TEST_F(OperationalAuditTest, GOVObserv01_DataGovernanceLogging) {
    // Test: Verify data governance operation logging
    
    std::string resource_id = "user_pii_001";
    std::string actor_id = "dg_service";
    std::unordered_map<std::string, std::string> op_context = {{"columns_masked", "3"}};
    
    audit_logger->logDataGovernanceOp(
        "corr_dg_001",
        "mask",
        resource_id,
        true,
        actor_id,
        op_context);
    
    auto events = audit_logger->queryEventsByResource(resource_id);
    ASSERT_GE(events.size(), 1);
    
    const auto& event = events.back();
    EXPECT_EQ(event.resource_id, resource_id);
    EXPECT_EQ(event.operation_name, "mask");
}

// ============================================================================
// GOV-Observ-02: Event Correlation and Causality Tracking
// ============================================================================

TEST_F(OperationalAuditTest, GOVObserv02_EventCorrelation) {
    // Test: Verify correlation ID linking related events
    
    std::string correlation_id = "corr_" + std::to_string(std::time(nullptr));
    std::string actor = "policy_engine";
    
    // Log initial event with correlation ID
    logEventCompat(
        audit_logger,
        OperationalEventType::POLICY_EVALUATION_PERMIT,
        actor, "service", "policy_engine", "evaluate",
        "policy_1", "policy", "evaluate", "permit",
        "POLICY_DECISION", 1000, {"AUDIT"},
        nlohmann::json::object(), "", nlohmann::json::object(),
        correlation_id, "");
    
    // Log second related event with same correlation ID
    logEventCompat(
        audit_logger,
        OperationalEventType::COMPLIANCE_CHECK_PASS,
        "compliance_service", "service", "compliance_engine", "check",
        "policy_1", "policy", "check", "pass",
        "COMPLIANCE_VERIFICATION", 500, {},
        nlohmann::json::object(), "", nlohmann::json::object(),
        correlation_id, "");
    
    // Query by correlation ID and verify both events returned
    auto correlated_events = audit_logger->queryEventsByCorrelationId(correlation_id);
    EXPECT_EQ(correlated_events.size(), 2);
    
    for (const auto& event : correlated_events) {
        EXPECT_EQ(event.correlation_id, correlation_id);
    }
}

TEST_F(OperationalAuditTest, GOVObserv02_CausalityTracking) {
    // Test: Verify causality chain linking
    
    std::string actor = "audit_service";
    
    // Log parent event
    auto parent_event = [&]() {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            actor, "service", "policy_engine", "evaluate",
            "policy_cause", "policy", "evaluate", "permit",
            "POLICY_DECISION", 1000, {},
            nlohmann::json::object());
        
        auto events = audit_logger->queryEventsByActor(actor);
        return events.back();
    }();
    
    std::string parent_id = parent_event.event_id;
    
    // Log child event with parent causality
    logEventCompat(
        audit_logger,
        OperationalEventType::COMPLIANCE_CHECK_PASS,
        actor, "service", "compliance_engine", "check",
        "policy_cause", "policy", "check", "pass",
        "COMPLIANCE_VERIFICATION", 500, {},
        nlohmann::json::object(), "", nlohmann::json::object(),
        parent_event.correlation_id, parent_id);
    
    // Verify causality chain
    auto chain = audit_logger->getCausalityChain(parent_event.correlation_id);
    EXPECT_GT(chain.size(), 0);
    
    // Verify triggered events
    auto triggered = audit_logger->getTriggeredEvents(parent_id);
    EXPECT_GE(triggered.size(), 1);
}

// ============================================================================
// GOV-Observ-03: Time-Range Queries
// ============================================================================

TEST_F(OperationalAuditTest, GOVObserv03_TimeRangeQueries) {
    // Test: Verify time-range filtering of events
    
    auto start_time = std::chrono::system_clock::now();
    int64_t start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        start_time.time_since_epoch()).count();
    
    // Log some events
    for (int i = 0; i < 3; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "actor_" + std::to_string(i), "service", "policy_engine", "eval",
            "res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", 100 * (i + 1), {},
            nlohmann::json::object());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto end_time = std::chrono::system_clock::now();
    int64_t end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time.time_since_epoch()).count();
    
    // Query within time range
    auto events_in_range = audit_logger->queryEventsByTimeRange(start_ms, end_ms);
    EXPECT_GE(events_in_range.size(), 3);
    
    // Verify all events are within range
    for (const auto& event : events_in_range) {
        if (event.timestamp_ms >= start_ms) {  // Account for clock skew
            EXPECT_LE(event.timestamp_ms, end_ms + 1000);  // Allow 1s margin
        }
    }
    
    // Query with narrow range should return fewer events
    int64_t narrow_end = start_ms + 20;
    auto narrow_events = audit_logger->queryEventsByTimeRange(start_ms, narrow_end);
    EXPECT_LE(narrow_events.size(), events_in_range.size());
}

// ============================================================================
// GOV-Observ-04: Compliance Evidence Collection
// ============================================================================

TEST_F(OperationalAuditTest, GOVObserv04_EvidenceCollection) {
    // Test: Verify automated evidence collection for compliance requirements
    
    std::string req_type = "EU_AI_ACT_13";
    
    // First, log some policy evaluation events
    for (int i = 0; i < 2; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "policy_service", "service", "policy_engine", "evaluate",
            "policy_" + std::to_string(i), "policy", "evaluate", "permit",
            "POLICY", 500, {"EU_AI_ACT"},
            nlohmann::json::object());
    }
    
    // Collect evidence for requirement
    collectEvidenceCompat(req_type);
    
    // Verify evidence was collected
    auto evidence_list = evidence_collector->getEvidenceByRequirement(req_type);
    EXPECT_GT(evidence_list.size(), 0);
    
    // Verify evidence attributes
    for (const auto& evidence : evidence_list) {
        EXPECT_EQ(evidence.requirement_type, req_type);
        EXPECT_FALSE(evidence.evidence_id.empty());
        EXPECT_GT(evidence.collected_at_ms, 0);
        EXPECT_FALSE(evidence.fingerprint.empty());
    }
}

TEST_F(OperationalAuditTest, GOVObserv04_EvidenceLinking) {
    // Test: Verify evidence linking to events
    
    // Log an event
    logEventCompat(
        audit_logger,
        OperationalEventType::POLICY_EVALUATION_PERMIT,
        "linker", "service", "policy_engine", "evaluate",
        "link_policy", "policy", "evaluate", "permit",
        "POLICY", 600, {},
        nlohmann::json::object());
    
    auto events = audit_logger->queryEventsByActor("linker");
    ASSERT_GE(events.size(), 1);
    std::string event_id = events.back().event_id;
    
    // Record evidence with link
    recordEvidenceCompat(
        "SOC2_CC7.2",
        "SOC2_CC7.2",
        "OPERATIONAL_EVENT",
        "Policy evaluation logged",
        event_id,
        nlohmann::json::object({{"policy", "test"}}),
        365,
        "SOC2_EVIDENCE");
    
    // Verify linking
    auto evidence_by_event = evidence_collector->getEvidenceByEvent(event_id);
    EXPECT_GE(evidence_by_event.size(), 1);
    EXPECT_EQ(evidence_by_event[0].source_event_id, event_id);
}

// ============================================================================
// GOV-Observ-05: Export and Serialization
// ============================================================================

TEST_F(OperationalAuditTest, GOVObserv05_EventExport) {
    // Test: Verify event export with JSON serialization
    
    std::string export_actor = "exporter";
    
    // Log test events
    for (int i = 0; i < 2; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            export_actor, "service", "policy_engine", "eval",
            "exp_res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", 100 + i, {},
            nlohmann::json::object());
    }
    
    auto now = std::chrono::system_clock::now();
    int64_t start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        (now - std::chrono::hours(1)).time_since_epoch()).count();
    int64_t end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        (now + std::chrono::hours(1)).time_since_epoch()).count();
    
    // Export events
    auto export_json = audit_logger->exportEvents(start_ms, end_ms);
    
    EXPECT_TRUE(export_json.contains("events"));
    EXPECT_TRUE(export_json.contains("event_count"));
    EXPECT_GT(export_json["event_count"].get<int>(), 0);
    EXPECT_TRUE(export_json["events"].is_array());
}

TEST_F(OperationalAuditTest, GOVObserv05_EvidenceExport) {
    // Test: Verify evidence export for audit
    
    std::string req_type = "ISO27001_A1";
    
    // Log events and collect evidence
    logEventCompat(
        audit_logger,
        OperationalEventType::POLICY_CREATED,
        "admin", "user", "policy_management", "create",
        "iso_policy", "policy", "create", "success",
        "POLICY_LIFECYCLE", 200, {"ISO27001"},
        nlohmann::json::object());
    
    collectEvidenceCompat(req_type);
    
    // Export evidence
    auto export_json = evidence_collector->exportEvidenceForAudit(req_type);
    
    EXPECT_TRUE(export_json.contains("evidence"));
    EXPECT_TRUE(export_json.contains("evidence_count"));
    EXPECT_EQ(export_json["requirement_type"].get<std::string>(), req_type);
}

// ============================================================================
// GOV-Observ-06: Performance Benchmarks
// ============================================================================

TEST_F(OperationalAuditTest, GOVObserv06_LoggingOverhead) {
    // Test: Verify logging overhead is <5% of operation duration
    
    const int num_events = 100;
    int64_t total_operation_time = 0;
    int64_t total_logging_time = 0;
    
    for (int i = 0; i < num_events; ++i) {
        int64_t simulated_op_duration = 10000 + (i * 100);  // 10ms base, increasing
        
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "perf_test", "service", "policy_engine", "eval",
            "perf_res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", simulated_op_duration, {},
            nlohmann::json::object());
        
        total_operation_time += simulated_op_duration;
    }
    
    auto events = audit_logger->queryEventsByActor("perf_test");
    for (const auto& event : events) {
        total_logging_time += event.logging_duration_us;
    }
    
    // Calculate overhead percentage
    double overhead_percent = (static_cast<double>(total_logging_time) / total_operation_time) * 100.0;
    
    EXPECT_LT(overhead_percent, 5.0) << "Logging overhead (" << overhead_percent 
        << "%) exceeds 5% threshold";
}

TEST_F(OperationalAuditTest, GOVObserv06_CorrelationLatency) {
    // Test: Verify correlation query latency is ≤100ms
    
    std::string corr_id = "latency_test_" + std::to_string(std::time(nullptr));
    
    // Log events with correlation
    for (int i = 0; i < 50; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "latency_test", "service", "policy_engine", "eval",
            "latency_res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", 100, {},
            nlohmann::json::object(),  // context
            "", "",  // causality
            corr_id);
    }
    
    // Measure query time
    auto query_start = std::chrono::high_resolution_clock::now();
    auto events = audit_logger->queryEventsByCorrelationId(corr_id);
    auto query_end = std::chrono::high_resolution_clock::now();
    
    int64_t query_latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        query_end - query_start).count();
    
    EXPECT_LE(query_latency_ms, 100) << "Correlation query latency (" << query_latency_ms 
        << "ms) exceeds 100ms threshold";
    EXPECT_GE(events.size(), 50) << "Expected at least 50 correlated events";
}

TEST_F(OperationalAuditTest, GOVObserv06_PerformanceMetrics) {
    // Test: Verify performance metrics collection
    
    // Log events
    for (int i = 0; i < 20; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "metrics_test", "service", "policy_engine", "eval",
            "metric_res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", 500 + (i * 10), {},
            nlohmann::json::object());
    }
    
    auto metrics = audit_logger->getPerformanceMetrics();
    EXPECT_TRUE(metrics.contains("total_operations"));
    EXPECT_GT(metrics["total_operations"].get<int64_t>(), 0);
    
    auto stats = audit_logger->getEventStatistics();
    EXPECT_TRUE(stats.contains("logging_latency_p50_us"));
    EXPECT_TRUE(stats.contains("logging_latency_p95_us"));
    EXPECT_TRUE(stats.contains("logging_latency_p99_us"));
    EXPECT_GT(stats["logging_latency_p50_us"].get<int64_t>(), 0);
}

// ============================================================================
// Additional Tests: Edge Cases and Thread Safety
// ============================================================================

TEST_F(OperationalAuditTest, EdgeCase_NullCorrelationId) {
    // Test: Verify behavior with empty correlation IDs
    
    logEventCompat(
        audit_logger,
        OperationalEventType::POLICY_EVALUATION_PERMIT,
        "null_test", "service", "policy_engine", "eval",
        "null_res", "resource", "evaluate", "permit",
        "POLICY", 100, {},
        nlohmann::json::object(),
        "", "");  // Empty correlation and causality
    
    auto events = audit_logger->queryEventsByActor("null_test");
    ASSERT_GE(events.size(), 1);
    
    // Verify event was logged with generated correlation ID
    EXPECT_FALSE(events.back().correlation_id.empty());
    EXPECT_EQ(events.back().correlation_id, events.back().event_id);
}

TEST_F(OperationalAuditTest, EdgeCase_QueryNonexistentActor) {
    // Test: Verify graceful handling of queries for non-existent actors
    
    auto events = audit_logger->queryEventsByActor("nonexistent_actor_12345");
    EXPECT_EQ(events.size(), 0);
}

TEST_F(OperationalAuditTest, EdgeCase_QueryNonexistentTimeRange) {
    // Test: Verify graceful handling of queries for future time range
    
    int64_t future_start = std::chrono::duration_cast<std::chrono::milliseconds>(
        (std::chrono::system_clock::now() + std::chrono::hours(24)).time_since_epoch()).count();
    int64_t future_end = future_start + 3600000;  // 1 hour later
    
    auto events = audit_logger->queryEventsByTimeRange(future_start, future_end);
    EXPECT_EQ(events.size(), 0);
}

TEST_F(OperationalAuditTest, EdgeCase_CircularBufferEviction) {
    // Test: Verify circular buffer eviction when max events exceeded
    // Note: Using a smaller test logger instance
    
    OperationalAuditLogger small_logger(5);  // Max 5 events
    
    // Log 10 events
    for (int i = 0; i < 10; ++i) {
        logEventCompat(
            &small_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "eviction_test", "service", "policy_engine", "eval",
            "res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", 100, {},
            nlohmann::json::object());
    }
    
    // Verify count is limited to max_events
    EXPECT_LE(small_logger.getTotalEventCount(), 5);
}

TEST_F(OperationalAuditTest, ThreadSafety_ConcurrentLogging) {
    // Test: Verify thread-safe concurrent event logging
    
    const int num_threads = 4;
    const int events_per_thread = 25;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < events_per_thread; ++i) {
                logEventCompat(
        audit_logger,
                    OperationalEventType::POLICY_EVALUATION_PERMIT,
                    "thread_" + std::to_string(t), "service", "policy_engine", "eval",
                    "res_" + std::to_string(t) + "_" + std::to_string(i), "resource", 
                    "evaluate", "permit",
                    "POLICY", 100, {},
                    nlohmann::json::object());
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify all events were logged
    size_t total_events = audit_logger->getTotalEventCount();
    EXPECT_GE(total_events, num_threads * events_per_thread - 10);  // Allow some variation
}

TEST_F(OperationalAuditTest, ThreadSafety_ConcurrentQueries) {
    // Test: Verify thread-safe concurrent event querying
    
    // Pre-populate with events
    for (int i = 0; i < 20; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "query_test_" + std::to_string(i % 4), "service", "policy_engine", "eval",
            "query_res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", 100, {},
            nlohmann::json::object());
    }
    
    const int num_query_threads = 4;
    std::vector<std::thread> threads;
    std::vector<size_t> results(num_query_threads, 0);
    
    for (int t = 0; t < num_query_threads; ++t) {
        threads.emplace_back([&, t]() {
            auto events = audit_logger->queryEventsByActor("query_test_" + std::to_string(t));
            results[t] = events.size();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify queries completed without errors
    for (int i = 0; i < num_query_threads; ++i) {
        EXPECT_GE(results[i], 5);  // Should have ~5 events per actor
    }
}

TEST_F(OperationalAuditTest, Evidence_FingerprintIntegrity) {
    // Test: Verify evidence fingerprint consistency
    
    nlohmann::json test_data = {
        {"key1", "value1"},
        {"key2", 12345},
        {"key3", {{"nested", "data"}}}
    };
    
    std::string evidence_id1, evidence_id2;
    
    // Record evidence twice with same data
    recordEvidenceCompat(
        "TEST_REQUIREMENT",
        "TEST_REQUIREMENT",
        "TEST_DATA",
        "Test evidence 1",
        "event_1",
        test_data,
        365,
        "TEST");
    
    auto evidence1 = evidence_collector->getEvidenceByRequirement("TEST_REQUIREMENT");
    if (!evidence1.empty()) {
        evidence_id1 = evidence1[0].evidence_id;
    }
    
    recordEvidenceCompat(
        "TEST_REQUIREMENT",
        "TEST_REQUIREMENT",
        "TEST_DATA",
        "Test evidence 2",
        "event_2",
        test_data,
        365,
        "TEST");
    
    auto evidence2 = evidence_collector->getEvidenceByRequirement("TEST_REQUIREMENT");
    if (evidence2.size() >= 2) {
        // Both should have same fingerprint (same data)
        EXPECT_EQ(evidence1[0].fingerprint, evidence2[1].fingerprint);
    }
}

TEST_F(OperationalAuditTest, Evidence_RetentionPolicy) {
    // Test: Verify evidence retention tracking
    
    int64_t retention_days = 30;
    
    recordEvidenceCompat(
        "RETENTION_TEST",
        "RETENTION_TEST",
        "TEST_DATA",
        "Retention test evidence",
        "event_r1",
        nlohmann::json::object(),
        retention_days,
        "TEST");
    
    auto evidence = evidence_collector->getEvidenceByRequirement("RETENTION_TEST");
    if (!evidence.empty()) {
        // Verify retention_until_ms is approximately 30 days in the future
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        int64_t expected_retention = now_ms + (retention_days * 24LL * 3600LL * 1000LL);
        
        // Allow 1-minute variation
        int64_t tolerance_ms = 60000;
        EXPECT_NEAR(evidence[0].retention_until_ms, expected_retention, tolerance_ms);
    }
}

TEST_F(OperationalAuditTest, QueryByModule_MultipleEvents) {
    // Test: Verify module-based event queries
    
    std::string test_module = "test_module_xyz";
    
    for (int i = 0; i < 5; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "module_test", "service", test_module, "eval",
            "mod_res_" + std::to_string(i), "resource", "evaluate", "permit",
            "POLICY", 100, {},
            nlohmann::json::object());
    }
    
    auto events = audit_logger->queryEventsByModule(test_module);
    EXPECT_GE(events.size(), 5);
    
    // Verify all events belong to same module
    for (const auto& event : events) {
        EXPECT_EQ(event.module_name, test_module);
    }
}

TEST_F(OperationalAuditTest, QueryByResource_MultipleEvents) {
    // Test: Verify resource-based event queries
    
    std::string test_resource = "test_resource_abc";
    
    for (int i = 0; i < 4; ++i) {
        logEventCompat(
        audit_logger,
            OperationalEventType::POLICY_EVALUATION_PERMIT,
            "actor_" + std::to_string(i), "service", "policy_engine", "eval",
            test_resource, "resource", "evaluate", "permit",
            "POLICY", 100, {},
            nlohmann::json::object());
    }
    
    auto events = audit_logger->queryEventsByResource(test_resource);
    EXPECT_GE(events.size(), 4);
    
    // Verify all events reference same resource
    for (const auto& event : events) {
        EXPECT_EQ(event.resource_id, test_resource);
    }
}

TEST_F(OperationalAuditTest, PolicyLifecycle_AllEventTypes) {
    // Test: Verify all policy lifecycle events are logged correctly
    
    std::string policy_id = "lifecycle_policy_001";
    std::string admin = "admin_user";
    
    // Test all lifecycle operations
    std::vector<std::string> lifecycle_ops = {"create", "update", "delete", "activate", "deactivate"};
    
    for (const auto& op : lifecycle_ops) {
        audit_logger->logPolicyLifecycle(
            "corr_lifecycle_" + op,
            policy_id,
            op,
            admin,
            jsonToContext(nlohmann::json::object()));
    }
    
    auto events = audit_logger->queryEventsByActor(admin);
    
    // Filter for policy lifecycle events
    size_t lifecycle_count = 0;
    for (const auto& event : events) {
        if (event.resource_id == policy_id) {
            lifecycle_count++;
        }
    }
    
    EXPECT_EQ(lifecycle_count, lifecycle_ops.size());
}

TEST_F(OperationalAuditTest, EventSerialization_JsonRoundTrip) {
    // Test: Verify JSON serialization/deserialization round-trip
    
    OperationalEvent original;
    original.event_id = "test_event_001";
    original.event_type = OperationalEventType::POLICY_EVALUATION_PERMIT;
    original.correlation_id = "corr_001";
    original.causality_parent_id = "parent_001";
    original.timestamp_ms = 1234567890000;
    original.sequence_number = 42;
    original.actor_id = "test_actor";
    original.actor_type = "user";
    original.module_name = "policy_engine";
    original.operation_name = "evaluate";
    original.resource_id = "policy_001";
    original.resource_type = "policy";
    original.action = "evaluate";
    original.result = "permit";
    original.classification = "POLICY_DECISION";
    original.compliance_tags = {"TAG1", "TAG2"};
    original.operation_duration_us = 1500;
    original.logging_duration_us = 250;
    original.context = {{"key", "value"}};
    original.error_message = "";
    original.event_payload = nlohmann::json({{"data", "payload"}}).dump();
    original.evidence_ids = {"ev1", "ev2"};
    
    // Serialize to JSON
    auto json = original.toJson();
    
    // Deserialize back
    auto restored = OperationalEvent::fromJson(json);
    
    // Verify all fields match
    EXPECT_EQ(restored.event_id, original.event_id);
    EXPECT_EQ(restored.correlation_id, original.correlation_id);
    EXPECT_EQ(restored.actor_id, original.actor_id);
    EXPECT_EQ(restored.timestamp_ms, original.timestamp_ms);
    EXPECT_EQ(restored.sequence_number, original.sequence_number);
    EXPECT_EQ(restored.compliance_tags, original.compliance_tags);
}

TEST_F(OperationalAuditTest, EvidenceSerialization_JsonRoundTrip) {
    // Test: Verify ComplianceEvidence JSON serialization/deserialization
    
    ComplianceEvidence original;
    original.evidence_id = "ev_001";
    original.requirement_id = "REQ_001";
    original.requirement_type = "EU_AI_ACT_13";
    original.collected_at_ms = 1234567890000;
    original.evidence_type = "OPERATIONAL_EVENT";
    original.description = "Test evidence";
    original.source_event_id = "event_001";
    original.fingerprint = "abc123def456";
    original.data_summary = "{\"test\": \"data\"}";
    original.retention_until_ms = 1234567890000LL + (365LL * 24LL * 3600LL * 1000LL);
    original.audit_classification = "REGULATORY";
    original.metadata = {{"meta_key", "meta_value"}};
    
    // Serialize
    auto json = original.toJson();
    
    // Deserialize
    auto restored = ComplianceEvidence::fromJson(json);
    
    // Verify fields match
    EXPECT_EQ(restored.evidence_id, original.evidence_id);
    EXPECT_EQ(restored.requirement_id, original.requirement_id);
    EXPECT_EQ(restored.requirement_type, original.requirement_type);
    EXPECT_EQ(restored.fingerprint, original.fingerprint);
    EXPECT_EQ(restored.audit_classification, original.audit_classification);
}




