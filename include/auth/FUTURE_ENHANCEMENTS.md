# Auth Module Headers - Future Enhancements

## Scope

- API-level enhancements to `include/auth/` headers — new authenticator interfaces: WebAuthn, device flow, and passkeys
- Multi-factor authentication engine API (`AdaptiveMFAEngine`) with risk-based challenge selection
- Advanced session manager API with device/IP pinning, anomaly detection, and concurrent session limits
- Backward-compatible JWT enhancements: token introspection, refresh token support, and structured `AuthError` codes
- Magic link passwordless authentication interface (`MagicLinkAuthenticator`)
- Error handling redesign: structured `Expected<T, AuthError>` return types replacing exception-based API

## Design Constraints

- [ ] All new authenticators must implement the `IAuthenticator` interface; no standalone authentication logic outside the contract
- [ ] Breaking changes (namespace reorganization, `JWTClaims` expansion) are isolated to major version v2.0.0
- [ ] All secret and token comparisons must use constant-time comparison to prevent timing-based side-channel attacks
- [ ] Session tokens must never appear in log output; `SessionInfo` must not expose raw token values
- [ ] New public headers must remain self-contained — no transitive inclusion of third-party crypto headers in the public API
- [ ] `AdaptiveMFAEngine` risk weights are runtime-configurable but must not be modifiable via unauthenticated API calls

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IAuthenticator` | All new authenticator implementations | Common `authenticate()` contract |
| `WebAuthnAuthenticator` | WebAuthn / FIDO2 registration and auth | W3C WebAuthn Level 2 |
| `PasskeyAuthenticator` | Passwordless login flows | Wraps `WebAuthnAuthenticator` with resident keys |
| `AdaptiveMFAEngine` | Risk-based MFA middleware | Configurable risk weights; thread-safe |
| `SessionManager` | Session lifecycle consumers | Device-pinned; anomaly-detecting |
| `JWTValidator` (enhanced) | Token introspection and refresh consumers | Backward-compatible additions only |

Planned API changes and new header files for ThemisDB authentication.

## Table of Contents

1. [New Header Files](#new-header-files)
2. [API Enhancements](#api-enhancements)
3. [Breaking Changes](#breaking-changes)
4. [Deprecations](#deprecations)
5. [ABI Compatibility](#abi-compatibility)

---

## New Header Files

### device_flow_authenticator.h
**Priority:** High
**Target Version:** v1.6.0

OAuth 2.0 Device Authorization Grant (RFC 8628) for CLI and IoT devices.

```cpp
#pragma once

#include <string>
#include <optional>
#include <chrono>

namespace themis {
namespace auth {

/**
 * @brief OAuth 2.0 Device Flow authenticator for headless devices
 *
 * Implements RFC 8628 Device Authorization Grant for devices without
 * browsers or limited input capability.
 */
class DeviceFlowAuthenticator {
public:
    struct DeviceCodeResponse {
        std::string device_code;              // Opaque device code
        std::string user_code;                // Human-readable code (e.g., "BDWP-HQMF")
        std::string verification_uri;         // URL to visit
        std::string verification_uri_complete; // QR code URL
        int expires_in;                       // Expiration in seconds
        int interval;                         // Polling interval in seconds
    };

    /**
     * @brief Initialize with OAuth 2.0 configuration
     */
    DeviceFlowAuthenticator(
        const std::string& device_auth_endpoint,
        const std::string& token_endpoint,
        const std::string& client_id
    );

    /**
     * @brief Request device code from authorization server
     *
     * @param scopes Optional scopes to request
     * @return Device code response with user code and verification URI
     * @throws std::runtime_error on HTTP or parse error
     */
    DeviceCodeResponse requestDeviceCode(
        const std::vector<std::string>& scopes = {}
    );

    /**
     * @brief Poll for token (after user authorizes)
     *
     * @param device_code Device code from requestDeviceCode()
     * @return JWT claims if authorized, nullopt if still pending
     * @throws std::runtime_error on authorization_denied or expired_token
     */
    std::optional<JWTClaims> pollForToken(const std::string& device_code);

private:
    std::string device_auth_endpoint_;
    std::string token_endpoint_;
    std::string client_id_;
};

} // namespace auth
} // namespace themis
```

**Use Case:**
```cpp
DeviceFlowAuthenticator device_flow(auth_endpoint, token_endpoint, client_id);

