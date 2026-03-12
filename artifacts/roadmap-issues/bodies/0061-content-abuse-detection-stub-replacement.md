### Context

This issue implements the roadmap item 'Abuse Detection Stub Replacement' for the content domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Abuse Detection Stub Replacement

### Goal

Deliver the scoped changes for Abuse Detection Stub Replacement in src/content/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Abuse Detection Stub Replacement
**Priority:** High
**Target Version:** v1.8.0

`content_security.cpp` has **2 confirmed stubs**: line 150 ("Check 3: Abuse detection (stub for future implementation)") and line 421 ("Stub implementation for future abuse detection"). Every content item passes abuse detection unconditionally. Malicious content (CSAM hashes, spam fingerprints) is not detected.

**Implementation Notes:**
- `[ ]` Define `IAbuseDetector` interface with `detect(content_data, metadata) → AbuseDetectionResult`.
- `[ ]` Implement `PhotoDNAAbuseDetector` backed by the PhotoDNA SDK (or open-source perceptual hash comparison against a blocklist) for image content; inject into `ContentSecurity` via constructor.
- `[ ]` Implement `TextAbuseDetector` using a configurable blocklist + regex patterns loaded from `config/security/abuse_patterns.yaml`; support `BLOCK` and `FLAG` actions per pattern.
- `[ ]` Wire both detectors into `ContentSecurity::check()` at line 150 (the stub location).
- `[ ]` Add unit tests for both `BLOCK` (content rejected) and `FLAG` (content stored with flag) outcomes.
- `[ ]` Audit log every detection event via `AuditLogger::logEvent()` with content hash, detector type, and action taken.

---

### Acceptance Criteria

- [ ] Define `IAbuseDetector` interface with `detect(content_data, metadata) → AbuseDetectionResult`.
- [ ] Implement `PhotoDNAAbuseDetector` backed by the PhotoDNA SDK (or open-source perceptual hash comparison against a blocklist) for image content; inject into `ContentSecurity` via constructor.
- [ ] Implement `TextAbuseDetector` using a configurable blocklist + regex patterns loaded from `config/security/abuse_patterns.yaml`; support `BLOCK` and `FLAG` actions per pattern.
- [ ] Wire both detectors into `ContentSecurity::check()` at line 150 (the stub location).
- [ ] Add unit tests for both `BLOCK` (content rejected) and `FLAG` (content stored with flag) outcomes.
- [ ] Audit log every detection event via `AuditLogger::logEvent()` with content hash, detector type, and action taken.

### Relationships

- Roadmap row: #61 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/content/FUTURE_ENHANCEMENTS.md#abuse-detection-stub-replacement
- Source key: roadmap:61:content:v1.8.0:abuse-detection-stub-replacement

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:61:content:v1.8.0:abuse-detection-stub-replacement -->
<!-- roadmap-ref: row=61;module=content;target=v1.8.0 -->
<!-- roadmap-detail: src/content/FUTURE_ENHANCEMENTS.md#abuse-detection-stub-replacement -->
