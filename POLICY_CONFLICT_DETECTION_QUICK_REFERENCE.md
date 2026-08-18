# Policy Conflict Detection — Quick Reference Guide

**Version:** 0.1.0  
**Last Updated:** 2026-08-18  
**Status:** Production Ready

## Quick Start

### 1. Include Header
```cpp
#include "governance/policy_conflict_detector.h"
using namespace themis::governance;
```

### 2. Create Detector Instance
```cpp
auto detector = std::make_unique<PolicyConflictDetector>();
```

### 3. Add Rules Atomically
```cpp
PolicyRule rule = createMyRule();
auto result = detector->atomicAddRule(rule, policy_manager);

if (result.success) {
    LOG(INFO) << "Rule added: " << result.transaction_id;
} else {
    LOG(ERROR) << "Conflicts: " << result.error_message;
    for (const auto& conflict : result.conflicts_detected) {
        LOG(WARNING) << conflict.description;
    }
}
```

### 4. Evaluate Rule Precedence
```cpp
auto prec = detector->evaluateRulePrecedence("my_rule_id", policy_manager);
LOG(INFO) << "Effective priority: " << prec.effective_priority;
LOG(INFO) << "Rationale: " << prec.rationale;
```

### 5. Detect Conflicts in Existing Policies
```cpp
auto conflicts = detector->detectAllConflicts(policy_manager);
LOG(INFO) << "Total conflicts: " << conflicts.size();

for (const auto& conflict : conflicts) {
    LOG(WARNING) << conflict.toJson();
}
```

## API Reference

### Detection Functions

#### `detectAllConflicts()`
Comprehensive conflict analysis across entire policy set.
```cpp
std::vector<PolicyConflict> detectAllConflicts(const PolicyManager& policy_mgr);
```
**Time:** O(n²) where n = number of rules  
**Returns:** All detected conflicts (cached until policies change)  
**Use:** Initial policy validation, periodic audits

#### `detectPermitDenyConflicts()`
Find rules with contradictory allow/deny effects.
```cpp
std::vector<PolicyConflict> detectPermitDenyConflicts(const PolicyManager& policy_mgr);
```
**Time:** O(n²)  
**Returns:** PERMIT-DENY conflicts only  
**Use:** Critical conflict detection, security-focused analysis

#### `checkRuleConflict()`
Check two specific rules for conflicts.
```cpp
std::optional<PolicyConflict> checkRuleConflict(
    const PolicyRule& rule1, 
    const PolicyRule& rule2
);
```
**Time:** O(1) constant time  
**Returns:** Conflict if detected, empty optional otherwise  
**Use:** Pre-validation before adding rules

### Precedence Functions

#### `evaluateRulePrecedence()`
Compute priority for a single rule.
```cpp
PrecedenceEvaluation evaluateRulePrecedence(
    const std::string& rule_id, 
    const PolicyManager& policy_mgr
);
```
**Returns:** Priority score, overrides/overridden_by lists, rationale  
**Time:** O(n log n) with caching  
**Outputs:**
- `effective_priority`: Lower = higher priority
- `has_explicit_precedence`: Whether priority is explicit
- `overrides`: Rules this one takes precedence over
- `overridden_by`: Rules that take precedence over this one
- `rationale`: Explanation of priority calculation

#### `evaluateAllPrecedence()`
Compute precedence for entire policy set.
```cpp
std::unordered_map<std::string, PrecedenceEvaluation> 
evaluateAllPrecedence(const PolicyManager& policy_mgr);
```
**Returns:** Map of rule_id → PrecedenceEvaluation  
**Time:** O(n² log n)  
**Use:** Build decision trees, audit precedence chains

### Atomic Update Functions

#### `atomicAddRule()`
Add rule with conflict validation and rollback.
```cpp
AtomicUpdateResult atomicAddRule(
    const PolicyRule& rule,
    PolicyManager& policy_mgr
);
```
**Returns:** AtomicUpdateResult with:
- `success`: Whether operation succeeded
- `transaction_id`: Unique operation identifier
- `conflicts_detected`: Any conflicts found (if success=false)
- `error_message`: Description of failure
- `affected_rules`: Rules impacted by operation
- `operation_time_us`: Microseconds to complete

**Semantics:**
1. Add rule to manager
2. Run full conflict detection
3. If conflicts found → restore state, return FAILURE
4. Otherwise → commit and return SUCCESS

**Guarantees:**
- ✅ Atomicity: All-or-nothing
- ✅ Isolation: No intermediate states visible
- ✅ Consistency: All validation runs before commit
- ✅ Durability: Once committed, survives crashes

#### `atomicUpdateRule()`
Update existing rule with validation.
```cpp
AtomicUpdateResult atomicUpdateRule(
    const PolicyRule& rule,
    PolicyManager& policy_mgr
);
```
**Same semantics as atomicAddRule()**

