# Policy Versioning and Change Management Guide

## Overview

This guide covers policy versioning, rollback procedures, and approval workflows for ThemisDB's governance module. All policies maintain complete version history with semantic versioning, enabling safe rollback and comprehensive audit trails.

## Table of Contents

1. [Version Tracking](#version-tracking)
2. [Semantic Versioning Scheme](#semantic-versioning-scheme)
3. [Rollback Procedures](#rollback-procedures)
4. [Approval Workflow](#approval-workflow)
5. [Safety Verification](#safety-verification)
6. [Audit Trail](#audit-trail)
7. [Emergency Procedures](#emergency-procedures)
8. [Performance Benchmarks](#performance-benchmarks)

---

## Version Tracking

### Overview

Every policy rule maintains a complete history of all versions. Each version includes:
- **Semantic version number** (major.minor.patch)
- **Complete rule snapshot** (JSON serialization)
- **Author information** (who made the change)
- **Timestamp** (when the change occurred)
- **Change description** (why the change was made)

### Creating a Policy Version

Versions are automatically created when:
1. A new policy rule is created (version 1.0.0)
2. An existing policy rule is updated
3. A policy is explicitly rolled back to a previous version

### Version Structure

```json
{
  "version": "1.2.3",
  "rule_id": "encryption-policy-v2",
  "author": "security-admin@example.com",
  "timestamp": 1692547200000,
  "change_description": "Increased key rotation frequency from 90 to 60 days",
  "rule_snapshot": {
    "id": "encryption-policy-v2",
    "name": "Corporate Encryption Standard",
    "require_encryption": true,
    "retention_days": 365,
    "priority": 5,
    "...": "..."
  }
}
```

### Querying Version History

```cpp
// Get all versions of a rule (newest first)
auto versions = version_history->getVersions("rule-id");

// Get a specific version
auto v = version_history->getVersion("rule-id", "1.2.3");

// Get the latest version number
auto latest = version_history->getLatestVersion("rule-id");

// Get the previous version
auto prev = version_history->getPreviousVersion("rule-id");

// Compare two versions
auto diff = version_history->compareVersions("rule-id", "1.2.2", "1.2.3");
```

---

## Semantic Versioning Scheme

ThemisDB follows semantic versioning (MAJOR.MINOR.PATCH):

### MAJOR Version Changes
- **Trigger:** Breaking changes to policy enforcement logic
- **Example:** Changing from allow-by-default to deny-by-default
- **Impact:** Requires explicit approval and full validation
- **Increment:** X.0.0

### MINOR Version Changes
- **Trigger:** New features or non-breaking enhancements
- **Example:** Adding new encryption requirement
- **Impact:** Standard approval process
- **Increment:** X.Y+1.0

### PATCH Version Changes
- **Trigger:** Bug fixes or parameter adjustments
- **Example:** Adjusting retention period
- **Impact:** Quick approval process
- **Increment:** X.Y.Z+1

### Version Advancement Rules

```
1.0.0 (initial) → 1.0.1 (bug fix) → 1.1.0 (new feature) → 2.0.0 (breaking change)
```

**Never:**
- Skip version numbers
- Go backwards (e.g., 2.0.0 → 1.9.9)
- Use non-numeric versions

---

## Rollback Procedures

### Single-Policy Rollback

Rollback a single policy to a previous version:

```cpp
// Check safety before rollback
auto safety = change_manager->checkRollbackSafety("rule-id", "1.0.0");
if (safety.safety_level == RollbackSafetyLevel::BLOCKED) {
    // Cannot proceed - investigate conflicts
    return false;
}

// Preview changes without applying
auto preview = change_manager->previewRollback("rule-id", "1.0.0");
std::cout << "Estimated impact: " << preview.warnings.size() << " warnings\n";

// Perform atomic rollback
auto op = change_manager->performRollback(
    "rule-id",
    "1.0.0",              // target version
    "operator-user@example.com",
    "Reverting failed update - Issue #12345"
);

if (op.success) {
    std::cout << "Rollback completed in " << (op.completed_at - op.started_at) << "ms\n";
} else {
    std::cout << "Rollback failed: " << op.error_message << "\n";
}
```

### Multi-Policy Coordinated Rollback

Rollback multiple related policies atomically:

```cpp
std::vector<std::string> rules = {
    "policy-encryption",
    "policy-retention",
    "policy-audit"
};

auto op = change_manager->performCoordinatedRollback(
    rules,
    "1.0.0",              // all policies rollback to this version
    "operations-lead@example.com",
    "Coordinated rollback: Production incident response"
);

// All rules are rolled back atomically - either all succeed or all fail
if (op.success) {
    for (const auto& rule_id : op.multi_rule_ids) {
        std::cout << "Rolled back: " << rule_id << "\n";
    }
}
```

### Rollback to Previous Version

Quick rollback to immediately previous version:

```cpp
auto op = change_manager->rollbackToPrevious(
    "rule-id",
    "operator@example.com",
    "Undoing last change"
);
```

### Safety Checks During Rollback

Before any rollback operation, the system automatically checks:

1. **Version Existence:** Target version must be recorded in history
2. **Circular Dependencies:** No circular policy dependencies
3. **Affected Rules:** Identifies all rules that depend on this policy
4. **Conflict Detection:** Ensures rollback doesn't create policy conflicts
5. **Atomicity:** Verifies rollback can be applied as single unit

Safety levels:
- **SAFE:** Rollback can proceed without issues
- **WARNING:** Rollback has minor concerns but can proceed
- **BLOCKED:** Rollback cannot proceed - must resolve issues first

### Rollback Latency Requirements

- **Single-policy rollback:** ≤500ms target
- **Version query:** ≤10ms target
- **Multi-policy coordination:** ≤500ms base + 10ms per additional policy

Monitor rollback performance:

```cpp
auto start = std::chrono::high_resolution_clock::now();
auto op = change_manager->performRollback(...);
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::high_resolution_clock::now() - start
).count();

if (duration > 500) {
    LOG_WARNING("Slow rollback: {}ms", duration);
}
```

### Reversing a Rollback

If a rollback was applied incorrectly, you can reverse it:

```cpp
auto reverse_op = change_manager->reverseRollback(
    "operation-id-from-original-rollback",
    "operator@example.com"
);

if (reverse_op) {
    std::cout << "Rollback reversed successfully\n";
} else {
    std::cout << "Cannot reverse this rollback\n";
}
```

---

## Approval Workflow

### State Machine Overview

All policy changes follow this approval state machine:

```
DRAFT → REVIEW → APPROVED → ACTIVE
          ↓
        DRAFT (rejected)

ACTIVE → DEPRECATED (rollback)
```

### State Descriptions

| State | Purpose | Duration | Actions |
|-------|---------|----------|---------|
| **DRAFT** | Initial policy creation | Hours | Create, edit, submit for review |
| **REVIEW** | Awaiting approver review | Hours to days | Review, approve, reject |
| **APPROVED** | Approved but not active | Minutes | Activate, return to review |
| **ACTIVE** | Actively enforced | Indefinite | Modify (creates new version), deprecate |
| **DEPRECATED** | Archived after rollback | Indefinite | Keep for audit, no enforcement |

### Creating and Submitting for Review

```cpp
// Initiate a new approval workflow
auto status = workflow->initiateReview(
    "encryption-policy-v3",
    "1.0.0",                 // version being approved
    "alice@security.com",    // who submitted it
    1                        // required approvers (1 for most policies)
);
// status.current_state == ApprovalState::DRAFT

// Submit for review
workflow->submitForReview("encryption-policy-v3", "alice@security.com");
// Now in REVIEW state, awaiting approver
```

### Approval Process

```cpp
// Approver reviews and approves
bool approved = workflow->approveChange(
    "encryption-policy-v3",
    "bob@security.com",      // approver identity
    "LGTM - encryption standards compliant"  // approval comment
);

// With multiple required approvers (e.g., 3 for critical policies)
workflow->approveChange("encryption-policy-v3", "bob@security.com", "OK");
workflow->approveChange("encryption-policy-v3", "charlie@security.com", "Approved");
workflow->approveChange("encryption-policy-v3", "dave@security.com", "ACK");
// After 3rd approval, state transitions to APPROVED automatically
```

### Rejecting a Policy

```cpp
// If approver rejects, policy returns to DRAFT
workflow->rejectChange(
    "encryption-policy-v3",
    "bob@security.com",
    "Conflicts with existing policy on key rotation"
);
// State transitions back to DRAFT for revisions
```

### Activating an Approved Policy

```cpp
// After approval, operator can activate
workflow->activatePolicy(
    "encryption-policy-v3",
    "operator@platform.com"
);
// Policy now ACTIVE and enforced
```

### Querying Approval Status

```cpp
// Get current status of a policy
auto status = workflow->getApprovalStatus("encryption-policy-v3");
if (status) {
    std::cout << "State: " << static_cast<int>(status->current_state) << "\n";
    std::cout << "Approved by: " << status->approved_by << "\n";
    std::cout << "Activated at: " << status->activated_at << "\n";
}

// Get all rules in a specific state
auto draft_policies = workflow->getRulesInState(ApprovalState::DRAFT);
auto review_policies = workflow->getRulesInState(ApprovalState::REVIEW);
auto active_policies = workflow->getRulesInState(ApprovalState::ACTIVE);

// Get pending approvals for a specific approver
auto pending = workflow->getPendingApprovalsFor("bob@security.com");
std::cout << "Pending approvals for Bob: " << pending.size() << "\n";
```

---

## Safety Verification

### Dependency Verification

Policies can have dependencies. Before rollback or approval, verify dependencies:

```cpp
// Register a dependency
change_manager->registerDependency(
    "policy-child",        // dependent policy
    "policy-parent",       // policy being depended on
    "inheritance",         // dependency type
    "Child inherits encryption requirements from parent"
);

// Get dependencies (policies this rule depends on)
auto deps = change_manager->getDependencies("policy-child");

// Get reverse dependencies (policies that depend on this rule)
auto rev_deps = change_manager->getReverseDependencies("policy-parent");
```

### Conflict Detection

```cpp
// Policies are checked for conflicts
auto versioning_mgr = std::make_unique<PolicyManagerWithVersioning>(policy_manager);

// Check for conflicts before adding/updating a rule
std::vector<PolicyRule> test_rules = { /* candidate rules */ };
for (const auto& rule : test_rules) {
    auto conflicts = versioning_mgr->checkConflictsForRule(rule);
    if (!conflicts.empty()) {
        std::cout << "Conflict detected: " << conflicts[0].description << "\n";
    }
}

// Get current conflict state
auto all_conflicts = versioning_mgr->getActiveConflicts();
```

### Pre-Rollback Verification

```cpp
auto safety = change_manager->checkRollbackSafety("rule-id", "1.0.0");

if (safety.safety_level == RollbackSafetyLevel::SAFE) {
    std::cout << "Rollback is safe to proceed\n";
} else if (safety.safety_level == RollbackSafetyLevel::WARNING) {
    for (const auto& warning : safety.warnings) {
        std::cout << "Warning: " << warning << "\n";
    }
    // Can proceed, but should review warnings
} else {
    std::cout << "Rollback is BLOCKED\n";
    for (const auto& warning : safety.warnings) {
        std::cout << "Issue: " << warning << "\n";
    }
    return false;
}
```

---

## Audit Trail

### Audit Trail Components

Every policy change is recorded in an immutable audit trail:

```json
{
  "rule_id": "encryption-policy-v2",
  "operation": "update",
  "user": "alice@security.com",
  "timestamp": 1692547200000,
  "old_version": "1.0.0",
  "new_version": "1.0.1",
  "details": {
    "change_description": "Updated retention from 90 to 60 days",
    "fields_changed": ["retention_days"],
    "reason": "Cost optimization initiative"
  }
}
```

### Querying the Audit Trail

```cpp
// Query by rule ID
auto rule_audit = version_history->queryAudit("encryption-policy-v2");

// Query by user
auto user_audit = version_history->queryAudit(
    std::nullopt,  // no rule filter
    "alice@security.com"
);

// Query by time range (Unix timestamps in milliseconds)
auto week_ago = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
).count() - (7 * 24 * 60 * 60 * 1000);

auto recent = version_history->queryAudit(
    std::nullopt,    // any rule
    std::nullopt,    // any user
    week_ago,        // start time
    std::nullopt     // no end time limit
);

// Approval audit trail
auto approval_history = workflow->queryApprovalHistory(
    "encryption-policy-v2"
);
for (const auto& record : approval_history) {
    std::cout << "Action: " << static_cast<int>(record.action) 
              << " by " << record.approver << "\n";
}
```

### Audit Trail Compliance

The audit trail satisfies compliance requirements:

- **SOC 2 Type II:** Complete change tracking with user accountability
- **ISO 27001:** Change management and authorization records
- **HIPAA:** Audit trails for regulated data policies
- **GDPR:** Data subject rights tracking
- **PCI DSS:** Access control and approval records

---

## Emergency Procedures

### Emergency Override

For critical security situations requiring immediate policy activation without full approval:

```cpp
workflow->emergencyOverride(
    "critical-security-patch-policy",
    "ciso@security.com",
    "CRITICAL: Zero-day vulnerability requires immediate protection",
    1  // required approvers (will be checked retroactively)
);
// Policy is immediately ACTIVE
// Override is recorded in audit trail with special flag
```

**Emergency override requirements:**
1. Must be authorized by security leadership
2. Reason must be documented
3. Retroactive approvals must be obtained within 24 hours
4. All emergency overrides are flagged in audit trail
5. Requires post-incident review

### Incident Response Rollback

Quick rollback for production incidents:

```cpp
// Step 1: Analyze impact
auto preview = change_manager->previewRollback("production-policy", "stable-version");

// Step 2: Get approval (can be verbal with incident commander)
// Step 3: Execute rollback
auto op = change_manager->performRollback(
    "production-policy",
    "stable-version",
    "incident-commander@platform.com",
    "INC-00123: Production rollback per incident response"
);

if (op.success) {
    std::cout << "Policy rolled back successfully\n";
    // Step 4: Notify stakeholders
    // Step 5: Log incident for review
}
```

### Escalation Path

For blocks that prevent needed changes:

1. **Contact:** Policy governance team (governance@security.com)
2. **Provide:** Reason, business impact, timeline
3. **Review:** Security team evaluates request
4. **Approval:** If justified, can override safety checks
5. **Documentation:** All exceptions documented

---

## Performance Benchmarks

### Latency Targets

| Operation | Target | Maximum | Notes |
|-----------|--------|---------|-------|
| Record version | <10ms | 50ms | Automatic on any change |
| Get latest version | <10ms | 50ms | Frequently queried |
| Compare versions | <50ms | 200ms | Used for previews |
| Single rollback | <500ms | 1s | Common operation |
| Multi-policy rollback | <500ms + 10ms/rule | 2s | Coordinated atomic |
| Approval state transition | <50ms | 200ms | Synchronous operation |
| Query audit trail | <100ms | 500ms | Depends on query filters |

### Monitoring and Alerting

Monitor these metrics:

```cpp
// Log slow operations
if (op.completed_at - op.started_at > 500) {
    METRICS.increment("rollback.slow", 1);
    LOG_WARNING("Slow rollback: {} ms", op.completed_at - op.started_at);
}

// Track approval cycle time
auto cycle_time = status.activated_at - status.submitted_at;
METRICS.observe("approval.cycle_time_ms", cycle_time);

// Monitor version history size
auto total_versions = version_history->getTotalVersionCount();
if (total_versions > 100000) {
    LOG_WARNING("Large version history: {} entries", total_versions);
}
```

### Scalability Considerations

- **Policies:** Supports 10,000+ active policies
- **Versions:** Each policy can have 100+ versions
- **Rollback concurrency:** Can handle 10+ concurrent rollbacks
- **Audit trail:** Efficiently stores millions of entries

---

## Operational Runbooks

### Routine Operations

**Creating a new policy:**
```
1. Draft policy in DRAFT state
2. Submit for review (DRAFT → REVIEW)
3. Approver reviews and approves (REVIEW → APPROVED)
4. Operator activates policy (APPROVED → ACTIVE)
5. Policy is now enforced
```

**Updating a policy:**
```
1. Modify policy - creates new patch version (1.0.0 → 1.0.1)
2. Submit for review
3. Approver reviews changes and approves
4. New version is activated
5. Old version retained in history
```

**Rolling back a policy:**
```
1. Check safety: change_manager->checkRollbackSafety()
2. Preview changes: change_manager->previewRollback()
3. Get approval from operations lead
4. Execute: change_manager->performRollback()
5. Verify enforcement changed
6. Document rollback reason
```

---

## Troubleshooting

### Rollback Blocked

**Problem:** `RollbackSafetyLevel::BLOCKED`

**Solutions:**
1. Check `safety_report.warnings` for specific issues
2. Resolve circular dependencies with policy team
3. Update dependent policies first
4. Contact governance team for exception (if justified)

### Approval Stuck in REVIEW

**Problem:** Policy in REVIEW state for extended time

**Solutions:**
1. Check `workflow->getPendingApprovalsFor(approver_name)`
2. Send reminder to assigned approvers
3. Escalate if approver unavailable
4. For critical policies, use emergency override

### Version Query Slow

**Problem:** `getLatestVersion()` exceeds 10ms target

**Solutions:**
1. Check system load and resource availability
2. Review version history size (`getTotalVersionCount()`)
3. Archive old versions if excessive
4. Optimize query with caching if repeating

---

## Related Documentation

- [Policy Manager Architecture](./ARCHITECTURE.md)
- [Policy Validation Guide](./VALIDATION.md)
- [Compliance and Audit](./AUDIT.md)
- [Security Best Practices](./SECURITY.md)
