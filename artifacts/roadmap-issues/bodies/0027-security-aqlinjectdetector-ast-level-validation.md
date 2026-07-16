### Context

This issue implements the roadmap item '`AQLInjectionDetector`: AST-Level Validation' for the security domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.4.0.

Primary detail section: `AQLInjectionDetector`: AST-Level Validation

### Goal

Deliver the scoped changes for `AQLInjectionDetector`: AST-Level Validation in src/security/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### `AQLInjectionDetector`: AST-Level Validation
**Priority:** High
**Target Version:** v1.4.0

`aql_injection_detector.cpp` line 246: "TODO (v1.4.0): Implement AST-level operation validation". The current detector operates on raw query strings using regex patterns. AST-level validation would catch injection attacks that evade regex by using non-standard whitespace, Unicode escapes, or concatenation.

**Implementation Notes:**
- `[ ]` Integrate with `src/query/aql_parser.cpp`: parse the query into an AST before validation.
- `[ ]` Walk the AST to detect disallowed operation nodes (e.g., `EXECUTE`, DDL operations in read-only contexts, unbounded `FOR` loops without `LIMIT`).
- `[ ]` Fall back to regex-based detection if AST parsing fails (defense in depth).
- `[ ]` Add unit tests: queries that bypass regex but have dangerous AST nodes are detected.

---

### Acceptance Criteria

- [ ] Integrate with `src/query/aql_parser.cpp`: parse the query into an AST before validation.
- [ ] Walk the AST to detect disallowed operation nodes (e.g., `EXECUTE`, DDL operations in read-only contexts, unbounded `FOR` loops without `LIMIT`).
- [ ] Fall back to regex-based detection if AST parsing fails (defense in depth).
- [ ] Add unit tests: queries that bypass regex but have dangerous AST nodes are detected.

### Relationships

- Roadmap row: #27 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/security/FUTURE_ENHANCEMENTS.md#aqlinjectdetector-ast-level-validation
- Source key: roadmap:27:security:v1.4.0:aqlinjectdetector-ast-level-validation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:27:security:v1.4.0:aqlinjectdetector-ast-level-validation -->
<!-- roadmap-ref: row=27;module=security;target=v1.4.0 -->
<!-- roadmap-detail: src/security/FUTURE_ENHANCEMENTS.md#aqlinjectdetector-ast-level-validation -->
