### Context

This issue implements the roadmap item 'Async / Non-Blocking LDAP and HTTP Authentication Calls' for the auth domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: 2. Async / Non-Blocking LDAP and HTTP Authentication Calls

### Goal

Deliver the scoped changes for Async / Non-Blocking LDAP and HTTP Authentication Calls in src/auth/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### 2. Async / Non-Blocking LDAP and HTTP Authentication Calls

**Priority:** High  
**Target Version:** v1.2.0

`ldap_authenticator.cpp` uses exclusively synchronous blocking calls: `ldap_simple_bind_s()` (line 222), `ldap_search_s()` (line 257), `ldap_search_ext_s()` (line 379), `ldap_start_tls_s()` (line 333). `jwt_validator.cpp:132` calls `curl_easy_perform()` synchronously with an inline `std::this_thread::sleep_for` retry loop (lines 118, 145). `oidc_provider.cpp:230` and `oauth_pkce_flow.cpp:214` and `oauth_device_flow.cpp:198` each call `curl_easy_perform()` / `httpPost()` on the caller's thread. This means any network timeout or LDAP server slowdown stalls the entire calling thread.

**Implementation Notes:**
- `[ ]` Introduce a dedicated `AuthWorkerThreadPool` (min 4, max 32 threads) in `ldap_authenticator.cpp` so that `authenticate()` dispatches to the pool and returns a `std::future<LDAPAuthResult>`
- `[ ]` Replace `curl_easy_perform()` in `jwt_validator.cpp:fetchJWKS()`, `oidc_provider.cpp:httpGet()`, `oauth_pkce_flow.cpp`, and `oauth_device_flow.cpp` with `curl_multi_perform()` calls on a shared multi-handle, or migrate to libcurl's async event interface (CURLMOPT_TIMERFUNCTION)
- `[ ]` Remove `std::this_thread::sleep_for` retry-back-off inside `fetchJWKS()` (lines 118, 145) — implement exponential back-off via timer callbacks that do not block the caller
- `[ ]` Expose async variants (`authenticateAsync()`, `validateAsync()`) on existing public interfaces so callers can use `co_await` / `std::future`

**Performance Targets:**
- LDAP bind latency P99 ≤ 50 ms visible to callers even when backend latency is 200 ms (no head-of-line blocking)
- JWT JWKS refresh never blocks the validation hot path for more than 1 ms

---

### Acceptance Criteria

- [ ] Introduce a dedicated `AuthWorkerThreadPool` (min 4, max 32 threads) in `ldap_authenticator.cpp` so that `authenticate()` dispatches to the pool and returns a `std::future<LDAPAuthResult>`
- [ ] Replace `curl_easy_perform()` in `jwt_validator.cpp:fetchJWKS()`, `oidc_provider.cpp:httpGet()`, `oauth_pkce_flow.cpp`, and `oauth_device_flow.cpp` with `curl_multi_perform()` calls on a shared multi-handle, or migrate to libcurl's async event interface (CURLMOPT_TIMERFUNCTION)
- [ ] Remove `std::this_thread::sleep_for` retry-back-off inside `fetchJWKS()` (lines 118, 145) — implement exponential back-off via timer callbacks that do not block the caller
- [ ] Expose async variants (`authenticateAsync()`, `validateAsync()`) on existing public interfaces so callers can use `co_await` / `std::future`
- [ ] LDAP bind latency P99 ≤ 50 ms visible to callers even when backend latency is 200 ms (no head-of-line blocking)
- [ ] JWT JWKS refresh never blocks the validation hot path for more than 1 ms

### Relationships

- Roadmap row: #8 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#2-async--non-blocking-ldap-and-http-authentication-calls
- Source key: roadmap:8:auth:v1.2.0:2-async-non-blocking-ldap-and-http-authentication-calls

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:8:auth:v1.2.0:2-async-non-blocking-ldap-and-http-authentication-calls -->
<!-- roadmap-ref: row=8;module=auth;target=v1.2.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#2-async--non-blocking-ldap-and-http-authentication-calls -->