// Step 1: Request device code
auto response = device_flow.requestDeviceCode();
std::cout << "Visit " << response.verification_uri
          << " and enter code: " << response.user_code << std::endl;

// Step 2: Poll for authorization
while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(response.interval));

    auto claims = device_flow.pollForToken(response.device_code);
    if (claims) {
        std::cout << "Authenticated as " << claims->email << std::endl;
        break;
    }
}
```

---

### webauthn_authenticator.h
**Priority:** High
**Target Version:** v1.6.0

WebAuthn/FIDO2 hardware security key and biometric authentication.

```cpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace auth {

/**
 * @brief WebAuthn/FIDO2 authenticator for hardware keys and biometrics
 *
 * Implements W3C WebAuthn Level 2 specification for phishing-resistant
 * authentication using hardware security keys, platform authenticators
 * (Touch ID, Face ID, Windows Hello), or passkeys.
 */
class WebAuthnAuthenticator {
public:
    /**
     * @brief Relying Party configuration
     */
    struct RelyingParty {
        std::string id;            // Domain (e.g., "example.com")
        std::string name;          // Display name (e.g., "ThemisDB")
    };

    /**
     * @brief User information for registration
     */
    struct User {
        std::string id;            // Opaque user ID
        std::string name;          // Username or email
        std::string display_name;  // Full name
    };

    /**
     * @brief Credential creation options for registration
     */
    struct CredentialCreationOptions {
        std::string challenge;     // Base64url random challenge
        RelyingParty rp;
        User user;
        std::vector<std::string> pub_key_cred_params;  // ["ES256", "RS256"]
        std::optional<int> timeout_ms;
        std::string attestation = "none";  // "none", "direct", "indirect"

        // Authenticator selection criteria
        struct AuthenticatorSelection {
            std::optional<std::string> authenticator_attachment;  // "platform", "cross-platform"
            bool require_resident_key = false;
            std::string user_verification = "preferred";  // "required", "preferred", "discouraged"
        } authenticator_selection;

        // Exclude existing credentials (prevent duplicate registration)
        std::vector<std::string> exclude_credentials;

        nlohmann::json to_json() const;
    };

    /**
     * @brief Credential request options for authentication
     */
    struct CredentialRequestOptions {
        std::string challenge;
        std::string rp_id;
        std::optional<int> timeout_ms;
        std::string user_verification = "preferred";

        // Allow specific credentials (empty = discoverable/passkey)
        std::vector<std::string> allow_credentials;

        nlohmann::json to_json() const;
    };

    /**
     * @brief Attestation result from registration
     */
    struct AttestationResult {
        std::string credential_id;
        std::vector<uint8_t> public_key;
        std::string algorithm;     // "ES256", "RS256", etc.
        int sign_count;
        std::vector<uint8_t> aaguid;  // Authenticator model
    };

    /**
     * @brief Assertion result from authentication
     */
    struct AssertionResult {
        std::string credential_id;
        int sign_count;
        std::optional<std::string> user_handle;  // For passkeys
    };

    explicit WebAuthnAuthenticator(const RelyingParty& rp);

    /**
     * @brief Generate credential creation options for registration
     *
     * @param user User information
     * @param resident_key If true, creates discoverable credential (passkey)
     * @return Options to send to client
     */
    CredentialCreationOptions startRegistration(
        const User& user,
        bool resident_key = false
    );

    /**
     * @brief Verify attestation response from client
     *
     * @param credential_response JSON response from navigator.credentials.create()
     * @return Attestation result with credential ID and public key
     * @throws std::runtime_error on verification failure
     */
    AttestationResult completeRegistration(const nlohmann::json& credential_response);

    /**
     * @brief Generate credential request options for authentication
     *
     * @param user_id Optional user ID for non-discoverable credentials
     * @return Options to send to client
     */
    CredentialRequestOptions startAuthentication(
        const std::optional<std::string>& user_id = std::nullopt
    );

    /**
     * @brief Verify assertion response from client
     *
     * @param credential_response JSON response from navigator.credentials.get()
     * @param stored_public_key Public key from registration
     * @param stored_sign_count Previous signature counter
     * @return Assertion result
     * @throws std::runtime_error on verification failure
     */
    AssertionResult completeAuthentication(
        const nlohmann::json& credential_response,
        const std::vector<uint8_t>& stored_public_key,
        int stored_sign_count
    );

private:
    RelyingParty rp_;

    // Generate cryptographically secure random challenge
    std::string generateChallenge();

    // Verify signature using stored public key
    bool verifySignature(
        const std::vector<uint8_t>& signature,
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& public_key,
        const std::string& algorithm
    );
};

} // namespace auth
} // namespace themis
```

---

### passkey_authenticator.h
**Priority:** High
**Target Version:** v1.6.0

Simplified passkey API built on WebAuthn with resident keys.

```cpp
#pragma once

