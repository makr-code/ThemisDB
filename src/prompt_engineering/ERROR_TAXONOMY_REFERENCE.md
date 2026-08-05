# Prompt Engineering Error Taxonomy - Phase 1 Reference

## Overview

This document provides a comprehensive reference for all prompt engineering error codes, their meanings, recovery strategies, and usage patterns.

**Phase:** 1 (Design / API Contract)  
**Status:** Frozen (Q3 2026)  
**Last Updated:** 2026-08-05

---

## Error Code Summary

### Template Errors (9500–9549)

These errors indicate problems with template definition, validation, or state.

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9500 | TEMPLATE_INVALID_ID | Template ID missing, empty, or malformed | Provide non-empty template ID matching pattern `[a-zA-Z0-9_-]{1,128}` | ❌ No |
| 9501 | TEMPLATE_NOT_FOUND | Template ID doesn't exist in registry | Verify template ID is correct; create template if needed | ❌ No |
| 9502 | TEMPLATE_ALREADY_EXISTS | Attempted to create duplicate template | Use different ID or delete existing template first | ❌ No |
| 9503 | TEMPLATE_VALIDATION_FAILED | Template structure is invalid (missing placeholders, cycles, syntax errors) | Fix template JSON/YAML syntax; remove circular references | ❌ No |
| 9504 | TEMPLATE_OVERSIZED | Template exceeds size limits (>1 MB or >50k tokens) | Split into smaller templates; compress content | ❌ No |
| 9505 | TEMPLATE_PLACEHOLDER_MISSING | Required placeholder `{key}` not found in template | Add placeholder to template or remove from required set | ❌ No |
| 9506 | TEMPLATE_PLACEHOLDER_RECURSIVE | Placeholder references itself directly or indirectly | Remove circular reference from template definition | ❌ No |
| 9507 | TEMPLATE_INJECTION_MISMATCH | Context value doesn't match placeholder type/format | Provide context value in correct format (string/number/array/etc) | ❌ No |
| 9508 | TEMPLATE_MALFORMED_JSON | Template JSON parsing failed | Fix JSON syntax (escaping, brackets, commas) | ❌ No |
| 9509 | TEMPLATE_UNSUPPORTED_MIME_TYPE | Multi-modal MIME type not supported | Use supported types (image/jpeg, image/png, image/webp, image/gif, video/mp4) | ❌ No |

### Injection Errors (9550–9599)

These errors indicate problems during context injection into templates.

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9550 | INJECTION_CONTEXT_MISSING | Required context key not provided | Provide all required context keys | ❌ No |
| 9551 | INJECTION_CONTEXT_TYPE_MISMATCH | Context value type doesn't match placeholder | Convert value to correct type (e.g., string vs number) | ❌ No |
| 9552 | INJECTION_SUBSTITUTION_FAILED | Variable substitution failed (encoding/escape error) | Verify context values are valid UTF-8; check escape sequences | ✅ Yes |
| 9553 | INJECTION_ENCODING_ERROR | Text encoding error (invalid UTF-8, etc) | Ensure all text is valid UTF-8; re-encode if needed | ✅ Yes |
| 9554 | INJECTION_SIZE_EXCEEDED | Injected content exceeds size bounds | Reduce context size; split into multiple injections | ❌ No |
| 9555 | INJECTION_SEMANTIC_MISMATCH | Value violates semantic constraint (e.g., enum value) | Provide value from allowed set; check schema | ❌ No |
| 9556 | INJECTION_CIRCULAR_REFERENCE | Circular dependency detected in context | Remove circular references; restructure context | ❌ No |

### Version Control Errors (9600–9649)

These errors indicate problems with version control, history, or concurrent edits.

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9600 | VERSION_CONFLICT | Conflicting concurrent edits to same version | Retry operation; may need to merge/resolve conflicts manually | ✅ Yes |
| 9601 | VERSION_NOT_FOUND | Requested version ID doesn't exist | Use correct version ID; check history | ❌ No |
| 9602 | VERSION_COMMIT_FAILED | Commit to version history failed (storage error) | Check disk space; retry operation | ✅ Yes |
| 9603 | VERSION_HISTORY_CORRUPTED | Version history integrity check failed | Contact support; may need database repair | ❌ No |
| 9604 | VERSION_MERGE_CONFLICT | Automatic merge of concurrent edits failed | Manually resolve conflict; cherry-pick desired changes | ❌ No |
| 9605 | VERSION_DRIFT_DETECTED | Version branches diverged beyond reconciliation threshold | Rebase one branch or create new version | ❌ No |
| 9606 | VERSION_ROLLBACK_FAILED | Unable to rollback to target version | Check version exists; ensure sufficient disk space | ✅ Yes |
| 9607 | VERSION_CLEANUP_FAILED | Garbage collection of old versions failed | Check disk space; retry cleanup later | ✅ Yes |

### Optimization Errors (9650–9699)

