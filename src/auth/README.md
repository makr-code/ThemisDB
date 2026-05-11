> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Authentication Module

Comprehensive authentication and authorization implementation for ThemisDB with enterprise SSO and multi-factor authentication support.

## Module Purpose

Provides enterprise-grade authentication and authorization for ThemisDB, including JWT/OpenID Connect, Kerberos/GSSAPI, TOTP MFA, and RBAC enforcement.

## Subsystem Scope

**In scope:** JWT validation with JWKS, Kerberos/GSSAPI SSO, TOTP MFA with recovery codes, RBAC principal-to-role mapping, brute force rate limiting, session management and revocation.

**Out of scope:** User management (external IdP), secrets storage (handled by security module), audit logging (handled by utils module).

## Relevant Interfaces

- `jwt_validator.cpp` — JWT/OpenID Connect validation
- `gssapi_authenticator.cpp` — GSSAPI/Kerberos handler
- `mfa_authenticator.cpp` — TOTP MFA with recovery codes
- `oauth_device_flow.cpp` — OAuth 2.0 device authorization flow (RFC 8628)
- `oauth_pkce_flow.cpp` — OAuth 2.0 Authorization Code Grant with PKCE for public clients (RFC 7636)
- `oidc_provider.cpp` — OIDC Provider Discovery and federated identity integration
- `saml_authenticator.cpp` — SAML 2.0 SP-initiated and IdP-initiated SSO
- `ldap_authenticator.cpp` — LDAP/Active Directory direct bind authentication
- `api_key_authenticator.cpp` — static API key + secret authentication
- `mtls_authenticator.cpp` — certificate-based mutual TLS authentication
- `webauthn_authenticator.cpp` — WebAuthn/FIDO2 hardware token authentication
- `federated_identity_manager.cpp` — federated identity across multiple realms
- `session_manager.cpp` — session lifecycle (create / validate / revoke / list) with idle and absolute timeouts
- `zero_trust_auth_verifier.cpp` — zero-trust continuous verification per request

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — JWT, Kerberos, TOTP, RBAC, OAuth 2.0 PKCE, OAuth 2.0 device flow, SAML 2.0, WebAuthn/FIDO2, mTLS, LDAP, API key authentication, federated identity, zero-trust verification, and session management are all production-ready.

## Table of Contents