#include "auth/webauthn_authenticator.h"
#include <string>

namespace themis {
namespace auth {

/**
 * @brief Simplified passkey authenticator (WebAuthn with resident keys)
 *
 * Passkeys are discoverable credentials that sync across user's devices
 * via iCloud Keychain, Google Password Manager, etc.
 *
 * Benefits:
 * - True passwordless (no username/password)
 * - Phishing-resistant
 * - Biometric verification
 * - Cross-device sync
 */
class PasskeyAuthenticator {
public:
    explicit PasskeyAuthenticator(const WebAuthnAuthenticator::RelyingParty& rp);

    /**
     * @brief Start passkey registration
     *
     * @param user_id User identifier
     * @param username Username or email
     * @param display_name User's full name
     * @return Options for client
     */
    WebAuthnAuthenticator::CredentialCreationOptions startRegistration(
        const std::string& user_id,
        const std::string& username,
        const std::string& display_name
    );

    /**
     * @brief Complete passkey registration
     *
     * @param credential_response JSON from client
     * @return Credential info to store
     */
    struct PasskeyCredential {
        std::string credential_id;
        std::string user_id;
        std::vector<uint8_t> public_key;
        std::string algorithm;
        int sign_count;
    };

    PasskeyCredential completeRegistration(const nlohmann::json& credential_response);

    /**
     * @brief Start passwordless authentication (no user_id needed!)
     *
     * @return Options for client (no allowCredentials = discoverable)
     */
    WebAuthnAuthenticator::CredentialRequestOptions startAuthentication();

    /**
     * @brief Complete passwordless authentication
     *
     * @param credential_response JSON from client
     * @param lookup_credential Callback to retrieve credential by ID
     * @return User ID of authenticated user
     */
    std::string completeAuthentication(
        const nlohmann::json& credential_response,
        std::function<PasskeyCredential(const std::string&)> lookup_credential
    );

private:
    WebAuthnAuthenticator webauthn_;
};

} // namespace auth
} // namespace themis
```

**Usage:**
```cpp
// Registration
PasskeyAuthenticator passkey(rp);
auto options = passkey.startRegistration("user-123", "alice@example.com", "Alice");
// Send options to client, get response
auto credential = passkey.completeRegistration(client_response);
// Store credential in database

// Authentication (no username needed!)
auto auth_options = passkey.startAuthentication();
// Send to client, user selects passkey, get response
auto user_id = passkey.completeAuthentication(
    client_response,
    [](const std::string& cred_id) { return lookupCredential(cred_id); }
);
// User authenticated!
```

---

### adaptive_mfa_engine.h
**Priority:** Medium
**Target Version:** v1.7.0

Risk-based adaptive multi-factor authentication.

```cpp
#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace themis {
namespace auth {

/**
 * @brief Adaptive MFA engine for risk-based authentication
 *
 * Adjusts MFA requirements based on contextual risk signals:
 * - IP reputation and geolocation
 * - Device fingerprint (known vs. unknown)
 * - Impossible travel detection
 * - Time-of-day patterns
 * - Velocity checks
 */
class AdaptiveMFAEngine {
public:
    enum class RiskLevel {
        Low,
        Medium,
        High,
        Critical
    };

    struct AuthenticationContext {
        std::string user_id;
        std::string ip_address;
        std::string user_agent;
        std::string device_fingerprint;
        std::optional<std::string> geolocation;  // Country code
        std::chrono::system_clock::time_point timestamp;
        bool new_device;
        bool vpn_detected;
        int failed_attempts_24h;
    };

