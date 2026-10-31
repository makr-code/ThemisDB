# PERFORMANCE_EXPECTATIONS - src/auth

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

## Scope

- Module: src/auth
- This file defines measurable auth module performance expectations for release gating.
- Inventory checked: 2026-10-31. The targets below are source-defined thresholds, not hardware measurements.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/auth/bench_auth_hotpaths.cpp  (AHP-02..11 registered; AHP-01 described only)
  - benchmarks/server/bench_auth_token_validation.cpp
  - benchmarks/security/bench_security.cpp

## Auth Hot-Path Cases and Source-Defined Gates

The benchmark file documents AHP-01, but the code registers only AHP-02 through
AHP-11. The source defines release thresholds for GATE-AHP-01 through
GATE-AHP-06 only, mapped to AHP-02 through AHP-07; AHP-08 through AHP-11 have
no gates in that source. **Hardware-measured values must be refreshed on
representative production hardware** (x86-64, ≥ 3.0 GHz, ≥ 16 GB RAM) before
each major release.  The thresholds below are design targets derived from the
auth module contract (§5 Revocation, §6 Provider capability).

| Gate ID     | Benchmark | Metric   | Design Target (p99) | Notes                        |
|-------------|-----------|----------|---------------------|------------------------------|
| GATE-AHP-01 | AHP-02    | p99      | ≤ 1 µs              | In-memory blacklist hit      |
| GATE-AHP-02 | AHP-03    | p99      | ≤ 1 µs              | In-memory blacklist miss     |
| GATE-AHP-03 | AHP-04    | p99      | ≤ 5 ms              | Session create               |
| GATE-AHP-04 | AHP-05    | p99      | ≤ 1 ms              | Session validate             |
| GATE-AHP-05 | AHP-06    | p99      | ≤ 2 ms              | Distributed add (RocksDB)    |
| GATE-AHP-06 | AHP-07    | p99      | ≤ 1 µs (warm)       | Distributed isRevoked        |
### Re-baseline procedure

1. Run on representative production hardware with `cmake --preset linux-release` and
   `THEMIS_BUILD_BENCHMARKS=ON`.
2. Execute `bench_auth_hotpaths --benchmark_format=json > ahp_baseline.json`.
3. Compare p99 values against design targets above.
4. If all p99 values are within target, record the measured values as the new
   hardware baseline in this file under "Measured Baselines" below.
5. If any p99 exceeds target by > 10%, open a performance regression issue
   before promoting the release.

### Measured Baselines

> **NOTE: Hardware-measured baselines pending.  The values above are design
> targets.  Run the bench_auth_hotpaths suite on representative production
> hardware and update this section with measured p95/p99 values.**

| Benchmark | p50 (µs) | p95 (µs) | p99 (µs) | Hardware | Date      |
|-----------|----------|----------|----------|----------|-----------|
| (pending) | —        | —        | —        | —        | pending   |

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| AUT-1 | JWT validation path remains within release baseline budget | BM_JWT_ValidToken_RS256, BM_JWT_ValidToken_WithBlacklist |
| AUT-2 | invalid/expired issuer and token-error paths remain bounded | BM_JWT_ExpiredToken, BM_JWT_WrongIssuer |
| AUT-3 | token blacklist hit path remains bounded | BM_TokenBlacklist_IsRevoked_Hit |
| AUT-4 | MFA/TOTP validation path remains bounded | BM_TOTP_Validate |
| AUT-5 | auth middleware static-token path remains bounded | BM_AuthMiddleware_StaticToken_Single, BM_AuthMiddleware_StaticToken_1000 |
| AUT-6 | security-policy and injection safety checks remain bounded | BM_AQLInjection_SafeQuery, BM_AQLInjection_MaliciousQuery |
| AUT-7 | RBAC permission-check path remains bounded | BM_RBAC_PermissionCheck_SingleRole, BM_RBAC_PermissionCheck_ManyRoles |
| AUT-8 | LDAP bind simulation path tracked (AHP-09) | BM_AHP09_LDAPBindSimulation |
| AUT-9 | OIDC token parse path tracked (AHP-10) | BM_AHP10_OIDCTokenParse |
| AUT-10 | Federation realm+trust lookup tracked (AHP-11) | BM_AHP11_FederationRealmTrustLookup |

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
  - benchmarks/auth/bench_auth_hotpaths.cpp
  - benchmarks/server/bench_auth_token_validation.cpp
  - benchmarks/security/bench_security.cpp
- Verified mapping surfaces:
  - JWT and blacklist benchmark paths
  - MFA and middleware benchmark paths
  - security policy/injection and RBAC benchmark paths
  - LDAP bind simulation, OIDC token parse, federation realm trust lookup (AHP-09..11)
- Result:
  - Registered AHP-02..11 cases and referenced token-validation/security cases exist in the source files listed above; AHP-01 is documented in comments but is not implemented/registered.
  - `bench_auth_hotpaths` and `bench_security` are registered in CMake. The inspected CMake files do not register a `bench_auth_token_validation` executable.
  - Only GATE-AHP-01..06 thresholds are present in `bench_auth_hotpaths.cpp`; do not treat other cases or the AUT mapping as release gates.
  - Hardware-measured p95/p99 baselines must be recorded in "Measured Baselines" table above.