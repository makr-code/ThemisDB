# Operational Audit & Evidence Collection Guide

**Version:** 1.0  
**Status:** 🟢 PRODUCTION-READY  
**Last Updated:** 2024-01-15  
**Author:** ThemisDB Governance Team

---

## Table of Contents

1. [Overview](#overview)
2. [Event Logging Procedures](#event-logging-procedures)
3. [Event Type Reference](#event-type-reference)
4. [Correlation ID Patterns](#correlation-id-patterns)
5. [Evidence Collection Workflow](#evidence-collection-workflow)
6. [Compliance Requirement Mappings](#compliance-requirement-mappings)
7. [Performance Tuning](#performance-tuning)
8. [Troubleshooting Guide](#troubleshooting-guide)

---

## Overview

The Operational Audit & Evidence Collection system provides comprehensive logging of all governance operations, automated correlation of related events across module boundaries, and continuous collection of compliance evidence for regulatory audits.

### Key Features

- **Structured Event Logging**: All governance operations produce machine-readable audit events with rich context
- **Event Correlation**: Related events across module boundaries are linked via correlation IDs and causality tracking
- **Compliance Evidence Automation**: Evidence is automatically collected for regulatory requirements (EU AI Act, SOC 2, ISO 27001)
- **Performance Optimized**: Event logging overhead <5% of operation time; correlation queries complete in <100ms
- **Thread-Safe**: All operations are thread-safe for deployment in multi-threaded governance services

### Global Accessor Pattern

Access audit functionality via global singleton accessors:

```cpp
#include "governance/operational_audit.h"

using namespace themis::governance;

// Log events
auto& logger = getGlobalAuditLogger();
logger.logPolicyEvaluation(policy_id, decision, duration_us, actor_id, context);

// Track correlations
auto& correlator = getGlobalCorrelationEngine();
correlator.createCorrelation(correlation_id, event);

// Collect evidence
auto& collector = getGlobalEvidenceCollector();
collector.collectEvidence("EU_AI_ACT_13");
```

---

## Event Logging Procedures

### Basic Event Logging

The `OperationalAuditLogger` class provides methods for logging governance events:

#### Policy Evaluation Events

```cpp
auto& logger = getGlobalAuditLogger();
logger.logPolicyEvaluation(
    policy_id,           // "policy_data_access_v2"
    decision,            // "permit" or "deny"
    evaluation_duration_us,
    actor_id,            // "user@company.com"
    evaluation_context   // nlohmann::json with policy context
);
```

**When to log:** Every policy evaluation by the policy engine, whether permit or deny.

#### Compliance Check Events

```cpp
logger.logComplianceCheck(
    check_id,            // "check_pii_classification"
    result,              // "pass" or "fail"
    check_duration_us,
    actor_id,            // Service account performing check
    compliance_tags      // {"EU_GDPR", "DATA_PROTECTION"}
);
```

**When to log:** Every compliance verification against defined policies and standards.

#### Data Governance Operations

```cpp
logger.logDataGovernanceOp(
    operation,           // "mask", "lineage", "redact"
    resource_id,         // Dataset or field identifier
    actor_id,
    operation_duration_us,
    operation_details    // Details of masking, lineage changes, etc.
);
```

**When to log:** Any data governance action (masking, lineage modification, access control changes).

#### Policy Lifecycle Events

```cpp
logger.logPolicyLifecycle(
    policy_id,
    lifecycle_event,     // "create", "update", "delete", "activate", "deactivate"
    actor_id,            // Admin user who performed action
    details              // Policy changes, approvals, etc.
);
```

**When to log:** When policies are created, updated, deleted, or activation status changes.

### Generic Event Logging

For custom event types, use the generic `logEvent()` method:

```cpp
logger.logEvent(
    event_type,              // OperationalEventType enum value
    actor_id,
    actor_type,              // "user", "service", "system"
    module_name,             // "policy_engine", "compliance_engine", etc.
    operation_name,
    resource_id,
    resource_type,
    action,
    result,                  // "success", "failure", "error", "unknown"
    classification,          // Business/regulatory classification
    operation_duration_us,
    compliance_tags,         // Vector of regulatory requirement tags
    context,                 // nlohmann::json with contextual data
    error_message = "",
    event_payload = {},
    correlation_id = "",     // Defaults to event_id if empty
    causality_parent_id = "" // Links to parent event for causality tracking
);
```

### Event Context Guidelines

Include relevant context in event logging for audit trail completeness:

```cpp
nlohmann::json context = {
    {"policy_version", "2.1"},
    {"evaluation_mode", "strict"},
    {"environment", "production"},
    {"decision_reason", "data_type_matches_rule_5"},
    {"remediation", "redact_personal_identifiers"}
};
```

---

## Event Type Reference

### Policy & Governance Events (5010-5019)

| Event Code | Type | Description | When Used |
|-----------|------|-------------|-----------|
| 5010 | `POLICY_EVALUATION` | Policy rule evaluation with permit/deny decision | Policy engine evaluation |
| 5011 | `POLICY_CREATED` | Policy definition created | Policy creation by admin |
| 5012 | `POLICY_UPDATED` | Policy modified | Policy version update |
| 5013 | `POLICY_DELETED` | Policy removed from system | Policy deletion (usually archived) |
| 5014 | `POLICY_ACTIVATED` | Policy made active | Policy activation after review |
| 5015 | `POLICY_DEACTIVATED` | Policy disabled | Policy deactivation |
| 5016 | `POLICY_CONFLICT_DETECTED` | Policy conflict or contradiction found | Conflict detection during policy review |
| 5017 | `POLICY_CONSENSUS_REACHED` | Policy consensus agreement achieved | Conflict resolution |

### Compliance Events (5020-5029)

| Event Code | Type | Description | When Used |
|-----------|------|-------------|-----------|
| 5020 | `COMPLIANCE_CHECK_PASSED` | Compliance verification passed | Compliance engine pass result |
| 5021 | `COMPLIANCE_CHECK_FAILED` | Compliance verification failed | Compliance engine fail result |
| 5022 | `COMPLIANCE_VIOLATION_DETECTED` | Potential compliance violation found | Policy violation detected |
| 5023 | `REMEDIATION_ACTION_TAKEN` | Corrective action executed | Auto-remediation triggered |
| 5024 | `COMPLIANCE_AUDIT_INITIATED` | Compliance audit started | Scheduled or manual audit |
| 5025 | `COMPLIANCE_AUDIT_COMPLETED` | Compliance audit finished | Audit completion with results |

### Data Governance Events (5030-5039)

| Event Code | Type | Description | When Used |
|-----------|------|-------------|-----------|
| 5030 | `RESOURCE_ACCESSED` | Data resource accessed | Any data access operation |
| 5031 | `DATA_MASKING_APPLIED` | Sensitive data masked | Data masking operation |
| 5032 | `DATA_LINEAGE_MODIFIED` | Data lineage changed | Lineage tracking update |
| 5033 | `CLASSIFICATION_CHANGED` | Data classification modified | Classification update |
| 5034 | `RETENTION_POLICY_APPLIED` | Retention action executed | Data retention enforcement |

### Audit Trail Events (5040-5049)

| Event Code | Type | Description | When Used |
|-----------|------|-------------|-----------|
| 5040 | `AUDIT_LOG_CREATED` | Audit record created | New audit event generated |
| 5041 | `AUDIT_TRAIL_VERIFIED` | Audit trail integrity verified | Periodic integrity check |
| 5042 | `EVIDENCE_COLLECTED` | Compliance evidence gathered | Evidence collection |
| 5043 | `AUDIT_EXPORT_INITIATED` | Audit data export started | Export for audit/investigation |
| 5044 | `AUDIT_RETENTION_ENFORCED` | Old audit records archived/deleted | Retention policy enforcement |

### System Events (5050-5062)

| Event Code | Type | Description | When Used |
|-----------|------|-------------|-----------|
| 5050 | `AUDIT_CONFIGURATION_CHANGED` | Audit system configuration modified | Config update |
| 5051 | `AUDIT_LOG_FAILURE` | Audit logging failed | Logging system error |
| 5052 | `CORRELATION_CREATED` | Event correlation group created | Correlation engine activity |
| 5053 | `EVIDENCE_REQUIREMENT_LINKED` | Evidence linked to requirement | Evidence-requirement linking |

---

## Correlation ID Patterns

Correlation IDs link related events across module boundaries and enable distributed tracing of governance operations.

### Pattern 1: Single-Module Operation Chain

For operations within one module that trigger multiple events:

```
Request ID: corr_policy_eval_20240115_001
├── Event 1: Policy fetched from database (timestamp: T+0ms)
├── Event 2: Policy rule evaluation (timestamp: T+5ms)
├── Event 3: Audit decision logged (timestamp: T+15ms)
└── Event 4: Response sent to requester (timestamp: T+20ms)

All events share: correlation_id = "corr_policy_eval_20240115_001"
```

**Usage Example:**
```cpp
std::string corr_id = "corr_" + request_id;

logger.logEvent(..., corr_id, "");  // First event
logger.logEvent(..., corr_id, "");  // Related events use same corr_id
```

### Pattern 2: Cross-Module Causality

For operations spanning multiple modules with parent-child relationships:

```
Correlation ID: corr_audit_flow_20240115_x
├── Policy Engine
│   └── Event: Policy evaluated (event_id: evt_001, parent: "")
├── Compliance Engine (triggered by policy eval)
│   └── Event: Compliance check started (parent: evt_001)
│       └── Event: Compliance check completed (parent: evt_002)
└── Audit Engine (triggered by compliance result)
    └── Event: Audit record created (parent: evt_003)
```

**Usage Example:**
```cpp
// Module 1: Policy evaluation
auto& logger = getGlobalAuditLogger();
logger.logEvent(
    POLICY_EVALUATION,
    actor_id, "service", "policy_engine", "evaluate",
    policy_id, "policy", "evaluate", decision,
    "POLICY",
    operation_duration_us,
    {},
    context,
    "",
    {},
    corr_id,  // New correlation ID
    ""        // No parent (root of chain)
);

// Later in Module 2: Compliance check
std::string parent_event_id = evt_001;  // From policy evaluation
logger.logEvent(
    COMPLIANCE_CHECK_PASSED,
    actor_id, "service", "compliance_engine", "check",
    policy_id, "policy", "check", "pass",
    "COMPLIANCE",
    compliance_duration_us,
    {},
    context,
    "",
    {},
    corr_id,           // Same correlation ID
    parent_event_id    // Parent event ID for causality
);

// Link causality for explicit tracking
logger.linkCausalityRelationship(parent_event_id, current_event_id);
```

### Pattern 3: Request/Response Tracing

For distributed systems where requests cross service boundaries:

```
HTTP Request:
  Header: X-Correlation-ID: corr_req_20240115_abc123
  
Service A (Policy Validation)
  ├─ Event: Request received (corr_id)
  └─ Event: Validation completed (corr_id)
     
Service B (Data Check)
  ├─ Event: Request forwarded (corr_id)
  ├─ Event: Data check started (corr_id)
  └─ Event: Check completed (corr_id)
```

**Usage:**
```cpp
// Extract correlation ID from request header
std::string corr_id = request_headers["X-Correlation-ID"];

// Log all events with same correlation ID
logger.logEvent(..., corr_id, "");
```

### Guidelines for Correlation ID Usage

1. **Create New Correlation ID When:**
   - Starting a new independent governance operation
   - User initiates new request
   - Scheduled job/audit cycle starts

2. **Reuse Correlation ID When:**
   - Event is part of already-initiated operation
   - Operation spans multiple modules
   - Tracing causality through complex workflows

3. **Time Window Boundaries:**
   - Correlation IDs typically span 100ms - 60 seconds
   - If operation exceeds 60s, consider creating sub-correlations
   - Use causality parent IDs for longer chains

---

## Evidence Collection Workflow

### Automated Evidence Collection

The `ComplianceEvidenceCollector` automatically gathers evidence for regulatory requirements:

```cpp
auto& collector = getGlobalEvidenceCollector();

// Collect evidence for specific requirement
collector.collectEvidence("EU_AI_ACT_13");

// Get statistics
auto stats = collector.getEvidenceStatistics();
std::cout << "Total evidence: " << stats["total_evidence"] << std::endl;
```

### Manual Evidence Recording

Record evidence from external sources:

```cpp
collector.recordEvidence(
    requirement_id,        // "SOC2_CC7.2"
    requirement_type,      // "SOC2_CC7.2"
    evidence_type,         // "POLICY_DOCUMENT", "OPERATIONAL_EVENT", "LOG_ENTRY"
    description,           // "Policy audit performed on 2024-01-15"
    source_event_id,       // Link to event if applicable
    evidence_data,         // nlohmann::json with evidence details
    retention_days,        // 365 for most compliance evidence
    audit_classification   // "SOC2_EVIDENCE", "REGULATORY", etc.
);
```

### Evidence-Event Linking

Link external evidence to audit events:

```cpp
// Get event ID from governance operation
auto events = logger.queryEventsByActor("audit_service");
std::string event_id = events[0].event_id;

// Create evidence and link
collector.recordEvidence(
    "ISO27001_A1",
    "ISO27001_A1",
    "SECURITY_ASSESSMENT",
    "Access control review",
    event_id,
    assessment_results,
    1095,  // 3 years
    "ISO27001_EVIDENCE"
);

// Or link separately
std::string evidence_id = "ev_123";
collector.linkEvidenceToEvent(evidence_id, event_id);
```

### Evidence Export for Audit

Export evidence for regulatory audits:

```cpp
// Export for specific requirement
auto export_json = collector.exportEvidenceForAudit("EU_AI_ACT_13");

// Use in audit submission
std::ofstream audit_file("eu_ai_act_audit_evidence.json");
audit_file << export_json.dump(4);
audit_file.close();
```

### Evidence Verification

Verify evidence integrity using fingerprints:

```cpp
auto all_evidence = collector.getEvidenceByRequirement("SOC2_CC7.2");

for (const auto& evidence : all_evidence) {
    // Recompute fingerprint
    std::string recalc_fingerprint = computeSHA256(evidence.data_summary);
    
    // Verify hasn't been tampered with
    if (recalc_fingerprint != evidence.fingerprint) {
        std::cerr << "Evidence integrity issue: " << evidence.evidence_id << std::endl;
    }
}
```

---

## Compliance Requirement Mappings

### EU AI Act § 13 (Audit Trail)

**Requirement:** Maintain detailed audit trail of AI system operations and decisions

**Mapped Event Types:**
- `POLICY_EVALUATION` (5010) — AI policy/rule application
- `COMPLIANCE_CHECK_PASSED/FAILED` (5020, 5021) — Compliance with legal requirements
- `AUDIT_TRAIL_VERIFIED` (5041) — Periodic audit trail verification

**Evidence Collection:**
```cpp
collector.collectEvidence("EU_AI_ACT_13");  // Auto-collects policy evaluation events
```

**Retention:** Minimum 3 years (configurable per jurisdiction)

---

### SOC 2 CC 7.2 (System Monitoring)

**Requirement:** Monitor system activity to detect anomalies and support incident response

**Mapped Event Types:**
- `COMPLIANCE_CHECK_PASSED/FAILED` (5020, 5021)
- `COMPLIANCE_VIOLATION_DETECTED` (5022)
- `REMEDIATION_ACTION_TAKEN` (5023)
- `AUDIT_LOG_FAILURE` (5051)

**Evidence Collection:**
```cpp
collector.collectEvidence("SOC2_CC7.2");
```

**Retention:** Minimum 1 year

---

### ISO 27001 A.1 (Information Security Policy)

**Requirement:** Implement and enforce information security policies

**Mapped Event Types:**
- `POLICY_CREATED/UPDATED/ACTIVATED` (5011, 5012, 5014)
- `POLICY_DELETED` (5013)
- `COMPLIANCE_AUDIT_INITIATED/COMPLETED` (5024, 5025)

**Evidence Collection:**
```cpp
collector.collectEvidence("ISO27001_A1");
```

**Retention:** Minimum 5 years

---

### GDPR Article 32 (Security Measures)

**Requirement:** Implement and maintain security measures for personal data

**Mapped Event Types:**
- `DATA_MASKING_APPLIED` (5031)
- `DATA_CLASSIFICATION_CHANGED` (5033)
- `RESOURCE_ACCESSED` (5030)
- `REMEDIATION_ACTION_TAKEN` (5023)

**Evidence Collection:**
```cpp
collector.collectEvidence("GDPR_ART32");
```

**Retention:** 5+ years per data subject requests

---

## Performance Tuning

### Monitoring Logging Overhead

**Target:** Logging overhead <5% of operation duration

Monitor using performance metrics:

```cpp
auto& logger = getGlobalAuditLogger();
auto metrics = logger.getPerformanceMetrics();

std::cout << "Total operations logged: " << metrics.total_operations << std::endl;

// Check latency percentiles
auto stats = logger.getEventStatistics();
std::cout << "Logging latency p50: " << stats["logging_latency_p50_us"] << "us" << std::endl;
std::cout << "Logging latency p95: " << stats["logging_latency_p95_us"] << "us" << std::endl;
std::cout << "Logging latency p99: " << stats["logging_latency_p99_us"] << "us" << std::endl;
```

**Overhead Calculation:**

```
If operation takes 10ms (10000us):
- Allow max 500us (5%) for logging
- Monitor p99 to ensure consistency
- If p99 > 500us, adjust max_events or evidence collection frequency
```

### Tuning Circular Buffer Size

```cpp
// Create smaller logger for lower-latency scenarios
OperationalAuditLogger small_logger(10000);  // 10k event limit

// Or larger for retention/analysis
OperationalAuditLogger large_logger(500000);  // 500k event limit
```

**Memory per Event:** ~1KB (varies with context/payload size)  
**Formula:** `max_events * 1KB ≈ memory_usage`

### Query Performance

**Target:** Correlation queries <100ms for typical time ranges

Optimize queries:

```cpp
auto& logger = getGlobalAuditLogger();

// Fast: Indexed queries (typically 1-10ms)
auto by_actor = logger.queryEventsByActor("user@company.com");
auto by_module = logger.queryEventsByModule("policy_engine");
auto by_resource = logger.queryEventsByResource("resource_id");

// Medium: Time-range with binary search (typically 10-50ms)
auto by_time = logger.queryEventsByTimeRange(start_ms, end_ms);

// Slower: Correlation queries (typically 50-100ms)
auto by_corr = logger.queryEventsByCorrelationId("corr_id");
```

**Optimization Tips:**
1. Use actor/module/resource indices when possible
2. For time-range queries, use narrow windows
3. Batch correlation queries instead of repeated single-ID lookups

### Evidence Collection Performance

```cpp
// Collect evidence lazily (not on every event)
if (total_events % 1000 == 0) {
    collector.collectEvidence("EU_AI_ACT_13");
}

// Or on-demand via background job
scheduler.scheduleJob("collect_evidence", 
    []() { getGlobalEvidenceCollector().collectEvidence("SOC2_CC7.2"); },
    std::chrono::hours(1));
```

---

## Troubleshooting Guide

### Issue: Missing Events in Audit Trail

**Symptoms:** Queries return fewer events than expected

**Diagnosis:**
```cpp
auto total = logger.getTotalEventCount();
if (total < expected_count) {
    // Check 1: Circular buffer overflow?
    if (total == max_events_) {
        std::cerr << "Circular buffer full, oldest events evicted" << std::endl;
    }
    
    // Check 2: Logging failures?
    auto stats = logger.getEventStatistics();
    std::cout << "Logged events: " << stats["total_events"] << std::endl;
}
```

**Solutions:**
1. Increase `max_events` parameter in logger construction
2. Reduce event logging frequency if appropriate
3. Implement event export/archival for older events

### Issue: Correlation Queries Slow (>100ms)

**Symptoms:** Correlation queries exceed benchmark latency

**Diagnosis:**
```cpp
auto start = std::chrono::high_resolution_clock::now();
auto events = logger.queryEventsByCorrelationId("corr_id");
auto end = std::chrono::high_resolution_clock::now();
auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

if (latency_ms > 100) {
    std::cout << "Query returned " << events.size() << " events in " << latency_ms << "ms" << std::endl;
}
```

**Solutions:**
1. Reduce time window for time-range queries
2. Use actor/module queries instead of full correlation scans
3. Increase logging interval if events dominate
4. Consider adding additional indices for common query patterns

### Issue: Thread Contention/Lock Timeouts

**Symptoms:** Occasional logging failures under concurrent load

**Diagnosis:**
```cpp
// Monitor from multiple threads
std::thread query_thread([&]() {
    for (int i = 0; i < 1000; ++i) {
        auto events = logger.queryEventsByTimeRange(start, end);
    }
});

std::thread log_thread([&]() {
    for (int i = 0; i < 1000; ++i) {
        logger.logEvent(...);
    }
});
```

**Solutions:**
1. Implement read-write locking (if write-heavy)
2. Batch log entries before writing
3. Use separate logger instances per thread if possible
4. Monitor mutex wait times with performance profiler

### Issue: Evidence Fingerprint Mismatches

**Symptoms:** Integrity verification failures during audit

**Diagnosis:**
```cpp
auto evidence = collector.getEvidenceByRequirement("REQ_ID")[0];

// Recalculate fingerprint
std::string data_str = evidence.data_summary;
std::string recalc = computeSHA256(data_str);

if (recalc != evidence.fingerprint) {
    std::cerr << "Fingerprint mismatch!" << std::endl;
    std::cerr << "  Expected: " << evidence.fingerprint << std::endl;
    std::cerr << "  Got:      " << recalc << std::endl;
}
```

**Solutions:**
1. Verify data hasn't been modified post-collection
2. Check fingerprinting algorithm (ensure consistent SHA-256 usage)
3. Review evidence retention and archival processes
4. Audit access logs for evidence records

### Issue: Compliance Requirement Not Linked to Evidence

**Symptoms:** `exportEvidenceForAudit()` returns empty for requirement

**Diagnosis:**
```cpp
auto stats = collector.getEvidenceStatistics();
std::cout << "Statistics: " << stats.dump(4) << std::endl;

auto by_req = collector.getEvidenceByRequirement("MISSING_REQUIREMENT");
std::cout << "Evidence for requirement: " << by_req.size() << std::endl;
```

**Solutions:**
1. Verify requirement ID matches exactly
2. Check that `collectEvidence()` was called for requirement type
3. Ensure audit logger has events of required type
4. Manual evidence recording if automatic collection insufficient

### Emergency Audit Recovery

If audit system becomes unavailable:

```cpp
// Switch to fallback file-based logging
std::ofstream fallback_audit("fallback_audit.jsonl");

OperationalEvent event;
// ... populate event ...
fallback_audit << event.toJson().dump() << std::endl;

// Later, replay into audit logger
std::ifstream replay_file("fallback_audit.jsonl");
std::string line;
while (std::getline(replay_file, line)) {
    auto event_json = nlohmann::json::parse(line);
    auto event = OperationalEvent::fromJson(event_json);
    // Re-log into primary system
}
```

---

## References

- **RFC 3339**: Date and Time on the Internet (timestamp format)
- **OpenSSL EVP**: Cryptographic functions for fingerprinting
- **nlohmann/json**: JSON serialization library
- **ISO 27001**: Information Security Management Standard
- **SOC 2**: Service Organization Control Framework
- **EU AI Act**: European Union AI Act requirements
- **GDPR**: General Data Protection Regulation

---

## Document Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024-01-15 | Governance Team | Initial production release |