    struct MFARequirement {
        bool mfa_required;
        RiskLevel risk_level;
        std::vector<std::string> acceptable_methods;  // ["totp", "webauthn", "push"]
        int step_up_factor;  // 1 = normal, 2 = require 2 MFA factors
        std::string reason;  // Human-readable explanation

        // Recommended action
        enum class Action {
            Allow,              // No MFA required
            RequireMFA,         // Standard MFA
            RequireStrongMFA,   // WebAuthn or better
            RequireAdminApproval,
            Block
        } action;
    };

    /**
     * @brief Evaluate risk and determine MFA requirements
     */
    MFARequirement evaluateRisk(const AuthenticationContext& context);

    /**
     * @brief Update user's behavior baseline
     */
    void updateBehaviorBaseline(
        const std::string& user_id,
        const AuthenticationContext& context
    );

    /**
     * @brief Configure risk scoring weights
     */
    struct RiskWeights {
        double ip_reputation_weight = 1.0;
        double geolocation_weight = 1.5;
        double device_fingerprint_weight = 2.0;
        double impossible_travel_weight = 3.0;
        double unusual_time_weight = 0.5;
        double velocity_weight = 1.0;
    };

    void setRiskWeights(const RiskWeights& weights);

private:
    RiskWeights weights_;

    // Calculate individual risk factors
    double calculateIPReputationScore(const std::string& ip);
    bool detectImpossibleTravel(const std::string& user_id, const std::string& location);
    bool isUnusualTime(const std::string& user_id, std::chrono::system_clock::time_point time);
};

} // namespace auth
} // namespace themis
```

---

### magic_link_authenticator.h
**Priority:** Medium
**Target Version:** v1.7.0

Email-based passwordless authentication.

```cpp
#pragma once

#include <string>
#include <chrono>
#include <optional>

namespace themis {
namespace auth {

/**
 * @brief Magic link authenticator for passwordless email authentication
 *
 * Generates time-limited, single-use tokens sent via email for authentication.
 * More user-friendly than passwords but requires secure email delivery.
 */
class MagicLinkAuthenticator {
public:
    struct Config {
        std::chrono::seconds link_ttl = std::chrono::seconds(600);  // 10 minutes
        std::string base_url;  // e.g., "https://app.example.com/auth/verify"
        bool require_same_device = false;  // More secure but less convenient
        bool single_use = true;
    };

    explicit MagicLinkAuthenticator(const Config& config);

    /**
     * @brief Generate magic link for user
     *
     * @param email User's email address
     * @param redirect_url Optional URL to redirect after authentication
     * @return Full magic link URL to send via email
     */
    std::string generateMagicLink(
        const std::string& email,
        const std::string& redirect_url = ""
    );

    /**
     * @brief Validate magic link token
     *
     * @param token Token from URL query parameter
     * @return Validation result with email if successful
     */
    struct ValidationResult {
        bool valid;
        std::string email;
        std::optional<std::string> redirect_url;
        std::string error_message;
    };

    ValidationResult validateMagicLink(const std::string& token);

private:
    Config config_;

    // Sign token with HMAC-SHA256
    std::string signToken(const std::string& email, int64_t expiration);

    // Verify token signature and expiration
    bool verifyToken(const std::string& token);

    // Mark token as used (if single_use enabled)
    void markTokenUsed(const std::string& token);
};

} // namespace auth
} // namespace themis
```

---

### session_manager.h
**Priority:** High
**Target Version:** v1.6.0

Advanced session management with pinning and anomaly detection.

```cpp
#pragma once

#include "auth/jwt_validator.h"
#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace themis {
namespace auth {

/**
 * @brief Advanced session manager with security features
 *
 * Features:
 * - Session pinning to device and IP
 * - Concurrent session management
 * - Anomaly detection
 * - Idle and absolute timeouts
 */
class SessionManager {
public:
    struct SessionInfo {
        std::string session_id;
        std::string user_id;
        std::string device_fingerprint;
        std::string ip_address;
        std::string user_agent;
        std::string location;  // GeoIP lookup
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_activity;
        bool is_current;
    };

