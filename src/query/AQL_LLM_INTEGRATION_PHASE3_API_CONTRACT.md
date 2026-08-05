# AQL LLM Integration: Phase 3 — API Contract

**Version:** 1.0  
**Date:** 2026-08-05  
**Phase:** 3 (Documentation Consolidation)  
**Status:** ✅ COMPLETE  
**Parent Issue:** makr-code/ThemisDB#5664

---

## Overview

This document defines the **public API contract** for AQL LLM Integration Phases 1-3. It is intended for:

- **SDK developers** building on top of ThemisDB
- **LLM orchestration layer** (`src/aql/`) consuming Query Engine APIs
- **Downstream consumers** integrating LLM-enhanced query capabilities

This API is **stable** and follows semantic versioning. All interfaces defined here will be maintained with backward compatibility guarantees.

---

## Table of Contents

1. [Section 1: AQLParser LLM Extensions](#section-1-aqlparser-llm-extensions)
2. [Section 2: QueryEngine LLM Execution Paths](#section-2-queryengine-llm-execution-paths)
3. [Section 3: Error Contract](#section-3-error-contract)
4. [Section 4: Backward Compatibility Guarantees](#section-4-backward-compatibility-guarantees)

---

## Section 1: AQLParser LLM Extensions

### 1.1 validateLLMBoundary() — Phase 1

**Purpose**: Validate LLM-provided query fragment before integration

**Signature**:
```cpp
namespace themis::query {

Result<ValidationResult> AQLParser::validateLLMBoundary(
    const QueryFragment& fragment,
    const LLMContext& context = {}
);
```

**Parameters**:

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `fragment` | `QueryFragment` | ✅ Yes | Raw query fragment from LLM |
| `context` | `LLMContext` | ❌ No | LLM execution context (defaults to current if not set) |

**Return Type**: `Result<ValidationResult>`

**Success Case**: Returns `Ok(ValidationResult)` with validation results
```cpp
struct ValidationResult {
    bool valid;                          // true if fragment passes all checks
    std::vector<std::string> errors;     // List of validation failures
    std::vector<std::string> warnings;   // Non-fatal issues
    ParserDiagnostics diagnostics;       // Detailed error location
};
```

**Error Cases**: Returns `Err(QueryException)` if:
- Fragment is `nullptr`
- Context is invalid
- Parser encounters internal error

**Throws**: 
- `QueryException` — for malformed input or internal errors
- `QueryLLMAccessDenied` — if fragment violates access control

**Examples**:

```cpp
// Example 1: Successful validation
AQLParser parser;
parser.setLLMContext(LLMContext{ .strict_mode = true });

QueryFragment fragment{ .raw_text = "SELECT * FROM users FILTER age > 30" };
Result<ValidationResult> result = parser.validateLLMBoundary(fragment);

if (result.ok()) {
    const auto& validation = result.value();
    if (validation.valid) {
        std::cout << "Query is valid\n";
    } else {
        for (const auto& error : validation.errors) {
            std::cout << "Error: " << error << "\n";
        }
    }
}

// Example 2: Access denied
fragment.raw_text = "SELECT * FROM admin_logs WHERE timestamp > '2024-01-01'";
result = parser.validateLLMBoundary(fragment);

if (!result.ok()) {
    if (auto access_denied = std::get_if<QueryLLMAccessDenied>(&result.error())) {
        std::cout << "Access denied for collection: " << access_denied->collection << "\n";
    }
}
```

---

### 1.2 getMetrics() — Phase 2

**Purpose**: Retrieve metrics collected from last parse/validation operation

**Signature**:
```cpp
struct ParserMetrics {
    double parse_latency_ms;              // Time from string to AST
    double validation_latency_ms;         // Time spent in validateLLMBoundary()
    uint32_t error_count;                 // Total errors encountered
    uint32_t warning_count;               // Total warnings
    std::string query_type;               // "SELECT", "INSERT", etc.
    std::string source;                   // "llm" or "user"
};

ParserMetrics AQLParser::getMetrics() const;
```

**Return Type**: `ParserMetrics` (struct with metrics)

**Behavior**:
- Metrics are per-parser instance
- Metrics are updated after each parse or validation call
- Metrics persist until next parse/validation call
- Metrics are NOT cumulative across calls

**Examples**:

```cpp
// Example 1: Monitor validation latency
AQLParser parser;
Result<ValidationResult> validation = parser.validateLLMBoundary(fragment);

ParserMetrics metrics = parser.getMetrics();
std::cout << "Validation took " << metrics.validation_latency_ms << " ms\n";

if (metrics.validation_latency_ms > 10.0) {
    std::cout << "WARNING: Validation is slow for this fragment\n";
}

// Example 2: Track error counts
if (validation.ok()) {
    metrics = parser.getMetrics();
    std::cout << "Errors: " << metrics.error_count << ", Warnings: " << metrics.warning_count << "\n";
}
```

---

### 1.3 setLLMContext() — Phase 1

**Purpose**: Configure LLM-specific validation rules and feature flags

**Signature**:
```cpp
struct LLMContext {
    std::string prompt_version;              // e.g., "gpt-4-turbo-v1.0"
    std::string model_id;                    // e.g., "openai:gpt-4"
    bool strict_mode = false;                // Enforce all boundary checks
    std::map<std::string, bool> feature_flags;  // Fine-grained control
    std::function<bool(const std::string&, const std::string&)> access_checker;  // Permission callback
};

void AQLParser::setLLMContext(const LLMContext& context);
```

**Parameters**:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `prompt_version` | string | "" | Version of LLM prompt (for tracking) |
| `model_id` | string | "" | LLM model identifier (e.g., "gpt-4") |
| `strict_mode` | bool | false | If true, enforce ALL boundary checks |
| `feature_flags` | map | {} | Enable/disable specific validation features |
| `access_checker` | function | nullptr | Callback to check collection access |

**Feature Flags**:

```
"enforce_boundary_checks" (default: true)
  - If true: Reject fragments missing required clauses
  - If false: Warn but allow fragments with missing clauses

"enforce_access_checks" (default: true)
  - If true: Reject fragments accessing unauthorized collections
  - If false: Warn but allow access check violations

"enforce_semantic_checks" (default: true)
  - If true: Reject fragments with type mismatches or invalid operators
  - If false: Warn only

"max_fragment_complexity" (default: "high")
  - "simple": Reject deep nesting (>5 levels)
  - "medium": Reject very deep nesting (>10 levels)
  - "high": Allow any depth
```

**Examples**:

```cpp
// Example 1: Standard LLM context (most common)
AQLParser parser;
parser.setLLMContext(LLMContext{
    .prompt_version = "gpt-4-turbo-v1.0",
    .model_id = "openai:gpt-4",
    .strict_mode = true,
    .feature_flags = {
        {"enforce_boundary_checks", true},
        {"enforce_access_checks", true},
    }
});

// Example 2: Permissive LLM context (for trusted LLM)
parser.setLLMContext(LLMContext{
    .prompt_version = "fine-tuned-v2.0",
    .model_id = "custom:themis-tuned",
    .strict_mode = false,  // Don't enforce all checks
    .feature_flags = {
        {"enforce_semantic_checks", false},  // Reduce warnings
    }
});

// Example 3: Custom access checker
parser.setLLMContext(LLMContext{
    .access_checker = [](const std::string& collection, const std::string& caller_id) {
        // Query your auth system
        return my_auth_service.canAccess(caller_id, collection);
    }
});
```

**Error Handling**:
- If `access_checker` is nullptr, access checks are skipped
- If `access_checker` throws, `validateLLMBoundary()` returns error
- Feature flags are case-sensitive

---

### 1.4 getLLMContext() — Phase 1

**Purpose**: Retrieve current LLM context from parser instance

**Signature**:
```cpp
const LLMContext& AQLParser::getLLMContext() const;
```

**Return Type**: `const LLMContext&`

**Behavior**:
- Returns the context set via `setLLMContext()`
- Returns empty context if `setLLMContext()` was never called
- Does NOT throw if context is empty

---

## Section 2: QueryEngine LLM Execution Paths

### 2.1 executeWithLLMContext() — Phase 1-2

**Purpose**: Execute query with LLM validation enabled

**Signature**:
```cpp
namespace themis::query {

Result<QueryResult> QueryEngine::executeWithLLMContext(
    const Query& query,
    const LLMContext& llm_context = {}
);
```

**Parameters**:

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `query` | `Query` | ✅ Yes | Parsed query to execute |
| `llm_context` | `LLMContext` | ❌ No | LLM context (for validation) |

**Return Type**: `Result<QueryResult>`

**Success Case**: Returns `Ok(QueryResult)` with results
```cpp
struct QueryResult {
    bool success;
    std::vector<Entity> entities;       // Result rows
    uint64_t execution_time_ms;         // Time to execute
    uint64_t result_count;              // Number of results
    std::string execution_plan;         // (optional) query plan used
};
```

**Error Cases**: Returns `Err(QueryException)` if:
- Query validation fails (syntax/semantic errors)
- LLM boundary validation fails
- Access control denies query
- Execution fails (timeout, resource limit, etc.)

**Guarantees**:
- ✅ Boundary validation is enforced before execution
- ✅ Access checks are applied (parser stage + execution stage)
- ✅ Metrics are collected and available via `getMetrics()`
- ✅ Query is either fully executed or atomically rolled back

**Examples**:

```cpp
// Example 1: Execute LLM-generated query
QueryEngine engine;
engine.setCollectionAccessChecker(auth_service.checker());

LLMContext llm_ctx{
    .prompt_version = "gpt-4-turbo-v1.0",
    .model_id = "openai:gpt-4",
    .strict_mode = true,
};

Query parsed_query = /* from LLM translation layer */;

Result<QueryResult> result = engine.executeWithLLMContext(parsed_query, llm_ctx);

if (result.ok()) {
    const auto& query_result = result.value();
    std::cout << "Query returned " << query_result.result_count << " rows\n";
    std::cout << "Executed in " << query_result.execution_time_ms << " ms\n";
} else {
    std::cout << "Query failed: " << result.error() << "\n";
}

// Example 2: With fallback to standard execution
Result<QueryResult> result = engine.executeWithLLMContext(parsed_query, llm_ctx);

if (!result.ok()) {
    // If LLM execution fails, try standard execution (if appropriate)
    if (should_retry_without_llm_context()) {
        result = engine.executeWithLLMContext(parsed_query, LLMContext{});
    }
}
```

---

### 2.2 Execution Guarantees

When executing via `executeWithLLMContext()`, the following guarantees hold:

#### Guarantee 1: Boundary Validation Before Execution
```
Query enters executeWithLLMContext()
  ↓
Parser validates LLM boundary
  ↓ (if valid)
Semantic checks performed
  ↓ (if valid)
Execution begins
```

#### Guarantee 2: Access Control Enforced
```
Query enters executeWithLLMContext()
  ↓
Parser stage: Check collection access
  ↓ (if OK)
Execution stage: Re-check collection access
  ↓ (if OK)
Federation stage: Remote access checks applied
```

#### Guarantee 3: Metrics Collected
```
Query execution completes
  ↓
Metrics updated in parser instance
  ↓
Metrics available via getMetrics()
  ↓
Metrics emitted to Prometheus
```

---

## Section 3: Error Contract

### 3.1 QueryLLMValidationError

**Purpose**: Thrown when LLM fragment fails validation

**Definition**:
```cpp
class QueryLLMValidationError : public QueryException {
public:
    // Fields
    std::string fragment_snippet;        // First 256 chars of invalid fragment
    std::string validation_stage;        // "parser" | "semantic" | "boundary"
    std::vector<std::string> error_details;  // Detailed error messages
    ParserDiagnostics diagnostics;      // Line/column/context
    
    // Methods
    const char* what() const noexcept override;
    QueryErrorCode error_code() const;   // Returns ERR_QUERY_LLM_VALIDATION_FAILED
};
```

**When Thrown**:
- Fragment has syntax errors
- Fragment violates semantic constraints (type mismatch, invalid operators, etc.)
- Fragment violates boundary constraints (missing required clauses)

**Example**:
```cpp
try {
    Result<ValidationResult> validation = parser.validateLLMBoundary(fragment);
    if (!validation.ok()) {
        throw validation.error();  // Throws QueryLLMValidationError
    }
} catch (const QueryLLMValidationError& e) {
    std::cout << "Validation failed at stage: " << e.validation_stage << "\n";
    std::cout << "Fragment: " << e.fragment_snippet << "\n";
    for (const auto& detail : e.error_details) {
        std::cout << "  - " << detail << "\n";
    }
}
```

---

### 3.2 QueryLLMAccessDenied

**Purpose**: Thrown when LLM query violates access control

**Definition**:
```cpp
class QueryLLMAccessDenied : public QueryException {
public:
    // Fields
    std::string collection;              // Collection that was denied
    std::string field;                   // Specific field (if applicable)
    std::string reason;                  // "parser_stage" | "execution_stage" | "federation_stage"
    std::string diagnostic_message;      // User-friendly explanation
    
    // Methods
    const char* what() const noexcept override;
    QueryErrorCode error_code() const;   // Returns ERR_QUERY_ACCESS_DENIED
};
```

**When Thrown**:
- LLM query references unauthorized collection
- LLM query attempts to access restricted field
- Remote cluster denies access during federation

**Example**:
```cpp
try {
    Result<ValidationResult> validation = parser.validateLLMBoundary(fragment);
    if (!validation.ok()) {
        throw validation.error();  // May throw QueryLLMAccessDenied
    }
} catch (const QueryLLMAccessDenied& e) {
    std::cout << "Access denied for collection: " << e.collection << "\n";
    std::cout << "Reason: " << e.diagnostic_message << "\n";
    // Log for audit trail
    audit_log.write("llm_access_denied", {
        {"user_id", current_user_id},
        {"collection", e.collection},
        {"timestamp", now()}
    });
}
```

---

### 3.3 Error Code Reference

**New Error Codes** (Phase 1-3):

```cpp
namespace themis::query {

enum class QueryErrorCode {
    // Existing codes (unchanged)
    // ERR_QUERY_SYNTAX_ERROR, ERR_QUERY_SEMANTIC_ERROR, etc.
    
    // New codes (Phase 1-3)
    ERR_QUERY_LLM_VALIDATION_FAILED = 4001,
    ERR_QUERY_LLM_ACCESS_DENIED = 4002,
    ERR_QUERY_LLM_BOUNDARY_VIOLATION = 4003,
    ERR_QUERY_LLM_CONTEXT_INVALID = 4004,
};

} // themis::query
```

**Mapping**:

| Error Code | Exception Type | HTTP Status | Action |
|-----------|---|---|---|
| 4001 | `QueryLLMValidationError` | 400 Bad Request | Retry LLM with corrective feedback |
| 4002 | `QueryLLMAccessDenied` | 403 Forbidden | Log audit event; notify user |
| 4003 | `QueryLLMValidationError` | 400 Bad Request | Simplify LLM prompt |
| 4004 | `QueryException` | 500 Internal Error | Fix LLM context configuration |

---

## Section 4: Backward Compatibility Guarantees

### 4.1 Stability Promises

**AQL LLM Integration v1.0** (Phases 1-3) commits to:

1. ✅ **No breaking changes to existing APIs**
   - `AQLParser::parse()` behavior unchanged for standard queries
   - `QueryEngine::execute()` behavior unchanged
   - All existing error codes remain the same

2. ✅ **New APIs are additive-only**
   - `validateLLMBoundary()` is new (doesn't modify existing methods)
   - `setLLMContext()` is new configuration (optional)
   - New error codes are in unused range (4001-4999)

3. ✅ **LLM features are opt-in**
   - LLM context defaults to empty (no validation unless configured)
   - Metrics collection is optional (not required for query execution)
   - Boundary validation only happens if explicitly enabled

4. ✅ **Deprecation policy**
   - Any future breaking change will be:
     - Announced at least 1 major version in advance
     - Shipped in new API (old API deprecated but still works)
     - Removed only in next major version

### 4.2 Migration Path for Existing Applications

**Current State** (no LLM integration):
```cpp
AQLParser parser;
Result<std::unique_ptr<ASTNode>> ast = parser.parse(user_aql);
// Works exactly as before — no changes needed
```

**Phase 1: Opt-in LLM validation** (no breaking changes):
```cpp
AQLParser parser;

// Step 1: Optionally set LLM context (new, doesn't break existing code)
parser.setLLMContext(LLMContext{ /* config */ });

// Step 2: For LLM fragments, call new method
QueryFragment llm_fragment = translate_nl_to_fragment(user_nl);
Result<ValidationResult> validation = parser.validateLLMBoundary(llm_fragment);

// Step 3: For standard queries, continue using parse() (unchanged)
Result<std::unique_ptr<ASTNode>> ast = parser.parse(user_aql);
```

**Phase 2-3: Monitor and optimize** (no breaking changes):
```cpp
// Access new metrics (optional)
ParserMetrics metrics = parser.getMetrics();
prometheus_metrics->record(metrics);

// All existing code continues to work unchanged
```

### 4.3 Version Compatibility Matrix

| Application Version | Query Module v2.1 | Status |
|---|---|---|
| v1.x (no LLM) | ✅ Fully compatible | No changes needed |
| v2.0 (early LLM) | ✅ Fully compatible | Inherit Phase 1-2 improvements |
| v2.1+ (LLM enabled) | ✅ Fully compatible | Access all Phase 1-3 features |

### 4.4 Known Limitations

1. **LLM context is per-instance**
   - Context set via `setLLMContext()` applies to all subsequent parse/validate calls
   - To mix LLM and non-LLM parsing, create separate parser instances

2. **Metrics are point-in-time**
   - Metrics from `getMetrics()` reflect last parse/validate call
   - Metrics are NOT cumulative; create new parser per thread/request if tracking trends

3. **Access control must be configured**
   - Access checks only apply if `access_checker` callback is set
   - If not set, no access control validation is performed (backward compatible)

---

## Summary

**Public API (Phases 1-3)**:

| API | Phase | Status | Purpose |
|---|---|---|---|
| `AQLParser::validateLLMBoundary()` | 1 | ✅ Stable | Validate LLM fragments |
| `AQLParser::setLLMContext()` | 1 | ✅ Stable | Configure LLM validation |
| `AQLParser::getLLMContext()` | 1 | ✅ Stable | Retrieve LLM context |
| `AQLParser::getMetrics()` | 2 | ✅ Stable | Get parser metrics |
| `QueryEngine::executeWithLLMContext()` | 1-2 | ✅ Stable | Execute with LLM validation |
| `QueryLLMValidationError` | 1 | ✅ Stable | Validation failure exception |
| `QueryLLMAccessDenied` | 1 | ✅ Stable | Access denial exception |

**Backward Compatibility**: ✅ Fully backward compatible. All changes are additive; existing code continues to work unchanged.

**Next Steps**:

1. **Phase 4**: SLA performance tests (verify API performance guarantees)
2. **Phase 5**: Documentation consolidation into GA readiness
3. **SDK updates**: Document APIs in SDK reference manuals
4. **Consumer libraries**: Update `src/aql/` to use new APIs

---

**Document Status**: ✅ COMPLETE  
**Last Updated**: 2026-08-05  
**Author**: Query Module Phase 6A Documentation Agent  
**Review Status**: Pending core team review
