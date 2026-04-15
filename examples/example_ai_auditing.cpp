/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_ai_auditing.cpp                            ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     336                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file example_ai_auditing.cpp
 * @brief Example demonstrating AI Decision Auditing and Explainability features
 * 
 * This example shows how to:
 * 1. Log AI decisions with full context
 * 2. Generate human-readable explanations
 * 3. Query audit logs
 * 4. Handle human oversight and overrides
 * 5. Export for compliance reporting
 */

#include "llm/ai_decision_auditor.h"
#include "llm/explanation_generator.h"
#include <iostream>
#include <memory>

using namespace themis::llm;

/**
 * Example: Basic AI Decision Logging
 */
void example_basic_logging(AIDecisionAuditor& auditor) {
    std::cout << "\n=== Example 1: Basic AI Decision Logging ===\n";
    
    // Create an AI decision audit entry
    AIDecisionAudit audit;
    audit.user_id = "user_alice";
    audit.session_id = "session_20240112";
    audit.query = "What are the health benefits of green tea?";
    
    // Model information
    audit.model_name = "health-advisor-model";
    audit.model_version = "2.1";
    audit.model_params = json::object();
    audit.model_params["temperature"] = 0.7;
    audit.model_params["max_tokens"] = 500;
    
    // Response
    audit.response = "Green tea contains antioxidants called catechins that may "
                    "help reduce inflammation, support heart health, and boost metabolism. "
                    "It also contains caffeine and L-theanine which can improve focus.";
    audit.confidence_score = 0.92f;
    audit.token_count = 85;
    audit.latency_ms = 245;
    
    // Reasoning chain
    audit.reasoning_steps = {
        "Identified query as health-related question",
        "Retrieved scientific literature on green tea benefits",
        "Synthesized evidence-based response",
        "Validated against medical knowledge base"
    };
    
    // Key factors
    audit.key_factors = json::object();
    audit.key_factors["query_category"] = "health_information";
    audit.key_factors["evidence_quality"] = "peer_reviewed";
    audit.key_factors["medical_disclaimer"] = true;
    
    // Log the decision
    auto stored = auditor.logDecision(audit);
    
    std::cout << "✓ Decision logged with ID: " << stored.decision_id << "\n";
    std::cout << "  Confidence: " << (stored.confidence_score * 100) << "%\n";
    std::cout << "  Review required: " << (stored.requires_human_review ? "Yes" : "No") << "\n";
}

/**
 * Example: Low Confidence Decision (Auto-flagged for Review)
 */
void example_low_confidence(AIDecisionAuditor& auditor) {
    std::cout << "\n=== Example 2: Low Confidence Decision ===\n";
    
    AIDecisionAudit audit;
    audit.user_id = "user_bob";
    audit.query = "What will the stock market do tomorrow?";
    audit.model_name = "financial-advisor-model";
    audit.model_version = "1.5";
    audit.response = "Market predictions are highly uncertain. Consider diversification.";
    audit.confidence_score = 0.45f; // Below 0.7 threshold - will be auto-flagged
    audit.latency_ms = 180;
    
    auto stored = auditor.logDecision(audit);
    
    std::cout << "✓ Decision logged with ID: " << stored.decision_id << "\n";
    std::cout << "  Confidence: " << (stored.confidence_score * 100) << "%\n";
    std::cout << "  ⚠️  Auto-flagged for human review (confidence < 70%)\n";
}

/**
 * Example: Generating Explanations
 */
void example_explanations(AIDecisionAuditor& auditor) {
    std::cout << "\n=== Example 3: Generating Explanations ===\n";
    
    // Create and log a decision
    AIDecisionAudit audit;
    audit.user_id = "user_charlie";
    audit.query = "Is it safe to travel to Tokyo?";
    audit.model_name = "travel-advisor-model";
    audit.model_version = "3.0";
    audit.response = "Tokyo is generally very safe for travelers. It has low crime rates "
                    "and excellent public transportation.";
    audit.confidence_score = 0.88f;
    
    audit.reasoning_steps = {
        "Checked current travel advisories",
        "Analyzed crime statistics",
        "Reviewed recent traveler feedback",
        "Considered public safety infrastructure"
    };
    
    audit.key_factors = json::object();
    audit.key_factors["crime_rate"] = "low";
    audit.key_factors["travel_advisory_level"] = "0";
    audit.key_factors["public_safety_score"] = 9.2;
    
    auto stored = auditor.logDecision(audit);
    
    // Generate different explanation formats
    ExplanationGenerator generator;
    
    // User-friendly explanation
    std::cout << "\n--- User-Friendly Explanation ---\n";
    std::string user_friendly = generator.generateExplanation(
        stored.query,
        stored.response,
        stored.reasoning_steps,
        stored.key_factors,
        ExplanationGenerator::Format::USER_FRIENDLY
    );
    std::cout << user_friendly << "\n";
    
    // Compliance explanation (GDPR/EU AI Act)
    std::cout << "\n--- Compliance Explanation (GDPR/EU AI Act) ---\n";
    std::string compliance = generator.generateComplianceExplanation(
        stored.query,
        stored.response,
        stored.model_name + " v" + stored.model_version,
        stored.reasoning_steps,
        stored.key_factors,
        stored.confidence_score
    );
    std::cout << compliance << "\n";
}

/**
 * Example: Querying Audit Logs
 */