    struct SessionLimits {
        int max_concurrent_sessions = 5;
        std::chrono::seconds idle_timeout = std::chrono::hours(24);
        std::chrono::seconds absolute_timeout = std::chrono::days(30);
        bool pin_to_ip = true;
        bool pin_to_device = true;
    };

    explicit SessionManager(const SessionLimits& limits = SessionLimits{});

    /**
     * @brief Create new session
     */
    std::string createSession(
        const JWTClaims& claims,
        const std::string& device_fingerprint,
        const std::string& ip_address,
        const std::string& user_agent
    );

    /**
     * @brief Validate existing session
     */
    struct ValidationResult {
        bool valid;
        std::optional<SessionInfo> session;
        std::string error_message;
    };

    ValidationResult validateSession(
        const std::string& session_id,
        const std::string& current_ip,
        const std::string& current_device_fingerprint
    );

    /**
     * @brief List all active sessions for user
     */
    std::vector<SessionInfo> listSessions(const std::string& user_id);

    /**
     * @brief Terminate specific session
     */
    void terminateSession(const std::string& session_id);

    /**
     * @brief Terminate all sessions except current
     */
    void terminateAllOtherSessions(
        const std::string& user_id,
        const std::string& current_session_id
    );

    /**
     * @brief Detect session anomalies
     */
    enum class AnomalyType {
        ImpossibleTravel,
        UnusualLocation,
        IPAddressChange,
        DeviceChange,
        SuspiciousActivity
    };

    struct Anomaly {
        AnomalyType type;
        int severity;  // 0-100
        std::string description;
        std::chrono::system_clock::time_point detected_at;
    };

    std::vector<Anomaly> detectAnomalies(const std::string& session_id);

private:
    SessionLimits limits_;

    // Check if IP or device changed (session hijacking)
    bool isPinValid(const SessionInfo& session, const std::string& ip, const std::string& device);

    // Enforce session limits (kick oldest if exceeded)
    void enforceSessionLimits(const std::string& user_id);
};

} // namespace auth
} // namespace themis
```

---

## API Enhancements

### JWTValidator Enhancements
**Target Version:** v1.6.0

Add token introspection and refresh token support.

```cpp
// Add to jwt_validator.h

class JWTValidator {
public:
    // ... existing methods ...

    /**
     * @brief Introspect token with authorization server (RFC 7662)
     *
     * Validates token in real-time with IdP, useful for:
     * - Checking revocation status
     * - Validating opaque tokens
     * - Getting extended token metadata
     */
    struct IntrospectionResponse {
        bool active;
        std::optional<std::string> scope;
        std::optional<std::string> client_id;
        std::optional<std::string> username;
        std::optional<int> exp;
        std::optional<int> iat;
        std::optional<std::string> sub;
        nlohmann::json extra;
    };

    IntrospectionResponse introspect(
        const std::string& token,
        const std::string& introspection_endpoint,
        const std::string& client_id,
        const std::string& client_secret
    );

    /**
     * @brief Refresh access token using refresh token
     */
    struct RefreshResult {
        std::string access_token;
        std::string refresh_token;  // New refresh token (if rotation enabled)
        int expires_in;
        std::string token_type;
    };

    RefreshResult refreshToken(
        const std::string& refresh_token,
        const std::string& token_endpoint,
        const std::string& client_id,
        const std::string& client_secret = ""
    );
};
```

---

### MFAAuthenticator Enhancements
**Target Version:** v1.6.0

Add support for multiple TOTP devices per user.

```cpp
// Add to mfa_authenticator.h

class MFAAuthenticator {
public:
    // ... existing methods ...

    /**
     * @brief Register additional TOTP device for user
     *
     * Allows users to have multiple devices (phone, tablet, hardware token)
     * for MFA redundancy.
     */
    struct DeviceEnrollment {
        std::string device_id;
        std::string device_name;  // User-friendly name
        std::string secret_base32;
        std::chrono::system_clock::time_point enrolled_at;
        std::chrono::system_clock::time_point last_used;
    };

    DeviceEnrollment registerDevice(
        const std::string& user_id,
        const std::string& device_name
    );

    /**
     * @brief Validate TOTP against any of user's devices
     */
    struct ValidationResult {
        bool valid;
        std::optional<std::string> device_id;  // Which device was used
    };

    ValidationResult validateTOTPAnyDevice(
        const std::string& user_id,
        const std::string& code
    );

