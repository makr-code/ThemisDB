### Context

This issue implements the roadmap item 'Credential Stuffing Detection: Persistent Cross-Session State' for the auth domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.3.0.

Primary detail section: 13. Credential Stuffing Detection: Persistent Cross-Session State

### Goal

Deliver the scoped changes for Credential Stuffing Detection: Persistent Cross-Session State in src/auth/ and complete the linked detail section in a release-ready state for v1.3.0.

### Detailed Scope

### 13. Credential Stuffing Detection: Persistent Cross-Session State

**Priority:** Medium  
**Target Version:** v1.3.0

`auth_rate_limiter.cpp:463` gates credential stuffing detection on `config_.enable_credential_stuffing_detection` but the underlying counters are in-memory only. Cross-session detection (tracking a user across multiple login sessions over hours) requires persisted, time-windowed counters. Currently, process restart resets all detection state.

**Implementation Notes:**
- `[ ]` Store credential-stuffing counters in the same `IRateLimiterBackend` (Redis) with a dedicated key namespace `cs:{user_id}:{day}`
- `[ ]` Implement exponential back-off lock-out: first breach triggers CAPTCHA requirement, second triggers email OTP, third triggers 24-hour account lock (`auth_rate_limiter.cpp:282-300`)
- `[ ]` Expose `credential_stuffing_attempts_total` metric counter in `auth_metrics.cpp` with labels `{user_id, ip, outcome}`

---

### Acceptance Criteria

- [ ] Store credential-stuffing counters in the same `IRateLimiterBackend` (Redis) with a dedicated key namespace `cs:{user_id}:{day}`
- [ ] Implement exponential back-off lock-out: first breach triggers CAPTCHA requirement, second triggers email OTP, third triggers 24-hour account lock (`auth_rate_limiter.cpp:282-300`)
- [ ] Expose `credential_stuffing_attempts_total` metric counter in `auth_metrics.cpp` with labels `{user_id, ip, outcome}`

### Relationships

- Roadmap row: #152 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#13-credential-stuffing-detection-persistent-cross-session-state
- Source key: roadmap:152:auth:v1.3.0:13-credential-stuffing-detection-persistent-cross-session-state

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:152:auth:v1.3.0:13-credential-stuffing-detection-persistent-cross-session-state -->
<!-- roadmap-ref: row=152;module=auth;target=v1.3.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#13-credential-stuffing-detection-persistent-cross-session-state -->
