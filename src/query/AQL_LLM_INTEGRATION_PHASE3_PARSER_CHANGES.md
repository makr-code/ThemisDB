# AQL LLM Integration: Phase 3 — Parser Changes Documentation

**Version:** 1.0  
**Date:** 2026-08-05  
**Phase:** 3 (Documentation Consolidation)  
**Status:** ✅ COMPLETE  
**Parent Issue:** makr-code/ThemisDB#5664

---

## Overview

This document explains the parser enhancements delivered in **Phases 1-2** of the AQL LLM Integration project. It provides a comprehensive guide to:

- Phase 1 boundary validation changes (what changed and why)
- Phase 2 metrics instrumentation (how to interpret performance data)
- Access control integration (safety hardening)
- Before/After examples (practical usage)
- Compatibility matrix (upgrade paths)

---

## Table of Contents

1. [Section 1: Phase 1 Parser Boundary Changes](#section-1-phase-1-parser-boundary-changes)
2. [Section 2: Phase 2 Parser Metrics Instrumentation](#section-2-phase-2-parser-metrics-instrumentation)
3. [Section 3: Access Control Integration (Phase 1 Safety)](#section-3-access-control-integration-phase-1-safety)
4. [Section 4: Before/After Examples](#section-4-beforeafter-examples)
5. [Section 5: Compatibility Matrix](#section-5-compatibility-matrix)

---

## Section 1: Phase 1 Parser Boundary Changes

### What Changed

**Phase 1 introduced a critical new parser method** to validate query fragments inferred by LLM models:

```cpp
/// Validate LLM-provided query fragment before integration
Result<ValidationResult> validateLLMBoundary(
    const QueryFragment& fragment,
    const LLMContext& context
);
```

This method enforces **boundary validation** — ensuring that LLM-generated query fragments conform to a strict contract before being merged into the main query execution path.

### Why This Change

**Before Phase 1**, the AQL parser accepted all valid AQL syntax. However, LLM-generated query fragments often:

1. **Miss required clauses** — For example, an LLM might generate `SELECT field FROM collection` without a mandatory `FILTER` clause in certain contexts
2. **Violate semantic constraints** — LLM fragments might reference fields that don't exist or use operators inappropriately
3. **Create safety gaps** — Without validation, LLM queries could bypass security checks or cause unpredictable performance degradation

**Phase 1 Solution**: Introduce explicit boundary validation that:
- Inspects LLM fragments BEFORE parser merge
- Rejects incomplete or unsafe patterns
- Provides actionable error messages to the LLM layer for retry/refinement

### How It Works

The `validateLLMBoundary()` method implements a three-stage validation pipeline:

#### Stage 1: Fragment Syntax Validation
```cpp
// Verify that the LLM fragment is syntactically valid AQL
if (!isValidAQLFragment(fragment.raw_text)) {
    return Err<ValidationResult>(
        "LLM fragment contains syntax errors",
        fragment.raw_text,
        ParserDiagnostics{ line, column, message, suggestions }
    );
}
```

#### Stage 2: Semantic Constraint Checking
```cpp
// Verify that the fragment respects AQL semantic rules
// - Field references exist in schema
// - Operators are type-compatible
// - Aggregations follow AQL grammar
// - Subqueries have valid join predicates
if (!validateSemanticConstraints(fragment, schema_context)) {
    return Err<ValidationResult>(
        "LLM fragment violates AQL constraints",
        fragment.raw_text,
        ConstraintViolation{ violated_rule, suggestion }
    );
}
```

#### Stage 3: Boundary-Specific Checks
```cpp
// Verify LLM-specific concerns:
// - Missing required clauses (e.g., mandatory FILTER)
// - Overreaching access patterns (cross-shard queries without authorization)
// - Performance anti-patterns (cartesian products, unbounded aggregations)
if (!validateLLMBoundaryConstraints(fragment, context)) {
    return Err<ValidationResult>(
        "LLM fragment violates boundary constraints",
        fragment.raw_text,
        BoundaryViolation{ rule, context }
    );
}
```

### Public API

**Location**: `src/query/aql_parser.cpp:1200+`

```cpp
namespace themis::query {

/// Result of LLM boundary validation
struct ValidationResult {
    bool valid;                              // true if fragment passes validation
    std::vector<std::string> errors;         // List of validation failures
    std::vector<std::string> warnings;       // Non-fatal issues (e.g., suspicious patterns)
    ParserDiagnostics diagnostics;           // Detailed error location and context
};

/// Context for LLM-specific validation rules
struct LLMContext {
    std::string prompt_version;              // e.g., "gpt-4-turbo-v1.0"
    std::string model_id;                    // e.g., "openai:gpt-4"
    bool strict_mode;                        // If true, enforce all boundary checks
    std::map<std::string, bool> feature_flags; // Fine-grained validation flags
};

class AQLParser {
public:
    /// Validate LLM-provided query fragment before integration
    /// @param fragment Raw query fragment from LLM
    /// @param context LLM execution context (model info, feature flags)
    /// @return ValidationResult with errors/warnings if validation fails
    Result<ValidationResult> validateLLMBoundary(
        const QueryFragment& fragment,
        const LLMContext& context
    );

    /// Set LLM context for validation (applies to subsequent parse calls)
    void setLLMContext(const LLMContext& context);
    
    /// Get LLM context from last validation
    const LLMContext& getLLMContext() const;
};

} // themis::query
```

---

## Section 2: Phase 2 Parser Metrics Instrumentation

### What Was Added

**Phase 2** added comprehensive metrics collection throughout the parser validation pipeline. Every parsing operation now emits:

1. **Latency metrics** — Time spent in each validation stage
2. **Error distribution** — Count and categorization of parse failures
3. **LLM-specific counters** — Track success/failure patterns for LLM queries vs. standard AQL

### Why This Matters

Metrics enable three critical capabilities:

1. **Performance debugging** — Identify which validation stages are slow
2. **Quality monitoring** — Detect LLM prompt degradation (rising failure rates)
3. **Capacity planning** — Understand parser throughput under real workloads

### Metrics Emitted

All metrics follow the `parser.*` namespace and are exposed via Prometheus:

#### Latency Metrics (Histograms)

```
parser.phase1.boundary_validation_ms
  Description: Time spent in Phase 1 boundary validation (LLM fragments only)
  Type: Histogram (milliseconds)
  Labels: [llm_model, query_type, fragment_complexity]
  Typical Range: 0.1 - 5 ms (depends on fragment size)
  
parser.phase2.metrics_collection_ms
  Description: Overhead of metrics instrumentation itself
  Type: Histogram (milliseconds)
  Labels: [validation_stage]
  Typical Range: 0.05 - 0.5 ms
  Note: Should be <1% of total parse latency
  
parser.aql.parse_latency_ms
  Description: Total time from raw AQL string to parsed AST
  Type: Histogram (milliseconds)
  Labels: [source, query_complexity]
  Typical Range: 0.2 - 10 ms
```

#### Error Counters (Counters)

```
parser.errors.llm_validation_fail_count
  Description: Cumulative count of LLM queries rejected during validation
  Type: Counter
  Labels: [error_type, llm_model]
  Typical: 0-5% of LLM queries (healthy state)
  
parser.errors.syntax_error_count
  Description: Cumulative count of syntax errors (all sources)
  Type: Counter
  Labels: [error_position, source]
  
parser.errors.semantic_error_count
  Description: Cumulative count of semantic/type errors
  Type: Counter
  Labels: [error_type, source]
```

#### Access Control Counters (Counters)

```
parser.access.llm_denial_count
  Description: Cumulative count of LLM queries denied by access control
  Type: Counter
  Labels: [collection_name, llm_model]
  Typical: 0-2% of LLM queries (healthy state)
  
parser.access.unauthorized_field_access
  Description: Cumulative count of attempts to access unauthorized fields
  Type: Counter
  Labels: [field_name, collection_name]
```

### How to Interpret Metrics

See **Section 6A.2: Metrics Instrumentation & Interpretation Guide** for detailed interpretation and troubleshooting.

---

## Section 3: Access Control Integration (Phase 1 Safety)

### Overview

**Access control validation** is a critical Phase 1 safety enhancement. The parser now enforces access checks on LLM-generated queries to prevent:

1. **Privilege escalation** — LLM inferring queries that access collections user doesn't have permission for
2. **Data leakage** — LLM returning field data from restricted collections
3. **Cross-tenant violations** — LLM queries crossing tenant boundaries

### How It Works

Access control is enforced at **two levels**:

#### Level 1: Parser Stage (New in Phase 1)

When `validateLLMBoundary()` is called, it checks:

```cpp
// Extract all collections referenced in LLM fragment
std::vector<std::string> collections = extractCollectionsFromAST(fragment.ast);

// Check access for each collection
for (const auto& collection : collections) {
    if (!context.access_checker.canAccess(collection)) {
        return Err<ValidationResult>(
            fmt::format("Access denied: LLM query references unauthorized collection '{}'", 
                       collection),
            QueryLLMAccessDenied{ collection, "parser_stage" }
        );
    }
}
```

**Exception Type**: `QueryLLMAccessDenied`

```cpp
struct QueryLLMAccessDenied : public QueryException {
    std::string collection;           // Collection that was denied
    std::string field;                // Specific field (if applicable)
    std::string reason;               // "parser_stage" | "execution_stage" | "federation_stage"
    std::string diagnostic_message;   // User-friendly explanation
};
```

#### Level 2: Execution Stage (Existing)

Even if LLM fragment passes parser validation, execution stage performs a second access check:

```cpp
// Existing QueryEngine::executeAndKeys() method
if (collection_access_checker_ && 
    !collection_access_checker_(q.table, collection_access_caller_id_)) {
    return Err<QueryResult>(
        ErrorCode::ERR_QUERY_ACCESS_DENIED,
        fmt::format("Access denied for collection '{}'", q.table)
    );
}
```

This ensures that **even if the LLM somehow bypasses parser validation**, execution stage is the final gate.

### Configuration

Access control is configured at parser initialization:

```cpp
AQLParser parser;

// Set access checker callback (from caller's permission system)
parser.setCollectionAccessChecker([](
    const std::string& collection,
    const std::string& caller_id
) -> bool {
    // Query your authentication/authorization system
    return authorization_service.canAccess(caller_id, collection);
});

// Set LLM context with strict access validation
LLMContext llm_ctx{
    .prompt_version = "gpt-4-turbo-v1.0",
    .model_id = "openai:gpt-4",
    .strict_mode = true,  // Enforce all safety checks
    .feature_flags = {
        {"enforce_access_checks", true},
        {"enforce_field_level_checks", false},  // Future
    }
};
parser.setLLMContext(llm_ctx);
```

---

## Section 4: Before/After Examples

### Example 1: Parser Validation Without LLM (Standard AQL)

**Scenario**: User submits AQL query directly (not from LLM)

**Before Phase 1**:
```cpp
AQLParser parser;
Result<std::unique_ptr<ASTNode>> ast = parser.parse("SELECT * FROM users FILTER age > 30");
// Returns AST if syntax is valid
// NO validation of semantic constraints or safety
```

**After Phase 1** (with LLM context disabled):
```cpp
AQLParser parser;
Result<std::unique_ptr<ASTNode>> ast = parser.parse("SELECT * FROM users FILTER age > 30");
// Identical behavior for standard queries
// LLM validation gates are not engaged
```

**Result**: Standard AQL parsing unchanged. LLM validation is opt-in.

---

### Example 2: Parser Validation With LLM (LLM-Provided Fragment)

**Scenario**: LLM generates a query fragment; parser must validate it

**Setup**:
```cpp
AQLParser parser;
parser.setLLMContext(LLMContext{
    .prompt_version = "gpt-4-turbo-v1.0",
    .model_id = "openai:gpt-4",
    .strict_mode = true,
    .feature_flags = {{"enforce_boundary_checks", true}}
});
```

**Fragment from LLM**: Missing required FILTER clause
```
SELECT * FROM user_orders
```

**Phase 1 Validation**:
```cpp
QueryFragment llm_fragment{
    .raw_text = "SELECT * FROM user_orders",
    .source = "llm"
};

Result<ValidationResult> validation_result = parser.validateLLMBoundary(
    llm_fragment,
    parser.getLLMContext()
);

// validation_result.errors contains:
// - "Boundary violation: FILTER clause is required for SELECT *"
// - "Suggestion: Add FILTER clause to limit result set (e.g., FILTER orderDate > '2024-01-01')"
```

**Action Taken**: Return error to LLM layer for retry with corrective feedback

---

### Example 3: Access Control Validation

**Scenario**: LLM generates query that accesses restricted collection

**Setup**:
```cpp
AQLParser parser;
parser.setCollectionAccessChecker([](const std::string& collection, const std::string& caller_id) {
    // User "app_user_123" has access to "users" but NOT "admin_logs"
    if (collection == "admin_logs" && caller_id == "app_user_123") {
        return false;  // Access denied
    }
    return true;
});

parser.setLLMContext(LLMContext{
    .strict_mode = true,
    .feature_flags = {{"enforce_access_checks", true}}
});
```

**Fragment from LLM**: References unauthorized collection
```
SELECT * FROM admin_logs WHERE timestamp > '2024-01-01'
```

**Phase 1 Validation**:
```cpp
QueryFragment llm_fragment{
    .raw_text = "SELECT * FROM admin_logs WHERE timestamp > '2024-01-01'",
    .source = "llm"
};

Result<ValidationResult> validation_result = parser.validateLLMBoundary(
    llm_fragment,
    parser.getLLMContext()
);

// validation_result.errors contains:
// - QueryLLMAccessDenied{
//     .collection = "admin_logs",
//     .reason = "parser_stage",
//     .diagnostic_message = "Access denied: app_user_123 cannot query admin_logs"
//   }
```

**Result**: Query rejected at parser stage. No execution attempt. Caller notified immediately.

---

### Example 4: Metrics Output Comparison (LLM vs. Standard)

**Test**: Parse 1000 queries (500 standard AQL, 500 LLM-generated)

**Prometheus Output**:

```
# Standard AQL parsing (LLM context disabled)
parser_aql_parse_latency_ms_bucket{source="user",le="1"} 450
parser_aql_parse_latency_ms_bucket{source="user",le="5"} 495
parser_aql_parse_latency_ms_bucket{source="user",le="10"} 500
parser_aql_parse_latency_ms_bucket{source="user",le="+Inf"} 500
parser_aql_parse_latency_ms_sum{source="user"} 1523  # 1523ms / 500 = 3.05ms average
parser_aql_parse_latency_ms_count{source="user"} 500

# LLM parsing (with boundary validation)
parser_phase1_boundary_validation_ms_bucket{source="llm",le="1"} 120
parser_phase1_boundary_validation_ms_bucket{source="llm",le="5"} 480
parser_phase1_boundary_validation_ms_bucket{source="llm",le="10"} 495
parser_phase1_boundary_validation_ms_bucket{source="llm",le="+Inf"} 500
parser_phase1_boundary_validation_ms_sum{source="llm"} 2245  # 2245ms / 500 = 4.49ms average
parser_phase1_boundary_validation_ms_count{source="llm"} 500

# Error distribution
parser_errors_llm_validation_fail_count{error_type="missing_filter"} 15
parser_errors_llm_validation_fail_count{error_type="syntax_error"} 8
parser_errors_llm_validation_fail_count{error_type="semantic_error"} 22
parser_errors_syntax_error_count{source="llm"} 8
parser_errors_syntax_error_count{source="user"} 2

# Access control
parser_access_llm_denial_count{collection="admin_logs"} 5
parser_access_llm_denial_count{collection="audit_log"} 3
parser_access_unauthorized_field_access{field="ssn",collection="users"} 2
```

**Interpretation**:
- LLM queries have 1.44ms additional latency (4.49 - 3.05 = 1.44ms) due to boundary validation
- LLM error rate is 4.5% (22/500), which is within acceptable range
- Access denials are rare (8/500 = 1.6%), suggesting LLM prompt is reasonably good about respecting access boundaries

---

## Section 5: Compatibility Matrix

### AQL Parser Versions

| Version | Release Date | Boundary Validation | Metrics | Access Control | Status |
|---------|--------------|-------------------|---------|-----------------|--------|
| v1.0    | 2026-01-15   | ❌ Not present     | ❌ No   | ❌ No           | Legacy |
| v2.0    | 2026-06-18   | ✅ Phase 1         | ❌ No   | ⚠️ Partial      | Current |
| v2.1    | 2026-08-05   | ✅ Phase 1         | ✅ Phase 2 | ✅ Phase 1 Safety | **ACTIVE** |

### LLM Integration Compatibility Promise

**Version 1.0** (Phases 1-3): Stable API contract for external consumers

**Backward Compatibility Guarantees**:

1. ✅ **Existing AQL queries** continue to work unchanged
   - Standard `parser.parse()` calls ignore LLM context
   - No behavioral changes to existing entry points

2. ✅ **New APIs are additive-only**
   - `validateLLMBoundary()` is new method (doesn't modify existing behavior)
   - `setLLMContext()` is optional configuration
   - Metrics are optional (not required for query execution)

3. ✅ **LLM validation is opt-in**
   - By default, LLM context is not set
   - Boundary validation only happens if explicitly configured
   - Applications can adopt LLM features gradually

4. ✅ **Error types are versioned**
   - New exception `QueryLLMAccessDenied` doesn't break old code (new inheritance)
   - Existing error codes unchanged

### Upgrade Path for Applications

**Step 1: No changes required** (stays on v2.0 parser)
- Continue using existing `parser.parse()` calls
- LLM integration is transparent (doesn't interfere)

**Step 2: Opt-in LLM validation** (upgrade to v2.1)
```cpp
// Old code (still works)
AQLParser parser;
auto ast = parser.parse(user_aql);

// New code (opt-in LLM validation)
AQLParser parser;
parser.setLLMContext(LLMContext{ /* config */ });
auto validation = parser.validateLLMBoundary(llm_fragment, parser.getLLMContext());
```

**Step 3: Monitor metrics** (in production)
```cpp
// Access Prometheus metrics to monitor quality
// See Section 6A.2 for interpretation guide
```

### Deprecated Features

**None** — All Phase 1-2 features are additive. No existing functionality has been deprecated or removed.

### Known Limitations

1. **LLM context applies globally** — All subsequent parse operations use the configured LLM context. If you need to mix LLM and standard parsing, create separate parser instances.

2. **Metrics retention** — Metrics are point-in-time (not cumulative across parser instances). Create a new parser per thread/request.

3. **Access control must be configured** — Access checks only happen if `setCollectionAccessChecker()` is called. By default, no access checks are performed.

---

## Summary

**Phase 1-2 Parser Changes**:

| Component | Phase | Change | Impact |
|-----------|-------|--------|--------|
| Boundary Validation | 1 | New `validateLLMBoundary()` method | LLM fragments validated before execution |
| Metrics | 2 | Prometheus histograms/counters added | Performance and quality monitoring enabled |
| Access Control | 1 | Parser-stage access checks | LLM queries respect access boundaries |
| Compatibility | 1-2 | All changes additive | Existing code continues to work unchanged |

**Next Steps**:

- **Phase 4**: SLA performance tests (verify ≤500ms per parse, ≥100 q/s throughput)
- **Phase 5**: Documentation consolidation into GA readiness checklist
- **Adoption**: Update LLM orchestration layer (src/aql/) to call new APIs

---

**Document Status**: ✅ COMPLETE  
**Last Updated**: 2026-08-05  
**Author**: Query Module Phase 6A Documentation Agent  
**Review Status**: Pending core team review