These errors indicate problems with the prompt optimization loop.

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9650 | OPTIMIZATION_DIVERGED | Optimizer diverged instead of converging | Adjust optimizer hyperparameters (learning rate, decay); try again | ✅ Yes |
| 9651 | OPTIMIZATION_NON_CONVERGENT | Failed to converge within iteration budget | Increase iteration limit; start from better initial state | ✅ Yes |
| 9652 | OPTIMIZATION_INVALID_PARAMETERS | Optimizer config invalid (bad hyperparams, ranges) | Fix hyperparameter values; use sensible defaults | ❌ No |
| 9653 | OPTIMIZATION_FEEDBACK_CORRUPT | Feedback signal corrupted or inconsistent | Verify feedback source; clear cache; restart optimization | ✅ Yes |
| 9654 | OPTIMIZATION_TIMEOUT | Optimization exceeded time budget | Increase time limit; reduce iteration count | ✅ Yes |
| 9655 | OPTIMIZATION_OUT_OF_MEMORY | Insufficient memory for optimization state | Reduce batch size; run on machine with more memory | ✅ Yes |
| 9656 | OPTIMIZATION_INITIALIZATION_FAILED | Optimizer failed to initialize | Check template and feedback data; verify model availability | ✅ Yes |
| 9657 | OPTIMIZATION_EARLY_TERMINATION | Optimizer stopped by safety/policy constraint | Review policy constraints; adjust if needed | ❌ No |

### Evaluator Errors (9700–9749)

These errors indicate problems with prompt evaluation and quality assessment.

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9700 | EVALUATION_INCONSISTENT | Multiple evaluations of same prompt diverge | Check evaluator model consistency; restart evaluation | ✅ Yes |
| 9701 | EVALUATION_SCORE_INVALID | Evaluation score out of valid range (typically 0–1) | Check evaluator model output; verify scoring function | ✅ Yes |
| 9702 | EVALUATION_TIMEOUT | Evaluation exceeded time budget | Increase timeout; use faster evaluator | ✅ Yes |
| 9703 | EVALUATION_MODEL_UNAVAILABLE | Evaluation model not loaded or service down | Load model; check service availability | ✅ Yes |
| 9704 | EVALUATION_CORRUPTED_STATE | Evaluator internal state corrupted | Restart evaluator; check logs for details | ✅ Yes |
| 9705 | EVALUATION_MEMORY_ERROR | Insufficient memory for evaluation | Reduce batch size; run on larger machine | ✅ Yes |
| 9706 | EVALUATION_CONSTRAINT_VIOLATION | Evaluation result violates domain constraint | Review domain constraints; verify evaluator logic | ❌ No |
| 9707 | EVALUATION_SAMPLING_FAILED | Monte-Carlo/sampling evaluation failed | Increase sample size; verify distribution parameters | ✅ Yes |

### Rewrite Errors (9750–9799)

These errors indicate problems with rewrite rule application (Phase 2+).

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9750 | REWRITE_RULE_NOT_FOUND | Rewrite rule ID not registered | Register rule before use; check rule ID spelling | ❌ No |
| 9751 | REWRITE_RULE_INVALID | Rewrite rule definition malformed | Fix rule definition (regex, conditions, transformations) | ❌ No |
| 9752 | REWRITE_PHASE_VIOLATION | Attempted out-of-order phase execution | Respect phase ordering: 1→2→3→4 | ❌ No |
| 9753 | REWRITE_MAX_STEPS_EXCEEDED | Rewrite exceeded max transformation steps | Increase max_steps in context; check for infinite loops | ❌ No |
| 9754 | REWRITE_DOCUMENT_INVALID | Document state invalid for rewrite | Provide valid document; check prior transformations | ❌ No |
| 9755 | REWRITE_UNSAFE_TRANSFORMATION | Rewrite produced unsafe/blocked content | Change input; remove problematic patterns | ❌ No |
| 9756 | REWRITE_TRACE_FAILED | Rewrite trace generation failed | Disable tracing if not needed; check disk space | ✅ Yes |
| 9757 | REWRITE_YAML_PARSE_ERROR | YAML rule config parse error | Fix YAML syntax; validate against schema | ❌ No |
| 9758 | REWRITE_REGEX_PATHOLOGICAL | Regex pattern triggers pathological behavior | Simplify regex; use bounded quantifiers | ❌ No |

### Concurrency Errors (9800–9849)

These errors indicate concurrency issues and race conditions.

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9800 | CONCURRENCY_RACE_DETECTED | Concurrent mutation race condition | Retry operation; may need to serialize access | ✅ Yes |
| 9801 | CONCURRENCY_DEADLOCK | Deadlock detected in concurrent access | Restart; avoid acquiring locks in circular order | ❌ No |
| 9802 | CONCURRENCY_STARVATION | Thread starvation under contention | Reduce contention; use separate locks for independent data | ❌ No |
| 9803 | CONCURRENCY_ORDERING_VIOLATION | Concurrent operations completed in invalid order | Retry; add explicit ordering constraints | ✅ Yes |
| 9804 | CONCURRENCY_STATE_INCONSISTENT | State machine inconsistency from concurrent access | Clear corrupted state; restart operation | ✅ Yes |