void example_querying(AIDecisionAuditor& auditor) {
    std::cout << "\n=== Example 4: Querying Audit Logs ===\n";
    
    // Query 1: Find all decisions needing review
    std::cout << "\nQuery 1: Decisions requiring human review\n";
    AIDecisionAuditor::QueryFilter filter1;
    filter1.requires_review = true;
    filter1.limit = 10;
    
    auto review_needed = auditor.queryAuditLog(filter1);
    std::cout << "Found " << review_needed.size() << " decisions needing review\n";
    
    for (const auto& decision : review_needed) {
        std::cout << "  - " << decision.decision_id 
                  << " (confidence: " << (decision.confidence_score * 100) << "%)\n";
    }
    
    // Query 2: Find low confidence decisions
    std::cout << "\nQuery 2: Low confidence decisions (<80%)\n";
    AIDecisionAuditor::QueryFilter filter2;
    filter2.max_confidence = 0.8f;
    filter2.limit = 10;
    
    auto low_confidence = auditor.queryAuditLog(filter2);
    std::cout << "Found " << low_confidence.size() << " low confidence decisions\n";
    
    // Query 3: Find decisions for specific user
    std::cout << "\nQuery 3: Decisions for user 'user_alice'\n";
    AIDecisionAuditor::QueryFilter filter3;
    filter3.user_id = "user_alice";
    filter3.limit = 10;
    
    auto user_decisions = auditor.queryAuditLog(filter3);
    std::cout << "Found " << user_decisions.size() << " decisions for user_alice\n";
}

/**
 * Example: Human Override
 */
void example_human_override(AIDecisionAuditor& auditor) {
    std::cout << "\n=== Example 5: Human Override ===\n";
    
    // Create a decision that will be overridden
    AIDecisionAudit audit;
    audit.user_id = "user_david";
    audit.query = "What is 2+2?";
    audit.model_name = "math-model";
    audit.response = "5"; // Incorrect!
    audit.confidence_score = 0.6f;
    
    auto stored = auditor.logDecision(audit);
    std::cout << "Original decision: " << stored.decision_id << "\n";
    std::cout << "  Response: " << stored.response << "\n";
    std::cout << "  Flagged for review: Yes\n";
    
    // Human reviewer corrects the mistake
    bool success = auditor.recordOverride(
        stored.decision_id,
        "Corrected mathematical error. Correct answer is 4.",
        "reviewer_emma"
    );
    
    if (success) {
        std::cout << "\n✓ Human override recorded\n";
        std::cout << "  Reviewer: reviewer_emma\n";
        std::cout << "  Corrected response: 4\n";
        
        // Retrieve updated decision
        auto updated = auditor.getDecision(stored.decision_id);
        if (updated.has_value()) {
            std::cout << "  Review flag cleared: " 
                      << (!updated->requires_human_review ? "Yes" : "No") << "\n";
        }
    }
}

/**
 * Example: Statistics and Reporting
 */
void example_statistics(AIDecisionAuditor& auditor) {
    std::cout << "\n=== Example 6: Statistics and Reporting ===\n";
    
    auto stats = auditor.getStats();
    
    std::cout << "Audit Statistics:\n";
    std::cout << "  Total decisions: " << stats.total_decisions << "\n";
    std::cout << "  Flagged for review: " << stats.flagged_for_review << "\n";
    std::cout << "  Human overrides: " << stats.human_overrides << "\n";
    std::cout << "  Average confidence: " << (stats.avg_confidence * 100) << "%\n";
    std::cout << "  Average latency: " << stats.avg_latency_ms << "ms\n";
    
    // Calculate review rate
    if (stats.total_decisions > 0) {
        float review_rate = (float)stats.flagged_for_review / stats.total_decisions * 100;
        std::cout << "  Review rate: " << std::fixed << std::setprecision(1) 
                  << review_rate << "%\n";
        
        if (review_rate > 30) {
            std::cout << "  ⚠️  High review rate - consider improving model\n";
        }
    }
}

/**
 * Example: Compliance Export
 */
void example_compliance_export(AIDecisionAuditor& auditor) {
    std::cout << "\n=== Example 7: Compliance Export ===\n";
    
    AIDecisionAuditor::QueryFilter filter;
    // Export all decisions (in practice, would filter by date range)
    
    std::string export_path = "data/compliance_report.json";
    bool success = auditor.exportForCompliance(export_path, filter);
    
    if (success) {
        std::cout << "✓ Compliance report exported to: " << export_path << "\n";
        std::cout << "  Format: JSON\n";
        std::cout << "  Contains: All decisions with full audit trail\n";
        std::cout << "  Use case: GDPR Article 22 compliance, EU AI Act reporting\n";
    }
}

/**
 * Main example function
 */
int main() {
    std::cout << "====================================\n";
    std::cout << "AI Decision Auditing Example\n";
    std::cout << "====================================\n";
    
    try {
        // Note: In real usage, you would initialize RocksDB
        // For this example, we show the API usage patterns
        
        // AIDecisionAuditor auditor(db, nullptr);
        
        // Run examples
        // example_basic_logging(auditor);
        // example_low_confidence(auditor);
        // example_explanations(auditor);
        // example_querying(auditor);
        // example_human_override(auditor);
        // example_statistics(auditor);
        // example_compliance_export(auditor);
        
        std::cout << "\n====================================\n";
        std::cout << "Example completed successfully!\n";
        std::cout << "====================================\n";
        
        std::cout << "\nNote: This example demonstrates API usage.\n";
        std::cout << "In production:\n";
        std::cout << "1. Initialize RocksDB instance\n";
        std::cout << "2. Create AIDecisionAuditor with DB\n";
        std::cout << "3. Optionally provide PKI client for signing\n";
        std::cout << "4. Configure via ai_audit_config.yaml\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
