### Context

This issue implements the roadmap item 'Mandatory JWT Issuer and Audience Validation' for the auth domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: 5. Mandatory JWT Issuer and Audience Validation

### Goal

Deliver the scoped changes for Mandatory JWT Issuer and Audience Validation in src/auth/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### 5. Mandatory JWT Issuer and Audience Validation

**Priority:** High (Security)  
**Target Version:** v1.1.0

`jwt_validator.cpp:506` validates `iss` only if `cfg_.expected_issuer` is non-empty, and `jwt_validator.cpp:345` validates `aud` only if `cfg_.expected_audience` is non-empty. A misconfigured deployment (empty strings, which is the default) silently accepts tokens from **any** issuer and **any** audience. In a multi-tenant or microservice environment this allows token substitution attacks.

**Implementation Notes:**
- `[ ]` In `JWTValidator::Config` (`jwt_validator.h:83-84`), replace `expected_issuer`/`expected_audience` plain strings with `std::optional<std::string>` and add a `bool require_issuer_validation = true` / `bool require_audience_validation = true` flag pair
- `[ ]` In `JWTValidator::validate()`, throw `std::runtime_error("Issuer validation not configured")` at startup (constructor) if `require_issuer_validation` is true but `expected_issuer` is empty (`jwt_validator.cpp:506`)
- `[ ]` Emit a `spdlog::warn` (audit-level) when either field is unset and the corresponding `require_*` flag is false, so operator misconfiguration is visible in logs
- `[ ]` Add unit test: validate token with correct issuer/audience, wrong issuer, wrong audience, missing issuer, missing audience — all permutations

---

### Acceptance Criteria

- [ ] In `JWTValidator::Config` (`jwt_validator.h:83-84`), replace `expected_issuer`/`expected_audience` plain strings with `std::optional<std::string>` and add a `bool require_issuer_validation = true` / `bool require_audience_validation = true` flag pair
- [ ] In `JWTValidator::validate()`, throw `std::runtime_error("Issuer validation not configured")` at startup (constructor) if `require_issuer_validation` is true but `expected_issuer` is empty (`jwt_validator.cpp:506`)
- [ ] Emit a `spdlog::warn` (audit-level) when either field is unset and the corresponding `require_*` flag is false, so operator misconfiguration is visible in logs
- [ ] Add unit test: validate token with correct issuer/audience, wrong issuer, wrong audience, missing issuer, missing audience — all permutations

### Relationships

- Roadmap row: #6 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#5-mandatory-jwt-issuer-and-audience-validation
- Source key: roadmap:6:auth:v1.1.0:5-mandatory-jwt-issuer-and-audience-validation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:6:auth:v1.1.0:5-mandatory-jwt-issuer-and-audience-validation -->
<!-- roadmap-ref: row=6;module=auth;target=v1.1.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#5-mandatory-jwt-issuer-and-audience-validation -->
