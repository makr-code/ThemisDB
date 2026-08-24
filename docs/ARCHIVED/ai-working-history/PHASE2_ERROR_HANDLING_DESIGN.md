# Phase 2: Unified Diagnostic System Implementation Plan

**Purpose:** Standardize error handling and diagnostics across all ThemisDB modules  
**Dependencies:** Phase 1 completion (security + resource management fixes)  
**Target Start:** After Phase 1 agents complete  
**Estimated Duration:** 6-8 hours

---

## Scope Summary

Phase 2 consolidates the fragmented error reporting patterns across ThemisDB into a unified diagnostic system with:
- Standardized error taxonomy
- Unified error classification
- Diagnostic aggregation capabilities
- Operator-facing error messages with actionable remediation

---

## Unified Error Taxonomy Design

### Error Classification Hierarchy

```
ThemisDBError (base)
├── SecurityError
│   ├── SignatureValidationError
│   ├── LicenseValidationError
│   ├── AccessDeniedError
│   └── IntegrityViolationError
├── WireProtocolError
│   ├── MessageFormatError
│   ├── ConnectionError
│   ├── TimeoutError
│   └── ProtocolVersionMismatchError
├── ResourceError
│   ├── MemoryError
│   ├── FileError
│   ├── ConnectionPoolError
│   └── TimeoutError
├── DataError
│   ├── ConsistencyError
│   ├── VersionMismatchError
│   ├── SchemaViolationError
│   └── IntegrityError
├── OperationalError
│   ├── HardwareError
│   ├── ConfigurationError
│   ├── DependencyError
│   └── InternalError
└── DiagnosticError
    └── AggregationError
```

---

## Implementation Tasks

### Task 1: Define Unified Error Base Class

**File:** `include/utils/unified_error_taxonomy.h`

```cpp
// Error severity levels
enum class ErrorSeverity {
    DEBUG,      // Non-operational diagnostics
    INFO,       // Informational (no action required)
    WARNING,    // Degraded operation but functional
    ERROR,      // Operation failed, recovery possible
    CRITICAL    // System-level failure or security risk
};

// Error category for routing/filtering
enum class ErrorCategory {
    SECURITY,
    WIRE_PROTOCOL,
    RESOURCE,
    DATA,
    OPERATIONAL
};

// Base unified error class
class ThemisDBError : public std::exception {
public:
    ThemisDBError(
        ErrorSeverity severity,
        ErrorCategory category,
        uint32_t error_code,
        const std::string& message,
        const std::string& remediation = ""
    );
    
    ErrorSeverity severity() const;
    ErrorCategory category() const;
    uint32_t error_code() const;
    const char* what() const noexcept override;
    const std::string& remediation() const;
    
    // Diagnostic context
    void add_context(const std::string& key, const std::string& value);
    const std::map<std::string, std::string>& context() const;
};
```

---

### Task 2: Consolidate License/Verify/Wire Error Codes

**Files to Unify:**
- `include/themis/license_info.h` errors
- `include/security/module_signature_verifier.h` errors
- `include/network/wire_protocol_server.h` errors

**Action:** Map all existing error codes to unified taxonomy

```cpp
// Unified error code ranges
constexpr uint32_t LICENSE_ERROR_BASE = 9420;      // 9420-9439
constexpr uint32_t SIGNATURE_ERROR_BASE = 9440;    // 9440-9459
constexpr uint32_t WIRE_PROTOCOL_ERROR_BASE = 9460; // 9460-9479
```

---

### Task 3: Implement Diagnostic Aggregator

**File:** `include/observability/diagnostic_aggregator.h`

```cpp
class DiagnosticAggregator {
public:
    // Record a diagnostic event
    void record_error(
        const ThemisDBError& error,
        const std::string& component,
        const std::string& operation
    );
    
    // Query aggregated diagnostics
    std::vector<DiagnosticEvent> get_errors_by_category(ErrorCategory cat);
    std::vector<DiagnosticEvent> get_errors_by_severity(ErrorSeverity sev);
    std::vector<DiagnosticEvent> get_recent_errors(std::chrono::seconds window);
    
    // Export diagnostic summary
    std::string export_summary() const;
    json export_json() const;
};
```

---

### Task 4: Create Operator-Facing Error Classification

**File:** `include/observability/operator_error_guide.h`

Maps errors to operator actions:

```cpp
struct OperatorGuidance {
    ErrorSeverity severity;
    std::string classification;  // e.g., "TRANSIENT", "PERMANENT", "SECURITY"
    std::string recommended_action; // e.g., "RETRY", "ESCALATE", "INVESTIGATE"
    int retry_attempts;
    std::chrono::seconds retry_delay;
    std::string doc_reference;   // Link to runbook/troubleshooting guide
};

OperatorGuidance classify_error(const ThemisDBError& error);
```

---

### Task 5: Add Actionable Error Messages

**File:** `src/observability/error_message_formatter.cpp`