    /**
     * @brief List all enrolled devices for user
     */
    std::vector<DeviceEnrollment> listDevices(const std::string& user_id);

    /**
     * @brief Remove device
     */
    void removeDevice(const std::string& user_id, const std::string& device_id);
};
```

---

## Breaking Changes

### Version 2.0.0

**1. Namespace Reorganization**

Current:
```cpp
namespace themis {
namespace auth {
```

Planned:
```cpp
namespace themis {
namespace auth {
namespace v2 {
```

**Reason:** Major API redesign for better modularity and extensibility.

**Migration:**
```cpp
// Old
using namespace themis::auth;

// New
using namespace themis::auth::v2;
// Or use inline namespace for compatibility
```

---

**2. JWTClaims Structure Expansion**

Current:
```cpp
struct JWTClaims {
    std::string sub;
    std::string email;
    // ... limited fields
};
```

Planned:
```cpp
struct JWTClaims {
    std::string sub;
    std::string email;
    // ... existing fields ...

    // New fields
    std::optional<std::string> phone_number;
    std::optional<bool> phone_number_verified;
    std::optional<bool> email_verified;
    std::optional<std::string> preferred_username;
    std::optional<std::string> given_name;
    std::optional<std::string> family_name;
    std::optional<std::string> locale;
    std::optional<std::string> zoneinfo;