#### `atomicRemoveRule()`
Remove rule, validating for orphaned dependencies.
```cpp
AtomicUpdateResult atomicRemoveRule(
    const std::string& rule_id,
    PolicyManager& policy_mgr
);
```

### Cache Functions

#### `getCachedConflicts()`
Retrieve previously computed conflicts.
```cpp
std::vector<PolicyConflict> getCachedConflicts(const PolicyManager& policy_mgr) const;
```
**Time:** <10µs (direct lookup)  
**Cache Key:** PolicyManager address  
**Expiry:** Automatic on any policy modification

#### `clearCache()`
Invalidate all cached results.
```cpp
void clearCache();
```
**Use:** After bulk policy updates

#### `setCachingEnabled()`
Enable/disable caching.
```cpp
void setCachingEnabled(bool enabled);
```

## Data Structures

### ConflictType (Enum)
```cpp
enum class ConflictType {
    PERMIT_DENY,           // Allow vs Deny contradiction
    OVERLAPPING,           // Ambiguous precedence
    CIRCULAR_DEPENDENCY,   // Policy cycles
    TYPE_MISMATCH,         // Incompatible types
    ENCRYPTION_CONFLICT,   // Encryption requirements differ
    EXPORT_CONFLICT,       // Export permissions differ
    RETENTION_CONFLICT,    // Retention periods differ
    COMPLIANCE_CONFLICT    // Cross-framework conflicts
};
```

### ConflictSeverity (Enum)
```cpp
enum class ConflictSeverity {
    LOW,       // Informational only
    MEDIUM,    // May cause unexpected behavior
    HIGH,      // Direct security impact
    CRITICAL   // System cannot function
};
```

### PolicyConflict (Struct)
```cpp
struct PolicyConflict {
    std::string conflict_id;                      // Unique identifier
    ConflictType conflict_type;                   // Type of conflict
    std::vector<std::string> conflicting_rule_ids; // Involved rules
    std::string description;                      // Human-readable description
    ConflictSeverity severity;                    // Severity level
    std::string resolution_strategy;              // Recommended fix
    int64_t detected_at;                          // Detection timestamp
    
    nlohmann::json toJson() const;  // Serialize to JSON
};
```

### PrecedenceEvaluation (Struct)
```cpp
struct PrecedenceEvaluation {
    std::string rule_id;                      // Evaluated rule
    int effective_priority;                   // Computed priority
    bool has_explicit_precedence;             // Explicitly set?
    std::vector<std::string> overrides;       // Rules this overrides
    std::vector<std::string> overridden_by;   // Rules that override this
    std::string rationale;                    // Calculation explanation
    
    nlohmann::json toJson() const;  // Serialize to JSON
};
```

### AtomicUpdateResult (Struct)
```cpp
struct AtomicUpdateResult {
    bool success;                             // Operation succeeded?
    std::string transaction_id;               // Operation identifier
    std::vector<PolicyConflict> conflicts_detected; // Found conflicts
    std::string error_message;                // Failure details
    std::vector<std::string> affected_rules;  // Impacted rules
    int64_t operation_time_us;                // Time in microseconds
};
```

## Common Patterns

### Pattern 1: Validate Policy Before Deployment
```cpp
auto detector = std::make_unique<PolicyConflictDetector>();
auto conflicts = detector->detectAllConflicts(policy_manager);

if (!conflicts.empty()) {
    for (const auto& conflict : conflicts) {
        if (conflict.severity == ConflictSeverity::CRITICAL ||
            conflict.severity == ConflictSeverity::HIGH) {
            throw std::runtime_error("Critical conflict: " + conflict.description);
        }
    }
}
```

### Pattern 2: Safe Rule Addition with Rollback
```cpp
auto result = detector->atomicAddRule(new_rule, policy_manager);

if (!result.success) {
    // Already rolled back automatically
    std::cerr << "Failed to add rule: " << result.error_message << std::endl;
    return false;
}

// Rule is safely committed
return true;
```

### Pattern 3: Priority-Based Rule Ordering
```cpp
auto all_precedence = detector->evaluateAllPrecedence(policy_manager);

std::vector<std::pair<std::string, int>> rule_priorities;
for (const auto& [rule_id, prec] : all_precedence) {
    rule_priorities.emplace_back(rule_id, prec.effective_priority);
}

// Sort by priority (lower = higher)
std::sort(rule_priorities.begin(), rule_priorities.end(),
          [](const auto& a, const auto& b) {
              return a.second < b.second;
          });

// Now rule_priorities is sorted by precedence
```

### Pattern 4: Audit Trail of Conflicts
```cpp
auto detector = std::make_unique<PolicyConflictDetector>();
auto conflicts = detector->detectAllConflicts(policy_manager);

for (const auto& conflict : conflicts) {
    audit_log->write(AuditEntry{
        .severity = conflict.severity,
        .event_type = "POLICY_CONFLICT",
        .description = conflict.description,
        .resolution = conflict.resolution_strategy,
        .timestamp = std::chrono::system_clock::now()
    });
}
```

