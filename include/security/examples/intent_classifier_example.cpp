/**
 * @file intent_classifier_example.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Example: IntentClassifier — semantic query-intent analysis
//
// Paper 2 — Layer 7: Security Anomaly Detection via Semantics
// Related issue: IMPL-B7 (docs/issues/optimization_layers/IMPL-B7-intent-classifier.md)
//
// This example demonstrates:
//   1. Classifying a SQL-injection-style query.
//   2. Classifying a benign SELECT query.
//   3. High-confidence alert updates ZeroTrustPolicyEnforcer::session_risk_score.
//   4. GDPR guard: evidence_snippet <= 128 chars.
//   5. DecisionRecord written to AIDecisionAuditor.
//
// NOTE: IntentClassifier (IMPL-B7) is not yet implemented.
//

#include <iostream>
#include <string>
#include <cassert>
#include <algorithm>

// Existing production headers
#include "security/zero_trust_policy_enforcer.h"  // ZeroTrustPolicyEnforcer, ZeroTrustContext
#include "security/access_control.h"               // SessionConfig

// PLANNED header (IMPL-B7)
// #include "security/intent_classifier.h"  // IntentClassifier, IntentAlert, IntentType

namespace {

struct QueryContext {
    std::string session_id;
    std::string query_text;
    std::string tenant_id;
};

// Simulates what IntentClassifier::classify() would produce (IMPL-B7)
struct SimulatedAlert {
    std::string intent_type;
    double      confidence;
    std::string session_id = {};
    std::string evidence_snippet;  // must be <= 128 chars
};

SimulatedAlert classifyQuery(const QueryContext& ctx) {
    const std::string& q = ctx.query_text;
    SimulatedAlert alert;
    alert.session_id = ctx.session_id;

    // Heuristic intent detection (real impl uses ML model)
    if (q.find("' OR '1'='1") != std::string::npos ||
        q.find("--") != std::string::npos) {
        alert.intent_type = "SQL_INJECTION_ATTEMPT";
        alert.confidence  = 0.94;
    } else if (q.find("SELECT *") != std::string::npos &&
               q.find("LIMIT") == std::string::npos) {
        alert.intent_type = "MASS_EXPORT";
        alert.confidence  = 0.71;
    } else {
        alert.intent_type = "NORMAL";
        alert.confidence  = 0.97;
    }

    // Evidence snippet: first 128 chars of query (GDPR limit)
    alert.evidence_snippet = q.substr(0, std::min<size_t>(q.size(), 128));
    return alert;
}

} // namespace

int main() {
    std::cout << "=== IntentClassifier Example (IMPL-B7) ===\n\n";

    // -----------------------------------------------------------------------
    // Step 1: SQL injection attempt
    // -----------------------------------------------------------------------
    std::cout << "Step 1: Classify SQL-injection-style query\n";

    QueryContext inj_ctx{
        "session-001",
        "SELECT * FROM users WHERE name = 'admin' OR '1'='1' -- bypass",
        "tenant-42"
    };

    /* PLANNED (IMPL-B7):
    IntentClassifier classifier;
    classifier.setAnomalyDetector(&ml_anomaly_detector);
    IntentAlert alert = classifier.classify(inj_ctx);
    assert(alert.intent_type == IntentType::SQL_INJECTION_ATTEMPT);
    assert(alert.confidence >= 0.92);
    assert(alert.evidence_snippet.size() <= 128);
    */
    const auto inj_alert = classifyQuery(inj_ctx);
    std::cout << "  Query:     \"" << inj_ctx.query_text.substr(0, 55) << "...\"\n"
              << "  Intent:    " << inj_alert.intent_type << "\n"
              << "  Confidence:" << inj_alert.confidence << "\n"
              << "  Evidence:  " << inj_alert.evidence_snippet.size() << " chars "
              << (inj_alert.evidence_snippet.size() <= 128 ? "✓" : "✗ GDPR violation") << "\n\n";

    assert(inj_alert.evidence_snippet.size() <= 128 && "GDPR: evidence_snippet > 128 chars");

    // -----------------------------------------------------------------------
    // Step 2: High-confidence alert updates ZeroTrustPolicyEnforcer
    // -----------------------------------------------------------------------
    std::cout << "Step 2: High-confidence alert → ZeroTrustContext::session_risk_score\n";

    if (inj_alert.confidence >= 0.85) {
        /* PLANNED (IMPL-B7 + existing ZeroTrustPolicyEnforcer API):
        ZeroTrustContext ctx;
        ctx.session_id = inj_ctx.session_id;
        ctx.session_risk_score = inj_alert.confidence;   // set by IntentClassifier
        enforcer.evaluate(ctx, request);
        */
        std::cout << "  [PLANNED] ZeroTrustContext::session_risk_score = "
                  << inj_alert.confidence << " for session " << inj_ctx.session_id << "\n"
                  << "  ZeroTrustPolicyEnforcer will block or challenge this session.\n\n";
    }

    // -----------------------------------------------------------------------
    // Step 3: Benign query — no action
    // -----------------------------------------------------------------------
    std::cout << "Step 3: Classify benign query\n";

    QueryContext benign_ctx{
        "session-002",
        "SELECT id, name FROM products WHERE category = 'database' LIMIT 20",
        "tenant-42"
    };

    /* PLANNED (IMPL-B7):
    IntentAlert benign_alert = classifier.classify(benign_ctx);
    assert(benign_alert.intent_type == IntentType::NORMAL);
    assert(benign_alert.confidence >= 0.95);
    */
    const auto benign_alert = classifyQuery(benign_ctx);
    std::cout << "  Query:     \"" << benign_ctx.query_text << "\"\n"
              << "  Intent:    " << benign_alert.intent_type << "\n"
              << "  Confidence:" << benign_alert.confidence << "\n"
              << "  No action taken (below alert threshold).\n\n";

    // -----------------------------------------------------------------------
    // Step 4: MASS_EXPORT below confidence threshold — log only
    // -----------------------------------------------------------------------
    std::cout << "Step 4: MASS_EXPORT (below 0.85 threshold) — advisory log only\n";

    QueryContext mass_ctx{
        "session-003",
        "SELECT * FROM customers",
        "tenant-42"
    };

    const auto mass_alert = classifyQuery(mass_ctx);
    std::cout << "  Query:     \"" << mass_ctx.query_text << "\"\n"
              << "  Intent:    " << mass_alert.intent_type << "\n"
              << "  Confidence:" << mass_alert.confidence
              << (mass_alert.confidence < 0.85 ? "  → log only (no block)\n\n"
                                               : "  → block triggered\n\n");

    // -----------------------------------------------------------------------
    // Step 5: DecisionRecord in AIDecisionAuditor
    // -----------------------------------------------------------------------
    std::cout << "Step 5: DecisionRecord in AIDecisionAuditor\n";

    /* PLANNED (IMPL-B7):
    auto records = ai_auditor.getRecords(DecisionType::INTENT_CLASSIFICATION);
    assert(!records.empty());
    // Evidence snippet must not contain PII beyond the 128-char limit
    for (const auto& rec : records) {
        assert(rec.detail.size() <= 200);
    }
    */
    std::cout << "  [PLANNED] DecisionRecord written for SQL_INJECTION_ATTEMPT alert\n"
              << "  [PLANNED] DecisionRecord written for MASS_EXPORT advisory\n\n";

    // -----------------------------------------------------------------------
    // Summary
    // -----------------------------------------------------------------------
    std::cout << "=== Summary ===\n"
              << "  SQL injection:       " << inj_alert.intent_type
              << "  (confidence " << inj_alert.confidence << ")  → ZeroTrust updated\n"
              << "  Benign SELECT:       " << benign_alert.intent_type
              << "  (confidence " << benign_alert.confidence << ")  → no action\n"
              << "  MASS_EXPORT below threshold → advisory log only\n"
              << "  IntentClassifier:    [PLANNED — IMPL-B7]\n"
              << "\nSee docs/issues/optimization_layers/IMPL-B7-intent-classifier.md\n";

    return 0;
}
