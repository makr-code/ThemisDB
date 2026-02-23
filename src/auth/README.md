# Authentication Module

Comprehensive authentication and authorization implementation for ThemisDB with enterprise SSO and multi-factor authentication support.

## Module Purpose

Provides enterprise-grade authentication and authorization for ThemisDB, including JWT/OpenID Connect, Kerberos/GSSAPI, TOTP MFA, and RBAC enforcement.

## Subsystem Scope

**In scope:** JWT validation with JWKS, Kerberos/GSSAPI SSO, TOTP MFA with recovery codes, RBAC principal-to-role mapping, brute force rate limiting.

**Out of scope:** User management (external IdP), secrets storage (handled by security module), audit logging (handled by utils module).

## Relevant Interfaces

- `jwt_validator.cpp` — JWT/OpenID Connect validation
- `kerberos_auth.cpp` — GSSAPI/Kerberos handler
- `totp_auth.cpp` — TOTP MFA
- `rbac_enforcer.cpp` — role-based access control
- `oauth_device_flow.cpp` — OAuth 2.0 device authorization flow (RFC 8628)
- `oidc_provider.cpp` — OIDC Provider Discovery and federated identity integration

## Current Delivery Status

**Maturity:** 🟡 Beta — JWT, Kerberos, TOTP, and RBAC operational; OAuth 2.0 device flow and SAML 2.0 in progress.

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

- **JWT Authentication**: OpenID Connect integration with Keycloak
- **OIDC Provider Federation**: Auto-configure from any OIDC provider's discovery endpoint
- **Kerberos/GSSAPI**: Enterprise SSO for Active Directory environments
- **Multi-Factor Authentication**: TOTP-based MFA with recovery codes
- **OAuth 2.0 Device Flow**: Headless device / CLI authentication (RFC 8628)
- **Signature Verification**: RS256 with JWKS caching
- **Principal-to-Role Mapping**: Flexible authorization system
- **Clock Skew Tolerance**: Distributed system friendly
- **Rate Limiting**: Brute force and replay attack prevention

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
```

### Integration Tests

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

### Load Testing

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
- [JWT Validator Implementation](../../docs/src/auth/jwt_validator.cpp.md)
- [Kerberos Authentication Guide](../../docs/en/security/KERBEROS_AUTHENTICATION.md)
- [RBAC Authorization](../../docs/rbac_authorization.md)
- [Security Hardening](../security/README.md)
- [API Authentication](../../docs/api/authentication.md)

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
