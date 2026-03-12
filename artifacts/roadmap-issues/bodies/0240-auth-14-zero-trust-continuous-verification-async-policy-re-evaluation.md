### Context

This issue implements the roadmap item 'Zero-Trust Continuous Verification: Async Policy Re-evaluation' for the auth domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.4.0.

Primary detail section: 14. Zero-Trust Continuous Verification: Async Policy Re-evaluation

### Goal

Deliver the scoped changes for Zero-Trust Continuous Verification: Async Policy Re-evaluation in src/auth/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### 14. Zero-Trust Continuous Verification: Async Policy Re-evaluation

**Priority:** Medium  
**Target Version:** v1.4.0

`zero_trust_auth_verifier.cpp` currently performs synchronous policy evaluation. For long-lived connections (WebSocket, gRPC streaming, DB connection pool), the zero-trust posture of a session must be re-evaluated periodically without dropping the connection.

**Implementation Notes:**
- `[ ]` Add `std::chrono::seconds re_evaluation_interval{300}` to `ZeroTrustConfig` in `include/auth/zero_trust_auth_verifier.h`
- `[ ]` Implement background re-evaluation loop: per-session timer fires every `re_evaluation_interval`; if policy check fails, signal the session manager to revoke the session via `session_manager.cpp:terminateSession()`
- `[ ]` Re-evaluation must not block the data-plane thread; dispatch to `AuthWorkerThreadPool` (see Feature 2)
- `[ ]` Emit audit event `zero_trust/re_evaluation_failed` via `auth_audit_logger.cpp` when continuous check revokes an active session

---

### Acceptance Criteria

- [ ] Add `std::chrono::seconds re_evaluation_interval{300}` to `ZeroTrustConfig` in `include/auth/zero_trust_auth_verifier.h`
- [ ] Implement background re-evaluation loop: per-session timer fires every `re_evaluation_interval`; if policy check fails, signal the session manager to revoke the session via `session_manager.cpp:terminateSession()`
- [ ] Re-evaluation must not block the data-plane thread; dispatch to `AuthWorkerThreadPool` (see Feature 2)
- [ ] Emit audit event `zero_trust/re_evaluation_failed` via `auth_audit_logger.cpp` when continuous check revokes an active session

### Relationships

- Roadmap row: #240 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#14-zero-trust-continuous-verification-async-policy-re-evaluation
- Source key: roadmap:240:auth:v1.4.0:14-zero-trust-continuous-verification-async-policy-re-evaluation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:240:auth:v1.4.0:14-zero-trust-continuous-verification-async-policy-re-evaluation -->
<!-- roadmap-ref: row=240;module=auth;target=v1.4.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#14-zero-trust-continuous-verification-async-policy-re-evaluation -->
