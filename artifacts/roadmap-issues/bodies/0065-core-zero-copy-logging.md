### Context

This issue implements the roadmap item 'Zero-Copy Logging' for the core domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Zero-Copy Logging

### Goal

Deliver the scoped changes for Zero-Copy Logging in src/core/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Zero-Copy Logging
**Priority:** High  
**Target Version:** v1.6.0

Reduce memory allocations in logging hot paths.

**Current:** String formatting and copying for every log call  
**Target:** Pre-allocated buffers and string_view usage

**Expected Improvement:** 30-50% reduction in logging overhead

---

### Acceptance Criteria

- [ ] Implement the scoped changes described in the linked detail section.
- [ ] Add or update tests that verify the intended behaviour.

### Relationships

- Roadmap row: #65 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/core/FUTURE_ENHANCEMENTS.md#zero-copy-logging
- Source key: roadmap:65:core:v1.6.0:zero-copy-logging

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:65:core:v1.6.0:zero-copy-logging -->
<!-- roadmap-ref: row=65;module=core;target=v1.6.0 -->
<!-- roadmap-detail: src/core/FUTURE_ENHANCEMENTS.md#zero-copy-logging -->