```cpp
class ErrorMessageFormatter {
public:
    // Format error for operator/developer consumption
    std::string format_for_operator(const ThemisDBError& error);
    std::string format_for_developer(const ThemisDBError& error);
    std::string format_for_logs(const ThemisDBError& error);
    
    // Include remediation guidance
    std::string format_with_guidance(const ThemisDBError& error);
};

// Example output:
// [WIRE_PROTOCOL] Connection timeout (code 9475)
// Severity: WARNING
// Details: Connection to replica 192.168.1.5:3001 timed out after 30s
// Classification: TRANSIENT - likely network congestion
// Recommended Action: RETRY - exponential backoff, max 3 attempts
// Retry Config: delay=1s, backoff_factor=2
// Documentation: https://themisdb.docs/troubleshooting/connection-timeout
```

---

## Error Codes Mapping

### License Errors (9420-9439)
| Code | Current | Mapped To | New Error |
|------|---------|-----------|-----------|
| 9420 | LICENSE_INVALID | SecurityError | LicenseValidationError |
| 9421 | LICENSE_EXPIRED | SecurityError | LicenseValidationError |
| 9422 | LICENSE_SIGNATURE_FAIL | SecurityError | SignatureValidationError |
| ... | | | |

### Signature Verification (9440-9459)
| Code | Current | Mapped To | New Error |
|------|---------|-----------|-----------|
| 9440 | SIG_INVALID | SecurityError | SignatureValidationError |
| 9441 | SIG_MISMATCH | SecurityError | IntegrityViolationError |
| ... | | | |

### Wire Protocol (9460-9479)
| Code | Current | Mapped To | New Error |
|------|---------|-----------|-----------|
| 9460 | WIRE_FORMAT_ERROR | WireProtocolError | MessageFormatError |
| 9461 | WIRE_TIMEOUT | WireProtocolError | TimeoutError |
| 9462 | WIRE_VERSION_MISMATCH | WireProtocolError | ProtocolVersionMismatchError |
| ... | | | |

---

## Backward Compatibility

### Strategy
1. Keep existing error codes for compatibility (Phase 2)
2. Map old codes to new unified taxonomy
3. Add deprecation notices in headers
4. Provide migration guide for downstream code
5. Plan removal for v2.5.0 (Phase 3)

### Example Mapping Function
```cpp
ThemisDBError map_legacy_error(uint32_t legacy_code, const std::string& msg) {
    switch (legacy_code) {
        case OLD_LICENSE_ERROR_1:
            return ThemisDBError(
                ErrorSeverity::ERROR,
                ErrorCategory::SECURITY,
                9420,  // New unified code
                msg,
                "License validation failed. Review license file and system clock."
            );
        // ... more mappings
        default:
            return ThemisDBError(
                ErrorSeverity::WARNING,
                ErrorCategory::OPERATIONAL,
                99999,  // Unknown
                msg
            );
    }
}
```

---

## Testing Requirements

### Unit Tests
- [ ] Error creation and serialization
- [ ] Error code mapping correctness
- [ ] Context addition and retrieval
- [ ] Message formatting for all error types

### Integration Tests
- [ ] Unified errors propagate correctly across module boundaries
- [ ] Diagnostic aggregator collects all categories
- [ ] Operator guidance maps all error codes
- [ ] Backward compatibility with legacy error codes

### System Tests
- [ ] License validation errors → unified taxonomy
- [ ] Signature verification errors → unified taxonomy
- [ ] Wire protocol errors → unified taxonomy
- [ ] Multi-module error flows maintain integrity

---

## Acceptance Criteria

- [ ] Unified error base class defined and documented
- [ ] License/verify/wire error codes consolidated
- [ ] Diagnostic aggregator fully functional
- [ ] Operator guidance covers all error types
- [ ] Actionable error messages implemented
- [ ] All existing error patterns migrated (100% module coverage)
- [ ] Backward compatibility maintained for v2.4.0
- [ ] All tests passing
- [ ] Documentation complete

---

## Files to Create/Modify

### New Files
- [ ] `include/utils/unified_error_taxonomy.h`
- [ ] `include/observability/diagnostic_aggregator.h`
- [ ] `include/observability/operator_error_guide.h`
- [ ] `src/observability/error_message_formatter.cpp`
- [ ] `src/observability/diagnostic_aggregator.cpp`
- [ ] `tests/observability/test_unified_error_taxonomy.cpp`

### Files to Modify
- [ ] `include/themis/license_info.h` (add backward compat mapping)
- [ ] `include/security/module_signature_verifier.h` (add mapping)
- [ ] `include/network/wire_protocol_server.h` (add mapping)
- [ ] `include/utils/error_registry.h` (extend with unified codes)

---

## Success Metrics

- **Error Code Standardization:** 100% of modules use unified taxonomy
- **Diagnostic Coverage:** All error types captured by aggregator
- **Operator Guidance:** 100% of error codes have remediation guidance
- **Documentation:** All errors documented with examples
- **Backward Compatibility:** Zero breaking changes in v2.4.0

---

**Status:** Ready for Agent 4 (Error Handling Specialist) implementation  
**Dependencies:** Phase 1 completion  
**Blockers:** None (design can proceed in parallel with Phase 1)