    // Extensible custom claims
    nlohmann::json custom_claims;
};
```

**Impact:** Binary incompatible (struct size changed)

**Migration:**
- Recompile all code that uses JWTClaims
- Access new fields via std::optional

---

**3. Error Handling Redesign**

Current:
```cpp
JWTClaims parseAndValidate(const std::string& token);  // throws std::runtime_error
```

Planned:
```cpp
// Use std::expected (C++23) or custom Result type
Expected<JWTClaims, AuthError> parseAndValidate(const std::string& token);

enum class AuthErrorCode {
    InvalidToken,
    ExpiredToken,
    InvalidSignature,
    NetworkError,
    ConfigurationError
};

struct AuthError {
    AuthErrorCode code;
    std::string message;
    std::optional<nlohmann::json> details;
};
```

**Reason:** More structured error handling, better for API consumers

**Migration:**
```cpp
// Old
try {
    auto claims = validator.parseAndValidate(token);
} catch (const std::exception& e) {
    // Handle error
}

// New
auto result = validator.parseAndValidate(token);
if (result) {
    auto claims = *result;
} else {
    auto error = result.error();
    switch (error.code) {
        case AuthErrorCode::ExpiredToken:
            // Handle expired
            break;
        // ...
    }
}
```

---

## Deprecations

### Version 1.7.0

**1. Deprecate Basic Auth Fallback**

```cpp
// In gssapi_authenticator.h
struct KerberosConfig {
    [[deprecated("Use separate authentication method instead")]]
    bool fallback_to_basic = true;
};
```

**Reason:** Mixing strong (Kerberos) and weak (basic) auth in same component is bad practice.

**Alternative:** Use separate authentication middleware.

---

**2. Deprecate Direct Secret Access**

```cpp
// In mfa_authenticator.h
struct EnrollmentData {
    [[deprecated("Use encrypted storage accessors instead")]]
    std::string secret_base32;
};
```

**Reason:** Discourage storing secrets in plaintext.

**Alternative:** Use encrypted storage layer.

---

## ABI Compatibility

### Semantic Versioning Policy

**Major Version (X.0.0):**
- Breaking ABI changes allowed
- Struct layout changes
- Function signature changes
- Namespace changes

**Minor Version (1.X.0):**
- New functions/classes added
- Optional parameters added (with defaults)
- New fields added to structs (at end only, with padding)
- ABI-compatible within same major version

**Patch Version (1.0.X):**
- Bug fixes only
- No API changes
- Fully ABI-compatible

---

### ABI Stability Guarantees

**Stable (v1.x):**
- `JWTValidator` class layout
- `JWTClaims` struct layout (until v2.0)
- `MFAAuthenticator` class layout
- `GSSAPIAuthenticator` class layout

**Unstable (may change in minor versions):**
- Internal implementation details
- Private members
- Config structs (can add new fields at end)

---

### Header Compatibility

**Forward Compatibility:**
```cpp
// Code compiled against v1.6 headers
// Will work with v1.7+ runtime (same major version)
```

**Backward Compatibility:**
```cpp
// Code compiled against v1.7 headers
// May not work with v1.6 runtime if using new features
```

---

## Implementation Timeline

### Version 1.6.0 (Q2 2025)
- ✅ New: `device_flow_authenticator.h`
- ✅ New: `webauthn_authenticator.h`
- ✅ New: `passkey_authenticator.h`
- ✅ New: `session_manager.h`
- ✅ Enhancement: JWT refresh token support
- ✅ Enhancement: MFA multiple devices

### Version 1.7.0 (Q4 2025)
- 🔶 New: `adaptive_mfa_engine.h`
- 🔶 New: `magic_link_authenticator.h`
- 🔶 Enhancement: JWT token introspection
- 🔶 Deprecation: Basic auth fallback

### Version 2.0.0 (2027)
- ⬜ Breaking: Namespace reorganization
- ⬜ Breaking: JWTClaims expansion
- ⬜ Breaking: Error handling redesign
- ⬜ Breaking: Remove deprecated APIs

---

## Migration Guides

### Migrating to v2.0.0

**Step 1: Update namespace imports**
```cpp
// Before
#include "auth/jwt_validator.h"
using namespace themis::auth;

// After
#include "auth/v2/jwt_validator.h"
using namespace themis::auth::v2;
```

**Step 2: Update error handling**
```cpp
// Before
try {
    auto claims = validator.parseAndValidate(token);
    // Use claims
} catch (const std::exception& e) {
    // Handle error
}

// After
auto result = validator.parseAndValidate(token);
if (result.has_value()) {
    auto claims = result.value();
    // Use claims
} else {
    auto error = result.error();
    // Handle error with structured error codes
}
```

**Step 3: Recompile**
- Recompile all code that depends on auth headers
- Run tests to verify behavior
- Update deployment

---

## Feedback

We welcome feedback on these planned API changes! Please:
- Open GitHub issues for concerns or suggestions
- Participate in design discussions
- Test alpha/beta releases
- Report compatibility issues

## Test Strategy

- Unit tests for each new authenticator: verify `IAuthenticator` contract compliance using mock-based assertions
- WebAuthn tests: verify challenge generation, attestation parsing, and signature verification with known FIDO2 test vectors
- Constant-time comparison: statistical timing-invariance tests for all secret and token comparison paths
- Session manager tests: verify device/IP pinning enforcement, concurrent session limit eviction, and anomaly detection triggers
- Adaptive MFA engine tests: verify risk level transitions and correct `MFARequirement` for each `RiskLevel` value
- Migration tests: verify v1.x headers compile and link correctly against v2.0 runtime with the provided compatibility shims

## Performance Targets

- JWT token validation (local, cached public key): ≤ 2 ms p99
- MFA TOTP challenge generation (secret derivation + QR encoding): ≤ 5 ms p99
- `SessionManager::createSession()`: ≤ 1 ms including device fingerprint storage
- `AdaptiveMFAEngine::evaluateRisk()`: ≤ 2 ms including cached IP reputation lookup
- `PasskeyAuthenticator::completeAuthentication()`: ≤ 10 ms including ECDSA signature verification
- `JWTValidator::introspect()` (remote, first call uncached): ≤ 50 ms p99 (network-dependent)

## Security / Reliability

- All new auth APIs require TLS transport; plaintext connections must be rejected at the interface level
- Session tokens must never be logged; `SessionInfo` struct must not expose raw token values in any field
- Constant-time comparison enforced for all secret, token, and HMAC verification operations
- `MagicLinkAuthenticator` tokens are single-use and HMAC-signed; replay attacks prevented by a token revocation store
- WebAuthn challenges are cryptographically random (≥ 16 bytes) and single-use per registration / authentication ceremony
- `AdaptiveMFAEngine` risk weights must not be reconfigurable via unauthenticated API calls

## Related Documentation

- [Auth Module Implementation Future Enhancements](../../src/auth/FUTURE_ENHANCEMENTS.md)
- [API Versioning Policy](../../docs/API_VERSIONING.md)
- [Breaking Changes Guide](../../docs/BREAKING_CHANGES.md)
