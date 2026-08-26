# Auth Module Contract

Datum: 2026-08-03  
**Status:** Active  
**Module:** auth (Authentication, authorization, principal contracts)  
**Primary:** include/auth/auth_principal_contract.h, src/auth/ROADMAP.md

## Public API Surface

| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `authenticate()` | `themis::auth::IAuthProvider` | Credentials (JWT/OAuth2/SAML, max 8KB), context (IP, UA) | AuthResult (principal, permissions, expiry_timestamp) | InvalidCredentialError, ExpiredTokenError, RateLimitError | ✅ Thread-safe (stateless validator) | Principal owned by return; valid until expiry | P0; GATE-AUTH-01 ≤5µs (JWT) |
| `authorize()` | `themis::auth::IAuthProvider` | Principal (from authenticate), resource, action (READ/WRITE/DELETE/ADMIN) | AuthorizationDecision (allowed=true/false, reason) | None (always returns decision; log denials) | ✅ Thread-safe (RBAC matrix lookup) | Decision owned by return | P0; GATE-AUTH-02 ≤1µs |
| `validatePrincipal()` | `themis::auth::IPrincipal` | N/A (called on principal itself) | bool (true if principal still valid, not revoked/expired) | None | ✅ Lock-free snapshot | N/A (validation only) | P0; GATE-AUTH-03 ≤1µs |
| `getPrincipal()` | `themis::auth::IAuthProvider` | Principal ID or name (string) | IPrincipal* (borrowed) or null if not found | None (returns null if not found) | ✅ Thread-safe (registry lookup) | Borrowed reference; valid for session lifetime | P1 (lookup) |
| `createPrincipal()` | `themis::auth::IAuthProvider` | PrincipalSpec (name, type, initial_roles), admin_context | std::unique_ptr<IPrincipal> | InvalidSpecError (bad name/type), PermissionError (not admin) | 🔒 Single-threaded (must serialize principal creation) | Caller owns returned principal; typically stored in registry | P1 (admin op); rare |
| `revokePrincipal()` | `themis::auth::IAuthProvider` | Principal ID (string) | void (principal immediately invalid; active sessions notified) | PrincipalNotFoundError, PermissionError | ✅ Thread-safe (atomic revocation) | N/A (mutation only) | P1 (admin); async notification |
| `grantRole()` | `themis::auth::IAuthProvider` | Principal, Role name (e.g., "data_analyst", "admin"), expires_at (optional) | void (role added to principal; takes effect immediately) | RoleNotFoundError, PermissionError | ✅ Thread-safe (atomic) | N/A (mutation only) | P1; role is string token |
| `revokeRole()` | `themis::auth::IAuthProvider` | Principal, Role name | void (role removed; existing tokens still valid until expiry) | RoleNotFoundError, PermissionError | ✅ Thread-safe (atomic) | N/A (mutation only) | P1; revocation doesn't recall tokens |
| `checkPermission()` | `themis::auth::IPrincipal` | Resource (string), action (READ/WRITE/DELETE/ADMIN) | bool (true if principal has permission) | None | ✅ Lock-free (role set immutable per session) | N/A (query only) | P0; called per-query |

## Principal Contract (v1.x Frozen)

| Property | Type | Immutable? | Details |
|---|---|---|---|
| principal_id | UUID | Yes | Unique identifier; set at creation |
| name | String | Yes (in v1.x) | Human-readable name; unique per type |
| principal_type | Enum (USER/SERVICE/ROLE) | Yes | Determines auth flow (JWT vs service key) |
| roles | Set<String> | No (may change) | Role membership; changes take effect in new sessions |
| expires_at | Timestamp | No (may extend) | Account expiration; null = no expiration |
| attributes | Map<String, String> | No | Custom metadata (team, department, etc.) |
| created_at | Timestamp | Yes | Audit trail |
| last_auth_at | Timestamp | No (updated) | Last successful authentication |

## Authentication Flows

| Flow | Input | Output | Timeout | Test |
|---|---|---|---|---|
| **JWT** | ****** (3 parts: header.payload.sig) | Principal + expiry | 5ms (sig verify) | test_auth_jwt_validation.cpp |
| **OAuth2** | Code + state (authorization code flow) | Access token + principal | 500ms (provider call) | test_auth_oauth2_flow.cpp |
| **SAML** | Assertion XML (signed) | Principal + session | 100ms (sig verify) | test_auth_saml_assertion.cpp |
| **Service Key** | API key (base64, 32B random) | Principal (service account) | 1ms (cache lookup) | test_auth_service_key.cpp |

