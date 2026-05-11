> **Build:** `cmake --preset release && cmake --build build/release`

# Auth Module Headers

Public interfaces and declarations for the ThemisDB authentication module.

## Table of Contents

1. [Overview](#overview)
2. [Header Files](#header-files)
3. [Data Structures](#data-structures)
4. [API Reference](#api-reference)
5. [Integration Guide](#integration-guide)
6. [Thread Safety](#thread-safety)

## Overview

This directory contains the public API headers for ThemisDB's authentication system.
These headers define the interfaces for all authentication methods, security utilities,
session management, audit logging, and metrics.

### Key Components

| Header | Key Classes | Purpose |
|--------|---------------|---------|
| `jwt_validator.h` | `JWTValidator`, `JWTClaims`, `JWTValidatorConfig` | JWT/OpenID Connect token validation with RS256/ES256/EdDSA |
| `gssapi_authenticator.h` | `GSSAPIAuthenticator`, `KerberosConfig` | Kerberos/GSSAPI enterprise SSO |
| `mfa_authenticator.h` | `MFAAuthenticator` | TOTP-based MFA with recovery codes (RFC 6238) |
| `kerberos_security.h` | `KerberosSecurityValidator`, `ChannelBindingGenerator` | Kerberos-specific security validations |
| `oauth_device_flow.h` | `OAuthDeviceFlow` | OAuth 2.0 Device Authorization Grant (RFC 8628) |
| `oauth_pkce_flow.h` | `OAuthPKCEFlow` | OAuth 2.0 Authorization Code + PKCE (RFC 7636) |
| `oidc_provider.h` | `OIDCProvider`, `OIDCProviderConfig` | OIDC provider discovery and federation |
| `saml_authenticator.h` | `SAMLAuthenticator`, `SAMLConfig`, `SAMLClaims` | SAML 2.0 SP/IdP-initiated SSO |
| `ldap_authenticator.h` | `LDAPAuthenticator`, `LDAPConfig` | LDAP/Active Directory direct bind |
| `mtls_authenticator.h` | `MTLSAuthenticator`, `MTLSClaims` | Mutual TLS client certificate authentication |
| `webauthn_authenticator.h` | `WebAuthnAuthenticator` | WebAuthn/FIDO2 hardware token support |
| `api_key_authenticator.h` | `ApiKeyAuthenticator`, `ApiKeyCredential` | Static API key + secret authentication |
| `federated_identity_manager.h` | `FederatedIdentityManager` | Multi-realm federated identity |
| `session_manager.h` | `SessionManager` | Session lifecycle management with revocation |
| `token_blacklist.h` | `TokenBlacklist` | Revoked token tracking (JTI-based) |
| `password_policy.h` | `PasswordPolicy` | Configurable password complexity enforcement |
| `principal_validator.h` | `PrincipalValidator`, `PrincipalValidatorPresets` | Principal format validation and role mapping |
| `auth_rate_limiter.h` | `AuthRateLimiter`, `AccountLockoutManager` | Brute-force and replay attack rate limiting |
| `auth_metrics.h` | `AuthMetrics`, `AuthDurationTimer` | Authentication latency and success/failure metrics |
| `auth_audit_logger.h` | `AuthAuditLogger` | Audit logging for all authentication events |
| `auth_error.h` | `AuthError`, `AuthException`, `AuthErrorCode` | Structured authentication error types |
| `jwks_validator.h` | `JWKSValidator` | JWKS endpoint validation and key fetching |
| `jwks_security.h` | `JWKSSecureFetcher`, `JWKSSecurityConfig` | JWKS security validation and key verification |
| `jwt_key_rotation_manager.h` | `JWTKeyRotationManager` | Automated JWT signing key rotation |
| `totp_replay_cache.h` | `TOTPReplayCache`, `SecureMFAValidator` | One-time code replay prevention |
| `totp_secret_encryption.h` | `TOTPSecretEncryption`, `TOTPSecretRotationManager` | Encrypted TOTP secret storage |
| `zero_trust_auth_verifier.h` | `ZeroTrustAuthVerifier` | Continuous per-request identity verification |
| `eid_authenticator.h` | `EIDAuthenticator` | Electronic ID (eID) authenticator |
| `passkey_authenticator.h` | `PasskeyAuthenticator` | Passkey (discoverable FIDO2) authenticator |
| `authorization_policy.h` | `AuthorizationPolicy` | RBAC/ABAC authorization policy evaluator |
| `auth_event_bus.h` | `AuthEventBus` | Publishes authentication lifecycle events |
| `auth_worker_thread_pool.h` | `AuthWorkerThreadPool` | Dedicated thread pool for async auth operations |
| `ldap_connection_pool.h` | `LDAPConnectionPool` | Pooled LDAP connection manager |
| `rate_limiter_backend.h` | `IRateLimiterBackend` | Pluggable storage backend for rate-limit counters |
| `redis_token_blacklist.h` | `RedisTokenBlacklist` | Redis-backed revoked-token store |
| `rocksdb_token_blacklist.h` | `RocksDBTokenBlacklist` | RocksDB-backed revoked-token store |
| `secure_memory.h` | `SecureMemory` | Guarded memory allocation for sensitive credential data |

## Header Files

### jwt_validator.h

**Purpose:** JWT token validation for OpenID Connect providers (Keycloak, Okta, Auth0, etc.)

**Key Classes:**
- `JWTClaims`: Token claims structure
- `JWTValidatorConfig`: Configuration options
- `JWTValidator`: Main validator class

**Features:**
- RS256 signature verification using JWKS
- Clock skew tolerance for distributed systems
- JWKS caching with configurable TTL
- Claim extraction (sub, email, tenant_id, roles, groups)
- User-specific key derivation (HKDF)
- Group-based access control

**Example:**
```cpp
#include "auth/jwt_validator.h"

using namespace themis::auth;

JWTValidator validator("https://keycloak.example.com/realms/prod/protocol/openid-connect/certs");
auto claims = validator.parseAndValidate(bearer_token);
std::cout << "User: " << claims.sub << std::endl;
```

**Thread Safety:** Yes (JWKS cache is internally synchronized)

---

### gssapi_authenticator.h

**Purpose:** Kerberos/GSSAPI authentication for enterprise single sign-on

**Key Classes:**
- `KerberosConfig`: Configuration and principal mappings
- `GSSAPIAuthResult`: Authentication result with principal and roles
- `GSSAPIAuthenticator`: Main authenticator class

**Features:**
- MIT Kerberos 5 support
- Active Directory integration
- Heimdal Kerberos support
- Cross-platform (Linux/GSSAPI, Windows/SSPI, macOS/Heimdal)
- Service principal validation with keytab
- Principal-to-role mapping with wildcard support
- Fallback to basic authentication

**Example:**
```cpp
#include "auth/gssapi_authenticator.h"

using namespace themis::auth;

KerberosConfig config;
config.service_principal = "themisdb/db.example.com@EXAMPLE.COM";
config.keytab_file = "/etc/themisdb/themisdb.keytab";
config.principal_mappings = {
    {"admin@EXAMPLE.COM", "admin"},
    {"*@EXAMPLE.COM", "user"}
};

GSSAPIAuthenticator auth;
auth.initialize(config);

auto result = auth.authenticateToken(negotiate_token);
if (result.success) {
    std::cout << "Authenticated: " << result.principal_name << std::endl;
}
```

**Thread Safety:** No (create separate instance per thread or use mutex)

**Platform-Specific:**
- Linux: Requires MIT Kerberos or Heimdal development libraries
- Windows: Uses native SSPI APIs
- macOS: Uses built-in Heimdal

---

### mfa_authenticator.h

**Purpose:** Time-based One-Time Password (TOTP) multi-factor authentication per RFC 6238

**Key Classes:**
- `MFAAuthenticator::Config`: MFA configuration
- `MFAAuthenticator::EnrollmentData`: User enrollment data
- `MFAAuthenticator`: Main MFA class

**Features:**
- TOTP generation and validation (RFC 6238)
- QR code provisioning URI generation
- Recovery codes (single-use, cryptographically secure)
- Configurable time window (default ±30 seconds)
- 6 or 8 digit codes
- Base32 encoding for secrets
- HMAC-SHA1 implementation

**Example:**
```cpp
#include "auth/mfa_authenticator.h"

using namespace themis::auth;

MFAAuthenticator mfa;

// Enrollment
auto enrollment = mfa.generateEnrollment("alice@example.com");
std::string qr_uri = mfa.generateProvisioningURI(enrollment);
// Display QR code to user

// Validation
if (mfa.validateTOTP(enrollment.secret_base32, user_entered_code)) {
    std::cout << "TOTP valid!" << std::endl;
}
```

**Thread Safety:** Yes (stateless validation)

**Security Notes:**
- Secrets must be encrypted at rest
- Recovery codes are single-use only
- Rate limiting should be applied externally
- Time window prevents replay attacks

---

### kerberos_security.h

**Purpose:** Kerberos-specific security validations and channel binding utilities

**Key Classes:**

- `KerberosSecurityValidator`: Ticket and token security validation
- `ChannelBindingGenerator`: Channel binding token generation for CBT

**Thread Safety:** Yes

---

### oauth_device_flow.h

**Purpose:** OAuth 2.0 Device Authorization Grant for headless/CLI authentication (RFC 8628)

**Key Classes:**

- `OAuthDeviceFlow`: Device flow orchestrator

**Example:**

```cpp
#include "auth/oauth_device_flow.h"

using namespace themis::auth;

OAuthDeviceFlow flow(authorization_endpoint, token_endpoint, client_id);
auto response = flow.startDeviceAuthorization({"openid", "profile"});
// Display response.verification_uri and response.user_code to user
auto token = flow.pollForToken(response.device_code, response.interval);
```

**Thread Safety:** Yes

---

### oauth_pkce_flow.h

**Purpose:** OAuth 2.0 Authorization Code Grant with PKCE for public clients (RFC 7636)

**Key Classes:**

- `OAuthPKCEFlow`: PKCE flow orchestrator

**Example:**

```cpp
#include "auth/oauth_pkce_flow.h"

using namespace themis::auth;

OAuthPKCEFlow flow(authorization_endpoint, token_endpoint, client_id, redirect_uri);
auto [auth_url, code_verifier] = flow.generateAuthorizationURL({"openid", "profile"});
// Redirect user to auth_url
auto token = flow.exchangeCodeForToken(authorization_code, code_verifier);
```

**Thread Safety:** Yes

---

### oidc_provider.h

**Purpose:** OIDC provider discovery, configuration, and federation

**Key Classes:**

- `OIDCProvider`: Provider discovery and token handling
- `OIDCProviderConfig`: Provider configuration
- `OIDCDiscoveryDocument`: Well-known OIDC discovery document

**Thread Safety:** Yes (discovery document cached with mutex)

---

### saml_authenticator.h

**Purpose:** SAML 2.0 SP-initiated and IdP-initiated Single Sign-On

**Key Classes:**

- `SAMLAuthenticator`: SAML 2.0 authenticator
- `SAMLConfig`: SAML configuration (IdP metadata, SP entity ID, certificates)
- `SAMLClaims`: Extracted SAML assertion attributes
- `AuthnRequestParams`: Parameters for authentication request generation

**Thread Safety:** Yes

---

### ldap_authenticator.h

**Purpose:** LDAP/Active Directory direct bind authentication

**Key Classes:**

- `LDAPAuthenticator`: LDAP bind authenticator
- `LDAPConfig`: LDAP connection and search configuration
- `LDAPAuthResult`: Authentication result with user attributes

**Example:**

```cpp
#include "auth/ldap_authenticator.h"

using namespace themis::auth;

LDAPConfig cfg;
cfg.server_uri = "ldaps://dc.example.com:636";
cfg.bind_dn_template = "uid={user},ou=people,dc=example,dc=com";
LDAPAuthenticator ldap(cfg);

auto result = ldap.authenticate("alice", "secret");
if (result.success) {
    // result.dn, result.attributes, result.roles available
}
```

**Thread Safety:** Yes (one connection per call)

---

### mtls_authenticator.h

**Purpose:** Mutual TLS client certificate authentication for service-to-service access

**Key Classes:**

- `MTLSAuthenticator`: Certificate validation and principal extraction
- `MTLSClaims`: Certificate subject, SAN, and organizational attributes

**Thread Safety:** Yes

---

### webauthn_authenticator.h

**Purpose:** WebAuthn/FIDO2 hardware token, passkey, and biometric authentication

**Key Classes:**

- `WebAuthnAuthenticator`: WebAuthn credential registration and assertion verification

**Thread Safety:** Yes

---

### api_key_authenticator.h

**Purpose:** Static API key + secret authentication for service accounts and integrations

**Key Classes:**

- `ApiKeyAuthenticator`: Key validation and metadata lookup
- `ApiKeyCredential`: Key credential including permissions and expiry
- `ApiKeyClaims`: Extracted API key claims

**Thread Safety:** Yes

---

### federated_identity_manager.h

**Purpose:** Unified identity namespace across multiple identity realms and providers

**Key Classes:**

- `FederatedIdentityManager`: Multi-provider identity federation and principal bridging
- `FederatedValidationResult`: Federated authentication result

**Thread Safety:** Yes

---

### session_manager.h

**Purpose:** Session lifecycle management with idle/absolute timeouts and revocation

**Key Classes:**

- `SessionManager`: Session creation, validation, refresh, and revocation

**Thread Safety:** Yes (internal mutex on session map)

---

### token_blacklist.h

**Purpose:** Revoked token tracking by JTI (JWT ID) for logout and forced revocation

**Key Classes:**

- `TokenBlacklist`: JTI-based revocation store (in-memory with optional RocksDB persistence)

**Thread Safety:** Yes

---

### password_policy.h

**Purpose:** Configurable password complexity, history, and expiry enforcement

**Key Classes:**

- `PasswordPolicy`: Policy evaluation and enforcement
- `PolicyEngine` (forward-declared): Structured JSON policy engine

**Thread Safety:** Yes (stateless evaluation)

---

### principal_validator.h

**Purpose:** Principal format validation, normalization, and role mapping

**Key Classes:**

- `PrincipalValidator`: Validates and maps principals to roles
- `PrincipalValidatorPresets`: Factory for common preset configurations

**Thread Safety:** Yes

---

### auth_rate_limiter.h

**Purpose:** Brute-force and replay attack rate limiting with exponential backoff

**Key Classes:**

- `AuthRateLimiter`: Per-identity token bucket rate limiter
- `AccountLockoutManager`: Temporary lockout after repeated failures
- `AuthRateLimitConfig`: Configuration (max attempts, window, lockout duration)
- `AuthAnomalyEvent`: Anomaly detection event

**Thread Safety:** Yes (lock-free per-identity buckets)

---

### auth_metrics.h

**Purpose:** Prometheus-compatible authentication latency, success, and failure metrics

**Key Classes:**

- `AuthMetrics`: Counters and histograms for auth operations
- `AuthDurationTimer`: RAII timer for measuring authentication duration
- `AuthMethod`: Enum of supported authentication methods

**Thread Safety:** Yes (atomic counters)

---

### auth_audit_logger.h

**Purpose:** Structured audit logging for all authentication events

**Key Classes:**

- `AuthAuditLogger`: Event logger delegating to the utils audit subsystem

**Thread Safety:** Yes

---

### auth_error.h

**Purpose:** Structured authentication error types for consistent error handling

**Key Classes:**

- `AuthError`: Structured error with code, message, and context
- `AuthException`: Throwable wrapper around `AuthError`
- `AuthErrorCode`: Enum of all authentication error codes

**Thread Safety:** N/A (value types)

---

### jwks_validator.h

**Purpose:** JWKS endpoint fetching, caching, and key-by-`kid` lookup

**Key Classes:**

- `JWKSValidator`: JWKS document fetcher with retry and in-memory cache

**Thread Safety:** Yes (mutex-protected cache)

---

### jwks_security.h

**Purpose:** Security validation of JWKS keys (algorithm allow-list, expiry, key strength)

**Key Classes:**

- `JWKSSecureFetcher`: Security-hardened JWKS fetcher
- `JWKSSecurityConfig`: Algorithm allow-list and validation options
- `CertificateUtils`: X.509 certificate parsing utilities
- `JWKKeyInfo`: Key metadata extracted from a JWK entry

**Thread Safety:** Yes

---

### jwt_key_rotation_manager.h

**Purpose:** Automated JWT signing key rotation with zero-downtime overlap period

**Key Classes:**

- `JWTKeyRotationManager`: Rotation scheduler and key lifecycle manager

**Thread Safety:** Yes

---

### totp_replay_cache.h

**Purpose:** One-time code replay prevention via a lock-free ring buffer

**Key Classes:**

- `TOTPReplayCache`: O(1) lock-free cache for used TOTP codes
- `SecureMFAValidator`: Combines replay-prevention with TOTP validation

**Thread Safety:** Yes (lock-free)

---

### totp_secret_encryption.h

**Purpose:** Encrypted TOTP secret storage using AES-256-GCM or libsodium secretbox

**Key Classes:**

- `TOTPSecretEncryption`: Encrypt/decrypt TOTP secrets
- `TOTPSecretRotationManager`: Key rotation for stored TOTP secrets

**Thread Safety:** Yes

---

### zero_trust_auth_verifier.h

**Purpose:** Continuous per-request identity and network policy verification (zero-trust model)

**Key Classes:**

- `ZeroTrustAuthVerifier`: Per-request trust score evaluation and policy enforcement

**Thread Safety:** Yes

---

## Data Structures

### JWTClaims

```cpp
struct JWTClaims {
    std::string sub;                          // Subject (user ID)
    std::string email;                        // User email
    std::string tenant_id;                    // Tenant ID
    std::vector<std::string> groups;          // Group memberships
    std::vector<std::string> roles;           // Role assignments
    std::string issuer;                       // Token issuer
    std::chrono::system_clock::time_point expiration;
    std::optional<std::chrono::system_clock::time_point> not_before;
    std::optional<std::chrono::system_clock::time_point> issued_at;
    std::vector<std::string> audience;

    bool isExpired() const;
};
```

**Usage:**
- Extract user identity and authorization info from JWT
- Check token expiration
- Derive user-specific encryption keys
- Enforce group-based access control

---

### KerberosConfig

```cpp
struct KerberosConfig {
    bool enabled = false;
    std::string service_principal;      // e.g., "themisdb/hostname@REALM"
    std::string keytab_file;            // Path to keytab
    std::string krb5_config;            // Optional krb5.conf path
    bool fallback_to_basic = true;      // Allow fallback

    struct PrincipalMapping {
        std::string principal_pattern;   // Supports wildcards
        std::string role;
    };
    std::vector<PrincipalMapping> principal_mappings;
};
```

**Principal Mapping Examples:**
```cpp
{"admin@EXAMPLE.COM", "admin"}           // Exact match
{"*@EXAMPLE.COM", "user"}                // Wildcard domain
{"dba/*@EXAMPLE.COM", "dba"}             // Wildcard prefix
{"service/*@*.EXAMPLE.COM", "service"}   // Multiple wildcards
```

---

### MFAAuthenticator::EnrollmentData

```cpp
struct EnrollmentData {
    std::string user_id;
    std::string secret_base32;           // Base32-encoded TOTP secret
    std::vector<std::string> recovery_codes;
    std::chrono::system_clock::time_point enrolled_at;
    bool enabled = false;

    nlohmann::json to_json() const;
    static EnrollmentData from_json(const nlohmann::json& j);
};
```

**Storage:**
- `secret_base32` must be encrypted at rest
- `recovery_codes` should be hashed (bcrypt, Argon2)
- Store in secure database with audit logging
- Use JSON serialization for persistence

---

## API Reference

### JWTValidator

#### Constructor
```cpp
explicit JWTValidator(const std::string& jwks_url);
explicit JWTValidator(const JWTValidatorConfig& cfg);
```

#### parseAndValidate()
```cpp
JWTClaims parseAndValidate(const std::string& token);
```
Parses JWT token, verifies signature, validates expiration, and extracts claims.

**Parameters:**
- `token`: Bearer token (with or without "Bearer " prefix)

**Returns:** `JWTClaims` structure

**Throws:** `std::runtime_error` if:
- Token format invalid
- Signature verification fails
- Token expired
- Issuer/audience mismatch

**Example:**
```cpp
try {
    auto claims = validator.parseAndValidate("Bearer eyJ...");
    // Token valid, use claims
} catch (const std::exception& e) {
    // Token invalid
    std::cerr << "Auth failed: " << e.what() << std::endl;
}
```

#### deriveUserKey() [static]
```cpp
static std::vector<uint8_t> deriveUserKey(
    const std::vector<uint8_t>& dek,
    const JWTClaims& claims,
    const std::string& field_name
);
```
Derives user-specific encryption key using HKDF.

**Use Case:** Field-level encryption with user-specific keys

**Parameters:**
- `dek`: Data Encryption Key (master key)
- `claims`: User's JWT claims
- `field_name`: Field identifier for context

**Returns:** Derived 256-bit key

**Security:** Uses HKDF with SHA-256, user ID as salt, field name as info

---

#### hasAccess() [static]
```cpp
static bool hasAccess(const JWTClaims& claims, const std::string& encryption_context);
```
Checks if user has access to group-encrypted data.

**Parameters:**
- `claims`: User's JWT claims
- `encryption_context`: Group name or user ID

**Returns:** `true` if `claims.groups` contains `encryption_context` or `claims.sub` matches

---

### GSSAPIAuthenticator

#### initialize()
```cpp
bool initialize(const KerberosConfig& config);
```
Initializes GSSAPI with service principal and keytab.

**Returns:** `true` on success, `false` on failure

**Side Effects:**
- Loads keytab file
- Acquires server credentials
- Initializes GSSAPI context

**Errors:**
- Keytab file not found or unreadable
- Invalid service principal
- KDC unreachable

---

#### authenticateToken()
```cpp
GSSAPIAuthResult authenticateToken(const std::string& token);
```
Authenticates Kerberos token and extracts principal.

**Parameters:**
- `token`: Base64-encoded GSSAPI token (from Negotiate header)

**Returns:** `GSSAPIAuthResult` with authentication status

**Example:**
```cpp
auto result = auth.authenticateToken(negotiate_token);
if (result.success) {
    std::cout << "Principal: " << result.principal_name << std::endl;
    for (const auto& role : result.roles) {
        std::cout << "Role: " << role << std::endl;
    }
} else {
    std::cerr << "Error: " << result.error_message << std::endl;
}
```

---

#### mapPrincipalToRoles()
```cpp
std::vector<std::string> mapPrincipalToRoles(const std::string& principal) const;
```
Maps Kerberos principal to ThemisDB roles based on configuration.

**Parameters:**
- `principal`: Kerberos principal (e.g., "alice@EXAMPLE.COM")

**Returns:** Vector of role names

**Mapping Rules:**
- Exact match: `"admin@EXAMPLE.COM"` → `["admin"]`
- Wildcard: `"*@EXAMPLE.COM"` → `["user"]`
- Prefix wildcard: `"dba/*@EXAMPLE.COM"` → `["dba"]`
- First match wins (check specific before wildcard)

---

### MFAAuthenticator

#### generateEnrollment()
```cpp
EnrollmentData generateEnrollment(const std::string& user_id);
```
Generates new TOTP secret and recovery codes for user enrollment.

**Parameters:**
- `user_id`: User identifier

**Returns:** `EnrollmentData` with secret and recovery codes

**Security:**
- Uses cryptographically secure random number generator
- Secret is 160 bits (20 bytes)
- Recovery codes are 12 characters each

---

#### generateProvisioningURI()
```cpp
std::string generateProvisioningURI(const EnrollmentData& enrollment) const;
```
Generates TOTP provisioning URI for QR code.

**Format:** `otpauth://totp/{issuer}:{user}?secret={secret}&issuer={issuer}`

**Example Output:**
```
otpauth://totp/ThemisDB:alice@example.com?secret=JBSWY3DPEHPK3PXP&issuer=ThemisDB
```

**Usage:**
- Generate QR code from URI
- User scans with authenticator app (Google Authenticator, Authy, etc.)
- App automatically configures TOTP

---

#### validateTOTP()
```cpp
bool validateTOTP(
    const std::string& secret_base32,
    const std::string& code,
    std::optional<std::chrono::system_clock::time_point> timestamp = std::nullopt
) const;
```
Validates TOTP code against secret.

**Parameters:**
- `secret_base32`: User's TOTP secret (Base32 encoded)
- `code`: 6 or 8 digit TOTP code from user
- `timestamp`: Optional timestamp (defaults to current time)

**Returns:** `true` if code valid within time window

**Time Window:**
- Default: ±1 time step (±30 seconds)
- Prevents replay attacks
- Accommodates clock drift

**Rate Limiting:**
- Implement externally (max 5 attempts per minute)
- Lock account after 10 failed attempts

---

#### validateRecoveryCode()
```cpp
bool validateRecoveryCode(
    EnrollmentData& enrollment,
    const std::string& recovery_code
);
```
Validates and marks recovery code as used.

**Parameters:**
- `enrollment`: User's enrollment data (modified in-place)
- `recovery_code`: Recovery code to validate

**Returns:** `true` if code valid and unused

**Side Effects:**
- Marks code as used (removes from `enrollment.recovery_codes`)
- Caller must save updated enrollment to database

**Security:**
- Recovery codes are single-use only
- Should be hashed in database
- Generate new codes after all are used

---

#### getCurrentTOTP()
```cpp
std::string getCurrentTOTP(
    const std::string& secret_base32,
    std::optional<std::chrono::system_clock::time_point> timestamp = std::nullopt
) const;
```
Computes current TOTP code for secret (for testing/validation).

**Parameters:**
- `secret_base32`: TOTP secret
- `timestamp`: Optional timestamp

**Returns:** Current TOTP code (6 or 8 digits)

**Use Cases:**
- Testing TOTP implementation
- Generating codes for automated testing
- Debugging time sync issues

---

## Integration Guide

### Basic JWT Authentication

```cpp
#include "auth/jwt_validator.h"

// In your HTTP handler
std::string bearer_token = request.getHeader("Authorization");

try {
    auto claims = jwt_validator.parseAndValidate(bearer_token);

    // Check user has required role
    bool is_admin = std::find(claims.roles.begin(), claims.roles.end(), "admin")
                    != claims.roles.end();

    if (!is_admin) {
        return response.sendError(403, "Forbidden");
    }

    // Process request
    // ...

} catch (const std::exception& e) {
    return response.sendError(401, "Unauthorized");
}
```

---

### Kerberos SSO with Fallback

```cpp
#include "auth/gssapi_authenticator.h"
#include "auth/jwt_validator.h"

// Try Kerberos first
std::string negotiate_header = request.getHeader("Authorization");
if (negotiate_header.starts_with("Negotiate ")) {
    auto result = gssapi_auth.authenticateToken(negotiate_header.substr(10));
    if (result.success) {
        // Kerberos success
        session.setPrincipal(result.principal_name);
        session.setRoles(result.roles);
        return;
    }
}

// Fallback to JWT
std::string bearer_header = request.getHeader("Authorization");
if (bearer_header.starts_with("Bearer ")) {
    auto claims = jwt_validator.parseAndValidate(bearer_header);
    session.setUserId(claims.sub);
    session.setRoles(claims.roles);
    return;
}

return response.sendError(401, "Unauthorized");
```

---

### MFA Enrollment Flow

```cpp
#include "auth/mfa_authenticator.h"

// Step 1: User requests MFA enrollment
auto enrollment = mfa.generateEnrollment(user_id);

// Step 2: Display QR code
std::string qr_uri = mfa.generateProvisioningURI(enrollment);
response.sendJSON({
    {"qr_uri", qr_uri},
    {"secret", enrollment.secret_base32},  // For manual entry
    {"recovery_codes", enrollment.recovery_codes}
});

// Step 3: User scans QR code and enters first TOTP
std::string totp_code = request.getParam("totp_code");
if (mfa.validateTOTP(enrollment.secret_base32, totp_code)) {
    enrollment.enabled = true;
    saveEnrollmentToDatabase(user_id, enrollment);
    response.sendJSON({{"status", "success"}});
} else {
    response.sendError(400, "Invalid TOTP code");
}
```

---

### MFA Login Flow

```cpp
// Step 1: Validate username/password (or JWT)
auto user = authenticateUser(username, password);

// Step 2: Check if MFA enabled
auto enrollment = loadEnrollmentFromDatabase(user.id);
if (!enrollment.enabled) {
    // No MFA, create session
    createSession(user);
    return;
}

// Step 3: Challenge for MFA
response.sendJSON({{"mfa_required", true}});

// Step 4: Validate TOTP or recovery code
std::string totp_code = request.getParam("totp_code");
if (mfa.validateTOTP(enrollment.secret_base32, totp_code)) {
    createSession(user);
    return;
}

std::string recovery_code = request.getParam("recovery_code");
if (mfa.validateRecoveryCode(enrollment, recovery_code)) {
    saveEnrollmentToDatabase(user.id, enrollment);  // Save updated enrollment
    createSession(user);
    return;
}

response.sendError(401, "Invalid MFA code");
```

---

## Thread Safety

### Thread-Safe Components

All components except `GSSAPIAuthenticator` are thread-safe and can be shared across threads:

| Component | Thread-Safe | Notes |
|-----------|-------------|-------|
| `JWTValidator` | ✅ Yes | JWKS cache uses internal mutex |
| `MFAAuthenticator` | ✅ Yes | Stateless validation |
| `OAuthDeviceFlow` | ✅ Yes | Immutable after construction |
| `OAuthPKCEFlow` | ✅ Yes | Immutable after construction |
| `OIDCProvider` | ✅ Yes | Discovery doc cached with mutex |
| `SAMLAuthenticator` | ✅ Yes | Stateless per-request validation |
| `LDAPAuthenticator` | ✅ Yes | New connection per call |
| `MTLSAuthenticator` | ✅ Yes | Stateless certificate validation |
| `WebAuthnAuthenticator` | ✅ Yes | |
| `ApiKeyAuthenticator` | ✅ Yes | Immutable key store |
| `FederatedIdentityManager` | ✅ Yes | |
| `SessionManager` | ✅ Yes | Internal mutex on session map |
| `TokenBlacklist` | ✅ Yes | |
| `PasswordPolicy` | ✅ Yes | Stateless evaluation |
| `PrincipalValidator` | ✅ Yes | |
| `AuthRateLimiter` | ✅ Yes | Lock-free per-identity buckets |
| `AuthMetrics` | ✅ Yes | Atomic counters |
| `AuthAuditLogger` | ✅ Yes | |
| `JWKSValidator` | ✅ Yes | Mutex-protected cache |
| `JWKSSecureFetcher` | ✅ Yes | |
| `JWTKeyRotationManager` | ✅ Yes | |
| `TOTPReplayCache` | ✅ Yes | Lock-free ring buffer |
| `TOTPSecretEncryption` | ✅ Yes | |
| `ZeroTrustAuthVerifier` | ✅ Yes | |
| `KerberosSecurityValidator` | ✅ Yes | |

### Non-Thread-Safe Components

**GSSAPIAuthenticator:**
- ❌ Not thread-safe
- GSSAPI context is stateful
- Create separate instance per thread or use mutex

**Recommendation:**
```cpp
// Per-thread instance (preferred)
thread_local GSSAPIAuthenticator gssapi_auth;

// Or use mutex
std::mutex gssapi_mutex;
{
    std::lock_guard<std::mutex> lock(gssapi_mutex);
    auto result = gssapi_auth.authenticateToken(token);
}
```

---

## Error Handling

### JWT Validation Errors

```cpp
try {
    auto claims = validator.parseAndValidate(token);
} catch (const std::runtime_error& e) {
    std::string error = e.what();

    if (error.find("expired") != std::string::npos) {
        // Token expired - prompt for refresh
    } else if (error.find("Invalid signature") != std::string::npos) {
        // Invalid token - reject
    } else if (error.find("JWKS HTTP error") != std::string::npos) {
        // Network issue - retry or fallback
    }
}
```

### Kerberos Errors

```cpp
auto result = gssapi_auth.authenticateToken(token);
if (!result.success) {
    if (result.error_message.find("Clock skew") != std::string::npos) {
        // Time sync issue
    } else if (result.error_message.find("Cannot find KDC") != std::string::npos) {
        // Network/DNS issue
    } else if (result.error_message.find("Invalid keytab") != std::string::npos) {
        // Configuration issue
    }
}
```

---

## Performance Considerations

### JWT Validation
- **JWKS Cache:** Default 10-minute TTL (configurable)
- **Latency:** ~5ms with cached JWKS, ~200ms on cache miss
- **Optimization:** Increase cache TTL in stable environments

### TOTP Validation
- **Latency:** ~5-10ms (HMAC-SHA1 computation)
- **No Network Calls:** All computation local
- **Optimization:** None needed (already fast)

### Kerberos Validation
- **Latency:** ~50-200ms (network-dependent)
- **KDC Dependency:** Requires KDC connectivity
- **Optimization:** Connection pooling, local credential cache

---

## Compliance

### Standards Implemented

- ✅ RFC 6238 (TOTP)
- ✅ RFC 7519 (JWT)
- ✅ RFC 7636 (OAuth 2.0 PKCE)
- ✅ RFC 8628 (OAuth 2.0 Device Authorization Grant)
- ✅ RFC 4120 (Kerberos v5)
- ✅ OpenID Connect Core 1.0
- ✅ SAML 2.0 (Web Browser SSO Profile)
- ✅ WebAuthn Level 2 / FIDO2
- ✅ NIST SP 800-63B Level 2
- ✅ SOC 2 CC6.1

---

## Implementation

See `../../src/auth/` for the implementation code.

## Full Documentation

- [Authentication Module README](../../src/auth/README.md) — Comprehensive guide
- [Architecture Guide](../../src/auth/ARCHITECTURE.md) — Component design and data flows
- [Security Guide](../../src/auth/SECURITY.md) — Threat model, controls, and limitations
- [Audit Report](../../src/auth/AUDIT.md) — Build/test/compliance verification snapshot
- [Changelog](../../src/auth/CHANGELOG.md) — Versioned module change history
- [Performance Expectations](../../src/auth/PERFORMANCE_EXPECTATIONS.md) — Release-gate performance targets
- [Roadmap](../../src/auth/ROADMAP.md) — Implementation status and planned features
- [Future Enhancements](../../src/auth/FUTURE_ENHANCEMENTS.md) — Planned features
- [Secondary Docs (DE)](../../docs/de/auth/README.md) — German-language overview

## Related Headers

- **Security Module** (`../security/`): Encryption, TLS, key management
- **Server Module** (`../server/`): HTTP request handling
- **API Module** (`../api/`): REST API definitions

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
