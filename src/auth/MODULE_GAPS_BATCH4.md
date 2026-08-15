# auth — MODULE_GAPS_BATCH4.md — CRITICAL Gap Closure

**Purpose**: Organize auth module CRITICAL findings into a fixable remediation batch.  
**Status**: Ready for gap-verifier review and themisdb-implementer fixes.  
**Target**: Q3 2026  
**Total Findings**: 57 CRITICAL + HIGH (filtered scope)

---

## Batch Overview

| Category | Count | Severity | Remediation Path |
|---|---|---|---|
| braces_imbalance | 8 files | CRITICAL | C++ linter fix; validate syntax |
| exception_in_destructor | 3 | CRITICAL | Remove throwing code; add noexcept |
| db_connection_leak | 1 | CRITICAL | Add RAII wrapper; resource guard |
| blocking_no_timeout | 2 | CRITICAL | Add timeout parameter; add guard |
| no_transit_encryption | 4+ | CRITICAL | Enforce TLS; code review + test |
| **Total Targeted** | **~20** | **CRITICAL** | **Focused hardening** |

---

## Findings by Category

### 1. Braces Imbalance (CRITICAL — 8 files)

Files affected:
- `auth_metrics.cpp:1`
- `federated_identity_manager.cpp:1`
- `oauth_device_flow.cpp:1`
- `oauth_pkce_flow.cpp:1`
- `oidc_provider.cpp:1`
- `saml_authenticator.cpp:1`
- `session_manager.cpp:1`
- `totp_secret_encryption.cpp:1`

**Remediation**: Validate C++ syntax; check for unmatched braces, missing `}`, or `#ifdef` imbalance.  
**Action**: gap-verifier confirms; fix via C++ linter or manual inspection.

---

### 2. Exception in Destructor (CRITICAL — 3)

| File | Line | Issue |
|---|---|---|
| totp_secret_encryption.cpp | 52 | Destructor may throw |
| mtls_authenticator.cpp | 122 | Destructor may throw |
| http_auth_async.cpp | 144 | Destructor may throw |

**Remediation**: Add `noexcept` to destructor; remove or wrap throwing operations.  
**Action**: gap-verifier confirms exception paths; themisdb-implementer adds noexcept guards.

---

### 3. Database Connection Leak (CRITICAL — 1)

| File | Line | Issue |
|---|---|---|
| gssapi_authenticator.cpp | 151 | Possible DB connection leak |

**Remediation**: Wrap connection in RAII guard; ensure cleanup in exception paths.  
**Action**: gap-verifier inspects code flow; themisdb-implementer adds RAII wrapper.

---

### 4. Blocking Without Timeout (CRITICAL — 2)

| File | Line | Issue |
|---|---|---|
| ldap_connection_pool.cpp | 157 | Blocking call; no timeout |
| jwt_validator.cpp | 181 | Blocking call; no timeout |

**Remediation**: Add timeout parameter or guard with `std::future::wait_for`.  
**Action**: gap-verifier confirms blocking path; themisdb-implementer adds timeout logic.

---

### 5. No Transit Encryption (CRITICAL — 4+)

| File | Line | Issue |
|---|---|---|
| http_auth_async.cpp | 183 | No TLS on HTTP auth |
| http_auth_async.cpp | 184 | No TLS on HTTP auth |
| http_auth_async.cpp | 188 | No TLS on HTTP auth |
| http_auth_async.cpp | 189 | No TLS on HTTP auth |

**Remediation**: Enforce HTTPS; add TLS/mTLS wrapper; audit HTTP usage.  
**Action**: gap-verifier confirms protocol; themisdb-implementer enforces TLS.

---

## High Priority (from CRITICAL + HIGH subset)

Beyond the targeted CRITICAL set, prioritize:

- **todo_as_productionlogic**: 62 instances — TODO blocks in production code
- **sensitive_data_logging**: 155 instances — Log PII or credentials
- **uncaught_exception**: 54 instances — Missing exception handlers
- **scope_mismatch** (selected): ~100 high-impact instances — Variable scope issues

---

## Execution Plan

1. **gap-verifier phase** (1 hour)
   - Confirm each finding's accuracy
   - Eliminate false positives
   - Re-assess severity if applicable
   - Document confirmation in findings section

2. **themisdb-implementer phase** (4–6 hours)
   - Fix each CRITICAL finding
   - Add tests to cover edge cases
   - Update ROADMAP.md to mark fixes
   - Commit with message format: `fix(auth): <category> - <file> - <brief description>`

3. **Verification phase** (1 hour)
   - Re-run gap scanner on fixed files
   - Confirm gap closure rate >= 80%
   - Document remediation in this file's **Status** section

---

## Success Criteria

- [ ] All 8 braces_imbalance files validate syntactically
- [ ] All 3 destructors marked noexcept and cleanup verified
- [ ] DB connection RAII wrapper in place
- [ ] Blocking calls guarded with timeouts
- [ ] HTTP auth converted to HTTPS/TLS
- [ ] Gap scanner re-run shows 80%+ CRITICAL closure rate
- [ ] ROADMAP.md updated with Wave D contribution progress

---

## References

- **MODULE_GAPS.md**: Full gap inventory (2759 findings)
- **ROADMAP.md**: Phase 6 documentation and Wave D planning
- **ARCHITECTURE.md**: Auth module design and contract
- **SECURITY.md**: Security hardening expectations

---

**Last Updated**: 2026-08-15  
**Assigned To**: gap-verifier → themisdb-implementer → code-review  
**Wave Context**: Wave A (release_critical); Wave D (Q1 2027 operability improvements)