## Performance Characteristics

### Latency (50 rules, 100 iterations)
| Operation | p50 | p99 | Target |
|-----------|-----|-----|--------|
| evaluateRulePrecedence() | 15µs | 50µs | ≤100µs ✅ |
| detectPermitDenyConflicts() | 150µs | 300µs | — |
| detectAllConflicts() | 200µs | 500µs | — |
| getCachedConflicts() | <1µs | 10µs | — |

### Memory Usage
| Component | Size |
|-----------|------|
| Per rule metadata | ~500 bytes |
| Conflict cache (empty) | <1 KB |
| Per conflict entry | ~200 bytes |

### Scalability
- **Linear in rules:** Memory usage O(n)
- **Quadratic in rules:** Conflict detection O(n²)
- **Quadratic in rules:** Precedence evaluation O(n²) with caching
- **Cached results:** Reuse until policies change

## Thread Safety

**Thread-Safe:** ✅ All public methods are thread-safe

**Concurrency Model:**
- Multiple threads can read simultaneously
- Writes are serialized
- No external synchronization required

**Example:**
```cpp
auto detector = std::make_shared<PolicyConflictDetector>();

// Safe: Multiple threads reading
std::thread t1([&] {
    auto conflicts = detector->detectAllConflicts(policy_manager);
});
std::thread t2([&] {
    auto prec = detector->evaluateRulePrecedence("rule1", policy_manager);
});
t1.join();
t2.join();

// Safe: Sequential writes automatically serialized
auto result1 = detector->atomicAddRule(rule1, policy_manager);
auto result2 = detector->atomicAddRule(rule2, policy_manager);
```

## Troubleshooting

### Issue: Unexpected Conflicts on Policy Load
**Possible Cause:** Policy file contains rules added in different order than expected

**Solution:** 
```cpp
// Check precedence ordering
auto all_prec = detector->evaluateAllPrecedence(policy_manager);
for (const auto& [id, prec] : all_prec) {
    std::cout << id << ": " << prec.effective_priority << std::endl;
}
```

### Issue: atomicAddRule() Fails with "Conflict Detected"
**Possible Cause:** New rule conflicts with existing rules

**Solution:**
```cpp
auto result = detector->atomicAddRule(rule, policy_manager);
if (!result.success) {
    for (const auto& conflict : result.conflicts_detected) {
        std::cout << "Conflict: " << conflict.description << std::endl;
        std::cout << "Recommendation: " << conflict.resolution_strategy << std::endl;
    }
    // Adjust rule and retry
}
```

### Issue: Cache Hit Rate Low
**Possible Cause:** Policy manager changing frequently

**Solution:**
```cpp
// Disable caching during bulk updates
detector->setCachingEnabled(false);
for (const auto& rule : many_rules) {
    detector->atomicAddRule(rule, policy_manager);
}
detector->setCachingEnabled(true);
detector->clearCache();  // Force refresh on next call
```

## Testing

### Run Policy Conflict Tests
```bash
cd build
ctest -R "PolicyConflictDetection" -V
```

### Run Benchmark Tests
```bash
cd build
ctest -R "ConflictDetectionBenchmark" -V
```

## FAQ

**Q: What's the difference between overlapping and PERMIT-DENY conflicts?**  
A: PERMIT-DENY conflicts have contradictory *effects* (one allows, one denies).  
Overlapping conflicts have ambiguous precedence (both allow, but unclear which applies).

**Q: Does Deny-Overrides-Permit mean deny always wins?**  
A: No, explicit priority still matters. But when priorities are equal, deny wins.  
This is security-first: denies are harder to override accidentally.

**Q: Can I disable conflict detection for performance?**  
A: You can skip `atomicAddRule()` and use `policy_manager->addRule()` directly,  
but you lose consistency guarantees. Not recommended for production.

**Q: How are ties broken in precedence?**  
A: Creation timestamp (older rules win). This ensures deterministic results  
and makes precedence reproducible across restarts.

**Q: What happens if policy_manager changes during detection?**  
A: The detector sees a snapshot at detection time. Concurrent modifications  
are isolated and don't cause race conditions.

## References

- **XACML 3.0:** https://docs.oasis-open.org/xacml/3.0/
- **AWS IAM Policies:** https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_evaluation-logic.html
- **OPA (Open Policy Agent):** https://www.openpolicyagent.org/docs/latest/
- **Design Document:** `src/governance/POLICY_CONFLICT_DETECTION_DESIGN.md`
- **Implementation Report:** `POLICY_CONFLICT_DETECTION_IMPLEMENTATION_REPORT.md`

---

**Last Updated:** 2026-08-18  
**Version:** 0.1.0  
**Status:** Production Ready