### Configuration Errors (9850–9899)

These errors indicate problems with configuration files or settings.

| Code | Name | Cause | Recovery | Retryable |
|------|------|-------|----------|-----------|
| 9850 | CONFIG_INVALID_YAML | YAML configuration parse error | Fix YAML syntax (indentation, brackets, escaping) | ❌ No |
| 9851 | CONFIG_MISSING_REQUIRED_FIELD | Required config field not provided | Add required field to config file | ❌ No |
| 9852 | CONFIG_INVALID_VALUE | Config value out of valid range | Provide value within allowed range | ❌ No |
| 9853 | CONFIG_INCOMPATIBLE_OPTIONS | Mutually incompatible config options | Remove conflicting options; choose one | ❌ No |
| 9854 | CONFIG_FILE_NOT_FOUND | Configuration file not found | Create config file; verify file path | ❌ No |
| 9855 | CONFIG_PERMISSION_DENIED | Insufficient permissions to read config | Change file permissions; run as appropriate user | ❌ No |
| 9856 | CONFIG_SCHEMA_MISMATCH | Config doesn't match expected schema version | Update config to current schema version | ❌ No |

---

## Error Context Best Practices

Every error MUST be accompanied by `PromptEngineeringErrorContext`:

```cpp
struct PromptEngineeringErrorContext {
    PromptEngineeringErrorCode code;      // The error code
    std::string message;                  // Human-readable message
    std::string template_id;              // If applicable
    std::string version_id;               // If applicable
    std::string operation;                // create/get/inject/optimize/evaluate/rewrite
    std::string remediation_hint;         // What to do next
    uint32_t retry_count;                 // How many retries already attempted
    bool is_retryable;                    // Can this operation be safely retried?
};
```

---

## Recovery Strategies by Category

### Template Errors (9500–9509)
**General strategy:** Fix template definition, validate, and retry
- **Immediate actions:** Check template ID, syntax, placeholders
- **Long-term:** Implement template validation before registration

### Injection Errors (9550–9559)
**General strategy:** Verify context matches template requirements
- **Immediate actions:** Check context keys, types, sizes
- **Long-term:** Implement schema validation for context

### Version Errors (9600–9649)
**General strategy:** Handle conflicts, retry if transient
- **Immediate actions:** Check version ID, disk space, permissions
- **Long-term:** Implement conflict resolution policies

### Optimization Errors (9650–9699)
**General strategy:** Adjust parameters, retry, escalate if persistent
- **Immediate actions:** Adjust learning rate, iteration limits
- **Long-term:** Monitor convergence; implement adaptive strategies

### Evaluator Errors (9700–9749)
**General strategy:** Restart evaluator, check dependencies
- **Immediate actions:** Verify model availability, restart
- **Long-term:** Implement fallback evaluators

### Rewrite Errors (9750–9799)
**General strategy:** Validate rules, check phase ordering
- **Immediate actions:** Fix YAML, verify rule registration
- **Long-term:** Implement rule validation before load

### Concurrency Errors (9800–9849)
**General strategy:** Retry, reduce contention
- **Immediate actions:** Retry operation; check lock contention
- **Long-term:** Implement better locking strategies

### Configuration Errors (9850–9899)
**General strategy:** Fix config, restart
- **Immediate actions:** Validate YAML, check file permissions
- **Long-term:** Implement config validation, schema versioning

---

## Usage in Code

### Checking Errors

```cpp
auto result = template_manager->get_template(template_id);
if (!result.success) {
    auto ctx = result.error_context;
    LOG(ERROR) << error_code_to_string(ctx.code)
               << " | Template: " << ctx.template_id
               << " | Hint: " << ctx.remediation_hint;
    
    if (ctx.is_retryable && ctx.retry_count < MAX_RETRIES) {
        // Retry after backoff
    } else {
        // Fail permanently
    }
}
```

### Documenting Errors in Functions

```cpp
/**
 * @brief Create a new prompt template.
 * @param template_id Unique template identifier
 * @return Result with error code on failure
 * @throws Throws PromptEngineeringException with error context
 *
 * Possible error codes:
 * - TEMPLATE_INVALID_ID: template_id is empty or malformed
 * - TEMPLATE_ALREADY_EXISTS: template_id already registered
 * - TEMPLATE_VALIDATION_FAILED: template structure invalid
 *
 * Error context includes template_id and remediation_hint.
 */
TemplateResult create_template(const std::string& template_id);
```

---

## References

- Header: `include/prompt_engineering/prompt_engineering_errors.h`
- Backward Compatibility: `src/prompt_engineering/PHASE_1_CONTRACT.md`
- ROADMAP: `src/prompt_engineering/ROADMAP.md`
