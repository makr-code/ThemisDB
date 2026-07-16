# PERFORMANCE_EXPECTATIONS - src/auth

## Scope

- Module: src/auth
- This file defines measurable auth module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_auth_token_validation.cpp
  - benchmarks/bench_security.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| AUT-1 | JWT validation path remains within release baseline budget | BM_JWT_ValidToken_RS256, BM_JWT_ValidToken_WithBlacklist |
| AUT-2 | invalid/expired issuer and token-error paths remain bounded | BM_JWT_ExpiredToken, BM_JWT_WrongIssuer |
| AUT-3 | token blacklist lookup hit/miss paths remain bounded | BM_TokenBlacklist_IsRevoked_Hit, BM_TokenBlacklist_IsRevoked_Miss |
| AUT-4 | MFA/TOTP validation path remains bounded | BM_TOTP_Validate |
| AUT-5 | auth middleware static-token path remains bounded | BM_AuthMiddleware_StaticToken_Single, BM_AuthMiddleware_StaticToken_1000 |
| AUT-6 | security-policy and injection safety checks remain bounded | BM_AQLInjection_SafeQuery, BM_AQLInjection_MaliciousQuery |
| AUT-7 | RBAC permission-check path remains bounded | BM_RBAC_PermissionCheck_SingleRole, BM_RBAC_PermissionCheck_ManyRoles |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| AG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| AG-2 | token/revocation/middleware path p99 <= release threshold | p99 from mapped auth token-validation benchmark cases |
| AG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: auth/performance)

- Verified benchmark sources:
  - benchmarks/bench_auth_token_validation.cpp
  - benchmarks/bench_security.cpp
- Verified mapping surfaces:
  - JWT and blacklist benchmark paths
  - MFA and middleware benchmark paths
  - security policy/injection and RBAC benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.