## Authorization (RBAC)

| Pattern | Rules | Enforcement |
|---|---|---|
| **Role-Based Access Control (RBAC)** | Principal has set of roles; each role grants actions on resources | Checked per request via role membership |
| **Resource-Level** | `data_analyst` role grants READ on schema.tables, no WRITE | Fine-grained role definition |
| **Action-Level** | READ (query), WRITE (insert/update), DELETE, ADMIN (schema changes) | Mapped to HTTP methods (GET, POST/PUT, DELETE, special) |

**Example:**
```
Principal: alice (roles: [data_analyst, project_lead])
Resource: project_123.dataset_1
Action: WRITE
Authorization: DENY (data_analyst role grants READ only on datasets)
```

## Concurrency & Session Management

| Scenario | Behavior | Test |
|---|---|---|
| Concurrent logins (same principal) | Multiple sessions allowed; each gets unique session_id | test_auth_concurrent_sessions.cpp |
| Revoke principal + active sessions | Sessions remain valid until natural expiry or token refresh | test_auth_principal_revocation.cpp |
| Role change during session | New role assignments take effect on next request (not retroactive) | test_auth_role_update_during_session.cpp |

## Error Categories & Codes

| Error | Code | When | Recovery |
|---|---|---|---|
| InvalidCredentialError | 401 Unauthorized | Bad signature, expired, tampered token | Retry with fresh credential |
| ExpiredTokenError | 401 Unauthorized | Token past expiry_at | Refresh token or re-authenticate |
| RateLimitError | 429 Too Many Requests | >100 failed auth attempts in 60s | Backoff + retry; account may be locked |
| PermissionError | 403 Forbidden | Principal lacks required role/permission | Escalate to admin or use different principal |
| PrincipalNotFoundError | 404 Not Found | Principal ID doesn't exist (never created or revoked) | Check principal ID; create if needed |
| InvalidSpecError | 400 Bad Request | PrincipalSpec has invalid field | Validate spec fields; retry |

## Cryptographic & Security Commitments

| Aspect | Requirement | Notes |
|---|---|---|
| **JWT Signature** | RS256 (RSA) or ES256 (ECDSA); min 2048-bit RSA | Verified in P0 gate ≤5µs |
| **SAML Signature** | RSA-SHA256 or ECDSA-SHA256 | Verified; assertion must have NotOnOrAfter attribute |
| **Service Keys** | 256-bit random (base64 encoded); rotated yearly | Stored hashed (Argon2id); never logged plain |
| **Password Hashing** | Argon2id (t=2, m=19, p=1) if password-based | Min 8 char; enforced by policy |
| **Rate Limiting** | Max 10 failed attempts / 60s per IP → 5min lockout | Configurable; defaults in code |

## Performance Commitments (Release Gates)

| Gate | Latency | Concurrency | Credential Type | Test |
|---|---|---|---|---|
| GATE-AUTH-01 | authenticate() ≤5 µs | 64 concurrent | JWT (cached key) | bench_auth_release_gates.cpp |
| GATE-AUTH-02 | authorize() ≤1 µs | 1K concurrent | RBAC matrix lookup | bench_auth_rbac_gates.cpp |
| GATE-AUTH-03 | validatePrincipal() ≤1 µs | 64 concurrent | Cache hit | bench_auth_validation_gates.cpp |
| GATE-AUTH-04 | grantRole() ≤10 µs | 1 admin | Atomic update | bench_auth_admin_gates.cpp |

## API Stability & Versioning

| Item | Status | Notes |
|---|---|---|
| IPrincipal interface | Public v1.x | Frozen; stored in persisted sessions |
| Principal contract (properties) | Frozen | Breaking changes = major version |
| Authentication flows (JWT/OAuth2/SAML) | Public v1.x | Stable; new flows in v2.0 |
| Authorization (RBAC) | Public v1.x | Stable; new models (ABAC, PBAC) in v2.0 |
| Error codes (12 auth codes) | Public v1.x | Frozen; new codes = new enum value |
| PrincipalSpec proto | Internal | May change; not exposed directly |

## Multi-Tenancy & Isolation

| Feature | Contract | Notes |
|---|---|---|
| Tenant ID | Part of principal context | Principals bound to single tenant |
| Cross-tenant access | Denied by default (all actions checked with tenant context) | Explicit admin override via special admin role |
| Session isolation | Sessions store tenant_id; verified on each op | Prevents token reuse across tenants |

---

**Zuletzt geprueft (Auth contracts):** 2026-08-03
