# AI Decision Auditing and Explainability - Developer Guide

## Overview

This module provides comprehensive AI decision auditing and explainability features to ensure compliance with EU AI Act, GDPR Article 22, and eIDAS regulations.

## Components

### 1. AIDecisionAuditor (`include/llm/ai_decision_auditor.h`)

Comprehensive AI decision logging with:
- Complete context capture (query, model info, response, confidence)
- Cryptographic signing for tamper-proof audit trail
- Human oversight mechanisms (review flags, overrides)
- Flexible querying and compliance export

**Key Features:**
- Auto-flags low confidence decisions (<0.7) for human review
- Supports cryptographic signatures (when PKI client available)
- Comprehensive filtering and querying
- Statistics and reporting
- Compliance export functionality

### 2. ExplanationGenerator (`include/llm/explanation_generator.h`)

Generates human-readable explanations in multiple formats:
- **User-Friendly**: Plain language for end users
- **Technical**: Detailed analysis for developers/auditors
- **Compliance**: Formal explanations for regulatory compliance
- **JSON**: Structured format for API consumption

**Key Features:**
- Reasoning chain generation
- Key factor identification
- Confidence interval explanation
- Multiple audience formats

### 3. Configuration (`config/ai_audit_config.yaml`)

YAML configuration with comprehensive settings for:
- EU AI Act compliance mode
- GDPR Article 22 support
- Retention policies (365 days default)
- Human oversight configuration
- Monitoring and alerting

## Usage Examples

### Basic Auditing

```cpp
#include "llm/ai_decision_auditor.h"

// Create auditor (requires RocksDB instance)
AIDecisionAuditor auditor(db, nullptr);

// Create audit entry
AIDecisionAudit audit;
audit.user_id = "user_123";
audit.query = "What is the capital of France?";
audit.model_name = "gpt-4";
audit.model_version = "1.0";
audit.response = "Paris is the capital of France.";
audit.confidence_score = 0.95f;

audit.reasoning_steps = {
    "Analyzed query as geographical question",
    "Retrieved capital from knowledge base",
    "Generated response"
};

// Log the decision
auto stored = auditor.logDecision(audit);
std::cout << "Decision ID: " << stored.decision_id << std::endl;
```

### Generating Explanations

```cpp
#include "llm/explanation_generator.h"

ExplanationGenerator generator;

// Generate user-friendly explanation
std::string explanation = generator.generateExplanation(
    query,
    response,
    reasoning_steps,
    key_factors,
    ExplanationGenerator::Format::USER_FRIENDLY
);

// Generate compliance explanation
std::string compliance = generator.generateComplianceExplanation(
    query,
    response,
    "Model v1.0",
    reasoning_steps,
    key_factors,
    0.88f
);
```

### Querying Audit Log

```cpp
// Query low-confidence decisions needing review
AIDecisionAuditor::QueryFilter filter;
filter.requires_review = true;

auto results = auditor.queryAuditLog(filter);
for (const auto& decision : results) {
    std::cout << "Review needed: " << decision.decision_id 
              << " (confidence: " << decision.confidence_score << ")"
              << std::endl;
}
```

### Human Override

```cpp
// Record human override of an AI decision
bool success = auditor.recordOverride(
    decision_id,
    "Corrected factual error in response",
    "reviewer_456"
);
```

### Compliance Export

```cpp
// Export decisions for compliance reporting
AIDecisionAuditor::QueryFilter filter;
filter.start_time = /* start date */;
filter.end_time = /* end date */;

auditor.exportForCompliance(
    "compliance_report_2024.json",
    filter
);
```

## Integration with Existing LLM Pipeline

### Option 1: Wrap Existing Inference

```cpp
// In your LLM inference code
std::string performInference(const std::string& query) {
    AIDecisionAudit audit;
    audit.query = query;
    audit.user_id = getCurrentUserId();
    audit.model_name = "my-model";
    
    auto start = std::chrono::system_clock::now();
    
    // Existing inference
    std::string response = model->generate(query);
    
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Populate audit
    audit.response = response;
    audit.confidence_score = model->getLastConfidence();
    audit.latency_ms = duration.count();
    audit.timestamp = start;
    
    // Log decision
    auditor.logDecision(audit);
    
    return response;
}
```

### Option 2: Extend LLMInteractionStore

The AIDecisionAuditor is designed to work alongside the existing `LLMInteractionStore`. You can:

1. Log basic interactions to LLMInteractionStore (for caching, etc.)
2. Log compliance-critical decisions to AIDecisionAuditor
3. Use AIDecisionAuditor for all decisions requiring explainability

## Testing

Unit tests are provided in:
- `tests/test_ai_decision_auditor.cpp` (17 tests)
- `tests/test_explanation_generator.cpp` (21 tests)

Run tests:
```bash
cd build
ctest --output-on-failure -R "AIDecisionAuditor|ExplanationGenerator"
```

## Compliance Features

### EU AI Act Compliance
- ✅ Complete logging of AI system operations
- ✅ Transparency through explanations
- ✅ Human oversight mechanisms
- ✅ Model version tracking
- ✅ Decision traceability

### GDPR Article 22 Compliance
- ✅ Right to explanation (generateExplanation)
- ✅ Right to human intervention (recordOverride)
- ✅ Information about decision logic (reasoning_steps, key_factors)

### eIDAS Regulation Compliance
- ✅ Comprehensive audit logs
- ✅ Cryptographic signatures (when enabled)
- ✅ Non-repudiation through signed entries
- ✅ Tamper-proof audit trail

## Configuration

See `config/ai_audit_config.yaml` for full configuration options:

```yaml
ai_auditing:
  enabled: true
  explainability:
    confidence_threshold_review: 0.7
  storage:
    retention_days: 365
  compliance:
    eu_ai_act_mode: true
    gdpr_article_22_mode: true
```

## Performance Considerations

- **Logging overhead**: < 5ms per decision (target)
- **Storage**: Compressed JSON in RocksDB
- **Querying**: Indexed by decision_id, supports filters
- **Signing**: Optional, adds ~1-2ms when enabled

## Security

- Audit logs stored in RocksDB with optional encryption
- Cryptographic signatures prevent tampering
- Access control via RocksDB permissions
- Sensitive data can be encrypted at field level

## Future Enhancements

Potential improvements for future releases:
- Integration with Prometheus for real-time monitoring
- Dashboard UI for audit log exploration
- Automatic model drift detection
- Enhanced explanation generation using LLM
- Multi-language explanation support
- Integration with external SIEM systems

## Support

For questions or issues:
- Check existing tests for usage examples
- See `config/ai_audit_config.yaml` for configuration details
- Review issue tracker for known limitations
