### Context

This issue implements the roadmap item 'JWT JTI Replay Prevention Warning' for the auth domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.2.0.

Primary detail section: 6. JWT JTI Replay Prevention Warning When JTI Is Absent

### Goal

Deliver the scoped changes for JWT JTI Replay Prevention Warning in src/auth/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### 6. JWT JTI Replay Prevention Warning When JTI Is Absent

**Priority:** Medium (Security)  
**Target Version:** v1.2.0

`jwt_validator.cpp:446` extracts `jti` but only connects it to the blacklist check at line 559 when `!claims.jti.empty()`. Tokens without a `jti` claim bypass per-token revocation entirely with no warning. This is spec-compliant (JTI is optional per RFC 7519) but dangerous in deployments where revocation is expected.

**Implementation Notes:**
- `[ ]` Add `bool require_jti = false` to `JWTValidator::Config`; when `true`, reject tokens missing `jti` with `throw std::runtime_error("Missing required jti claim")` (`jwt_validator.cpp:446`)
- `[ ]` When `token_blacklist_` is set but incoming token has no `jti`, emit `spdlog::warn("JWT has no jti; per-token revocation impossible for this token")` — operators need visibility (`jwt_validator.cpp:558`)
- `[ ]` Document the `require_jti` flag and its security implications in the module README

---

### Acceptance Criteria

- [ ] Add `bool require_jti = false` to `JWTValidator::Config`; when `true`, reject tokens missing `jti` with `throw std::runtime_error("Missing required jti claim")` (`jwt_validator.cpp:446`)
- [ ] When `token_blacklist_` is set but incoming token has no `jti`, emit `spdlog::warn("JWT has no jti; per-token revocation impossible for this token")` — operators need visibility (`jwt_validator.cpp:558`)
- [ ] Document the `require_jti` flag and its security implications in the module README

### Relationships

- Roadmap row: #149 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#6-jwt-jti-replay-prevention-warning-when-jti-is-absent
- Source key: roadmap:149:auth:v1.2.0:6-jwt-jti-replay-prevention-warning-when-jti-is-absent

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:149:auth:v1.2.0:6-jwt-jti-replay-prevention-warning-when-jti-is-absent -->
<!-- roadmap-ref: row=149;module=auth;target=v1.2.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#6-jwt-jti-replay-prevention-warning-when-jti-is-absent -->