1. [Overview](#overview)
2. [Components](#components)
3. [Authentication Flows](#authentication-flows)
4. [Configuration](#configuration)
5. [Security Features](#security-features)
6. [Compliance](#compliance)
7. [Usage Examples](#usage-examples)
8. [Best Practices](#best-practices)

## Overview

The Authentication Module provides enterprise-grade authentication mechanisms for ThemisDB, supporting modern security standards and compliance requirements. It implements multiple authentication strategies with seamless integration into existing enterprise infrastructure.

### Key Features

- **JWT Authentication**: OpenID Connect integration with Keycloak, Okta, Auth0, and any standards-compliant provider
- **OIDC Provider Federation**: Auto-configure from any OIDC provider's discovery endpoint
- **Kerberos/GSSAPI**: Enterprise SSO for Active Directory environments
- **Multi-Factor Authentication**: TOTP-based MFA with recovery codes
- **WebAuthn/FIDO2**: Hardware security keys, passkeys, and biometric authenticators
- **OAuth 2.0 Device Flow**: Headless device / CLI authentication (RFC 8628)
- **OAuth 2.0 PKCE Flow**: Authorization Code Grant with PKCE for public clients (RFC 7636)
- **SAML 2.0**: SP-initiated and IdP-initiated SSO with assertion encryption
- **LDAP/Active Directory**: Direct bind authentication for enterprise directories
- **API Key Authentication**: Static key + secret for service-to-service access
- **mTLS**: Certificate-based mutual TLS for service identity
- **Federated Identity**: Unified principal namespace across multiple identity realms
- **Signature Verification**: RS256 with JWKS caching
- **Principal-to-Role Mapping**: Flexible authorization system
- **Clock Skew Tolerance**: Distributed system friendly
- **Rate Limiting**: Brute force and replay attack prevention
- **Session Management**: Cryptographically random session IDs, idle/absolute timeouts, per-user limits, revocation endpoint
- **Zero-Trust Verification**: Continuous per-request identity and network policy enforcement

### Architecture

```
┌─────────────────┐
│   Client App    │
└────────┬────────┘
         │
    ┌────▼─────┐
    │  Method  │
    │ Selection│
    └────┬─────┘
         │
    ┌────▼──────────────────────────────────────────┐
    │  JWT  │  Kerberos  │   MFA  │  Device Flow    │
    └───┬───┴──────┬─────┴────┬───┴──────┬──────────┘
        │          │          │          │
    ┌───▼──────────▼──────────▼──────────▼──────────┐
    │            Principal Extraction                 │
    └───────────────────────┬────────────────────────┘
                            │
    ┌───────────────────────▼────────────────────────┐
    │            Role Mapping & RBAC                  │
    └───────────────────────┬────────────────────────┘
                            │
    ┌───────────────────────▼────────────────────────┐
    │            Access Control Decision              │
    └────────────────────────────────────────────────┘
```

## Components

### JWT Validator (`jwt_validator.cpp`)

Validates JWT tokens from OpenID Connect providers (Keycloak, Okta, Auth0, etc.).

**Features:**
- RS256 signature verification using OpenSSL
- JWKS endpoint caching (configurable TTL)
- Claim extraction (subject, email, tenant, roles, groups)
- Clock skew tolerance (default: 60 seconds)
- Audience and issuer validation
- User-specific key derivation (HKDF-based)

**Token Format:**
```
header.payload.signature
```

**Supported Claims:**
- `sub`: Subject (user ID)
- `email`: User email address
- `tenant_id`: Multi-tenancy support
- `groups`: Group memberships
- `roles`: Role assignments
- `iss`: Issuer URL
- `exp`: Expiration timestamp
- `nbf`: Not-before timestamp
- `iat`: Issued-at timestamp
- `aud`: Audience

### GSSAPI Authenticator (`gssapi_authenticator.cpp`)

Implements Kerberos/GSSAPI authentication for enterprise single sign-on.

**Features:**
- MIT Kerberos 5 support
- Active Directory integration
- Heimdal Kerberos support
- Cross-platform (Linux, Windows, macOS)
- Service principal validation
- Keytab-based authentication
- Principal-to-role mapping with wildcards
- Fallback to basic authentication

**Supported Platforms:**
- Linux: MIT Kerberos, Heimdal
- Windows: SSPI (Windows native)
- macOS: Heimdal (built-in)

### MFA Authenticator (`mfa_authenticator.cpp`)

Time-based One-Time Password (TOTP) implementation per RFC 6238.

**Features:**
- TOTP code generation and validation
- QR code provisioning URI generation
- Recovery codes (single-use)
- Configurable time window (default: ±30 seconds)
- 6 or 8 digit codes
- Rate limiting (prevents brute force)
- Replay attack prevention

**TOTP Algorithm:**
```
TOTP = HOTP(K, T)
where:
  K = shared secret
  T = floor((current_time - T0) / time_step)
  HOTP = HMAC-SHA1(K, T) truncated to 6-8 digits
```

### OAuth 2.0 Device Flow (`oauth_device_flow.cpp`)

OAuth 2.0 Device Authorization Grant (RFC 8628) for headless devices, CLI tools, and IoT clients that cannot open a browser.

**Features:**
- RFC 8628-compliant device authorization flow
- Automatic polling with `slow_down` backoff
- `id_token` validation via existing `JWTValidator` (OIDC)
- Public and confidential client support (optional `client_secret`)
- TLS certificate verification always enforced
- High-level `authenticate()` with progress callback for CLI UX
- Testable via injected HTTP mock (`setHttpPostForTesting`)

**Flow Steps:**
1. Client calls `requestDeviceCode()` → receives `user_code` and `verification_uri`
2. User visits `verification_uri` and enters `user_code` on their browser
3. Client polls `pollForToken()` at `interval` second intervals
4. On success, `validateIdToken()` returns `JWTClaims`

### Zero-Trust Auth Verifier (`zero_trust_auth_verifier.cpp`)

Auth-layer bridge that enforces the zero-trust "never trust, always verify" principle by
re-validating the caller's identity and network location for **every** inbound request
(no session cache).

**Features:**
- Injectable `TokenVerifier` callback — wire in `JWTValidator::parseAndValidate` for JWT re-validation
- Per-identity CIDR network policies delegated to `security::ZeroTrustPolicyEnforcer`
- Configurable minimum trust-score threshold (default: 0.7)
- Audit logging via `AuthAuditLogger` for every allow/deny decision
- Thread-safe; forwards `getMetrics()` from the underlying enforcer

**Verification flow per request:**
1. Token re-validated via injected callback (no cached session)
2. Source IP checked against registered `NetworkPolicy` CIDR allow/deny lists
3. Composite trust score computed (identity + network + device + freshness)
4. Score compared against `min_trust_score`; denied if below threshold
5. Audit event emitted

**Usage:**
```cpp
#include "auth/zero_trust_auth_verifier.h"

ZeroTrustAuthVerifier::Config cfg;
cfg.min_trust_score = 0.8;
ZeroTrustAuthVerifier verifier(cfg, [&jwt](const std::string& tok, const std::string& uid) {
    try { return jwt.parseAndValidate(tok).sub == uid; }
    catch (...) { return false; }
});

verifier.addNetworkPolicy({"corp", "alice", {"10.0.0.0/8"}, {}, true});

ZeroTrustAuthVerifier::Request req;
req.request_id = generate_uuid();
req.user_id    = claims.sub;
req.token      = bearer_token;
req.client_ip  = peer_ip;
req.resource   = "data";
req.action     = "read";

auto decision = verifier.verify(req);
if (!decision.allowed) { return http_403(decision.reason); }
```
### OAuth 2.0 PKCE Flow (`oauth_pkce_flow.cpp`)

OAuth 2.0 Authorization Code Grant with Proof Key for Code Exchange (RFC 7636) for public clients (native/mobile/SPA apps) that cannot safely store a client secret.

**Features:**
- RFC 7636-compliant PKCE with S256 challenge method (SHA-256)
- CSPRNG-based `code_verifier` generation (96 random bytes → 128 Base64URL chars)
- `code_verifier` is never sent to the authorization endpoint
- `id_token` validation via existing `JWTValidator` (OIDC)
- TLS certificate verification always enforced
- Testable via injected HTTP mock (`setHttpPostForTesting`) and random source (`setRandBytesForTesting`)

**Flow Steps:**
1. Client calls `generateChallenge()` → receives `code_verifier` / `code_challenge` pair
2. Client redirects user to `buildAuthorizationUrl(challenge)` at the authorization endpoint
3. User authenticates; authorization server redirects back with `authorization_code`
4. Client calls `exchangeCode(authorization_code, code_verifier)` to obtain tokens
5. Optionally, call `validateIdToken(token_response)` to extract and verify identity claims

### Session Manager (`session_manager.cpp`)

Manages the full lifecycle of user authentication sessions.  All REST operations are
exposed via `server/session_api_handler.cpp` at `/auth/sessions`.

**Features:**
- Cryptographically random 128-bit session IDs (OpenSSL `RAND_bytes`), encoded as 32 lowercase hex characters, prefixed `sess_` (total length: 37 characters, e.g. `sess_a1b2c3d4…`)
- Configurable idle timeout (default: 8 h) and absolute lifetime (default: 30 days)
- Per-user concurrent session limit with LRU eviction of the oldest session (default: 10)
- Thread-safe: single `std::mutex` guards the session map
- Automatic pruning of expired entries on `createSession()` and `validateSession()`

**REST Endpoints (require `auth:sessions` scope):**

| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/auth/sessions` | Create a new session; returns session ID and metadata |
| `GET` | `/auth/sessions` | List all active sessions for the authenticated user |
| `DELETE` | `/auth/sessions/{session_id}` | Revoke a specific session (owner or `admin:all` scope) |
| `DELETE` | `/auth/sessions` | Revoke all sessions except the one identified by `current_session_id` in the JSON body ("logout everywhere") |

**Configuration (`SessionManager::SessionLimits`):**

| Field | Default | Description |
|-------|---------|-------------|
| `max_sessions_per_user` | `10` | Max concurrent sessions; `0` = unlimited |
| `idle_timeout` | `8 h` | Session invalidated if unused for this duration |
| `absolute_timeout` | `30 days` | Hard session lifetime regardless of activity |

**Usage:**
```cpp
#include "auth/session_manager.h"

SessionManager::SessionLimits lim;
lim.max_sessions_per_user = 5;
lim.idle_timeout          = std::chrono::hours(1);
lim.absolute_timeout      = std::chrono::hours(24);

SessionManager mgr(lim);

// Create
std::string sid = mgr.createSession("alice", "fp-xyz", "10.0.0.1", "MyApp/1.0");

// Validate (updates last_accessed_at)
auto result = mgr.validateSession(sid);
if (result.valid) { /* use result.session->user_id */ }

// Revoke single session
mgr.terminateSession(sid);

// Revoke all other sessions (e.g. "logout everywhere except this device")
int removed = mgr.terminateAllOtherSessions("alice", sid);

// List active sessions for user
auto sessions = mgr.listSessions("alice");
```

### LDAP Authenticator (`ldap_authenticator.cpp`)

Direct bind authentication against an LDAP or Active Directory server.

**Features:**
- LDAP simple bind and SASL GSSAPI bind
- Active Directory and OpenLDAP support
- Group membership lookup and role mapping
- Cross-platform: OpenLDAP (Linux) and WinLDAP (Windows)
- TLS/LDAPS enforcement
- Configurable with `-DTHEMIS_ENABLE_LDAP=ON` (default)

**Configuration (`LDAPConfig`):**

| Field | Description |
|-------|-------------|
| `host` | LDAP server hostname or URI |
| `port` | LDAP port (default: 389; LDAPS: 636) |
| `use_tls` | Enforce TLS (`STARTTLS` or LDAPS) |
| `bind_dn` | Service account DN for search bind |
| `bind_password` | Service account password |
| `search_base` | Base DN for user search |
| `user_filter` | LDAP filter to locate user entry |
| `group_attribute` | Attribute containing group memberships |

### API Key Authenticator (`api_key_authenticator.cpp`)

Static API key + secret authentication for service-to-service and programmatic access.

**Features:**
- Key ID + SHA-256-hashed secret credential pairs
- Per-key role assignment and optional expiry
- Constant-time comparison to prevent timing attacks
- Audit logging of every authentication attempt (PII-redacted)

**Usage:**
```cpp
#include "auth/api_key_authenticator.h"

using namespace themis::auth;

ApiKeyAuthenticator auth;
ApiKeyAuthenticator::ApiKeyCredential cred;
cred.key_id          = "svc-reports";
// Hash the secret before storing — never persist the raw secret
cred.key_secret_hash = sha256hex(raw_secret);
cred.roles           = {"reports:read"};
auth.addCredential(cred);

// authenticate() compares the provided secret against the stored hash
auto result = auth.authenticate(key_id, raw_secret);
if (result.success) { /* use result.roles */ }
```

### mTLS Authenticator (`mtls_authenticator.cpp`)

Certificate-based mutual TLS authentication for service-to-service calls.

**Features:**
- Full X.509 certificate chain validation using OpenSSL
- Subject CN / SAN extraction as service identity
- Configurable CA certificate bundle and CRL checking
- Optional certificate expiry enforcement
- Maps certificate subject to roles via configurable mappings

**Configuration (`MTLSAuthenticator::Config`):**

| Field | Description |
|-------|-------------|
| `ca_cert_pem` | PEM-encoded CA certificate(s) used to verify client certs |
| `require_client_cert` | Reject connections without a client certificate |
| `verify_expiry` | Enforce `notAfter` (default: true) |
| `principal_mappings` | Map CN/SAN patterns to internal role names |

### SAML 2.0 Authenticator (`saml_authenticator.cpp`)

SAML 2.0 Service Provider for SP-initiated and IdP-initiated SSO.

**Features:**
- SP-initiated AuthnRequest with HTTP-Redirect binding
- IdP-initiated Response with HTTP-POST binding
- XML signature verification (RSA-SHA256 / RSA-SHA512) via OpenSSL
- Encrypted SAML assertions (AES-256-CBC / AES-128-CBC)
- Attribute extraction and mapping to internal roles
- Single Logout (SLO) support
- Configurable AuthnContextClassRef (password, Kerberos, smart card, etc.)
- Thread-safe after construction

**Configuration (`SAMLConfig`):**

| Field | Description |
|-------|-------------|
| `sp_entity_id` | SP entity ID URI |
| `sp_acs_url` | Assertion Consumer Service URL |
| `sp_slo_url` | Single Logout URL |
| `idp_entity_id` | IdP entity ID URI |
| `idp_sso_url` | IdP SSO endpoint |
| `idp_cert_pem` | PEM-encoded IdP signing certificate |
| `sp_private_key_pem` | SP private key for assertion decryption |
| `attribute_mappings` | Map SAML attributes to role names |

### WebAuthn / FIDO2 Authenticator (`webauthn_authenticator.cpp`)

Hardware security key and platform authenticator (passkey) support via the WebAuthn standard.

**Features:**
- FIDO2 registration and authentication ceremonies
- ES256 (P-256) and RS256 public-key credential verification
- CBOR-encoded authenticatorData parsing
- Attestation verification (`none`, `packed`, `fido-u2f`)
- Replay protection via stored challenge nonces
- Supports YubiKey, Titan Key, TPM, Touch ID, Face ID, Windows Hello
- Thread-safe credential store

**Registration flow:**
1. Server calls `startRegistration(user_id)` → returns `CredentialCreationOptions`
2. Client creates credential (browser / native) and returns `AttestationObject`
3. Server calls `completeRegistration(user_id, attestation_response)`

**Authentication flow:**
1. Server calls `startAuthentication(user_id)` → returns `CredentialRequestOptions`
2. Client signs challenge → returns `AssertionResponse`
3. Server calls `completeAuthentication(user_id, assertion_response)`

### OIDC Provider (`oidc_provider.cpp`)

Auto-configures JWT validation from any OIDC provider's discovery endpoint (RFC 8414).

**Features:**
- Fetches and caches `/.well-known/openid-configuration`
- Builds a `JWTValidator` from the discovered JWKS URI, issuer, and algorithms
- Factory methods `createDeviceFlow()` and `createPKCEFlow()` for OAuth 2.0 flows
- Supports Keycloak, Okta, Auth0, Azure AD, and any standards-compliant provider

**Usage:**
```cpp
#include "auth/oidc_provider.h"

using namespace themis::auth;

OIDCProviderConfig cfg;
cfg.discovery_url = "https://accounts.google.com/.well-known/openid-configuration";
cfg.client_id     = "my-app";

OIDCProvider provider(cfg);
auto validator = provider.createJWTValidator();
auto claims    = validator->parseAndValidate(bearer_token);
```

### Federated Identity Manager (`federated_identity_manager.cpp`)

Bridges multiple identity realms into a unified principal namespace.

**Features:**
- Register multiple `OIDCProvider` or `SAMLAuthenticator` instances under named realms
- Unified `authenticate(realm, token)` entry point
- Cross-realm principal normalization
- Per-realm role-mapping override
- Thread-safe realm registry

### Password Policy (`password_policy.cpp`)

Configurable password complexity and history enforcement.

**Features:**
- Minimum length, uppercase, lowercase, digit, and symbol requirements
- Password history (prevents reuse of N previous passwords)
- Bcrypt / Argon2id hashing for stored passwords
- Configurable maximum age and forced rotation
- Pluggable into any local-credential authentication flow

## Authentication Flows

### 1. JWT Bearer Token Flow

```
┌────────┐                 ┌─────────┐                 ┌──────────┐
│ Client │                 │ ThemisDB│                 │ Keycloak │
└───┬────┘                 └────┬────┘                 └────┬─────┘
    │                           │                           │
    │  1. Authenticate          │                           │
    ├──────────────────────────►│                           │
    │     (no token)            │                           │
    │                           │                           │
    │  2. 401 Unauthorized      │                           │
    │◄──────────────────────────┤                           │
    │                           │                           │
    │  3. Login                 │                           │
    ├───────────────────────────┼──────────────────────────►│
    │                           │                           │
    │  4. JWT Token             │                           │
    │◄──────────────────────────┼───────────────────────────┤
    │                           │                           │
    │  5. Request + Bearer Token│                           │
    ├──────────────────────────►│                           │
    │                           │                           │
    │                           │  6. Fetch JWKS (if cached │
    │                           │     expired)              │
    │                           ├──────────────────────────►│
    │                           │                           │
    │                           │  7. JWKS Response         │
    │                           │◄──────────────────────────┤
    │                           │                           │
    │                           │  8. Verify Signature      │
    │                           │  9. Extract Claims        │
    │                           │  10. Map to Roles         │
    │                           │                           │
    │  11. Success Response     │                           │
    │◄──────────────────────────┤                           │
    │                           │                           │
```

### 2. Kerberos/GSSAPI Flow

```
┌────────┐           ┌─────────┐           ┌─────┐           ┌──────────┐
│ Client │           │ ThemisDB│           │ KDC │           │   LDAP   │
└───┬────┘           └────┬────┘           └──┬──┘           └────┬─────┘
    │                     │                   │                   │
    │  1. kinit           │                   │                   │
    ├─────────────────────┼──────────────────►│                   │
    │                     │                   │                   │
    │  2. TGT             │                   │                   │
    │◄────────────────────┼───────────────────┤                   │
    │                     │                   │                   │
    │  3. Request Service │                   │                   │
    │     Ticket          │                   │                   │
    ├─────────────────────┼──────────────────►│                   │
    │                     │                   │                   │
    │  4. Service Ticket  │                   │                   │
    │◄────────────────────┼───────────────────┤                   │
    │                     │                   │                   │
    │  5. Negotiate Header│                   │                   │
    │     + Service Ticket│                   │                   │
    ├────────────────────►│                   │                   │
    │                     │                   │                   │
    │                     │  6. Verify Ticket │                   │
    │                     │     (keytab)      │                   │
    │                     │                   │                   │
    │                     │  7. Query Groups  │                   │
    │                     ├───────────────────┼──────────────────►│
    │                     │                   │                   │
    │                     │  8. Group List    │                   │
    │                     │◄──────────────────┼───────────────────┤
    │                     │                   │                   │
    │                     │  9. Map to Roles  │                   │
    │                     │                   │                   │
    │  10. Success        │                   │                   │
    │◄────────────────────┤                   │                   │
    │                     │                   │                   │
```

### 3. MFA Enrollment and Verification Flow

```
┌────────┐                              ┌─────────┐
│  User  │                              │ ThemisDB│
└───┬────┘                              └────┬────┘
    │                                        │
    │  1. Enable MFA Request                │
    ├───────────────────────────────────────►│
    │                                        │
    │                                        │  2. Generate Secret
    │                                        │  3. Generate Recovery Codes
    │                                        │
    │  4. Secret + QR Code + Recovery Codes │
    │◄───────────────────────────────────────┤
    │     (otpauth://totp/...)               │
    │                                        │
    │  5. Scan QR with Authenticator App    │
    │     (Google Authenticator, Authy...)  │
    │                                        │
    │  6. Enter TOTP Code                   │
    ├───────────────────────────────────────►│
    │                                        │
    │                                        │  7. Validate TOTP
    │                                        │     (time window ±1)
    │                                        │
    │  8. MFA Enabled Confirmation          │
    │◄───────────────────────────────────────┤
    │                                        │
    │  === Future Logins ===                │
    │                                        │
    │  9. Login (username + password)       │
    ├───────────────────────────────────────►│
    │                                        │
    │  10. MFA Challenge                    │
    │◄───────────────────────────────────────┤
    │                                        │
    │  11. TOTP Code                        │
    ├───────────────────────────────────────►│
    │                                        │
    │                                        │  12. Verify TOTP
    │                                        │      (rate limiting)
    │                                        │
    │  13. Login Success + Session Token    │
    │◄───────────────────────────────────────┤
    │                                        │
```

### 4. OAuth 2.0 Device Authorization Flow (RFC 8628)

```
┌─────────────┐        ┌─────────────┐        ┌──────────────────┐
│  CLI/Device │        │  ThemisDB   │        │ Authorization    │
│  (headless) │        │  (client)   │        │ Server (OAuth AS)│
└──────┬──────┘        └──────┬──────┘        └────────┬─────────┘
       │                      │                        │
       │  1. Login request    │                        │
       ├─────────────────────►│                        │
       │                      │                        │
       │                      │  2. POST /device_auth  │
       │                      ├───────────────────────►│
       │                      │                        │
       │                      │  3. device_code,       │
       │                      │     user_code,         │
       │                      │◄───────────────────────┤
       │                      │     verification_uri   │
       │                      │                        │
       │  4. Display user_code│                        │
       │◄─────────────────────┤                        │
       │     + verify URL     │                        │
       │                      │                        │
       │ [User opens browser, visits URL, enters code] │
       │                      │                        │
       │                      │  5. Poll POST /token   │
       │                      ├───────────────────────►│
       │                      │  (authorization_pending│
       │                      │   or slow_down)        │
       │                      │◄───────────────────────┤
       │                      │                        │
       │                      │  6. Poll POST /token   │
       │                      ├───────────────────────►│
       │                      │                        │
       │                      │  7. access_token +     │
       │                      │◄───────────────────────┤
       │                      │     id_token           │
       │                      │                        │
       │                      │  8. Validate id_token  │
       │                      │     (JWKS/JWTValidator)│
       │                      │                        │
       │  9. JWTClaims (sub,  │                        │
       │◄─────────────────────┤                        │
       │     email, roles)    │                        │
       │                      │                        │
```

## Configuration

### JWT Configuration

```cpp
#include "auth/jwt_validator.h"

using namespace themis::auth;

// Basic configuration
JWTValidator jwt_validator("https://keycloak.example.com/realms/production/protocol/openid-connect/certs");

// Advanced configuration
JWTValidatorConfig config;
config.jwks_url = "https://keycloak.example.com/realms/production/protocol/openid-connect/certs";
config.expected_issuer = "https://keycloak.example.com/realms/production";
config.expected_audience = "themisdb-api";
config.cache_ttl = std::chrono::seconds(600);  // Cache JWKS for 10 minutes
config.clock_skew = std::chrono::seconds(60);   // Allow 60s clock skew

// Optional: require every token to carry a jti claim.
// When true, tokens without a jti are rejected with std::runtime_error("Missing required jti claim").
// Enable this in deployments where per-token revocation via TokenBlacklist is mandatory.
// When false (default), tokens without a jti are accepted but cannot be individually revoked;
// a warning is logged whenever a TokenBlacklist is attached and an incoming token has no jti.
config.require_jti = false;

JWTValidator jwt_validator(config);
```

### Kerberos Configuration

```cpp
#include "auth/gssapi_authenticator.h"

using namespace themis::auth;

KerberosConfig krb_config;
krb_config.enabled = true;
krb_config.service_principal = "themisdb/db.example.com@EXAMPLE.COM";
krb_config.keytab_file = "/etc/themisdb/themisdb.keytab";
krb_config.krb5_config = "/etc/krb5.conf";  // Optional
krb_config.fallback_to_basic = true;        // Allow fallback

// Principal-to-role mapping
krb_config.principal_mappings = {
    {"admin@EXAMPLE.COM", "admin"},
    {"*@EXAMPLE.COM", "user"},
    {"service/*@EXAMPLE.COM", "service"},
    {"dba/*@CORP.EXAMPLE.COM", "dba"}
};

GSSAPIAuthenticator gssapi_auth;
gssapi_auth.initialize(krb_config);
```

### MFA Configuration

```cpp
#include "auth/mfa_authenticator.h"

using namespace themis::auth;

MFAAuthenticator::Config mfa_config;
mfa_config.time_step_seconds = 30;        // 30-second time step (RFC 6238)
mfa_config.code_length = 6;               // 6-digit codes
mfa_config.time_window = 1;               // Accept ±1 time step (±30s)
mfa_config.recovery_codes_count = 8;      // 8 recovery codes
mfa_config.issuer = "ThemisDB Production";

MFAAuthenticator mfa(mfa_config);
```

### OAuth Device Flow Configuration

```cpp
#include "auth/oauth_device_flow.h"

using namespace themis::auth;

OAuthDeviceFlow::Config cfg;
cfg.device_authorization_endpoint = "https://auth.example.com/realms/prod/protocol/openid-connect/auth/device";
cfg.token_endpoint                 = "https://auth.example.com/realms/prod/protocol/openid-connect/token";
cfg.client_id                      = "themisdb-cli";
cfg.client_secret                  = "";                // empty for public clients
cfg.scopes                         = {"openid", "email", "profile"};
cfg.jwks_url                       = "https://auth.example.com/realms/prod/protocol/openid-connect/certs";
cfg.http_timeout_seconds           = 10;
cfg.max_poll_interval_seconds      = 30;

OAuthDeviceFlow flow(cfg);
```

### OAuth PKCE Flow Configuration

```cpp
#include "auth/oauth_pkce_flow.h"

using namespace themis::auth;

OAuthPKCEFlow::Config cfg;
cfg.authorization_endpoint = "https://auth.example.com/realms/prod/protocol/openid-connect/auth";
cfg.token_endpoint         = "https://auth.example.com/realms/prod/protocol/openid-connect/token";
cfg.client_id              = "myapp-public";
cfg.redirect_uri           = "myapp://callback";
cfg.scopes                 = {"openid", "email", "profile"};
cfg.jwks_url               = "https://auth.example.com/realms/prod/protocol/openid-connect/certs";
cfg.http_timeout_seconds   = 10;

OAuthPKCEFlow flow(cfg);
```

## Security Features

### 1. Signature Verification (JWT)

ThemisDB verifies JWT signatures using RS256 (RSA with SHA-256):

```cpp
// Automatic signature verification
auto claims = jwt_validator.parseAndValidate(bearer_token);
// Token is valid and signature verified at this point
```

**Implementation Details:**
- Fetches public keys from JWKS endpoint
- Caches keys with configurable TTL
- Verifies RSA signature using OpenSSL
- Validates key ID (kid) matches
- Checks algorithm is RS256

### 2. Clock Skew Tolerance

Distributed systems may have clock drift. ThemisDB handles this:

```cpp
// Default: 60 seconds tolerance
config.clock_skew = std::chrono::seconds(60);

// Token is valid if:
// current_time >= (exp - clock_skew)
// current_time >= (nbf - clock_skew)
```

### 3. Rate Limiting (MFA)

Prevents brute force attacks on TOTP codes:

```cpp
// Pseudocode - internal rate limiting
if (failed_attempts_in_last_minute[user_id] > 5) {
    return AuthResult::RateLimited;
}
```

**Rate Limits:**
- Max 5 failed attempts per minute per user
- 15-minute lockout after 10 failed attempts
- Exponential backoff on repeated failures

### 4. Replay Attack Prevention

**TOTP Time Window:**
- Each TOTP code valid for 30 seconds only
- Time window prevents replay beyond ±30s
- Used codes can be tracked (optional)

**JWT Token:**
- Expiration (`exp`) claim enforced
- Not-before (`nbf`) claim enforced
- Tokens cannot be reused after expiration

### 5. User-Specific Key Derivation

For field-level encryption with user-specific keys:

```cpp
// Derive user-specific key from Data Encryption Key (DEK)
auto user_key = JWTValidator::deriveUserKey(
    dek,                    // Base encryption key
    claims,                 // User's JWT claims
    "sensitive_field"       // Field name
);

// Key is derived using HKDF with:
// - DEK as input key material
// - User ID (sub claim) as salt
// - Field name as context
```

**Access Control:**
```cpp
// Check if user has access to group-encrypted data
bool has_access = JWTValidator::hasAccess(
    claims,                 // User's JWT claims
    "finance-team"          // Encryption context (group name)
);
// Returns true if user.groups contains "finance-team"
```

## Compliance

### SOC 2 CC6.1

**Control Objective:** Logical and physical access controls restrict access to authorized personnel.

**Implementation:**
- ✅ Multi-factor authentication (TOTP)
- ✅ Role-based access control (RBAC)
- ✅ Audit logging of authentication events
- ✅ Session management with timeouts
- ✅ Encrypted credential storage
- ✅ Failed login attempt tracking
- ✅ Password complexity enforcement (external IdP)

### NIST SP 800-63B Level 2

**Authenticator Assurance Level (AAL2)**

Requirements:
- ✅ Multi-factor authentication
- ✅ Cryptographic mechanisms (TOTP, JWT signatures)
- ✅ Authenticated protected channel (TLS)
- ✅ Verifier impersonation resistance (Kerberos mutual auth)
- ✅ Replay resistance (time-bound tokens)
- ✅ Authentication intent (user interaction required)

**Password Requirements:**
- ✅ Minimum 8 characters (enforced by IdP)
- ✅ No composition rules (modern NIST guidance)
- ✅ Compromised password checking (IdP feature)
- ✅ Rate limiting on authentication attempts

### Additional Compliance

**GDPR:**
- ✅ User data minimization (only necessary claims)
- ✅ Right to erasure (MFA data can be deleted)
- ✅ Data portability (standard JWT format)
- ✅ Audit logs for authentication events

**HIPAA:**
- ✅ Unique user identification (sub claim)
- ✅ Emergency access procedure (recovery codes)
- ✅ Automatic logoff (token expiration)
- ✅ Encryption and decryption (TLS + JWT)

## Usage Examples

### Example 1: JWT Token Validation

```cpp
#include "auth/jwt_validator.h"
#include <iostream>

using namespace themis::auth;

int main() {
    // Initialize validator
    JWTValidator validator("https://keycloak.example.com/realms/prod/protocol/openid-connect/certs");

    // Token from Authorization header
    std::string auth_header = "Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...";

    try {
        // Parse and validate token
        auto claims = validator.parseAndValidate(auth_header);

        // Extract user information
        std::cout << "User ID: " << claims.sub << std::endl;
        std::cout << "Email: " << claims.email << std::endl;
        std::cout << "Tenant: " << claims.tenant_id << std::endl;

        // Check expiration
        if (claims.isExpired()) {
            std::cout << "Token expired!" << std::endl;
            return 1;
        }

        // Access roles
        std::cout << "Roles: ";
        for (const auto& role : claims.roles) {
            std::cout << role << " ";
        }
        std::cout << std::endl;

        // Access groups
        std::cout << "Groups: ";
        for (const auto& group : claims.groups) {
            std::cout << group << " ";
        }
        std::cout << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Authentication failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

### Example 2: Kerberos Authentication

```cpp
#include "auth/gssapi_authenticator.h"
#include <iostream>

using namespace themis::auth;

int main() {
    // Configure Kerberos
    KerberosConfig config;
    config.enabled = true;
    config.service_principal = "themisdb/db.example.com@EXAMPLE.COM";
    config.keytab_file = "/etc/themisdb/themisdb.keytab";
    config.principal_mappings = {
        {"admin@EXAMPLE.COM", "admin"},
        {"*@EXAMPLE.COM", "user"}
    };

    // Initialize authenticator
    GSSAPIAuthenticator auth;
    if (!auth.initialize(config)) {
        std::cerr << "Failed to initialize GSSAPI" << std::endl;
        return 1;
    }

    // Authenticate token from client (Negotiate header)
    std::string negotiate_token = "YIIGfwYGKwYBBQUCoIIG...";

    auto result = auth.authenticateToken(negotiate_token);

    if (result.success) {
        std::cout << "Authentication successful!" << std::endl;
        std::cout << "Principal: " << result.principal_name << std::endl;
        std::cout << "Roles: ";
        for (const auto& role : result.roles) {
            std::cout << role << " ";
        }
        std::cout << std::endl;
    } else {
        std::cerr << "Authentication failed: " << result.error_message << std::endl;
        return 1;
    }

    return 0;
}
```

### Example 3: MFA Enrollment

```cpp
#include "auth/mfa_authenticator.h"
#include <iostream>

using namespace themis::auth;

int main() {
    // Initialize MFA
    MFAAuthenticator::Config config;
    config.issuer = "ThemisDB Production";
    MFAAuthenticator mfa(config);

    // Generate enrollment for user
    std::string user_id = "alice@example.com";
    auto enrollment = mfa.generateEnrollment(user_id);

    // Display QR code URI (scan with authenticator app)
    std::string qr_uri = mfa.generateProvisioningURI(enrollment);
    std::cout << "Scan this QR code with your authenticator app:" << std::endl;
    std::cout << qr_uri << std::endl;

    // Display recovery codes (save securely!)
    std::cout << "\nRecovery Codes (save these!):" << std::endl;
    for (const auto& code : enrollment.recovery_codes) {
        std::cout << "  " << code << std::endl;
    }

    // Save enrollment data (encrypted!)
    auto json = enrollment.to_json();
    // saveToDatabase(user_id, json);

    // Verify TOTP code
    std::cout << "\nEnter TOTP code from app: ";
    std::string totp_code;
    std::cin >> totp_code;

    if (mfa.validateTOTP(enrollment.secret_base32, totp_code)) {
        std::cout << "TOTP code valid! MFA enabled." << std::endl;
        enrollment.enabled = true;
    } else {
        std::cout << "Invalid TOTP code." << std::endl;
    }

    return 0;
}
```

### Example 4: MFA Validation

```cpp
#include "auth/mfa_authenticator.h"
#include <iostream>

using namespace themis::auth;

int main() {
    MFAAuthenticator mfa;

    // Load enrollment from database
    // auto enrollment = loadFromDatabase(user_id);
    MFAAuthenticator::EnrollmentData enrollment;
    enrollment.user_id = "alice@example.com";
    enrollment.secret_base32 = "JBSWY3DPEHPK3PXP";  // Example secret
    enrollment.enabled = true;

    // Get TOTP code from user
    std::cout << "Enter TOTP code: ";
    std::string totp_code;
    std::cin >> totp_code;

    // Validate TOTP
    if (mfa.validateTOTP(enrollment.secret_base32, totp_code)) {
        std::cout << "Authentication successful!" << std::endl;
        return 0;
    }

    // Try recovery code if TOTP failed
    std::cout << "Invalid TOTP. Enter recovery code: ";
    std::string recovery_code;
    std::cin >> recovery_code;

    if (mfa.validateRecoveryCode(enrollment, recovery_code)) {
        std::cout << "Recovery code accepted. Update your authenticator app!" << std::endl;
        // saveToDatabase(user_id, enrollment);  // Save updated enrollment
        return 0;
    }

    std::cout << "Authentication failed." << std::endl;
    return 1;
}
```

### Example 5: Field-Level Encryption with User Keys

```cpp
#include "auth/jwt_validator.h"
#include "security/encryption.h"
#include <iostream>

using namespace themis::auth;
using namespace themis::security;

int main() {
    // Master Data Encryption Key (from key management)
    std::vector<uint8_t> dek = loadDEK();

    // Validate JWT and get claims
    JWTValidator validator("https://keycloak.example.com/realms/prod/protocol/openid-connect/certs");
    auto claims = validator.parseAndValidate(bearer_token);

    // Derive user-specific key for sensitive field
    auto user_key = JWTValidator::deriveUserKey(dek, claims, "ssn");

    // Encrypt data with user-specific key
    std::string ssn = "123-45-6789";
    auto encrypted_ssn = encrypt_aes_gcm(ssn, user_key);

    // Only this user can decrypt (requires same JWT claims)

    // Check group access for shared data
    if (JWTValidator::hasAccess(claims, "finance-team")) {
        // Derive group key
        auto group_key = deriveGroupKey(dek, "finance-team");
        auto encrypted_salary = encrypt_aes_gcm(salary, group_key);
        // All finance team members can decrypt
    }

    return 0;
}
```

### Example 6: OAuth 2.0 Device Flow (CLI Authentication)

```cpp
#include "auth/oauth_device_flow.h"
#include <iostream>
#include <thread>

using namespace themis::auth;

int main() {
    // Configure device flow (Keycloak example)
    OAuthDeviceFlow::Config cfg;
    cfg.device_authorization_endpoint =
        "https://auth.example.com/realms/prod/protocol/openid-connect/auth/device";
    cfg.token_endpoint =
        "https://auth.example.com/realms/prod/protocol/openid-connect/token";
    cfg.client_id  = "themisdb-cli";
    cfg.scopes     = {"openid", "email"};
    cfg.jwks_url   =
        "https://auth.example.com/realms/prod/protocol/openid-connect/certs";

    OAuthDeviceFlow flow(cfg);

    try {
        // authenticate() handles the full flow:
        // 1. Request device code
        // 2. Show instructions to user
        // 3. Poll until authorized or expired
        // 4. Validate id_token and return JWTClaims
        auto claims = flow.authenticate(
            [](const OAuthDeviceFlow::DeviceCodeResponse& resp) {
                std::cout << "\nTo authenticate, visit:\n  "
                          << resp.verification_uri << "\n\nEnter code: "
                          << resp.user_code << "\n\nWaiting...\n";
            }
        );

        std::cout << "Authenticated as: " << claims.email << std::endl;
        std::cout << "User ID:          " << claims.sub   << std::endl;
        for (const auto& role : claims.roles) {
            std::cout << "Role: " << role << std::endl;
        }

    } catch (const AuthException& ex) {
        std::cerr << "Authentication failed: "
                  << ex.error().publicMessage() << std::endl;
        return 1;
    }

    return 0;
}
```

**Step-by-step (manual polling):**

```cpp
// Step 1: request device code
auto resp = flow.requestDeviceCode();
std::cout << "Visit " << resp.verification_uri
          << " and enter: " << resp.user_code << std::endl;

// Step 2: poll until authorized
while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(resp.interval));

    OAuthDeviceFlow::PollStatus status;
    auto token = flow.pollForToken(resp.device_code, status);

    if (status == OAuthDeviceFlow::PollStatus::Authorized) {
        auto claims = flow.validateIdToken(token);
        std::cout << "Logged in as: " << claims.email << std::endl;
        break;
    }
    if (status == OAuthDeviceFlow::PollStatus::SlowDown) {
        resp.interval += 5;  // back off per RFC 8628
    }
    // AuthorizationPending → keep polling; AccessDenied/ExpiredToken → throws
}
```

### Example 7: OAuth 2.0 PKCE Flow (Native/SPA Authentication)

```cpp
#include "auth/oauth_pkce_flow.h"
#include <iostream>

using namespace themis::auth;

int main() {
    // Configure PKCE flow (Keycloak example, public client)
    OAuthPKCEFlow::Config cfg;
    cfg.authorization_endpoint =
        "https://auth.example.com/realms/prod/protocol/openid-connect/auth";
    cfg.token_endpoint =
        "https://auth.example.com/realms/prod/protocol/openid-connect/token";
    cfg.client_id  = "myapp-public";
    cfg.redirect_uri = "myapp://callback";
    cfg.scopes     = {"openid", "email"};
    cfg.jwks_url   =
        "https://auth.example.com/realms/prod/protocol/openid-connect/certs";

    OAuthPKCEFlow flow(cfg);

    try {
        // Step 1: generate code_verifier and code_challenge
        auto challenge = flow.generateChallenge();

        // Step 2: redirect the user to the authorization URL
        std::string state = "random-csrf-state";
        std::string auth_url = flow.buildAuthorizationUrl(challenge, state);
        std::cout << "Open this URL in a browser:\n  " << auth_url << std::endl;

        // Step 3: receive the authorization_code from the redirect URI (out of band)
        std::string auth_code;
        std::cout << "Enter the authorization code from the redirect: ";
        std::cin >> auth_code;

        // Step 4: exchange code for tokens
        auto token = flow.exchangeCode(auth_code, challenge.code_verifier);

        // Step 5: validate id_token and extract identity claims
        auto claims = flow.validateIdToken(token);

        std::cout << "Authenticated as: " << claims.email << std::endl;
        std::cout << "User ID:          " << claims.sub   << std::endl;

    } catch (const AuthException& ex) {
        std::cerr << "Authentication failed: "
                  << ex.error().publicMessage() << std::endl;
        return 1;
    }

    return 0;
}
```

## Best Practices

### 1. JWT Token Management

**DO:**
- ✅ Use short-lived tokens (15-60 minutes)
- ✅ Implement token refresh mechanism
- ✅ Validate tokens on every request
- ✅ Cache JWKS with appropriate TTL
- ✅ Use HTTPS only (never HTTP)
- ✅ Validate issuer and audience claims
- ✅ Handle token expiration gracefully

**DON'T:**
- ❌ Store tokens in localStorage (use httpOnly cookies)
- ❌ Send tokens in URL query parameters
- ❌ Use long-lived tokens without refresh
- ❌ Skip signature verification
- ❌ Ignore expiration claims
- ❌ Trust client-provided claims without validation

### 2. Kerberos Deployment

**DO:**
- ✅ Use dedicated service account
- ✅ Secure keytab file (0400 permissions)
- ✅ Rotate keytabs regularly
- ✅ Monitor KDC connectivity
- ✅ Implement fallback authentication
- ✅ Log all authentication attempts
- ✅ Use specific principal mappings

**DON'T:**
- ❌ Use user accounts for service principals
- ❌ Store keytabs in version control
- ❌ Allow wildcard mappings to admin roles
- ❌ Disable fallback in production
- ❌ Ignore Kerberos errors silently

### 3. MFA Implementation

**DO:**
- ✅ Encrypt TOTP secrets at rest
- ✅ Generate sufficient recovery codes (8+)
- ✅ Display QR codes securely
- ✅ Implement rate limiting
- ✅ Allow account recovery process
- ✅ Support multiple TOTP devices
- ✅ Audit MFA enrollment/changes

**DON'T:**
- ❌ Store TOTP secrets in plaintext
- ❌ Skip time window validation
- ❌ Allow unlimited verification attempts
- ❌ Reuse recovery codes
- ❌ Send TOTP secrets via email/SMS
- ❌ Disable MFA without verification

### 4. General Security

**DO:**
- ✅ Use TLS 1.2+ for all connections
- ✅ Implement comprehensive audit logging
- ✅ Monitor authentication failures
- ✅ Set up alerting for suspicious activity
- ✅ Regular security updates
- ✅ Penetration testing
- ✅ Incident response plan

**DON'T:**
- ❌ Log sensitive data (tokens, secrets)
- ❌ Ignore failed authentication patterns
- ❌ Disable security features in production
- ❌ Use default credentials
- ❌ Mix authentication methods insecurely

### 5. Performance Optimization

**DO:**
- ✅ Cache JWKS responses (10+ minutes)
- ✅ Reuse GSSAPI contexts when possible
- ✅ Implement connection pooling for KDC
- ✅ Use async validation for non-critical paths
- ✅ Monitor authentication latency
- ✅ Set appropriate timeouts

**DON'T:**
- ❌ Fetch JWKS on every request
- ❌ Create new Kerberos contexts unnecessarily
- ❌ Block request threads during validation
- ❌ Use infinite timeouts
- ❌ Skip performance monitoring

### 6. Error Handling

**DO:**
- ✅ Return generic error messages to clients
- ✅ Log detailed errors server-side
- ✅ Implement retry logic for transient failures
- ✅ Handle network timeouts gracefully
- ✅ Provide clear user feedback
- ✅ Monitor error rates

**DON'T:**
- ❌ Expose internal errors to clients
- ❌ Leak existence of users via errors
- ❌ Retry indefinitely on failures
- ❌ Ignore KDC/IdP connectivity issues
- ❌ Crash on authentication failures

### 7. OAuth 2.0 Device Flow

**DO:**
- ✅ Always set `jwks_url` so `id_token` signatures are verified
- ✅ Display both `verification_uri` and `user_code` clearly to the user
- ✅ Respect the `interval` from the server; back off on `slow_down`
- ✅ Treat the `device_code` as a short-lived secret (don't log it)
- ✅ Catch `AuthException` and surface `publicMessage()` to the user
- ✅ Use public-client mode (empty `client_secret`) for CLI tools

**DON'T:**
- ❌ Log `device_code`, `access_token`, or `refresh_token`
- ❌ Poll faster than the server-specified `interval`
- ❌ Disable TLS certificate verification (`SSL_VERIFYPEER`)
- ❌ Cache access tokens beyond their `expires_in` lifetime
- ❌ Request broader scopes than needed (principle of least privilege)

### 8. OAuth 2.0 PKCE Flow

**DO:**
- ✅ Always set `jwks_url` when using `openid` scope so `id_token` signatures are verified
- ✅ Generate a fresh `PKCEChallenge` for every authorization request
- ✅ Keep `code_verifier` secret; store it only for the duration of the flow
- ✅ Use a random `state` parameter to prevent CSRF attacks
- ✅ Catch `AuthException` and surface `publicMessage()` to the user
- ✅ Use HTTPS redirect URIs in production

**DON'T:**
- ❌ Log or persist `code_verifier`, `access_token`, or `refresh_token`
- ❌ Reuse a `code_verifier` across multiple authorization requests
- ❌ Disable TLS certificate verification (`SSL_VERIFYPEER`)
- ❌ Use the `plain` challenge method — only `S256` is accepted
- ❌ Request broader scopes than needed (principle of least privilege)

## Testing

### Unit Tests

```bash
# Run auth module tests
cd /home/runner/work/ThemisDB/ThemisDB
cmake --build build --target test_auth

# Run specific test suites
./build/tests/test_jwt_validator
./build/tests/test_mfa_authenticator
./build/tests/test_gssapi_authenticator
./build/tests/test_oauth_device_flow
./build/tests/test_oauth_pkce_flow

# Run session management tests only (32 test cases)
ctest --test-dir build -R SessionManagerTests
# or via gtest filter:
./build/tests/themis_tests --gtest_filter=SessionManagerTest*:SessionApiHandlerTest*
```

## Integration Tests

```bash
# Test with Keycloak
export KEYCLOAK_URL=https://keycloak.example.com
export KEYCLOAK_REALM=test
./build/tests/test_auth_integration

# Test with Kerberos
export KRB5_CONFIG=/etc/krb5.conf
export KRB5_KTNAME=/etc/themisdb/test.keytab
./build/tests/test_kerberos_integration
```

## Load Testing

```bash
# Benchmark JWT validation
./build/benchmarks/bench_jwt_validation

# Benchmark TOTP validation
./build/benchmarks/bench_totp_validation
```

## Troubleshooting

### JWT Issues

**Problem:** "JWKS HTTP error: 000"
- **Cause:** Cannot reach JWKS endpoint
- **Solution:** Check network connectivity, firewall rules, JWKS URL

**Problem:** "Invalid signature"
- **Cause:** Token signed with different key or algorithm
- **Solution:** Verify issuer matches, check key rotation, refresh JWKS cache

**Problem:** "Token expired"
- **Cause:** Clock skew or genuinely expired token
- **Solution:** Increase clock_skew tolerance, implement token refresh

### Kerberos Issues

**Problem:** "Cannot find KDC for realm"
- **Cause:** DNS or krb5.conf misconfiguration
- **Solution:** Check DNS SRV records, verify krb5.conf, test with kinit

**Problem:** "Cannot read keytab"
- **Cause:** File permissions or wrong path
- **Solution:** Check file exists, verify permissions (0400), check ownership

**Problem:** "Clock skew too great"
- **Cause:** Time difference between client/server/KDC
- **Solution:** Sync clocks with NTP, configure clock skew tolerance

### MFA Issues

**Problem:** "TOTP code invalid"
- **Cause:** Clock drift, wrong time window, or code already used
- **Solution:** Sync clocks, verify time_step_seconds, check time window

**Problem:** "Rate limited"
- **Cause:** Too many failed attempts
- **Solution:** Wait for cooldown period, use recovery code, check audit logs

## Documentation

For more detailed documentation, see:
- [JWT Validator Implementation](./jwt_validator.cpp)
- [Kerberos Authentication Guide](../../docs/en/security/KERBEROS_AUTHENTICATION.md)
- [Auth API Authentication & Authorization Guide](../../docs/security/api_authentication_authorization.md)
- [Security Hardening](../security/README.md)
- [Public API Headers](../../include/auth/README.md)
- [Architecture Guide](./ARCHITECTURE.md)
- [Security Guide](./SECURITY.md)
- [Audit Report](./AUDIT.md)
- [Changelog](./CHANGELOG.md)
- [Performance Expectations](./PERFORMANCE_EXPECTATIONS.md)
- [Auth Roadmap](./ROADMAP.md)
- [Auth Future Enhancements](./FUTURE_ENHANCEMENTS.md)

## Related Modules

- **Security Module** (`../security/`): Encryption, TLS, key management
- **API Module** (`../api/`): HTTP authentication middleware
- **Server Module** (`../server/`): Request handling and routing
- **Governance Module** (`../governance/`): Audit logging and compliance

## Support

For security issues, see [SECURITY.md](../../SECURITY.md)

For general support, see [SUPPORT.md](../../SUPPORT.md)

## Scientific References

1. Jones, M., Bradley, J., & Sakimura, N. (2015). **JSON Web Token (JWT)**. RFC 7519. IETF. https://doi.org/10.17487/RFC7519

2. Hardt, D. (2012). **The OAuth 2.0 Authorization Framework**. RFC 6749. IETF. https://doi.org/10.17487/RFC6749

3. Sakimura, N., Bradley, J., Jones, M. B., de Medeiros, B., & Mortimore, C. (2014). **OpenID Connect Core 1.0**. OpenID Foundation. https://openid.net/specs/openid-connect-core-1_0.html

4. Bonneau, J., Herley, C., van Oorschot, P. C., & Stajano, F. (2012). **The Quest to Replace Passwords: A Framework for Comparative Evaluation of Web Authentication Schemes**. *Proceedings of the 2012 IEEE Symposium on Security and Privacy*, 553–567. https://doi.org/10.1109/SP.2012.44

5. Grassi, P. A., Garcia, M. E., & Fenton, J. L. (2017). **Digital Identity Guidelines: Authentication and Lifecycle Management**. NIST Special Publication 800-63B. National Institute of Standards and Technology. https://doi.org/10.6028/NIST.SP.800-63b

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
