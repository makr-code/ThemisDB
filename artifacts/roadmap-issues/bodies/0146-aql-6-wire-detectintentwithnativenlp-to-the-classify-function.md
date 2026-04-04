### Context

This issue implements the roadmap item 'Wire `detectIntentWithNativeNLP()` to the CLASSIFY Function' for the aql domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: 6 · Wire `detectIntentWithNativeNLP()` to the CLASSIFY Function

### Goal

Deliver the scoped changes for Wire `detectIntentWithNativeNLP()` to the CLASSIFY Function in src/aql/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### 6 · Wire `detectIntentWithNativeNLP()` to the CLASSIFY Function
**Priority:** Medium
**Target Version:** v1.7.0

**Problem (from code):** `docs_assistant_functions.cpp:201` contains an explicit `TODO`:
```
// TODO: Integrate with native CLASSIFY function when execution context available
```
`detectIntentWithNativeNLP()` (line 196) always returns `"unknown"` and then falls through to the slower LLM path. The comment (lines 203–209) describes the intended call signature: `CLASSIFY(text, categories) -> {category, confidence, scores}`, but the function has no access to the AQL function registry at call time. This means every docs-assistant query that could be handled cheaply via the local CLASSIFY function instead triggers a full LLM round-trip.

**Implementation Notes:**
- `[ ]` Add a `FunctionRegistry*` or `IClassifyFn` interface pointer parameter to `DocsAssistantFunctions` (injectable via constructor or `setClassifier()`); when non-null, call it directly in `detectIntentWithNativeNLP()` instead of returning `"unknown"`
- `[ ]` Define an `IClassifyFn` interface: `virtual ClassifyResult classify(const std::string& text, const std::vector<std::string>& categories) const = 0`; provide a `NullClassifyFn` no-op fallback
- `[ ]` Register `AQLFunctionClassifyBridge` as the concrete implementation in the AQL module initialiser, binding it to the global function registry
- `[ ]` Remove the `return "unknown"` early exit once a real implementation is wired; the `catch` block at line 215 serves as the fallback
- `[ ]` Add an integration test that verifies `detectIntentWithNativeNLP("how do I create an index?")` returns `"configuration"` with confidence > 0.7 when the bridge is wired

---

### Acceptance Criteria

- [ ] Add a `FunctionRegistry*` or `IClassifyFn` interface pointer parameter to `DocsAssistantFunctions` (injectable via constructor or `setClassifier()`); when non-null, call it directly in `detectIntentWithNativeNLP()` instead of returning `"unknown"`
- [ ] Define an `IClassifyFn` interface: `virtual ClassifyResult classify(const std::string& text, const std::vector<std::string>& categories) const = 0`; provide a `NullClassifyFn` no-op fallback
- [ ] Register `AQLFunctionClassifyBridge` as the concrete implementation in the AQL module initialiser, binding it to the global function registry
- [ ] Remove the `return "unknown"` early exit once a real implementation is wired; the `catch` block at line 215 serves as the fallback
- [ ] Add an integration test that verifies `detectIntentWithNativeNLP("how do I create an index?")` returns `"configuration"` with confidence > 0.7 when the bridge is wired

### Relationships

- Roadmap row: #146 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/aql/FUTURE_ENHANCEMENTS.md#6--wire-detectintentwithnativenlp-to-the-classify-function
- Source key: roadmap:146:aql:v1.7.0:6-wire-detectintentwithnativenlp-to-the-classify-function

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:146:aql:v1.7.0:6-wire-detectintentwithnativenlp-to-the-classify-function -->
<!-- roadmap-ref: row=146;module=aql;target=v1.7.0 -->
<!-- roadmap-detail: src/aql/FUTURE_ENHANCEMENTS.md#6--wire-detectintentwithnativenlp-to-the-classify-function -->
